#include <vector>
#include <cstdio>
#include "values.hpp"

namespace pars {

static std::vector<TypeEntry> types;

static int find_ref_value(void *ptr, Value *refs) {
    refs[0] = (Value)ptr;
    return 1;
}

static void destructor_str(void *ptr) {
    free(ptr);
}

void register_builtin_types() {
    types.clear();

    // The order of these shall match the pre-defined values of Type

    // Immediate types
    register_type("nil", nullptr, nullptr);
    register_type("cons", nullptr, nullptr);
    register_type("num", nullptr, nullptr);
    register_type("sym", nullptr, nullptr);

    // Actually tagged types
    register_type("func", find_ref_value, nullptr);
    register_type("builtin", nullptr, nullptr);
    register_type("str", nullptr, destructor_str);
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
