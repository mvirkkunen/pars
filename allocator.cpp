#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <valgrind/memcheck.h>
#include "allocator.hpp"

// Great platform support
#if defined(__x86_64__) && defined(__linux__)
#define GC_UNIX64
#else
#define GC_NONE
#endif

namespace pars {

ValueCell *ensure_pointer(Value v) {
    return (ValueCell *)((uintptr_t)v & ~0x7);
}

bool gc_maybe_pointer(ValueCell *cell) {
    uintptr_t tag = (uintptr_t)cell & 0x7;

    return tag == 0 || tag == 0x3;
}

bool gc_is_tagged(ValueCell *cell) {
    return (cell->tag & 0x7) == 0x7;
}

Type gc_get_type(ValueCell *cell) {
    return (Type)(cell->tag >> 3);
}

void Allocator::collect_core(void *stack_bottom) {
    // clear marks
    for (size_t i = 0; i < chunks.size(); i++)
        memset(chunks[i]->marks, 0, size / 8 + 1);

    // MARK

    std::vector<Value> roots, new_roots;

    bool first = true;
    Value *start, *end;

    if (stack_bottom) {
        // start with stack

        VALGRIND_MAKE_MEM_DEFINED((char *)stack_bottom, (char *)stack_top - (char *)stack_bottom);

        start = (Value *)stack_bottom;
        end = (Value *)stack_top;
    } else {
        // skip stack

        start = end = nullptr;
    }

    while (first || start < end) {
        // on first pass add any pinned objects to list
        if (first) {
            new_roots.assign(pins.begin(), pins.end());
            first = false;
        }

        // loop through current range assuming anything may be a value
        for (Value *iter = start; iter < end; iter++) {
            if (!gc_maybe_pointer(*iter))
                continue;

            // strip off value tag bits
            ValueCell *cell = ensure_pointer(*iter);

            // loop through chunks
            for (size_t i = 0; i < chunks.size(); i++) {
                Chunk *c = chunks[i];

                // is val a valid pointer to this chunk?
                if (cell >= c->mem && cell < c->mem + size) {
                    int offs = (int)(cell - c->mem);

                    if (cell->tag == 0x7)
                        break; // free cell

                    unsigned int bit = 1 << (offs % 8);

                    if (c->marks[offs / 8] & bit)
                        break; // already marked

                    c->marks[offs / 8] |= bit; // mark cell

                    // recurse to referenced values

                    if (gc_is_tagged(cell)) {
                        Value val = cell;

                        switch (gc_get_type(cell)) {
                            case Type::func:
                                new_roots.push_back(val->tagged.value);
                                continue;

                            default: break;
                        }
                    } else { // cons
                        Value val = cell;

                        new_roots.push_back(car(val));
                        new_roots.push_back(cdr(val));
                    }

                    break;
                }
            }
        }

        /*for (size_t i = 0; i < chunks.size(); i++) {
            Chunk *c = chunks[i];

            printf("Chunk %lu marks: ", i);

            for (int offs = 0; offs < size; offs++)
                putchar(c->marks[offs / 8] & (1 << (offs % 8)) ? '1' : '0');

            printf("\n");
        }*/

        roots.assign(new_roots.begin(), new_roots.end());
        new_roots.clear();

        start = &roots[0];
        end = &roots[roots.size()];
    }

    // SWEEP

    for (size_t i = 0; i < chunks.size(); i++) {
        Chunk *c = chunks[i];

        for (int offs = 0; offs < size; offs++) {
            // such inefficient

            ValueCell *cell = c->mem + offs;

            if (cell->tag == 0x7)
                continue; // free cell

            if (c->marks[offs / 8] & (1 << (offs % 8)))
                continue; // marked, spare

            // free referenced values

            if (gc_is_tagged(cell)) {
                Value val = cell;

                switch (gc_get_type(cell)) {
                    case Type::str:
                        free(val->tagged.str);
                        break;

                    // Type::func points to a list which doesn't need special treatment

                    default: break;
                }
            }

            // put cell back on free list

            cell->tag = 0x7; // nil tag
            cell->tagged.next_free = c->first_free;
            c->first_free = cell;

            c->free++;
        }
    }
}

void Allocator::collect(bool consider_stack) {
#ifdef GC_NONE
    fprintf(stderr, "allocator: Out of memory and GC not supported.\n");
    exit(1);
#endif

    void *stack_bottom;

#ifdef GC_UNIX64
    asm(R"(
        pushq %%rbp
        pushq %%rbx
        pushq %%r12
        pushq %%r13
        pushq %%r14
        pushq %%r15
        movq %%rsp, %0
    )" : "=r"(stack_bottom) :: "sp");
#endif

    collect_core(consider_stack ? stack_bottom : nullptr);

#ifdef GC_UNIX64
    asm(R"(
        popq %%r15
        popq %%r14
        popq %%r13
        popq %%r12
        popq %%rbx
        popq %%rbp
    )" ::: "sp");
#endif
}

Value Allocator::alloc() {
    Chunk *c = chunks[0];

    if (c->free == 0)
        collect();

    if (c->free == 0) {
        printf("Out of memory. TODO: allocate new chunk\n");
        exit(1);
    }

    Value allocated = c->first_free;

    c->first_free = c->first_free->tagged.next_free;
    c->free--;

    return allocated;
}

Allocator::Allocator(int size) : size(size) {
    new_chunk();
}

Allocator::~Allocator() {
    pins.clear();
    collect(false);

    for (size_t i = 0; i < chunks.size(); i++) {
        Chunk *c = chunks[i];

        free(c->mem);
        free(c->marks);
        free(c);
    }

    for (size_t i = 0; i < sym_names.size(); i++)
        free((void *)sym_names[i]);
}

void Allocator::mark_stack_top(void *stack_top) {
    this->stack_top = stack_top;
}

void Allocator::pin(Value val) {
    for (size_t i = 0; i < pins.size(); i++) {
        if (pins[i] == val)
            return;
    }

    pins.push_back(val);
}

void Allocator::unpin(Value val) {
    for (int i = (int)pins.size(); i >= 0; i--) {
        if (pins[i] == val)
            pins.erase(pins.begin() + i);
    }
}

void Allocator::new_chunk() {
    Chunk *c = (Chunk *)malloc(sizeof(Chunk));
    c->free = size;

    c->mem = (ValueCell *)memalign(sizeof(ValueCell), size * sizeof(ValueCell));

    ValueCell *v = c->mem;

    for (int i = 0; i < size - 1; i++, v++) {
        v->tag = 0x7; // nil tag
        v->tagged.next_free = v + 1;
    }

    c->first_free = c->mem;

    c->marks = (char *)malloc(size / 8 + 1);

    chunks.push_back(c);
}

Value Allocator::sym(const char *name) {
    int id = -1;

    for (size_t i = 0; i < sym_names.size(); i++) {
        if (!strcmp(sym_names[i], name)) {
            id = (int)i;
            break;
        }
    }

    if (id == -1) {
        id = sym_names.size();

        char *copy = (char *)malloc(strlen(name) + 1);
        strcpy(copy, name);

        sym_names.push_back(copy);
    }

    return (Value)(((uintptr_t)id << 2) | 0x2);
}

const char *Allocator::sym_name(Value sym) {
    int id = sym_id(sym);

    if (id >= (int)sym_names.size())
        return "<sym!?>";

    return sym_names[id];
}

Value Allocator::func(Value env, Value arg_names, Value body, Value name) {
    Value v = alloc();
    v->tagged.value =
        cons(env,
        cons(arg_names,
        cons(body,
        cons(name, nil))));

    return tag(v, Type::func);
}

const char *Allocator::func_name(Value func) {
    Value name = car(cdr(cdr(cdr(func_val(func)))));

    if (is_sym(name))
        return sym_name(name);

    return "<lambda>";
}

Value Allocator::builtin(BuiltinFunc func) {
    Value v = alloc();
    v->tagged.builtin_func = func;

    return tag(v, Type::builtin);
}

Value Allocator::str(const char *s) {
    char *copy = (char *)malloc(strlen(s) + 1);
    strcpy(copy, s);

    Value v = alloc();
    v->tagged.str = copy;

    return tag(v, Type::str);
}

}
