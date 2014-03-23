#include <cstdio>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("print") print(Context &c, Value rest) {
    for (; is_cons(rest); rest = cdr(rest)) {
        if (type_of(car(rest)) == Type::str) {
            printf("%.*s", str_len(car(rest)), str_data(car(rest)));
        } else {
            c.print(car(rest), false);
            printf(" ");
        }
    }

    printf("\n");

    return nil;
}

} }
