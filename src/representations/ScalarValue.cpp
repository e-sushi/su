namespace amu {

DString* ScalarValue::
display() {
    switch(this->kind) {
        case scalar::unsigned64: return DString::create(_u64);
        case scalar::unsigned32: return DString::create(_u32);
        case scalar::unsigned16: return DString::create(_u16);
        case scalar::unsigned8 : return DString::create(_u8);
        case scalar::signed64: return DString::create(_s64);
        case scalar::signed32: return DString::create(_s32);
        case scalar::signed16: return DString::create(_s16);
        case scalar::signed8 : return DString::create(_s8);
        case scalar::float64: return DString::create(_f64);
        case scalar::float32: return DString::create(_f32);
    }
}

DString* ScalarValue::
dump() {
    return DString::create("ScalarValue<", ScopedDeref(display()).x, ">");
}

b32 ScalarValue::
is_signed() {
    return kind == scalar::signed8 || kind == scalar::signed16 || kind == scalar::signed32 || kind == scalar::signed64;
}

b32 ScalarValue::
is_float() {
    return kind == scalar::float32 || kind == scalar::float64;
}

b32 ScalarValue::
is_negative() {
    switch(kind) {
        case scalar::float32:  return _f32 < 0;
        case scalar::float64:  return _f64 < 0;
        case scalar::signed8:  return _s8  < 0;
        case scalar::signed16: return _s16 < 0;
        case scalar::signed32: return _s32 < 0;
        case scalar::signed64: return _s64 < 0;
    }
    return false;
}

// casts this ScalarValue IN PLACE.
void ScalarValue::
cast_to(scalar::kind k) {
    #define inner(from)                                  \
        switch(k) {                                      \
            case scalar::unsigned64: _u64 = from; break; \
            case scalar::unsigned32: _u32 = from; break; \
            case scalar::unsigned16: _u16 = from; break; \
            case scalar::unsigned8 : _u8  = from; break; \
            case scalar::signed64: _s64 = from; break;   \
            case scalar::signed32: _s32 = from; break;   \
            case scalar::signed16: _s16 = from; break;   \
            case scalar::signed8 : _s8  = from; break;   \
            case scalar::float64: _f64 = from; break;    \
            case scalar::float32: _f32 = from; break;    \
        }
     switch(kind) {
        case scalar::unsigned64: inner(_u64); break;
        case scalar::unsigned32: inner(_u32); break;
        case scalar::unsigned16: inner(_u16); break;
        case scalar::unsigned8 : inner(_u8 ); break;
        case scalar::signed64: inner(_s64); break;
        case scalar::signed32: inner(_s32); break;
        case scalar::signed16: inner(_s16); break;
        case scalar::signed8 : inner(_s8 ); break;
        case scalar::float64: inner(_f64); break;
        case scalar::float32: inner(_f32); break;
    }
    this->kind = k;

    #undef inner

}
} // namespace amu
