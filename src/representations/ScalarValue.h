/*

    Internal representation of a singular number in amu. This represents signed, unsigned, and floating 
    point numbers and has an interface for converting between them.

    If amu ever comes to natively support complex numbers, those should probably be implemented as part
    of this structure.

    The purpose of this structure is to provide a uniform representation of all of the possible scalars that
    may appear in amu. This isn't meant to be allocated anywhere, just used on certain structs, such as
    ScalarLiteral and Arg. The type of Scalar being represented is determined by the Scalar type it points
    to.

    The motivation for this comes from finding myself writing out unions of different scalar values and enums to 
    determine what kind of scalar it is in several places and having to convert between them everytime they mixed. 
    This should allow me to just straight copy between the different usages when necessary.


    TODO

    
    Unfortunately doing this this way causes some duplication between Scalar and ScalarValue, particularly
    is_signed and is_float. It also causes there to be two kinds to keep track of with ScalarLiteral. I wanted
    to try and make this inherit Scalar and then have ScalarLiteral inherit ScalarValue, but this runs into the
    diamond problem, and that's too OOP for me to want to deal with. 

    Ideally this can be cleaned up some other way, but this should do for now. 
*/

#ifndef AMU_SCALARVALUE_H
#define AMU_SCALARVALUE_H

#include "Base.h"
#include "storage/String.h"

namespace amu {

struct ScalarValue : public Base {
    enum class Kind {
		Unsigned8,
		Unsigned16,
		Unsigned32,
		Unsigned64,
		Signed8,
		Signed16,
		Signed32,
		Signed64,
		Float32,
		Float64,
	};

	constexpr static String kind_strings[] = {
		"Unsigned8",
		"Unsigned16",
		"Unsigned32",
		"Unsigned64",
		"Signed8",
		"Signed16",
		"Signed32",
		"Signed64",
		"Float32",
		"Float64",
	};

	Kind kind;

    union {
        u64 _u64;
        u32 _u32;
        u16 _u16;
        u8  _u8;

        s64 _s64;
        s32 _s32;
        s16 _s16;
        s8  _s8;

        f64 _f64;
        f32 _f32;
    };

    DString
    display();

    DString
    dump();

    b32
    is_signed();

    b32
    is_float();

    b32
    is_negative();

    u64
    size();

    // casts this ScalarValue IN PLACE.
    void
    cast_to(Kind k);

    ScalarValue() : _u64(0), kind(Kind::Unsigned64), Base(Base::Kind::ScalarValue) {}

	ScalarValue(Kind k) : _u64(0), kind(k), Base(Base::Kind::ScalarValue) {}

    ScalarValue(u64 x) : _u64(x), kind(Kind::Unsigned64), Base(Base::Kind::ScalarValue) {}
    ScalarValue(u32 x) : _u32(x), kind(Kind::Unsigned32), Base(Base::Kind::ScalarValue) {}
    ScalarValue(u16 x) : _u16(x), kind(Kind::Unsigned16), Base(Base::Kind::ScalarValue) {}
    ScalarValue(u8  x) : _u8 (x), kind(Kind::Unsigned8), Base(Base::Kind::ScalarValue) {}
    ScalarValue(s64 x) : _s64(x), kind(Kind::Signed64), Base(Base::Kind::ScalarValue) {}
    ScalarValue(s32 x) : _s32(x), kind(Kind::Signed32), Base(Base::Kind::ScalarValue) {}
    ScalarValue(s16 x) : _s16(x), kind(Kind::Signed16), Base(Base::Kind::ScalarValue) {}
    ScalarValue(s8  x) : _s8 (x), kind(Kind::Signed8), Base(Base::Kind::ScalarValue) {}
    ScalarValue(f64 x) : _f64(x), kind(Kind::Float64), Base(Base::Kind::ScalarValue) {}
    ScalarValue(f32 x) : _f32(x), kind(Kind::Float32), Base(Base::Kind::ScalarValue) {}
};



} // namespace amu 

#endif // AMU_SCALARVALUE_H
