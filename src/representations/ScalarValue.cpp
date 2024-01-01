#include "ScalarValue.h"
#include "storage/DString.h"

namespace amu {

DString ScalarValue::
display() {
	using enum ScalarValue::Kind;
    switch(this->kind) {
		case Unsigned64: return DString(_u64);
        case Unsigned32: return DString(_u32);
        case Unsigned16: return DString(_u16);
        case Unsigned8 : return DString(_u8);
        case Signed64: return DString(_s64);
        case Signed32: return DString(_s32);
        case Signed16: return DString(_s16);
        case Signed8 : return DString(_s8);
        case Float64: return DString(_f64);
        case Float32: return DString(_f32);
    }
}

DString ScalarValue::
dump() {
    return DString("ScalarValue<", display(), ">");
}

b32 ScalarValue::
is_signed() {
	using enum ScalarValue::Kind;
    return util::any_match(kind, Signed8, Signed16, Signed32, Signed64);
}

b32 ScalarValue::
is_float() {
    return kind == ScalarValue::Kind::Float32 || kind == ScalarValue::Kind::Float64;
}

b32 ScalarValue::
is_negative() {
	using enum ScalarValue::Kind;
    switch(kind) {
        case Float32:  return _f32 < 0;
        case Float64:  return _f64 < 0;
        case Signed8:  return _s8  < 0;
        case Signed16: return _s16 < 0;
        case Signed32: return _s32 < 0;
        case Signed64: return _s64 < 0;
		default: return false;
    }
    return false;
}

u64 ScalarValue::
size() {
	using enum ScalarValue::Kind;
    switch(kind) {
        case Signed8:
        case Unsigned8: return 1;
        case Signed16:
        case Unsigned16: return 2;
        case Float32:
        case Signed32:
        case Unsigned32: return 4;
        case Float64:
        case Signed64:
        case Unsigned64: return 8;
    }
}

// casts this ScalarValue IN PLACE.
void ScalarValue::
cast_to(ScalarValue::Kind k) {
	using enum ScalarValue::Kind;
    #define inner(from)                          \
        switch(k) {                              \
            case Unsigned64: _u64 = from; break; \
            case Unsigned32: _u32 = from; break; \
            case Unsigned16: _u16 = from; break; \
            case Unsigned8 : _u8  = from; break; \
            case Signed64: _s64 = from; break;   \
            case Signed32: _s32 = from; break;   \
            case Signed16: _s16 = from; break;   \
            case Signed8 : _s8  = from; break;   \
            case Float64: _f64 = from; break;    \
            case Float32: _f32 = from; break;    \
        }
     switch(kind) {
        case Unsigned64: inner(_u64); break;
        case Unsigned32: inner(_u32); break;
        case Unsigned16: inner(_u16); break;
        case Unsigned8 : inner(_u8 ); break;
        case Signed64: inner(_s64); break;
        case Signed32: inner(_s32); break;
        case Signed16: inner(_s16); break;
        case Signed8 : inner(_s8 ); break;
        case Float64: inner(_f64); break;
        case Float32: inner(_f32); break;
    }
    this->kind = k;

    #undef inner
}

} // namespace amu
