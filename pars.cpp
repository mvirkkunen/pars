#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdarg>

#include "pars.hpp"

namespace pars {

Context::Context() : alloc(1024) {
    reset();

    root_env = cons(nil, nil);

    builtins::define_all(*this);
}

Value Context::apply(Value func, Value args) {
    Value prev_cur_func = cur_func;
    cur_func = func;

    //fprintf(stderr, "Stack depth: %10llu\n", (unsigned long long)&prev_cur_func);

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

        if (failing) {
            fail_message = "In " + alloc.func_name(func) + ":\n" + fail_message;
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

bool Context::parse(std::istream &source, Value &result, int level) {
    result = nil;

    char c;
    while (source.get(c) && std::isspace(c))
        ;

    if (!source)
        return false;

    if (c == ')') {
        if (level == 0) {
            result = error("Spurious '('");
            return false;
        }

        source.unget();

        return false;
    } else if (c == '(') {
        Value tail;
        result = nil;

        for (Value item; parse(source, item, level + 1); ) {
            if (failing)
                return false;

            Value new_tail = cons(item, nil);

            if (type_of(result) == Type::nil) {
                result = tail = new_tail;
            } else {
                set_cdr(tail, new_tail);
                tail = new_tail;
            }
        }

        char c;
        while (source.get(c) && std::isspace(c))
            ;

        if (c != ')') {
            result = error("Missing ')'");
            return false;
        }

        return true;
    } else if (c == '\'') {
        Value expr;
        if (!parse(source, expr, level))
            return false;

        result = cons(sym("quote"), cons(expr, nil));
        return true;
    } else if (c == '"') {
        std::string buf;

        while (source.get(c) && c != '"')
            buf += c;

        if (c == '"') {
            result = str(buf.c_str());
            return true;
        }

        result = error("Missing closing '\"'");
        return false;
    } else {
        bool numeric = true;
        std::string buf;

        buf += c;
        if (c != '-' && c != '+' && !std::isdigit(c))
            numeric = false;

        while (source.get(c) && !(std::isspace(c) || c == '(' || c == ')')) {
            if (!std::isdigit(c))
                numeric = false;

            buf += c;
        }

        if (source)
            source.unget();

        if (numeric && buf != "+" && buf != "-")
            result = num(std::atoi(buf.c_str()));
        else
            result = sym(buf);

        return true;
    }
}

void Context::reset() {
    cur_func = nil;
    will_tail_call = false;
    failing = false;
    fail_message = "";
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

    error(std::string("Bad extraction: '") + type + "'");
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

    return error(std::string("Not defined: '") + sym_name(key) + "'");
}

Value Context::eval_list(Value env, Value list) {
    Value result = nil, tail;

    for (; is_cons(list); list = cdr(list)) {
        Value evaluated = eval(env, car(list));
        if (failing)
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
            if (is_sym(car(expr))) {
                auto it = syntax.find(sym_id(car(expr)));
                if (it != syntax.end())
                    return it->second(*this, env, cdr(expr), tail_position);
            }

            expr = eval_list(env, expr);
            if (failing)
                return nil;

            Value func = car(expr), args = cdr(expr);

            if (type_of(func) == Type::func) {
                if (tail_position && func == cur_func) {
                    will_tail_call = true;
                    return args;
                }

                return apply(func, args);
            }

            if (type_of(func) == Type::builtin) {
                return builtin_val(func)(*this, args);
            }

            return error("eval: Invalid application");
        }
    }

    return error("eval: This shouldn't happen");
}

Value Context::error(std::string msg) {
    failing = true;
    fail_message = msg;

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
                        error(std::string("Bad array extraction: '") + type + "'");
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
                        printf("single ext fail\n");
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

void Context::define_builtin(std::string name, BuiltinFunc func) {
    env_define(root_env, sym(name), alloc.builtin(func));
}

void Context::define_syntax(std::string name, SyntaxFunc func) {
    syntax[sym_id(sym(name))] = func;
}

Value Context::exec(std::string code, bool report_errors) {
    Value result = nil;

    std::istringstream source(code);

    while (true) {
        reset();

        Value body;
        if (!parse(source, body))
            break;

        reset();
        result = eval(root_env, body);

        if (failing) {
            if (report_errors)
                std::cerr << "Error: " << fail_message << std::endl;

            break;
        }
    }

    return result;
}

void Context::repl() {
    while (true) {
        std::cout << "> ";

        std::string source;
        std::getline(std::cin, source);

        if (source == "")
            break;

        Value result = exec(source, true);

        if (!is_nil(result))
            print(result);
    }
}

void Context::print(Value val, bool newline) {
    switch (type_of(val)) {
        case Type::nil:
            printf("'()");
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
            std::cout << sym_name(val);
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
