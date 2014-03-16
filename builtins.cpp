namespace pars { namespace builtins {

#define MAKE_MATH_BUILTIN(NAME, OP) \
Value NAME(Context &c, Value args) { \
    int *nums; \
    int count; \
    if (!c.extract(args, "*n", &nums, &count)) \
        return nil; \
    if (count < 2) \
        return c.error("Too few arguments for "#NAME); \
    \
    int accu = nums[0]; \
    for (int i = 1; i < count; i++) \
        accu = accu OP nums[i]; \
    \
    free(nums); \
    return c.num(accu); \
}

MAKE_MATH_BUILTIN(op_add, +)
MAKE_MATH_BUILTIN(op_sub, -)
MAKE_MATH_BUILTIN(op_mul, *)
MAKE_MATH_BUILTIN(op_div, /)

#undef MAKE_MATH_BUILTIN

#define MAKE_CMP_BUILTIN(NAME, OP) \
Value NAME(Context &c, Value args) { \
    int *nums; \
    int count; \
    if (!c.extract(args, "*n", &nums, &count)) \
        return nil; \
    \
    for (int i = 0; i < count - 1; i++) { \
        if (!(nums[i] OP nums[i + 1])) \
            return nil; \
    } \
    free(nums); \
    \
    return c.num(1); \
}

MAKE_CMP_BUILTIN(op_eq, ==)
MAKE_CMP_BUILTIN(op_lt, <)
MAKE_CMP_BUILTIN(op_gt, >)

#undef MAKE_CMP_BUILTIN

Value list_ref(Context &c, Value args) {
    Value list;
    int index;
    if (!c.extract(args, "ln", &list, &index))
        return nil;

    Value item;
    int i;

    for (item = list, i = 0; is_cons(item); item = cdr(item), item++) {
        if (i == index)
            return item;
    }

    if (is_nil(item))
        return c.error("list-ref: Index out of range");

    return c.error("list-ref: Invalid list");
}

Value length(Context &c, Value args) {
    Value list;
    if (!c.extract(args, "l", &list))
        return nil;

    int count = 0;
    for (Value item = list; is_cons(item); item = cdr(item))
        count++;

    return c.num(count);
}

Value print(Context &c, Value args) {
    Value list;
    if (!c.extract(args, "l", &list))
        return nil;

    for (Value item = list; is_cons(item); item = cdr(item)) {
        c.print(car(item), false);
        printf(" ");
    }

    printf("\n");

    return nil;
}

Value quote(Context &c, Value env, Value args, bool tail_position) {
    (void)c;
    (void)env;
    (void)tail_position;

    return args;
}

Value define(Context &c, Value env, Value args, bool tail_position) {
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
Value lambda(Context &c, Value env, Value args, bool tail_position) {
    (void)tail_position;

    return c.func(
        env,
        car(args),
        cdr(args),
        nil);
}

// (if test then else)
Value if_(Context &c, Value env, Value args, bool tail_position) {
    Value res = c.eval(env, car(args));
    if (c.is_failing())
        return nil;

    return is_truthy(res)
        ? c.eval(env, car(cdr(args)), tail_position)
        : c.eval(env, car(cdr(cdr(args))), tail_position);
}

void define_all(Context &c) {
    c.define_syntax("quote", quote);
    c.define_syntax("define", define);
    c.define_syntax("lambda", lambda);
    c.define_syntax("if", if_);

    c.define_builtin("=", op_eq);
    c.define_builtin("<", op_lt);
    c.define_builtin(">", op_gt);

    c.define_builtin("+", op_add);
    c.define_builtin("-", op_sub);
    c.define_builtin("*", op_mul);
    c.define_builtin("/", op_div);
}

} }
