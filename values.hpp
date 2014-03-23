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
    native = 5,  // fptr = ptr to func
    str = 6,     // ptr = String instance
};

using VoidFunc = void (*)();

struct ValueCell {
    union {
        struct {
            // low 3 bits of type are 1 to indicate this is a tagged cell
            // tag == 0x7 means this is a free cell
            uintptr_t tag;
            union {
                void *ptr;
                VoidFunc fptr;
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

struct TypeEntry {
    const char *name;
    FindRefsFunc find_refs;
    DestructorFunc destructor;
};

const Value nil = (Value)0;

inline Value car(Value cons) { return cons->car; }
inline Value cdr(Value cons) { return cons->cdr; }
inline void set_car(Value cons, Value car) { cons->car = car; }
inline void set_cdr(Value cons, Value cdr) { cons->cdr = cdr; }

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

inline VoidFunc fptr_of(Value val) {
    return ((Value)((char *)val - 3))->fptr;
}

inline bool is_nil(Value val) { return (uintptr_t)val == 0; }
inline bool is_cons(Value val) { return val != 0 && (((uintptr_t)val) & 0x3) == 0x0; }
inline bool is_num(Value val) { return (((uintptr_t)val) & 0x3) == 0x1; }
inline bool is_sym(Value val) { return (((uintptr_t)val) & 0x3) == 0x2; }

inline bool is_truthy(Value val) { return !is_nil(val); }

Type type_of(Value val);
const char *type_name(Type t);

Type register_type(const char *name, FindRefsFunc find_refs, DestructorFunc destructor);

TypeEntry *get_type_entry(Type t);

}
