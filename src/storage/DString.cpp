namespace amu{

DString* DString::
create(String s) {
    auto out = (DString*)memory::allocate(sizeof(DString));
    out->count = s.count;
    out->space = util::round_up_to(s.count, 8);
    out->str = (u8*)memory::allocate(out->space); 
    out->refs = 1;
    if(s.str) memcpy(out->str, s.str, s.count);
    return out;
}

template<typename... T> DString* DString::
create(T... args) {
    DString* out = create();
    (to_string(out, args), ...);
    return out;
}

void DString::
destroy() {
#if BUILD_SLOW
    if(refs) messenger::qdebug(MessageSender::Compiler, String("WARNING: DString being destroyed but still has refs!"));
#endif
    memory::free(str);
    memory::free(this);
}

DString* DString::
ref() {
    refs++;
    return this;
}

void DString::
deref() {
    refs--;
    if(!refs) {
        destroy();
    } 
}

void DString::
append(String s) {
    s64 offset = this->count;
    this->count += s.count;
    if(this->space < this->count+1) {
        this->space = util::round_up_to(this->count+1, 8);
        this->str = (u8*)memory::reallocate(this->str, this->space);
    }
    memory::copy(this->str+offset, s.str, s.count); 
} 

template<typename... T> void DString::
append(T... args) {
    (to_string(this, args), ...);
}

void DString::
insert(u64 offset, String s) {
    if(s && offset <= this->count) {
        s64 required_space = this->count + s.count + 1;
        if(required_space > this->space) grow(required_space - this->space);
        memory::copy(this->str + offset + s.count, this->str + offset, ((this->count - offset)+1));
        memory::copy(this->str + offset, s.str, s.count);
        this->count += s.count;
    }
}

void DString::
prepend(String s) {
    insert(0, s);
}

u64 DString::
remove(u64 offset) {
    if((offset < this->count) && !string::internal::utf8_continuation_byte(*(this->str+offset))) {
        DecodedCodepoint decoded = string::internal::decoded_codepoint_from_utf8(this->str+offset, 4);
        memory::copy(this->str+offset, this->str+offset+decoded.advance, this->count - offset);
        this->count -= decoded.advance;
        return decoded.advance;
    }
    return 0;
}

u64 DString::
remove(u64 offset, u64 count) {
    // if offset is beyond a, or we are at a continuation byte at either ends of the range
    // return 0
    if(offset > this->count || 
        string::internal::utf8_continuation_byte(*(this->str+offset)) || 
        string::internal::utf8_continuation_byte(*(this->str+offset+count))) 
            return 0;
    
    u64 actual_remove = util::Min(this->count - offset, count);
    memory::copy(this->str+offset, this->str+offset+actual_remove, this->count-actual_remove+1);

    this->count -= actual_remove;
    return actual_remove;
}

void DString::
grow(u64 bytes) {
    if(bytes) {
        this->space = util::round_up_to(this->space + bytes, 8);
        this->str = (u8*)memory::reallocate(this->str, this->space);
    }
}
} // namespace amu