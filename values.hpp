#pragma once

#include <vector>
#include <cstdlib>
#include <cstdint>

namespace pars {

class Context;

struct ValueCell;
using Value = ValueCell *;

using BuiltinFunc = Value (*)(Context &ctx, Value args);

enum class Type : uintptr_t  {
    nil = 0,     // 0 pointer
    // tagged:      x011
    // Rest of bits is a pointer to tagged value cell
    // Lowest 3 bits of tag in cell = always 1 to distinguish from actual pointer
    func = 1,    // tagged.value = (list env arg_names body name)
    builtin = 2, // tagged.builtin_func = ptr to func
    str = 3,     // tagged.string = c-string
    // tag in value itself
    cons = 4,    // x000
    num = 5,     // xx01 (30 bit)
    sym = 6,     // xx10 (30 bit id)

    free = (uintptr_t)~1, // for debugging
};

struct ValueCell {
    union {
        struct {
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

inline int sym_id(Value sym) {
    return ((uintptr_t)sym & 0xFFFFFFFC) >> 2;
}

inline Type type_of(Value val) {
    uintptr_t ival = (uintptr_t)val;

    if (ival == 0)
        return Type::nil;

    int tag = ival & 0x3;
    if (tag == 0x0)
        return Type::cons;
    else if (tag == 0x1)
        return Type::num;
    else if (tag == 0x2)
        return Type::sym;

    Type type = (Type)(tagged_val(val)->tag >> 3);
    return (type != Type::nil) ? type : Type::free;
}

inline const char *type_name(Type t) {
    switch (t) {
        case Type::nil: return "nil";
        case Type::func: return "func";
        case Type::builtin: return "builtin";
        case Type::str: return "str";
        case Type::cons: return "cons";
        case Type::num: return "num";
        case Type::sym: return "sym";
        case Type::free: return "FREE";
    }

    return "error";
}

inline bool is_cons(Value val) { return type_of(val) == Type::cons; }
inline bool is_nil(Value val) { return type_of(val) == Type::nil; }
inline bool is_sym(Value val) { return type_of(val) == Type::sym; }
inline bool is_num(Value val) { return type_of(val) == Type::num; }
inline bool is_truthy(Value val) { return !is_nil(val); }

}
