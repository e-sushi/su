namespace amu {

DString* Literal::
name() {
    switch(this->kind) {
        case literal::_u64: return DString::create(_u64); 
        case literal::_u32: return DString::create(_u32); 
        case literal::_u16: return DString::create(_u16); 
        case literal::_u8: return DString::create(_u8); 
        case literal::_s64: return DString::create(_s64); 
        case literal::_s32: return DString::create(_s32); 
        case literal::_s16: return DString::create(_s16); 
        case literal::_s8: return DString::create(_s8); 
        case literal::_f64: return DString::create(_f64); 
        case literal::_f32: return DString::create(_f32); 
        case literal::string: return DString::create(str); 
        case literal::chr: return DString::create("'", (char)chr, "'"); 
        case literal::array: return array->name();
        case literal::tuple: return tuple->name(); 
    }
    return DString::create("bad literal");
}

DString* Literal::
dump() {
    DString* out = DString::create("Literal<");
    switch(kind) {
        case literal::_u64: out->append("u64: ", _u64); break;
        case literal::_u32: out->append("u32: ", _u32); break;
        case literal::_u16: out->append("u16: ", _u16); break;
        case literal::_u8: out->append("u8: ", _u8); break;
        case literal::_s64: out->append("s64: ", _s64); break;
        case literal::_s32: out->append("s32: ", _s32); break;
        case literal::_s16: out->append("s16: ", _s16); break;
        case literal::_s8: out->append("s8: ", _s8); break;
        case literal::_f64: out->append("f64: ", _f64); break;
        case literal::_f32: out->append("f32: ", _f32); break;
        case literal::string: out->append("\"", str, "\""); break;
        case literal::chr: out->append("'", (char)chr, "'"); break;
        case literal::array: out->append(ScopedDStringRef(array->name()).x); break;
        case literal::tuple: out->append(ScopedDStringRef(tuple->name()).x); break;
    }
    out->append(">");
    return out;
}

} // namespace amu