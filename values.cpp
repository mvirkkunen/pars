#include "values.hpp"

namespace pars {

void Allocator::collect() {
    // TODO: Implement garbage collection

    printf("allocator: Out of memory.\n");
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
    for (int i = 0; i < size - 1; i++)
        pool[i].tagged.next_free = &pool[i + 1];

    first_free = &pool[0];
}

Allocator::~Allocator() {
    delete[] pool;
}

Value Allocator::sym(std::string name) {
    int id = -1;

    for (int i = 0; i < (int)syms.size(); i++) {
        if (syms[i] == name) {
            id = i;
            break;
        }
    }

    if (id == -1) {
        id = syms.size();
        syms.push_back(name);
    }

    return (Value)(((intptr_t)id << 2) | 0x2);
}

std::string Allocator::sym_name(Value sym) {
    int id = sym_id(sym);

    if (id >= (int)syms.size())
        return "<sym!?>";

    return syms[id];
}

Value Allocator::func(Value env, Value arg_names, Value body, Value name) {
    Value v = alloc();
    v->tag = Type::func;
    v->tagged.value =
        cons(env,
        cons(arg_names,
        cons(body,
        cons(name, nil))));

    return tagged(v);
}

std::string Allocator::func_name(Value func) {
    Value name = car(cdr(cdr(cdr(func_val(func)))));

    if (is_sym(name))
        return sym_name(name);

    return "<lambda>";
}

Value Allocator::builtin(BuiltinFunc func) {
    Value v = alloc();
    v->tag = Type::builtin;
    v->tagged.builtin_func = func;

    return tagged(v);
}

Value Allocator::str(const char *s) {
    char *copy = (char *)malloc((strlen(s) + 1) * sizeof(char));
    std::strcpy(copy, s);

    Value v = alloc();
    v->tag = Type::str;
    v->tagged.str = copy;

    return tagged(v);
}

}
