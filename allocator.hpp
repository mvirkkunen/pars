#include <vector>
#include <cctype>
#include <cstring>
#include <cstdlib>

using std::intptr_t;

namespace pars {

class Context;

struct ValueCell;
using Value = ValueCell *;

using BuiltinFunc = Value (*)(Context &ctx, Value args);

enum class Type : intptr_t  {
    nil = 0,     // 0 pointer
    // tagged:      xx11
    // Rest of bits is a pointer to tagged value cell
    func = 1,    // tagged.value = (list env arg_names body name)
    builtin = 2, // tagged.builtin_func = ptr to func
    str = 3,     // tagged.string = c-string
    // tag in value itself
    cons = 4,    // xx00
    num = 5,     // xx01 (30 bit)
    sym = 6,     // xx10 (30 bit id)
};

struct alignas(sizeof(Value) * 2) ValueCell {
    union {
        struct {
            Type tag;
            union {
                Value value;
                BuiltinFunc builtin_func;
                char *str;
                ValueCell *next_free;
            } tagged;
        };

        struct {
            Value car, cdr;
        };
    };
};

const Value nil = (Value)0;

Value car(Value cons) { return cons->car; }
Value cdr(Value cons) { return cons->cdr; }
void set_car(Value cons, Value car) { cons->car = car; }
void set_cdr(Value cons, Value cdr) { cons->cdr = cdr; }

int num_val(Value num) {
    return (int)((intptr_t)num & 0xFFFFFFFC) >> 2;
}

Value tagged_val(Value val) {
    return (Value)((intptr_t)val & ~(intptr_t)0x3);
}

Value func_val(Value func) {
    return tagged_val(func)->tagged.value;
}

BuiltinFunc builtin_val(Value builtin) {
    return tagged_val(builtin)->tagged.builtin_func;
}

char *str_val(Value str) {
    return tagged_val(str)->tagged.str;
}

int sym_id(Value sym) {
    return ((intptr_t)sym & 0xFFFFFFFC) >> 2;
}

Type type_of(Value val) {
    intptr_t ival = (intptr_t)val;

    if (ival == 0)
        return Type::nil;

    int tag = ival & 0x3;
    if (tag == 0x0)
        return Type::cons;
    else if (tag == 0x1)
        return Type::num;
    else if (tag == 0x2)
        return Type::sym;

    return tagged_val(val)->tag;
}

const char *type_name(Type t) {
    switch (t) {
        case Type::nil: return "nil";
        case Type::func: return "func";
        case Type::builtin: return "builtin";
        case Type::str: return "str";
        case Type::cons: return "cons";
        case Type::num: return "num";
        case Type::sym: return "sym";
    }

    return "error";
}

bool is_cons(Value val) { return type_of(val) == Type::cons; }
bool is_nil(Value val) { return type_of(val) == Type::nil; }
bool is_sym(Value val) { return type_of(val) == Type::sym; }
bool is_num(Value val) { return type_of(val) == Type::num; }
bool is_truthy(Value val) { return !is_nil(val); }

class Allocator {
    int size, free;
    ValueCell *pool;
    ValueCell *first_free;

    std::vector<std::string> syms;

    void collect() {
        // TODO: Implement garbage collection

        printf("allocator: Out of memory.\n");
        exit(1);
    }

    Value alloc() {
        if (free == 0)
            collect();

        Value allocated = first_free;

        first_free = first_free->tagged.next_free;
        free--;

        return allocated;
    }

    Value tagged(Value val) {
        return (Value)((intptr_t)val | 0x3);
    }

public:

    Allocator(int size) : size(size), free(size) {
        pool = new ValueCell[size];
        for (int i = 0; i < size - 1; i++)
            pool[i].tagged.next_free = &pool[i + 1];

        first_free = &pool[0];
    }

    ~Allocator() {
        delete[] pool;
    }

    Value cons(Value car, Value cdr) {
        Value val = alloc();
        val->car = car;
        val->cdr = cdr;

        return (Value)val;
    }

    Value num(int num) {
        return (Value)(((intptr_t)*(unsigned int *)&num << 2) | 0x1);
    }

    Value sym(std::string name) {
        int id = -1;

        for (int i = 0; i < (int)syms.size(); i++) {
            if (syms[i] == name) {
                id = i;
                break;
            }
        }

        if (id == -1) {
            id = syms.size();
            syms.push_back(name);
        }

        return (Value)(((intptr_t)id << 2) | 0x2);
    }

    std::string sym_name(Value sym) {
        int id = sym_id(sym);

        if (id >= (int)syms.size())
            return "<sym!?>";

        return syms[id];
    }

    Value func(Value env, Value arg_names, Value body, Value name) {
        Value v = alloc();
        v->tag = Type::func;
        v->tagged.value =
            cons(env,
            cons(arg_names,
            cons(body,
            cons(name, nil))));

        return tagged(v);
    }

    std::string func_name(Value func) {
        Value name = car(cdr(cdr(cdr(func_val(func)))));

        if (is_sym(name))
            return sym_name(name);

        return "<lambda>";
    }

    Value builtin(BuiltinFunc func) {
        Value v = alloc();
        v->tag = Type::builtin;
        v->tagged.builtin_func = func;

        return tagged(v);
    }

    Value str(char *s) {
        char *copy = (char *)malloc((strlen(s) + 1) * sizeof(char));
        std::strcpy(copy, s);

        Value v = alloc();
        v->tag = Type::str;
        v->tagged.str = copy;

        return tagged(v);
    }

};

}
