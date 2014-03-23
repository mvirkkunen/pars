#include <cstring>

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

BUILTIN("set-car!") set_car_(Context &c, Value cons, Value val) {
    (void)c;
    VERIFY_ARG_CONS(cons, 1);

    set_car(cons, val);

    return nil;
}

BUILTIN("set-cdr!") set_cdr_(Context &c, Value cons, Value val) {
    (void)c;
    VERIFY_ARG_CONS(cons, 1);

    set_cdr(cons, val);

    return nil;
}

BUILTIN("equal?") equal_p(Context &c, Value a, Value b) {
    if (type_of(a) != type_of(b))
        return c.boolean(false);

    switch (type_of(a)) {
        case Type::num:
            return c.boolean(num_val(a) == num_val(b));
        case Type::str:
            return c.boolean(
                str_len(a) == str_len(b) && !memcmp(str_data(a), str_data(b), str_len(a)));

        default: return c.boolean(a == b);

    }
}

BUILTIN("not") not_(Context &c, Value val) {
    return c.boolean(!is_truthy(val));
}

} }
