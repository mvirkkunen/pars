#pragma once

#include <vector>
#include "values.hpp"

namespace pars {

using SyntaxFunc = Value (*)(Context &, Value, Value, bool);

class Context {
    struct SyntaxEntry {
        Value sym;
        SyntaxFunc func;
    };

    std::vector<SyntaxEntry> syntax;
    Allocator alloc;

    Value root_env;

    Value cur_func;
    bool will_tail_call;

    bool _failing;
    char _fail_message[1024];

    void reset();

    Value eval_list(Value env, Value list);

    Value apply(Value func, Value args);

    bool parse(char **source, Value &result);

    bool extract_one(Value arg, char type, void *dest);

public:
    Context();

    bool failing() { return _failing; }
    const char *fail_message() { return _fail_message; }

    Value cons(Value car, Value cdr) { return alloc.cons(car, cdr); }
    Value num(int num) { return alloc.num(num); }
    Value sym(const char *name) { return alloc.sym(name); }
    Value func(Value env, Value arg_names, Value body, Value name) {
        return alloc.func(env, arg_names, body, name);
    }
    Value str(char *s) { return alloc.str(s); }

    const char *sym_name(Value sym) { return alloc.sym_name(sym); }

    void env_define(Value env, Value key, Value value);

    Value env_get(Value env, Value key);

    Value eval(Value env, Value expr, bool tail_position = false);

    Value error(const char *msg, ...);

    bool extract(Value args, const char *format, ...);

    void define_builtin(const char *name, BuiltinFunc func);
    void define_syntax(const char *name, SyntaxFunc func);

    Value exec(char *code, bool report_errors = false, bool print_results = false);
    Value exec_file(char *path, bool report_errors = false, bool print_results = false);
    void repl();
    void print(Value val, bool newline = true);
};

}
