#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("func?") func_p(Context &c, Value val) {
    return c.boolean(type_of(val) == Type::func || type_of(val) == Type::native);
}

BUILTIN("apply") apply(Context &c, Value func, Value args) {
    VERIFY_ARG_LIST(args, 2);

    // apply will verify the rest
    return c.apply(func, args);
}

} }
