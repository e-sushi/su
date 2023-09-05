namespace amu {

DString Literal::
name() {
    switch(this->kind) {
        case literal::_u64: return dstring::init(_u64); 
        case literal::_u32: return dstring::init(_u32); 
        case literal::_u16: return dstring::init(_u16); 
        case literal::_u8: return dstring::init(_u8); 
        case literal::_s64: return dstring::init(_s64); 
        case literal::_s32: return dstring::init(_s32); 
        case literal::_s16: return dstring::init(_s16); 
        case literal::_s8: return dstring::init(_s8); 
        case literal::_f64: return dstring::init(_f64); 
        case literal::_f32: return dstring::init(_f32); 
        case literal::string: return dstring::init(str); 
        case literal::chr: return dstring::init("'", (char)chr, "'"); 
        case literal::array: return array->name();
        case literal::tuple: return tuple->name(); 
    }
    return dstring::init("bad literal");
}

DString Literal::
dump() {
    DString out = dstring::init("Literal<");
    switch(kind) {
        case literal::_u64: dstring::append(out, "u64: ", _u64); break;
        case literal::_u32: dstring::append(out, "u32: ", _u32); break;
        case literal::_u16: dstring::append(out, "u16: ", _u16); break;
        case literal::_u8: dstring::append(out, "u8: ", _u8); break;
        case literal::_s64: dstring::append(out, "s64: ", _s64); break;
        case literal::_s32: dstring::append(out, "s32: ", _s32); break;
        case literal::_s16: dstring::append(out, "s16: ", _s16); break;
        case literal::_s8: dstring::append(out, "s8: ", _s8); break;
        case literal::_f64: dstring::append(out, "f64: ", _f64); break;
        case literal::_f32: dstring::append(out, "f32: ", _f32); break;
        case literal::string: dstring::append(out, "\"", str, "\""); break;
        case literal::chr: dstring::append(out, "'", (char)chr, "'"); break;
        case literal::array: dstring::append(out, array->name()); break;
        case literal::tuple: dstring::append(out, tuple->name()); break;
    }
    dstring::append(out, ">");
    return out;
}

} // namespace amu