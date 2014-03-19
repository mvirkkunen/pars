#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdarg>

#include "pars.hpp"

namespace pars {

namespace builtins { void define_all(Context &c); }

Context::Context() : alloc(1024) {
    reset();

    root_env = cons(nil, nil);

    builtins::define_all(*this);
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
            return error("Not enough arguments for function");

        env_define(func_env, car(name), car(args));
    }

    Value result = nil;
    for (Value e = body; is_cons(e); e = cdr(e)) {
        result = eval(func_env, car(e), is_nil(cdr(e)));

        if (failing()) {
            // TODO: maybe report function name for debuggin?
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

char *strslice(char *start, char *end) {
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
            result = str(strslice(start, s));

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

bool Context::extract_one(Value arg, char type, void *dest) {
    switch (type) {
        case 'l': {
            Value item = arg;
            bool first = true;
            for (; is_cons(item); item = cdr(item)) {
                if (!first && item == arg) {
                    error("Value is not a list (circular reference)");
                    return false;
                }

                first = false;
            }

            if (is_nil(item)) {
                *(Value *)dest = arg;
                return true;
            }

            error("Value is not a list");
            return false;
        }
        case 'n': {
            if (!is_num(arg)) {
                error("Value is not a number");
                return false;
            }

            *(int *)dest = num_val(arg);
            return true;
        }
    }

    error("Bad extraction: '%c'", type);
    return false;
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

Value Context::eval(Value env, Value expr, bool tail_position) {
    switch (type_of(expr)) {
        case Type::nil:
        case Type::num:
        case Type::func:
        case Type::builtin:
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

            expr = eval_list(env, expr);
            if (failing())
                return nil;

            Value func = car(expr), args = cdr(expr);

            if (type_of(func) == Type::func) {
                if (tail_position && func == cur_func) {
                    will_tail_call = true;
                    return args;
                }

                return apply(func, args);
            }

            if (type_of(func) == Type::builtin)
                return builtin_val(func)(*this, args);

            return error("eval: Invalid application");
        }
    }

    return error("eval: This shouldn't happen");
}

Value Context::error(const char *msg, ...) {
    va_list va;
    va_start(va, msg);

    _failing = true;
    vsnprintf(_fail_message, sizeof(_fail_message), msg, va);

    va_end(va);

    return nil;
}

// Format:
//  n = number -> int n
//  l = list (will validate) -> Value list
//  * = extract next token as array -> type *arr, int count
bool Context::extract(Value args, const char *format, ...) {
    va_list va;
    va_start(va, format);

    const char *fp = format;
    Value arg = args;

    for (; is_cons(arg) && *fp; arg = cdr(arg), fp++) {
        char type = *fp;

        switch (type) {
            case '*': {
                fp++;
                type = *fp;

                size_t value_size;

                switch (type) {
                    case 'n': value_size = sizeof(int); break;
                    default:
                        error("Bad array extraction: '%c'", type);
                        goto out;
                }

                Value list;
                if (!extract_one(arg, 'l', &list))
                    goto out;

                int count = 0;
                for (Value item = list; is_cons(item); item = cdr(item))
                    count++;

                char *result = (char *)malloc(count * value_size);

                int index = 0;
                for (Value item = list; is_cons(item); item = cdr(item)) {
                    if (!extract_one(car(item), type, result + (index * value_size))) {
                        free(result);
                        goto out;
                    }

                    index++;
                }

                *va_arg(va, void **) = result;
                *va_arg(va, int *) = count;

                arg = nil;
                fp++;

                // Consumed all arguments
                goto out;
            }

            default:
                if (!extract_one(car(arg), type, va_arg(va, void *)))
                    goto out;
        }
    }

out:
    va_end(va);

    if (*fp) {
        error("Too few arguments.");
        return false;
    }

    if (!is_nil(arg)) {
        error("Too many arguments.");
        return false;
    }

    return true;
}

void Context::define_builtin(const char *name, BuiltinFunc func) {
    env_define(root_env, sym(name), alloc.builtin(func));
}

void Context::define_syntax(const char *name, SyntaxFunc func) {
    SyntaxEntry ent;
    ent.sym = sym(name);
    ent.func = func;

    syntax.push_back(ent);
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

        case Type::builtin:
            printf("#BUILTIN");
            break;

        case Type::str:
            printf("\"%s\"", str_val(val));
    }

    if (newline)
        printf("\n");
}

}
