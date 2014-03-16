#include <vector>
#include <cctype>

using std::intptr_t;

namespace pars {

class Context;

void panic(std::string msg) {
    std::cout << msg << std::endl;
    std::exit(1);
}

enum class Type {
                 // tag
    nil = 0,     // 0 pointer
    cons = 1,    // xx00
    num = 2,     // xx01 (30 bit)
    func = 3,    // xx10 (points to (list env arg_names body)) -> can be freed just like cons
    // indexed (bits -> point to table)
    sym = 4,  // 0011
    builtin = 5, // 0111
};

enum class Tag : intptr_t {
    func = 1,
    string = 2,
};

struct ValueCell;
typedef ValueCell *Value;

struct alignas(sizeof(Value) * 2) ValueCell {
    union {
        ValueCell *next_free;
        struct {
            union {
                Value tag_value;
                char *tag_string;
            };
            intptr_t tag;
        };
        struct { Value car, cdr; };
    };
};

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
        return Type::func;

    tag = ival & 0xF;
    if (tag == 0x3)
        return Type::sym;
    else if (tag == 0x7)
        return Type::builtin;

    panic("Weird type.");
    return Type::nil;
}

bool is_cons(Value val) { return type_of(val) == Type::cons; }
bool is_nil(Value val) { return type_of(val) == Type::nil; }
bool is_sym(Value val) { return type_of(val) == Type::sym; }
bool is_num(Value val) { return type_of(val) == Type::num; }
bool is_truthy(Value val) { return !is_nil(val); }

const Value nil = (Value)0;

Value car(Value cons) { return cons->car; }
Value cdr(Value cons) { return cons->cdr; }
void set_car(Value cons, Value car) { cons->car = car; }
void set_cdr(Value cons, Value cdr) { cons->cdr = cdr; }

int num_val(Value val) {
    return ((intptr_t)val & 0xFFFFFFFC) >> 2;
}

int indexed_val(Value val) {
    return ((intptr_t)val & 0xFFFFFFF0) >> 4;
}

Value func_val(Value func) {
    return (Value)((intptr_t)func & 0xFFFFFFFC);
}

int sym_id(Value sym) {
    return indexed_val(sym);
}

class Allocator {
    int size, free;
    ValueCell *pool;
    ValueCell *first_free;

    void collect() {
        // TODO: Implement garbage collection

        panic("Out of memory.");
    }

    ValueCell *alloc() {
        if (free == 0)
            collect();

        ValueCell *allocated = first_free;

        first_free = first_free->next_free;
        free--;

        return allocated;
    }

public:

    std::vector<std::string> syms;
    std::vector<std::function<Value(Context &, Value)>> builtins;

    Allocator(int size) : size(size), free(size) {
        pool = new ValueCell[size];
        for (int i = 0; i < size - 1; i++)
            pool[i].next_free = &pool[i + 1];

        first_free = &pool[0];
    }

    Value cons(Value car, Value cdr) {
        ValueCell *val = alloc();
        val->car = car;
        val->cdr = cdr;

        return (Value)val;
    }

    Value num(int num) {
        return (Value)(((intptr_t)(num & 0x3FFFFFFF) << 2) | 0x1);
    }

    Value sym(std::string name) {
        int index = -1;

        for (int i = 0; i < syms.size(); i++) {
            if (syms[i] == name) {
                index = i;
                break;
            }
        }

        if (index == -1) {
            index = syms.size();
            syms.push_back(name);
        }

        return make_indexed(index, 0x3);
    }

    std::string sym_name(Value sym) {
        return syms[sym_id(sym)];
    }

    Value make_indexed(int index, int tag) {
        return (Value)(((intptr_t)index << 4) | tag);
    }

    Value make_func(Value env, Value arg_names, Value body) {
        Value func = cons(env, cons(arg_names, cons(body, nil)));

        return (Value)((intptr_t)func | 0x2);
    }

    Value make_builtin(std::function<Value(Context &, Value)> impl) {
        builtins.push_back(impl);

        return make_indexed(builtins.size() - 1, 0x7);
    }

};

}
