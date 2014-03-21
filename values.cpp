#include "values.hpp"

namespace pars {

Type type_of(Value val) {
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

    return (type == Type::nil) ? Type::free : type;
}

const char *type_name(Type t) {
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

}
