#pragma once

#include "../pars.hpp"

#define BUILTIN(NAME, FUNC_NAME) \
    Value FUNC_NAME(Context &c, Value args)

#define SYNTAX(NAME, FUNC_NAME) \
    Value FUNC_NAME(Context &c, Value env, Value args, bool tail_position)

#define ASSERT_ARG_NUM(ARG, N) \
    if (!is_num(ARG)) return c.error("Argument %d must be a number.", N);

#define ASSERT_ARG_STR(ARG, N) \
    if (type_of(ARG) != Type::str) return c.error("Argument %d must be a string.", N);
