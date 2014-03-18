#include <cstdio>
#include <cstring>
#include "values.hpp"

namespace pars {

void Allocator::collect() {
    // TODO: Implement garbage collection

    fprintf(stderr, "allocator: Out of memory.\n");
    exit(1);
}

Value Allocator::alloc() {
    if (free == 0)
        collect();

    Value allocated = first_free;

    first_free = first_free->tagged.next_free;
    free--;

    return allocated;
}

Allocator::Allocator(int size) : size(size), free(size) {
    pool = new ValueCell[size];
    for (int i = 0; i < size - 1; i++) {
        pool[i].tag = 0x7; // nil tag
        pool[i].tagged.next_free = &pool[i + 1];
    }

    first_free = &pool[0];
}

Allocator::~Allocator() {
    delete[] pool;
}

Value Allocator::sym(const char *name) {
    int id = -1;

    for (int i = 0; i < (int)sym_names.size(); i++) {
        if (!strcmp(sym_names[i], name)) {
            id = i;
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
