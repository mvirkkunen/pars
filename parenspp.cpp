#include <vector>
#include <map>
#include <string>
#include <functional>
#include <iostream>
#include <sstream>

#include "allocator.hpp"

namespace pars {

class Context {
    std::map<int, std::function<Value(Context &, Value, Value, bool)>> syntax;
    Allocator alloc;

    Value cur_func;
    bool will_tail_call;

    bool failing;
    std::string fail_message;

    Value root_env;

    Value eval_list(Value env, Value list) {
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

    Value eval(Value env, Value expr, bool tail_position = false) {
        switch (type_of(expr)) {
            case Type::nil:
            case Type::num:
            case Type::func:
            case Type::builtin:
                return expr;
        }

        if (type_of(expr) == Type::sym) {
            return env_get(env, expr);
        } else if (type_of(expr) == Type::cons) {
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

            if (type_of(func) == Type::builtin)
                return alloc.builtins[indexed_val(func)](*this, args);

            return error("eval: Invalid application");
        }

        return error("eval: This shouldn't happen");
    }

    Value apply(Value func, Value args) {
        Value prev_cur_func = cur_func;
        cur_func = func;

        //std::cout << "Stack depth: " << (intptr_t)&prev_cur_func << "\n";

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
            if (failing)
                return nil;

            if (will_tail_call) {
                will_tail_call = false;
                args = result;
                goto tail_call;
            }
        }

        cur_func = prev_cur_func;

        return result;
    }

    bool parse(std::istream &source, Value &result, int level = 0) {
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
        } if (c == '(') {
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

    void env_define(Value env, Value key, Value value) {
        Value vars = cdr(env);

        for (; type_of(vars) == Type::cons; vars = cdr(vars)) {
            if (type_of(car(vars)) == Type::cons && car(car(vars)) == key) {
                set_cdr(car(vars), value);
                return;
            }
        }

        set_cdr(env, cons(cons(key, value), cdr(env)));
    }

    Value env_get(Value env, Value key) {
        Value vars = cdr(env);

        for (; type_of(vars) == Type::cons; vars = cdr(vars)) {
            if (type_of(car(vars)) == Type::cons && car(car(vars)) == key)
                return cdr(car(vars));
        }

        if (type_of(car(env)) == Type::cons)
            return env_get(car(env), key);

        return error("Not defined: '" + sym_name(key) + "'");
    }

    void define_math_builtin(std::string name, std::function<int(int, int)> func) {
        define_builtin(name, [name, func](Context &c, Value args) {
            if (is_nil(args))
                return c.num(0);

            int accu = 0;

            Value v = car(args);
            if (type_of(v) == Type::num)
                accu = num_val(v);

            for (args = cdr(args); is_cons(args); args = cdr(args)) {
                v = car(args);

                if (type_of(v) == Type::num)
                    accu = func(accu, num_val(v));
            }

            return c.num(accu);
        });
    }

    void define_cmp_builtin(std::string name, std::function<bool(int, int)> func) {
        define_builtin(name, [func](Context &c, Value args) {
            if (!is_cons(args) || !is_cons(cdr(args)))
                return nil;

            Value a = car(args), b = car(cdr(args));

            return (is_num(a) && is_num(b) && func(num_val(a), num_val(b))) ? c.num(1) : nil;
        });
    }

    void reset() {
        cur_func = nil;
        will_tail_call = false;
        failing = false;
        fail_message = "";
    }

    Value make_func(Value env, Value arg_names, Value body) { return alloc.make_func(env, arg_names, body); }

public:

    Value cons(Value car, Value cdr) { return alloc.cons(car, cdr); }
    Value num(int num) { return alloc.num(num); }
    Value sym(std::string name) { return alloc.sym(name); }

    std::string sym_name(Value sym) { alloc.sym_name(sym); }

    Value error(std::string msg) {
        failing = true;
        fail_message = msg;

        return nil;
    }

    void define_builtin(std::string name, std::function<Value(Context &, Value)> func) {
        env_define(root_env, sym(name), alloc.make_builtin(func));
    }

    void define_syntax(std::string name, std::function<Value(Context &, Value, Value, bool)> func) {
        syntax[sym_id(sym(name))] = func;
    }

    Value execute(std::string code) {
        Value result = nil;

        std::istringstream source(code);

        Value body;
        while (true) {
            reset();

            Value body;
            if (!parse(source, body))
                break;

            reset();
            result = eval(root_env, body);

            if (failing)
                break;
        }

        return result;
    }

    void repl() {
        while (true) {
            std::cout << "> ";

            std::string source;
            std::getline(std::cin, source);

            if (source == "")
                break;

            Value result = execute(source);

            if (!is_nil(result))
                print(result);

            if (failing)
                std::cout << "Error: " << fail_message << std::endl;
        }
    }

    void print(Value val, bool newline = true) {
        switch (type_of(val)) {
            case Type::nil:
                std::cout << "'()";
                break;

            case Type::cons:
                std::cout << "(";

                while (true) {
                    print(car(val), false);

                    if (type_of(cdr(val)) != Type::cons) {
                        if (type_of(cdr(val)) != Type::nil) {
                            std::cout << " . ";
                            print(cdr(val), false);
                        }

                        break;
                    }

                    std::cout << " ";

                    val = cdr(val);
                }

                std::cout << ")";

                break;

            case Type::num:
                std::cout << num_val(val);
                break;

            case Type::func:
                std::cout << "#FUNC";
                break;

            case Type::sym:
                std::cout << sym_name(val);
                break;

            case Type::builtin:
                std::cout << "#BUILTIN";
                break;
        }

        if (newline)
            std::cout << std::endl;
    }

    Context() : alloc(1024) {
        reset();

        root_env = cons(nil, nil);

        define_syntax("define", [](Context &c, Value env, Value args, bool tail_position) {
            if (!is_cons(args))
                return nil;

            Value name = car(args);
            args = cdr(args);

            // (define x 2)
            if (is_sym(name))
                c.env_define(env, name, c.eval(env, car(args)));

            // (define (double x) (* x x))
            if (is_cons(name)) {
                c.env_define(
                    env,
                    car(name),
                    c.make_func(
                        env,
                        cdr(name),
                        args));
            }

            return nil;
        });

        // (lambda (x y) body)
        define_syntax("lambda", [](Context &c, Value env, Value args, bool tail_position) {
            return c.make_func(env,
                car(args),
                cdr(args));
        });

        // (if test then else)
        define_syntax("if", [](Context &c, Value env, Value args, bool tail_position) {
            return is_truthy(c.eval(env, car(args)))
                ? c.eval(env, car(cdr(args)), tail_position)
                : c.eval(env, car(cdr(cdr(args))), tail_position);
        });

        define_syntax("quote", [](Context &c, Value env, Value args, bool tail_position) {
            return car(args);
        });

        define_builtin("print", [](Context &c, Value args) {
            for (; type_of(args) == Type::cons; args = cdr(args)) {
                c.print(car(args), false);
                std::cout << " ";
            }

            std::cout << std::endl;

            return nil;
        });

        define_math_builtin("+", [](int a, int b) { return a + b; });
        define_math_builtin("-", [](int a, int b) { return a - b; });
        define_math_builtin("*", [](int a, int b) { return a * b; });
        define_math_builtin("/", [](int a, int b) { return a / b; });

        define_cmp_builtin("=", [](int a, int b) { return a == b; });
        define_cmp_builtin(">", [](int a, int b) { return a > b; });
    }
};

}

int main() {
    pars::Context ctx;

    //ctx.execute("(define x 2) (define (double x) (print (quote (x is)) x (quote (let's double it))) (* x 2))");
    //ctx.execute("(define x 2) (define (double x) (* x 2))");

    //ctx.print(ctx.execute("(double 5)"));
    //ctx.print(ctx.execute("(double x)"));
    //ctx.print(ctx.execute("((lambda (x) (+ x 1)) 2)"));

    //ctx.print(ctx.execute("(if (= 1 1) (quote yep) (quote nope))"));
    //ctx.print(ctx.execute("(if (= 1 2) (quote yep) (quote nope))"));

    ctx.execute("(define (fact1 x) \
  (if (= x 0) 1 \
      (* x (fact1 (- x 1))))) \
\
(define (fact2 x) \
  (define (fact-tail x accum) \
    (if (= x 0) accum \
        (fact-tail (- x 1) (* x accum)))) \
  (fact-tail x 1))");

    ctx.print(ctx.execute("(fact1 10)"));
    ctx.print(ctx.execute("(fact2 10)"));

    ctx.repl();
}
