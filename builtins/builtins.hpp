#pragma once

#include "../pars.hpp"

#define BUILTIN(NAME) Value
#define SYNTAX(NAME) Value

#define VERIFY_ARG_CONS(ARG, N) \
    if (!is_cons(ARG)) return c.error("Argument %d must be a cons.", N)

// TODO: properly validate
#define VERIFY_ARG_LIST(ARG, N) \
    if (!(is_nil(ARG) || is_cons(ARG))) return c.error("Argument %d must be a list.", N)

#define VERIFY_ARG_NUM(ARG, N) \
    if (!is_num(ARG)) return c.error("Argument %d must be a number.", N)

#define VERIFY_ARG_STR(ARG, N) \
    if (type_of(ARG) != Type::str) return c.error("Argument %d must be a string.", N)
