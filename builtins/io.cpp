#include <cstdio>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("print", print, (Context &c, Value args), 0, 0, 1) {
    for (Value item = args; is_cons(item); item = cdr(item)) {
        if (type_of(car(item)) == Type::str) {
            printf("%.*s", str_len(car(item)), str_data(car(item)));
        } else {
            c.print(car(item), false);
            printf(" ");
        }
    }

    printf("\n");

    return nil;
}

} }
