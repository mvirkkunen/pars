#include <cstdio>
#include <cstring>
#include <malloc.h>
#include "allocator.hpp"

namespace pars {

void Allocator::collect() {
    // TODO: Implement garbage collection

    fprintf(stderr, "allocator: Out of memory.\n");
    exit(1);
}

Value Allocator::alloc() {
    Chunk *c = chunks[0];

    if (c->free == 0)
        collect();

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
        delete chunks[i]->mem;
}

void Allocator::new_chunk() {
    Chunk *c = (Chunk *)malloc(sizeof(Chunk));
    c->size = size;
    c->free = size;

    c->mem = (ValueCell *)memalign(sizeof(ValueCell) * 2, size * sizeof(ValueCell));

    ValueCell *v = c->mem;

    for (int i = 0; i < size - 1; i++, v++) {
        v->tag = 0x7; // nil tag
        v->tagged.next_free = v + 1;
    }

    c->first_free = c->mem;

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

    return (Value)(((intptr_t)id << 2) | 0x2);
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
