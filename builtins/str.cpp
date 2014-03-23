#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("str-len", str_len_, (Context &c, Value str), 1, 0, 0) {
    VERIFY_ARG_STR(str, 1);

    return c.num(str_len(str));
}

} }
