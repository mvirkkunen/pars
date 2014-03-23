#pragma once

#include <vector>
#include "values.hpp"
#include "allocator.hpp"

namespace pars {

struct String {
    int len;
    char data[];
};

class Context;
using SyntaxFunc = Value (*)(Context &, Value, Value, bool);
using BuiltinFunc = Value (*)(Context &ctx, Value args);

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

    Value builtin(BuiltinFunc func);

public:
    Context();

    bool failing() { return _failing; }
    const char *fail_message() { return _fail_message; }

    Value cons(Value car, Value cdr) { return alloc.cons(car, cdr); }
    Value num(int num) { return alloc.num(num); }
    Value ptr(Type type, void *ptr) { return alloc.ptr(type, ptr); }
    Value fptr(Type type, VoidFunc fptr) { return alloc.fptr(type, fptr); }

    Value func(Value env, Value arg_names, Value body, Value name);
    const char *func_name(Value func);
    Value str(const char *s);

    void env_define(Value env, Value key, Value value);

    void env_undefine(Value env, Value key);

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

inline Value func_val(Value func) {
    return (Value)ptr_of(func);
}

inline BuiltinFunc builtin_val(Value builtin) {
    return (BuiltinFunc)fptr_of(builtin);
}

inline int str_len(Value str) {
    return ((String *)ptr_of(str))->len;
}

inline char *str_data(Value str) {
    return ((String *)ptr_of(str))->data;
}

}
