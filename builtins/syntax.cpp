#include "builtins.hpp"

namespace pars { namespace builtins {

SYNTAX("quote", quote) {
    (void)c;
    (void)env;
    (void)tail_position;

    return car(args);
}

SYNTAX("define", define) {
    (void)tail_position;

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
            c.func(
                env,
                cdr(name),
                args,
                name));
    }

    return nil;
}

// (lambda (x y) body)
SYNTAX("lambda", lambda) {
    (void)tail_position;

    return c.func(
        env,
        car(args),
        cdr(args),
        nil);
}

// (if test then else)
SYNTAX("if", if_) {
    Value res = c.eval(env, car(args));
    if (c.failing())
        return nil;

    return is_truthy(res)
        ? c.eval(env, car(cdr(args)), tail_position)
        : c.eval(env, car(cdr(cdr(args))), tail_position);
}

} }
