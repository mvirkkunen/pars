#pragma once

#include <vector>
#include <cstdlib>
#include <cstdint>

namespace pars {

struct ValueCell;
using Value = ValueCell *;

class Context;
using BuiltinFunc = Value (*)(Context &ctx, Value args);

enum class Type : uintptr_t  {
    nil = 0,     // = 0

    // tagged:      x011
    // Rest of bits is a pointer to tagged value cell
    func = 1,    // tagged.value = (list env arg_names body name)
    builtin = 2, // tagged.builtin_func = ptr to func
    str = 3,     // tagged.string = c-string

    // tag in value itself
    cons = 4,    // x000
    num = 5,     // xx01 (30 bit)
    sym = 6,     // xx10 (30 bit id)

    free = ~(uintptr_t)0, // for debugging
};

struct ValueCell {
    union {
        struct {
            // low 3 bits of tag are 1 to indicate this is a tagged cell
            // tag == 0x7 means this is a free cell
            uintptr_t tag;
            union {
                Value value;
                BuiltinFunc builtin_func;
                char *str;
                ValueCell *next_free;
            } tagged;
        };

        struct {
            Value car, cdr;
        };
    };
};

const Value nil = (Value)0;

inline Value car(Value cons) { return cons->car; }
inline Value cdr(Value cons) { return cons->cdr; }
inline void set_car(Value cons, Value car) { cons->car = car; }
inline void set_cdr(Value cons, Value cdr) { cons->cdr = cdr; }

inline int num_val(Value num) {
    return (int)((uintptr_t)num & 0xFFFFFFFC) >> 2;
}

inline int sym_val(Value sym) {
    return ((uintptr_t)sym & 0xFFFFFFFC) >> 2;
}

inline Value tagged_val(Value val) {
    return (Value)((uintptr_t)val - 3);
}

inline Value func_val(Value func) {
    return tagged_val(func)->tagged.value;
}

inline BuiltinFunc builtin_val(Value builtin) {
    return tagged_val(builtin)->tagged.builtin_func;
}

inline char *str_val(Value str) {
    return tagged_val(str)->tagged.str;
}

inline bool is_nil(Value val) { return (uintptr_t)val == 0; }
inline bool is_cons(Value val) { return val != 0 && (((uintptr_t)val) & 0x3) == 0x0; }
inline bool is_num(Value val) { return (((uintptr_t)val) & 0x3) == 0x1; }
inline bool is_sym(Value val) { return (((uintptr_t)val) & 0x3) == 0x2; }

inline bool is_truthy(Value val) { return !is_nil(val); }

Type type_of(Value val);

const char *type_name(Type t);

}
