#pragma once

#include "../pars.hpp"

#define BUILTIN(NAME, FUNC_NAME, ARGS, NREQ, NOPT, HAS_REST) \
    Value FUNC_NAME ARGS; \
    NativeInfo FUNC_NAME##_native_info = { NAME, NREQ, NOPT, HAS_REST, (VoidFunc)FUNC_NAME }; \
    Value FUNC_NAME ARGS

#define SYNTAX(NAME, FUNC_NAME) \
    Value FUNC_NAME(Context &c, Value env, Value args, bool tail_position)

// TODO: properly validate
#define VERIFY_ARG_LIST(ARG, N) \
    if (!(is_nil(ARG) || is_cons(ARG))) return c.error("Argument %d must be a list.", N);

#define VERIFY_ARG_NUM(ARG, N) \
    if (!is_num(ARG)) return c.error("Argument %d must be a number.", N);

#define VERIFY_ARG_STR(ARG, N) \
    if (type_of(ARG) != Type::str) return c.error("Argument %d must be a string.", N);
