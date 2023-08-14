namespace amu{
namespace dstring {

DString
init(String s) {
    DString out;
    out.count = s.count;
    out.space = util::round_up_to(s.count, 8);
    out.str = (u8*)memory::allocate(out.space); 
    if(s.str) memcpy(out.str, s.str, s.count);
    return out;
}

template<typename... T> FORCE_INLINE DString
init(T...args) {
    DString out = dstring::init();
    (to_string(out, args), ...);
    return out;
}

void
deinit(DString& s) {
    memory::free(s.str);
    memory::zero(&s,sizeof(DString));
}

void
grow(DString& s, u64 bytes) {
    if(bytes) {
        s.space = util::round_up_to(s.space + bytes, 8);
        s.str = (u8*)memory::reallocate(s.str, s.space);
    }
}

// appends 'b' to 'a'
void
append(DString& a, String b) {
    s64 offset = a.count;
    a.count += b.count;
    if(a.space < a.count+1) {
        a.space = util::round_up_to(a.count+1, 8);
        a.str = (u8*)memory::reallocate(a.str, a.space);
    }
    memory::copy(a.str+offset, b.str, b.count); 
}

template<typename... T> void
append(DString& a, T... args) {
    (to_string(a, args), ...);
}

global void
insert(DString& a, u64 offset, String b) {
    if(b && offset <= a.count) {
        s64 required_space = a.count + b.count + 1;
        if(required_space > a.space) grow(a, required_space - a.space);
        memory::copy(a.str + offset + b.count, a.str + offset, ((a.count - offset)+1));
        memory::copy(a.str + offset, b.str, b.count);
        a.count += b.count;
    }
}

void
prepend(DString& a, String b) {
    insert(a, 0, b);
}

// concatenates a String to a String and returns a new String
DString
concat(DString& a, String b) {
    DString out = init();
    append(out, b);
    return out;
}

global u64
remove(DString& a, u64 offset) {
    if((offset < a.count) && !string::internal::utf8_continuation_byte(*(a.str+offset))) {
        DecodedCodepoint decoded = string::internal::decoded_codepoint_from_utf8(a.str+offset, 4);
        memory::copy(a.str+offset, a.str+offset+decoded.advance, a.count - offset);
        a.count -= decoded.advance;
        return decoded.advance;
    }
    return 0;
}

global u32
remove(DString& a, u64 offset, u64 count) {
    // if offset is beyond a, or we are at a continuation byte at either ends of the range
    // return 0
    if(offset > a.count || 
        string::internal::utf8_continuation_byte(*(a.str+offset)) || 
        string::internal::utf8_continuation_byte(*(a.str+offset+count))) 
            return 0;
    
    u64 actual_remove = util::Min(a.count - offset, count);
    memory::copy(a.str+offset, a.str+offset+actual_remove, a.count-actual_remove+1);

    a.count -= actual_remove;
    return actual_remove;
}

} // namespace string
} // namespace amu