#include <vector>
#include <cstdio>
#include "values.hpp"

namespace pars {

static std::vector<TypeEntry> types;

inline Type tagged_type(Value val) {
    return (Type)(((Value)((char *)val - 3))->tag >> 3);
}

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

    return tagged_type(val);
}

const char *type_name(Type t) {
    return types[(size_t)t].name;
}

Type register_type(const char *name, FindRefsFunc find_refs, DestructorFunc destructor) {
    TypeEntry ent;
    ent.name = name;
    ent.find_refs = find_refs;
    ent.destructor = destructor;

    types.push_back(ent);

    return (Type)(types.size() - 1);
}

TypeEntry *get_type_entry(Type t) {
    return &types[(size_t)t];
}

}
