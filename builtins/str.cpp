#include <cstring>

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

    String *str = (String *)malloc(sizeof(String) + len + 1);
    str->len = len;
    memset(str->data, num_val(chr_), len);
    str->data[len] = '\0';

    return c.ptr(Type::str, str);
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

        str = (String *)realloc((void *)str, sizeof(String) + len + slen + 1);
        memcpy(str->data + len, str_data(car(rest)), slen);

        len += slen;
    }

    if (str == nullptr)
        return c.str_empty();

    str->len = len;
    str->data[len] = '\0';

    return c.ptr(Type::str, str);
}

} }
