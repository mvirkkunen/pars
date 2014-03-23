#include <vector>
#include <cstring>
#include "values.hpp"

namespace pars {

class symvector : public std::vector<const char *> {
public:
    ~symvector() {
        for (size_t i = 0; i < size(); i++)
            free((void *)(*this)[i]);
    }
};

static symvector sym_names;

static std::vector<TypeInfo> types;

void register_builtin_types();

static std::vector<TypeInfo> &ensure_types() {
    if (types.size() == 0) {
        // Immediate types
        types.emplace_back(TypeInfo { "nil", nullptr, nullptr });
        register_type("cons", nullptr, nullptr);
        register_type("num", nullptr, nullptr);
        register_type("sym", nullptr, nullptr);

        register_builtin_types();
    }

    return types;
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
    return ensure_types()[(size_t)t].name;
}

Type register_type(const char *name, FindRefsFunc find_refs, DestructorFunc destructor) {
    ensure_types().emplace_back(TypeInfo { name, find_refs, destructor });

    return (Type)(ensure_types().size() - 1);
}

TypeInfo *get_type_info(Type t) {
    return &ensure_types()[(size_t)t];
}

Value sym(const char *name) {
    int id = -1;

    for (size_t i = 0; i < sym_names.size(); i++) {
        if (!strcmp(sym_names[i], name)) {
            id = (int)i;
            break;
        }
    }

    if (id == -1) {
        id = sym_names.size();

        char *copy = (char *)malloc(strlen(name) + 1);
        strcpy(copy, name);

        sym_names.push_back(copy);
    }

    return (Value)(((uintptr_t)id << 2) | 0x2);
}

const char *sym_name(Value sym) {
    int id = sym_val(sym);

    return (id < (int)sym_names.size()) ? sym_names[id] : "<sym!?>";
}

}
