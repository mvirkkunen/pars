#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("nil?") nil_p(Context &c, Value val) {
    (void)c;

    return c.boolean(is_nil(val));
}

BUILTIN("cons") cons_(Context &c, Value car, Value cdr) {
    return c.cons(car, cdr);
}

BUILTIN("cons?") cons_p(Context &c, Value val) {
    return c.boolean(is_cons(val));
}

BUILTIN("car") car_(Context &c, Value cons) {
    (void)c;
    VERIFY_ARG_CONS(cons, 1);

    return car(cons);
}

BUILTIN("cdr") cdr_(Context &c, Value cons) {
    (void)c;
    VERIFY_ARG_CONS(cons, 1);

    return cdr(cons);
}

BUILTIN("list") list(Context &c, Value rest) {
    (void)c;

    return rest;
}

BUILTIN("list-ref") list_ref(Context &c, Value list, Value index) {
    VERIFY_ARG_LIST(list, 1);
    VERIFY_ARG_NUM(index, 2);

    for (int i = num_val(index); is_cons(list); list = cdr(list), i--) {
        if (i == 0)
            return car(list);
    }

    return c.error("list-ref: Index out of range");
}

BUILTIN("length") length(Context &c, Value list) {
    VERIFY_ARG_LIST(list, 1);

    int len = 0;
    for (; is_cons(list); list = cdr(list))
        len++;

    return c.num(len);
}

} }
