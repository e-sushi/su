/*

    Internal representation of literals. This could be a scalar, string, array, or tuple.

    Intended as a unified representation of literals across the project, since before literals
    were just stored on Tokens and it was limited to f64, s64, and strings. This way we can 
    store a Literal on anything that may need it, such as Expr and Arg.

    Array and tuple literals just store a pointer to the entity representing them. Array literals
    are always StaticArrays.

*/

#ifndef AMU_LITERAL_H
#define AMU_LITERAL_H

namespace amu {

struct Tuple;
struct StaticArray;

namespace literal {
enum kind {
    _u64,
    _u32,
    _u16,
    _u8,
    _s64,
    _s32,
    _s16,
    _s8,
    _f64,
    _f32,
    array,
    tuple, 
    string,
    chr,
};
} // namespace literal

struct Literal : public Base {
    literal::kind kind;

    union {
        u64 _u64;
        u32 _u32;
        u16 _u16;
        u8  _u8;
        u8  chr;

        s64 _s64;
        s32 _s32;
        s16 _s16;
        s8  _s8;

        f64 _f64;
        f32 _f32;

        String str;
        Tuple* tuple;
        StaticArray* array;
    };

    DString
    name();

    DString
    dump();

    

    Literal() : Base(base::literal) {}
    Literal(const Literal& l) : Base(l.Base::kind) {memory::copy(this, (void*)&l, sizeof(Literal));}
};


} // namespace amu 

#endif // AMU_LITERAL_H