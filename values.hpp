#pragma once

#include <cstdlib>
#include <cstdint>

namespace pars {

struct ValueCell;
using Value = ValueCell *;

enum class Type : uintptr_t  {
    nil = 0,     // = 0

    // tag in value itself
    cons = 1,    // xx00
    num = 2,     // xx01 (30 bit)
    sym = 3,     // xx10 (30 bit id)

    // tagged:      x011
    // Rest of bits is a pointer to tagged value cell
    func = 4,    // ptr = (list env arg_names body name)
    native = 5,  // ptr = NativeInfo instance
    str = 6,     // ptr = String instance
};

struct ValueCell {
    union {
        struct {
            // low 3 bits of type are 1 to indicate this is a tagged cell
            // tag == 0x7 means this is a free cell
            uintptr_t tag;
            union {
                void *ptr;
                Value next_free;
            };
        };

        struct {
            Value car, cdr;
        };
    };
};

using FindRefsFunc = int (*)(void *ptr, Value *refs);
using DestructorFunc = void (*)(void *ptr);

struct TypeInfo {
    const char *name;
    FindRefsFunc find_refs;
    DestructorFunc destructor;
};

const Value nil = (Value)0;

inline Value car(Value cons) { return cons->car; }
inline Value cdr(Value cons) { return cons->cdr; }
inline void set_car(Value cons, Value car) { cons->car = car; }
inline void set_cdr(Value cons, Value cdr) { cons->cdr = cdr; }

inline Value caar(Value cons) { return car(car(cons)); }
inline Value cadr(Value cons) { return car(cdr(cons)); }
inline Value cdar(Value cons) { return cdr(car(cons)); }
inline Value cddr(Value cons) { return cdr(cdr(cons)); }

inline Value caddr(Value cons) { return car(cdr(cdr(cons))); }
inline Value cadddr(Value cons) { return car(cdr(cdr(cdr(cons)))); }

inline int num_val(Value num) {
    return (int)((uintptr_t)num & 0xFFFFFFFC) >> 2;
}

Value sym(const char *name);

const char *sym_name(Value sym);

inline int sym_val(Value sym) {
    return ((uintptr_t)sym & 0xFFFFFFFC) >> 2;
}

inline void *ptr_of(Value val) {
    return ((Value)((char *)val - 3))->ptr;
}

inline void set_ptr_of(Value val, void *ptr) {
    ((Value)((char *)val - 3))->ptr = ptr;
}

inline bool is_nil(Value val) { return (uintptr_t)val == 0; }
inline bool is_cons(Value val) { return val != 0 && (((uintptr_t)val) & 0x3) == 0x0; }
inline bool is_num(Value val) { return (((uintptr_t)val) & 0x3) == 0x1; }
inline bool is_sym(Value val) { return (((uintptr_t)val) & 0x3) == 0x2; }

inline bool is_truthy(Value val) { return !is_nil(val); }

Type type_of(Value val);
const char *type_name(Type t);

Type register_type(const char *name, FindRefsFunc find_refs, DestructorFunc destructor);

TypeInfo *get_type_info(Type t);

}
