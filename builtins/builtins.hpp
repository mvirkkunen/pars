#include "../pars.hpp"

#define BUILTIN(NAME, FUNC_NAME) \
    Value FUNC_NAME(Context &c, Value args)

#define SYNTAX(NAME, FUNC_NAME) \
    Value FUNC_NAME(Context &c, Value env, Value args, bool tail_position)
