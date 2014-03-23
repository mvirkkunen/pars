#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("list", list, (Context &c, Value rest), 0, 0, 1) {
    (void)c;

    return rest;
}

BUILTIN("list-ref", list_ref, (Context &c, Value list, Value index), 2, 0, 0) {
    VERIFY_ARG_LIST(list, 1);
    VERIFY_ARG_NUM(index, 2);

    for (int i = num_val(index); is_cons(list); list = cdr(list), i--) {
        if (i == 0)
            return car(list);
    }

    return c.error("list-ref: Index out of range");
}

BUILTIN("length", length, (Context &c, Value list), 1, 0, 0) {
    VERIFY_ARG_LIST(list, 1);

    int len = 0;
    for (; is_cons(list); list = cdr(list))
        len++;

    return c.num(len);
}

} }
