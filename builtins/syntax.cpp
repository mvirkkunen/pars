#include "builtins.hpp"

namespace pars { namespace builtins {

SYNTAX("quote") quote(Context &c, Value env, Value args, bool tail_position) {
    (void)c;
    (void)env;
    (void)tail_position;

    return car(args);
}

SYNTAX("begin") begin(Context &c, Value env, Value args, bool tail_position) {
    Value result = nil;

    for (; is_cons(args); args = cdr(args)) {
        result = c.eval(env, car(args), tail_position && is_nil(cdr(args)));
        if (c.failing())
            return nil;
    }

    return result;
}

// (if test then else)
SYNTAX("if") if_(Context &c, Value env, Value args, bool tail_position) {
    if (is_nil(car(args)) || is_nil(cdr(args)) || is_nil(cadr(args)))
        return c.error("Invalid if");

    Value res = c.eval(env, car(args));
    if (c.failing())
        return nil;

    return is_truthy(res)
        ? c.eval(env, cadr(args), tail_position)
        : !is_nil(cddr(args))
        ? c.eval(env, caddr(args), tail_position)
        : nil;
}

SYNTAX("and") and_(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    for (; is_cons(args); args = cdr(args)) {
        Value res = c.eval(env, car(args));
        if (c.failing())
            return nil;

        if (!is_truthy(res))
            return c.boolean(false);
    }

    return c.boolean(true);
}

SYNTAX("or") or_(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    for (; is_cons(args); args = cdr(args)) {
        Value res = c.eval(env, car(args));
        if (c.failing())
            return nil;

        if (is_truthy(res))
            return c.boolean(true);
    }

    return c.boolean(false);
}

SYNTAX("let") let(Context &c, Value env, Value args, bool tail_position) {
    if (is_nil(args) || !is_cons(car(args)) || is_nil(cdr(args)))
        return c.error("Invalid let");

    Value new_env = c.make_env(env);

    Value item = car(args);
    for (; is_cons(item); item = cdr(item)) {
        if (!is_cons(car(item)) || !is_sym(caar(item)) || is_nil(cdar(item)))
            return c.error("Invalid let");

        Value val = c.eval(new_env, cadar(item));
        if (c.failing())
            return nil;

        c.env_define(new_env, caar(item), val);
    }

    if (!is_nil(item))
        return c.error("Invalid let");

    return begin(c, new_env, cdr(args), tail_position);
}

SYNTAX("define") define(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    if (!is_cons(args))
        return c.error("Invalid definition");

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
            c.func(
                env,
                cdr(name),
                args,
                car(name)));
    }

    return nil;
}

// (lambda (x y) body)
SYNTAX("lambda") lambda(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    return c.func(
        env,
        car(args),
        cdr(args),
        nil);
}

SYNTAX("set!") set(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    if (!is_cons(args) || !is_sym(car(args)) || !is_cons(cdr(args)))
        return c.error("Invalid set!");

    Value val = c.eval(env, cadr(args));
    if (c.failing())
        return nil;

    c.env_set(env, car(args), val);

    return nil;
}

} }
