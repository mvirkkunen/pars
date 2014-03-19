#pragma once

#include <vector>
#include <cstdlib>
#include <cstdint>
#include "values.hpp"

namespace pars {

class Allocator {
    struct Chunk {
        int size, free;
        ValueCell *mem, *first_free;
    };

    int size;

    std::vector<const char *> sym_names;
    std::vector<Chunk *> chunks;

    void new_chunk();

    void collect();

    Value alloc();

    Value tag(Value val, Type tag) {
        val->tag = ((intptr_t)tag << 3) | 0x7;

        return (Value)((intptr_t)val | 0x3);
    }

public:

    Allocator(int size);
    ~Allocator();

    Value cons(Value car, Value cdr) {
        Value val = alloc();
        val->car = car;
        val->cdr = cdr;

        return (Value)val;
    }

    Value num(int num) {
        return (Value)(((intptr_t)*(unsigned int *)&num << 2) | 0x1);
    }

    Value sym(const char *name);
    const char *sym_name(Value sym);

    Value func(Value env, Value arg_names, Value body, Value name);
    const char *func_name(Value func);

    Value builtin(BuiltinFunc func);

    Value str(const char *s);
};

}
