#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("list-ref", list_ref) {
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

    return c.error("list-ref: Index out of range");
}

BUILTIN("length", length) {
    Value list;
    if (!c.extract(args, "l", &list))
        return nil;

    int count = 0;
    for (Value item = list; is_cons(item); item = cdr(item))
        count++;

    return c.num(count);
}

} }
