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

using VoidFunc = void (*)();

struct NativeInfo {
    const char *name;
    int nreq, nopt;
    bool has_rest;
    VoidFunc func;
};

class Context {
    struct SyntaxInfo {
        Value sym;
        SyntaxFunc func;
    };

    std::vector<SyntaxInfo> syntax;
    Allocator alloc;

    Value root_env;

    Value _str_empty;

    Value cur_func;
    bool will_tail_call;

    bool _failing;
    char _fail_message[1024];

    void reset();

    Value eval_list(Value env, Value list);

    Value call_native_func(VoidFunc func, int nargs, Value *args);

    bool parse(char **source, Value &result);

    Value native(NativeInfo *info);
    Value native(const char *name, int nreq, int nopt, bool has_rest, VoidFunc func);

public:
    Context();

    bool failing() { return _failing; }
    const char *fail_message() { return _fail_message; }

    Value cons(Value car, Value cdr) { return alloc.cons(car, cdr); }
    Value num(int num) { return alloc.num(num); }
    Value ptr(Type type, void *ptr) { return alloc.ptr(type, ptr); }

    Value boolean(bool v) { return v ? num(1) : nil; }

    Value func(Value env, Value arg_names, Value body, Value name);
    const char *func_name(Value func);
    Value str(const char *s);
    Value str(const char *s, int len);
    Value str_empty() { return _str_empty; }

    inline Value make_env(Value parent) { return cons(parent, nil); }
    void env_define(Value env, Value key, Value value);
    void env_undefine(Value env, Value key);
    Value env_get(Value env, Value key);

    Value eval(Value env, Value expr, bool tail_position = false);
    Value apply(Value func, Value args);

    Value error(const char *msg, ...);

    void define_native(const char *name, NativeInfo *info);
    void define_native(const char *name, int nreq, int nopt, bool has_rest, VoidFunc func);
    void define_syntax(const char *name, SyntaxFunc func);

    Value exec(char *code, bool report_errors = false, bool print_results = false);
    Value exec_file(char *path, bool report_errors = false, bool print_results = false);
    void repl();
    void print(Value val, bool newline = true);
};

inline Value func_val(Value func) {
    return (Value)ptr_of(func);
}

inline int str_len(Value str) {
    return ((String *)ptr_of(str))->len;
}

inline char *str_data(Value str) {
    return ((String *)ptr_of(str))->data;
}

}

#include "builtins/generated.hpp"
