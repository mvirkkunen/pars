#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdarg>
#include <vector>

#include "pars.hpp"

namespace pars {

static int find_ref_value(void *ptr, Value *refs) {
    refs[0] = (Value)ptr;
    return 1;
}

void register_builtin_types() {
    // The order of these shall match the pre-defined values of Type
    register_type("func", find_ref_value, nullptr);
    register_type("native", nullptr, free);
    register_type("str", nullptr, free);
}

namespace builtins { void define_all(Context &c); }

Context::Context() : alloc(1024) {
    // Good enough for now
    alloc.mark_stack_top((void *)this);

    reset();

    root_env = cons(nil, nil);
    alloc.pin(root_env);

    builtins::define_all(*this);
}

Value Context::func(Value env, Value arg_names, Value body, Value name) {
    return ptr(Type::func,
        cons(env,
        cons(arg_names,
        cons(body,
        cons(name, nil)))));
}

const char *Context::func_name(Value func) {
    Value name = car(cdr(cdr(cdr(func_val(func)))));

    if (is_sym(name))
        return sym_name(name);

    return "<lambda>";
}

Value Context::native(NativeInfo *info) {
    NativeInfo *copy = (NativeInfo *)malloc(sizeof(NativeInfo));
    memcpy(copy, info, sizeof(NativeInfo));

    return ptr(Type::native, copy);
}

Value Context::native(const char *name, int nreq, int nopt, bool has_rest, VoidFunc func) {
    NativeInfo *info = (NativeInfo *)malloc(sizeof(NativeInfo));
    info->name = name;
    info->nreq = nreq;
    info->nopt = nopt;
    info->has_rest = has_rest;
    info->func = func;

    return ptr(Type::native, info);
}

Value Context::str(const char *s) {
    size_t len = strlen(s);

    String *str = (String *)malloc(sizeof(String) + len);
    str->len = len;
    memcpy(str->data, s, len);

    return ptr(Type::str, str);
}

Value Context::apply(Value func, Value args) {
    Value prev_cur_func = cur_func;
    cur_func = func;

    //printf("Stack depth: %10llu\n", (unsigned long long)&prev_cur_func);

    Value value = func_val(func);

    Value env = car(value),
          arg_names = car(cdr(value)),
          body = car(cdr(cdr(value)));

    Value func_env = cons(env, nil);

tail_call:
    for (Value name = arg_names; is_cons(name); args = cdr(args), name = cdr(name)) {
        if (!is_cons(args))
            return error("Too few arguments for function");

        env_define(func_env, car(name), car(args));
    }

    if (!is_nil(args))
        return error("Too many arguments for function");

    Value result = nil;
    for (Value e = body; is_cons(e); e = cdr(e)) {
        result = eval(func_env, car(e), is_nil(cdr(e)));

        if (failing()) {
            // TODO: maybe report function name for debugging?
            return nil;
        }

        if (will_tail_call) {
            will_tail_call = false;
            args = result;
            goto tail_call;
        }
    }

    cur_func = prev_cur_func;

    return result;
}

static char *strslice(char *start, char *end) {
    int len = end - start;

    char *str = (char *)malloc(len + 1);
    memcpy(str, start, len);
    str[len] = '\0';

    return str;
}

bool Context::parse(char **source, Value &result) {
    result = nil;

    char *s = *source;

    for (; isspace(*s); s++)
        ;

    if (*s == ')') {
        result = error("Unexpected '('");
    } else if (*s == '(') {
        Value tail;

        s++;

        while (true) {
            for (; isspace(*s); s++)
                ;

            if (*s == ')')
                break;

            Value item;
            if (!parse(&s, item))
                break;

            Value new_tail = cons(item, nil);

            if (type_of(result) == Type::nil) {
                result = tail = new_tail;
            } else {
                set_cdr(tail, new_tail);
                tail = new_tail;
            }
        }

        if (!failing() && *s == ')') {
            s++;

            *source = s;
            return true;
        }

        result = error("Expected ')'");
    } else if (*s == '\'') {
        s++;

        Value expr;
        if (parse(&s, expr)) {
            result = cons(sym("quote"), cons(expr, nil));

            *source = s;
            return true;
        }
    } else if (*s == '"') {
        char *start = s + 1;

        s++;

        for (; *s && *s != '"'; s++)
            ;

        if (*s == '"') {
            char *slice = strslice(start, s);
            result = str(slice);
            free(slice);

            s++;

            *source = s;
            return true;
        }

        result = error("Missing closing '\"'");
    } else if (*s) {
        char *start = s;
        bool numeric = *s == '-' || *s == '+' || isdigit(*s);

        s++;

        for (; !(isspace(*s) || *s == '(' || *s == ')' || *s == '"'); s++) {
            if (!isdigit(*s))
                numeric = false;
        }

        char *str = strslice(start, s);

        result = (numeric && (str[1] != '\0' || isdigit(str[0])))
            ? num(atoi(str))
            : sym(str);

        free(str);

        *source = s;
        return true;
    }

    return false;
}

void Context::reset() {
    cur_func = nil;
    will_tail_call = false;
    _failing = false;
}

void Context::env_define(Value env, Value key, Value value) {
    Value vars = cdr(env);

    for (; is_cons(vars); vars = cdr(vars)) {
        if (is_cons(car(vars)) && car(car(vars)) == key) {
            set_cdr(car(vars), value);
            return;
        }
    }

    set_cdr(env, cons(cons(key, value), cdr(env)));
}

void Context::env_undefine(Value env, Value key) {
    Value vars = cdr(env), prev = env;

    for (; is_cons(vars); vars = cdr(vars)) {
        if (is_cons(car(vars)) && car(car(vars)) == key) {
            set_cdr(prev, cdr(vars));
            return;
        }

        env = vars;
    }
}

Value Context::env_get(Value env, Value key) {
    Value vars = cdr(env);

    for (; is_cons(vars); vars = cdr(vars)) {
        if (is_cons(car(vars)) && car(car(vars)) == key)
            return cdr(car(vars));
    }

    if (is_cons(car(env)))
        return env_get(car(env), key);

    return error("Not defined: '%s'", sym_name(key));
}

Value Context::eval_list(Value env, Value list) {
    Value result = nil, tail;

    for (; is_cons(list); list = cdr(list)) {
        Value evaluated = eval(env, car(list));
        if (failing())
            return nil;

        Value new_tail = cons(evaluated, nil);

        if (type_of(result) == Type::nil) {
            result = tail = new_tail;
        } else {
            set_cdr(tail, new_tail);
            tail = new_tail;
        }
    }

    return result;
}

Value Context::call_native_func(VoidFunc func, int nargs, Value *args) {
    switch (nargs) {
        case 0: return ((Value (*)(Context &))func)(*this);

        case 1: return ((Value (*)(Context &, Value))func)(*this, args[0]);

        case 2: return ((Value (*)(Context &, Value, Value))func)(*this, args[0], args[1]);

        case 3:
            return ((Value (*)(Context &, Value, Value, Value))func)
                (*this, args[0], args[1], args[2]);

        case 4:
            return ((Value (*)(Context &, Value, Value, Value, Value))func)
                (*this, args[0], args[1], args[2], args[3]);

        case 5:
            return ((Value (*)(Context &, Value, Value, Value, Value, Value))func)
                (*this, args[0], args[1], args[2], args[3], args[4]);

        default: return error("Native func has too many arguments.");
    }
}

Value Context::eval(Value env, Value expr, bool tail_position) {
    switch (type_of(expr)) {
        case Type::nil:
        case Type::num:
        case Type::str:
            return expr;

        case Type::sym:
            return env_get(env, expr);

        case Type::cons:
        {
            Value first = car(expr);

            if (is_sym(first)) {
                for (size_t i = 0; i < syntax.size(); i++) {
                    if (syntax[i].sym == first)
                        return syntax[i].func(*this, env, cdr(expr), tail_position);
                }
            }

            Value evald = eval_list(env, expr);
            if (failing())
                return nil;

            Value func = car(evald), args = cdr(evald);

            if (type_of(func) == Type::func) {
                if (tail_position && func == cur_func) {
                    will_tail_call = true;
                    return args;
                }

                return apply(func, args);
            }

            if (type_of(func) == Type::native) {
                NativeInfo *info = (NativeInfo *)ptr_of(func);

                int nargs = 0;
                Value aargs[5];

                for (int i = 0; i < info->nreq; i++) {
                    if (is_nil(args))
                        return error("Too few arguments for function");

                    aargs[nargs++] = car(args);
                    args = cdr(args);
                }

                for (int i = 0; i < info->nopt; i++) {
                    if (!is_nil(args)) {
                        aargs[nargs++] = car(args);
                        args = cdr(args);
                    } else {
                        aargs[nargs++] = nil;
                    }
                }

                if (info->has_rest)
                    aargs[nargs++] = args;
                else if (!is_nil(args))
                    return error("Too many arguments for function");

                return call_native_func(info->func, nargs, aargs);
            }

            return error("Invalid application");
        }

        default:
            return error("Invalid evaluation");
    }
}

Value Context::error(const char *msg, ...) {
    va_list va;
    va_start(va, msg);

    _failing = true;
    vsnprintf(_fail_message, sizeof(_fail_message), msg, va);

    va_end(va);

    return nil;
}

void Context::define_native(const char *name, NativeInfo *info) {
    env_define(root_env, sym(name), native(info));
}

void Context::define_native(const char *name, int nreq, int nopt, bool has_rest, VoidFunc func) {
    env_define(root_env, sym(name), native(name, nreq, nopt, has_rest, func));
}

void Context::define_syntax(const char *name, SyntaxFunc func) {
    syntax.emplace_back(SyntaxInfo { sym(name), func });
}

Value Context::exec(char *code, bool report_errors, bool print_results) {
    Value result = nil;

    while (true) {
        reset();

        Value body;
        if (!parse(&code, body))
            break;

        reset();
        result = eval(root_env, body);

        if (failing())
            break;

        if (print_results && !is_nil(result))
            print(result);
    }

    if (failing() && report_errors)
        fprintf(stderr, "Error: %s\n", _fail_message);

    return result;
}

Value Context::exec_file(char *path, bool report_errors, bool print_results) {
    FILE *f = fopen(path, "r");
    if (!f)
        return nil;

    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);

    fseek(f, 0, SEEK_SET);

    char *code = (char *)malloc(length + 1);
    fread(code, 1, length, f);
    code[length] = '\0';

    fclose(f);

    Value result = exec(code, report_errors, print_results);

    free(code);

    return result;
}

void Context::repl() {
    char buf[1024];

    while (true) {
        printf("> ");
        if (!fgets(buf, sizeof(buf), stdin))
            break;

        exec(buf, true, true);
    }
}

void Context::print(Value val, bool newline) {
    switch (type_of(val)) {
        case Type::nil:
            printf("()");
            break;

        case Type::cons:
            printf("(");

            while (true) {
                print(car(val), false);

                if (type_of(cdr(val)) != Type::cons) {
                    if (type_of(cdr(val)) != Type::nil) {
                        printf(" . ");
                        print(cdr(val), false);
                    }

                    break;
                }

                printf(" ");

                val = cdr(val);
            }

            printf(")");

            break;

        case Type::num:
            printf("%d", num_val(val));
            break;

        case Type::func:
            printf("#FUNC");
            break;

        case Type::sym:
            printf("%s", sym_name(val));
            break;

        case Type::native:
            printf("#BUILTIN");
            break;

        case Type::str:
            printf("\"%.*s\"", str_len(val), str_data(val));
            break;

        default:
            printf("#WAT");
            break;
    }

    if (newline)
        printf("\n");
}

}
