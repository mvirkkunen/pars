#include <cstdio>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("+") add(Context &c, Value rest) {
    int a = 0;
    for (; is_cons(rest); rest = cdr(rest)) {
        VERIFY_ARG_NUM(car(rest), 1);

        a += num_val(car(rest));
    }

    return c.num(a);
}

BUILTIN("*") mul(Context &c, Value rest) {
    int a = 1;
    for (; is_cons(rest); rest = cdr(rest)) {
        VERIFY_ARG_NUM(car(rest), 1);

        a *= num_val(car(rest));
    }

    return c.num(a);
}

BUILTIN("-") sub(Context &c, Value first, Value rest) {
    int a = num_val(first);

    if (is_nil(rest))
        return c.num(-a);

    for (; is_cons(rest); rest = cdr(rest)) {
        VERIFY_ARG_NUM(car(rest), 1);

        a -= num_val(car(rest));
    }

    return c.num(a);
}

#define CMP_BUILTIN(OP) \
    { \
        if (!is_nil(_first)) {\
            VERIFY_ARG_NUM(_first, 1); \
            for (Value prev = _first; is_cons(rest); rest = cdr(rest)) { \
                VERIFY_ARG_NUM(car(rest), 2); \
                if (!(num_val(prev) OP num_val(car(rest)))) \
                    return c.boolean(false); \
                prev = car(rest); \
            } \
        } \
        return c.boolean(true); \
    }

BUILTIN(">") gt(Context &c, Value _first, Value rest)
CMP_BUILTIN(>)

BUILTIN("<") lt(Context &c, Value _first, Value rest)
CMP_BUILTIN(<)

BUILTIN(">=") gte(Context &c, Value _first, Value rest)
CMP_BUILTIN(>=)

BUILTIN("<=") lte(Context &c, Value _first, Value rest)
CMP_BUILTIN(<=)

BUILTIN("=") eq(Context &c, Value _first, Value rest)
CMP_BUILTIN(==)

#undef CMP_BUILTIN

} }
