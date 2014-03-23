#include <cstdio>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("+", add) {
    int a = 0;
    for (Value i = args; is_cons(i); i = cdr(i)) {
        if (!is_num(car(i)))
            return c.error("Wrong type of argument");

        a += num_val(car(i));
    }

    return c.num(a);
}

BUILTIN("*", mul) {
    int a = 1;
    for (Value i = args; is_cons(i); i = cdr(i)) {
        if (!is_num(car(i)))
            return c.error("Wrong type of argument");

        a *= num_val(car(i));
    }

    return c.num(a);
}

BUILTIN("-", sub) {
    if (is_nil(args))
        return c.error("Too few arguments");

    Value first = car(args), rest = cdr(args);
    if (!is_cons(rest))
        return c.num(-num_val(first));

    int a = num_val(first);
    for (Value i = rest; is_cons(i); i = cdr(i)) {
        if (!is_num(car(i)))
            return c.error("Wrong type of argument");

        a -= num_val(car(i));
    }

    return c.num(a);
}

#define CMP_BUILTIN(OP) \
    { \
        if (!is_nil(args)) { \
            for (Value prev = car(args), i = cdr(args); is_cons(i); i = cdr(i)) { \
                if (!is_num(prev) || !is_num(car(i))) \
                    return c.error("Wrong type of argument"); \
                if (!(num_val(prev) OP num_val(car(i)))) \
                    return nil; \
                prev = car(i); \
            } \
        } \
        return c.num(1); \
    }

BUILTIN(">", gt)
CMP_BUILTIN(>)

BUILTIN("<", lt)
CMP_BUILTIN(<)

BUILTIN(">=", gte)
CMP_BUILTIN(>=)

BUILTIN("<=", lte)
CMP_BUILTIN(<=)

BUILTIN("=", eq)
CMP_BUILTIN(==)

#undef CMP_BUILTIN

} }
