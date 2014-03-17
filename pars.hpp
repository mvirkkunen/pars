#pragma once

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdarg>

#include "values.hpp"

namespace pars {

class Context;

namespace builtins { void define_all(Context &c); }

using SyntaxFunc = Value (*)(Context &, Value, Value, bool);

class Context {
    std::map<int, SyntaxFunc> syntax;
    Allocator alloc;

    Value cur_func;
    bool will_tail_call;

    bool failing;
    std::string fail_message;

    Value root_env;

    void reset();

    Value eval_list(Value env, Value list);

    Value apply(Value func, Value args);

    bool parse(std::istream &source, Value &result, int level = 0);

    bool extract_one(Value arg, char type, void *dest);

public:
    Context();

    bool is_failing() { return failing; }

    Value cons(Value car, Value cdr) { return alloc.cons(car, cdr); }
    Value num(int num) { return alloc.num(num); }
    Value sym(std::string name) { return alloc.sym(name); }
    Value func(Value env, Value arg_names, Value body, Value name) {
        return alloc.func(env, arg_names, body, name);
    }
    Value str(const char *s) { return alloc.str(s); }

    std::string sym_name(Value sym) { return alloc.sym_name(sym); }

    void env_define(Value env, Value key, Value value);

    Value env_get(Value env, Value key);

    Value eval(Value env, Value expr, bool tail_position = false);

    Value error(std::string msg);

    bool extract(Value args, const char *format, ...);

    void define_builtin(std::string name, BuiltinFunc func);
    void define_syntax(std::string name, SyntaxFunc func);

    Value exec(std::string code, bool report_errors = false);
    Value exec_file(std::string path, bool report_errors = false);
    void repl();
    void print(Value val, bool newline = true);
};

}
