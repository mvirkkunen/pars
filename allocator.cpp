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

    // start with stack

    VALGRIND_MAKE_MEM_DEFINED((char *)stack_bottom, (char *)stack_top - (char *)stack_bottom);

    bool add_pins;
    Value *start, *end;

    if (stack_bottom) {
        start = (Value *)stack_bottom;
        end = (Value *)stack_top;
        add_pins = true;
    } else {
        start = &pins[0];
        end = &pins[pins.size()];
        add_pins = false;
    }

    while (start < end) {
        // loop through current range assuming anything may be a value
        for (Value *iter = start; iter < end; iter++) {
            ValueCell *cell = ensure_pointer(*iter);

            // loop through chunks
            for (size_t i = 0; i < chunks.size(); i++) {
                Chunk *c = chunks[i];

                // is val a valid pointer to this chunk?
                if (cell >= c->mem && cell < c->mem + size) {
                    int offs = (int)(cell - c->mem);

                    if (cell->tag == 0x7)
                        break; // free cell

                    if (c->marks[offs / 8] & (1 << (offs % 8)))
                        break; // already marked

                    c->marks[offs / 8] |= 1 << (offs % 8); // mark cell

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

        // on first normal pass add any pinned objects to list
        if (add_pins) {
            roots.insert(roots.end(), pins.begin(), pins.end());
            add_pins = false;
        }

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

            // TODO: free dependent values

            if (gc_is_tagged(cell)) {
                Value val = cell;

                switch (gc_get_type(cell)) {
                    case Type::str:
                        free(val->tagged.str);
                        break;

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

void Allocator::collect() {
#if GC_NONE
    fprintf(stderr, "allocator: Out of memory and GC not supported.\n");
    exit(1);
#endif

    void *stack_bottom;

#if __x86_64__ && __linux__
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

    collect_core(stack_bottom);

#if __x86_64__ && __linux__
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
    for (size_t i = 0; i < chunks.size(); i++)
        free(chunks[i]->mem);
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

    c->mem = (ValueCell *)memalign(sizeof(ValueCell) * 2, size * sizeof(ValueCell));

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
