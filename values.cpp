#include <vector>
#include <cstdio>
#include "values.hpp"

namespace pars {

void register_builtin_types();

static std::vector<TypeEntry> types_;

static std::vector<TypeEntry> &types() {
    if (types_.size() == 0) {
        // Immediate types
        types_.emplace_back(TypeEntry { "nil", nullptr, nullptr });
        register_type("cons", nullptr, nullptr);
        register_type("num", nullptr, nullptr);
        register_type("sym", nullptr, nullptr);

        register_builtin_types();
    }

    return types_;
}

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
    return types()[(size_t)t].name;
}

Type register_type(const char *name, FindRefsFunc find_refs, DestructorFunc destructor) {
    types().emplace_back(TypeEntry { name, find_refs, destructor });

    return (Type)(types().size() - 1);
}

TypeEntry *get_type_entry(Type t) {
    return &types()[(size_t)t];
}

}
