#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("str-len") str_len_(Context &c, Value str) {
    VERIFY_ARG_STR(str, 1);

    return c.num(str_len(str));
}

} }
