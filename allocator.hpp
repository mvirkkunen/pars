#pragma once

#include <vector>
#include <cstdlib>
#include <cstdint>
#include "values.hpp"

namespace pars {

class Allocator {
    struct Chunk {
        int free;
        ValueCell *mem, *first_free;
        char *marks;
    };

    std::vector<const char *> sym_names;

    int size;
    std::vector<Chunk *> chunks;

    void *stack_top;
    std::vector<Value> pins;

    void new_chunk();

    void collect_core(void *stack_bottom);

    Value alloc();

public:

    Allocator(int size);
    ~Allocator();

    void mark_stack_top(void *stack_top);
    void collect(bool consider_stack = true);
    void pin(Value val);
    void unpin(Value val);

    Value cons(Value car, Value cdr) {
        Value val = alloc();
        val->car = car;
        val->cdr = cdr;

        return (Value)val;
    }

    Value num(int num) {
        return (Value)((uintptr_t)(num << 2) | 0x1);
    }

    Value sym(const char *name);
    const char *sym_name(Value sym);

    Value ptr(Type type, void *ptr);
    Value fptr(Type type, VoidFunc fptr);
};

}
