#include <cstring>
#include <cstdio>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("str?") str_p(Context &c, Value val) {
    return c.boolean(type_of(val) == Type::str);
}

BUILTIN("str-make") str(Context &c, Value chr_, Value _len_) {
    VERIFY_ARG_NUM(chr_, 1);

    int len = 1;
    if (!is_nil(_len_)) {
        VERIFY_ARG_NUM(_len_, 2);
        len = num_val(_len_);
    }

    String *str = string_alloc(len);
    memset(str->data, num_val(chr_), len);

    return c.str(str);
}

BUILTIN("str-len") str_len_(Context &c, Value str) {
    VERIFY_ARG_STR(str, 1);

    return c.num(str_len(str));
}

BUILTIN("str-at") str_at(Context &c, Value str, Value index_) {
    VERIFY_ARG_STR(str, 1);
    VERIFY_ARG_NUM(index_, 2);

    int index = num_val(index_);
    if (index >= str_len(str))
        return c.error("String index out of range");

    return c.num(str_data(str)[index]);
}

BUILTIN("str-sub") str_sub(Context &c, Value str, Value start_, Value _len_) {
    VERIFY_ARG_STR(str, 1);
    VERIFY_ARG_NUM(start_, 2);

    int start = num_val(start_), len = -1;
    if (!is_nil(_len_)) {
        VERIFY_ARG_NUM(_len_, 3);
        len = num_val(_len_);
    }

    if (start < 0 || start >= str_len(str))
        return c.error("Invalid start position");

    if (len < 0)
        len = str_len(str) - start;

    if (start + len > str_len(str))
        return c.error("Length out of range");

    return c.str(str_data(str) + start, len);
}

BUILTIN("str-cat") str_cat(Context &c, Value rest) {
    int len = 0;
    String *str = nullptr;

    for (; !is_nil(rest); rest = cdr(rest)) {
        VERIFY_ARG_STR(car(rest), 1);

        int slen = str_len(car(rest));
        if (slen == 0)
            continue;

        string_realloc(&str, len + slen);
        memcpy(str->data + len, str_data(car(rest)), slen);

        len += slen;
    }

    if (str == nullptr)
        return c.str_empty();

    return c.ptr(Type::str, str);
}

BUILTIN("str-index-of") str_index_of(Context &c, Value str_, Value find_, Value _start_) {
    VERIFY_ARG_STR(str_, 1);
    VERIFY_ARG_STR(find_, 2);

    int start = 0;
    if (!is_nil(_start_)) {
        VERIFY_ARG_NUM(_start_, 3);
        start = num_val(_start_);
    }

    if (start > str_len(str_))
        return c.error("String index out of range");

    if (str_len(find_) == 0)
        return c.num(0);

    char *str = str_data(str_), *find = str_data(find_);
    int strl = str_len(str_), findl = str_len(find_);

    for (int i = start, fi = 0; i < strl; i++) {
        if (str[i] == find[fi]) {
            fi++;
            if (fi == findl)
                return c.num(i - findl + 1);
        } else {
            fi = 0;
        }
    }

    return c.num(-1);
}

BUILTIN("->string") to_string(Context &c, Value val) {
    switch (type_of(val)) {
        case Type::nil:
            return c.str("()");
            break;

        case Type::cons:
            {
                // This is so wildly inefficient it's not even funny

                Value res = c.str("(");

                while (true) {
                    res = str_cat(c, c.cons(to_string(c, car(val)), nil));

                    if (type_of(cdr(val)) != Type::cons) {
                        if (type_of(cdr(val)) != Type::nil) {
                            res = str_cat(c, c.cons(c.str(" . "), nil));
                            res = str_cat(c, c.cons(to_string(c, cdr(val)), nil));
                        }

                        break;
                    }

                    res = str_cat(c, c.cons(c.str(" "), nil));

                    val = cdr(val);
                }

                res = str_cat(c, c.cons(c.str(")"), nil));

                return res;
            }

            break;

        case Type::num:
            {
                char buf[20];
                snprintf(buf, sizeof(buf), "%d", num_val(val));

                return c.str(buf);
            }
            break;

        case Type::sym:
            return c.str(sym_name(val));
            break;

        case Type::str:
            return val;

        default:
            return c.str(type_name(type_of(val)));
            break;
    }
}

BUILTIN("string->num") string_to_num(Context &c, Value str) {
    VERIFY_ARG_STR(str, 1);

    return c.num(atoi(str_data(str)));
}

} }
