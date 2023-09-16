namespace amu {
using namespace string;
DecodedCodepoint String::
advance(u32 n) {
    DecodedCodepoint decoded{};
    while(*this && n--){
        decoded = internal::decoded_codepoint_from_utf8(this->str, 4);
        internal::increment(*this, decoded.advance);
    }
    return decoded;
}

void String::
advance_until(u32 c){
    DecodedCodepoint decoded{};
    while(*this){
        decoded = internal::decoded_codepoint_from_utf8(this->str, 4);
        if(decoded.codepoint == c) break;
        internal::increment(*this, decoded.advance);
    }
}

void String::
advance_while(u32 c){
    DecodedCodepoint decoded{};
    while(*this){
        decoded = internal::decoded_codepoint_from_utf8(this->str, 4);
        if(decoded.codepoint != c) break;
        internal::increment(*this, decoded.advance);
    }
}

inline DecodedCodepoint String::
index(u64 n){
    return advance(n+1);
}

inline s64 String::
length(){
    s64 result = 0;
    while(advance().codepoint) result++;
    return result;
}

s64 String::
compare(String s, u64 n){
    if(this->str == s.str && this->count == s.count) return 0;
    s64 diff = 0;
    while(!diff && n-- && (*this || s)) diff = (s64)advance().codepoint - (s64)s.advance().codepoint;
    return diff;
}

b32 String::
equal(String s) {
    return this->count == s.count && !compare(s);
}

inline b32 String::
nequal(String s, u64 n){
    return compare(s, n) == 0;
}

inline b32 String::
begins_with(String s){
    if(this->str == s.str && this->count == s.count) return true;
    return this->count >= s.count && !compare(s, s.length());
}

inline b32 String::
ends_with(String s){
    if(this->str == s.str && this->count == s.count) return true;
    return this->count >= s.count && compare(String(this->str+this->count-s.count,s.count), s) == 0;
}

b32 String::
contains(String s){
    auto a = *this;
    if(this->str == s.str && this->count == s.count) return true;
    u32 s_len = s.length();
    while(a){
        if(s.count > a.count) return false;
        if(a.nequal(s, s_len)) return true;
        a.advance();
    }
    return false;
}

u32 String::
find_first(u32 codepoint){
    auto a = *this;
    u32 iter = 0;
    while(a){
        DecodedCodepoint d = a.advance();
        if(d.codepoint==codepoint) return iter;
        iter++;
    }
    return npos;
}

u32 String::
find_last(u32 codepoint) {
    auto a = *this;
    u32 iter = 0;
    while(iter < a.count){
        iter += internal::utf8_move_back(this->str+this->count-iter);
        iter++;
        if(internal::decoded_codepoint_from_utf8(this->str+this->count-iter,4).codepoint == codepoint) return this->count-iter;
    }
    return npos;
}

inline String String::
eat(u64 n){
    auto a = *this;
    a.advance(n);
    if(!a) return *this;
    return String(this->str, this->count-a.count);
}


inline String String::
eat_until(u32 c) {
    auto a = *this;
    DecodedCodepoint decoded{};
    while(a){
        decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
        if(decoded.codepoint == c) break;
        internal::increment(a, decoded.advance);
    }
    if(!a) return *this;
    return String{this->str, this->count-a.count};
}

inline String String::
eat_until_last(u32 c) {
    auto a = *this;
    s64 count = 0;
    DecodedCodepoint decoded{};
    while(a){
        decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
        if(decoded.codepoint == c) count = a.count;
        internal::increment(a, decoded.advance);
    }
    return String(this->str, this->count-a.count);
}

inline String String::
eat_until_str(String s) {
    auto a = *this;
    while(a){
        if(a.begins_with(s)) break;
        advance(a);
    }
    if(!a) return *this;
    return String{this->str, this->count-a.count};
}

inline String String::
eat_whitespace(){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = advance(a); 
        if(!isspace(dc.codepoint)) break;
        out.count += dc.advance;
    }
    return out;
}

inline String String::
eat_word(b32 include_underscore){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = advance(a);
        if(!isalnum(dc.codepoint)) break;
        if(dc.codepoint == '_' && !include_underscore) break;
        out.count += dc.advance;
    }
    return out;
}

inline String String::
eat_int(){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = advance(a);
        if(!isdigit(dc.codepoint)) break;
        out.count += dc.advance;
    }
    return out;
}

inline String String::
skip(u64 n) {
    auto a = *this;
    a.advance(n);
    return a;
}

inline String String::
skip_until(u32 c) {
    auto a = *this;
    while(a){
        DecodedCodepoint decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
        if(decoded.codepoint == c) break;
        internal::increment(a, decoded.advance);
    }
    return a;
}

inline String String::
skip_until_last(u32 c) {
    auto a = *this;
    String b{};
    while(a){
        DecodedCodepoint decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
        if(decoded.codepoint == c) b = a;
        internal::increment(a, decoded.advance);
    }
    return b;
}

inline String String::
skip_whitespace() {
    auto a = *this;
    String out = a;
    while(a){
        if(!isspace(*out.str)) break;
        out.advance();
    }
    return out;
}

// eats and returns a line
String String::
eat_line() {
    auto s = *this;
    String out = {s.str, 0};
    while(s && *s.str != '\n') {
        s.advance();
        out.count += 1;
    }
    return out;
}

Array<String> String::
find_lines() {
    auto s = *this;
    Array<String> out = Array<String>::create();
    String cur = {s.str, 0};
    while(s) {
        if(*s.str == '\n') {
            out.push(cur);
            cur = {s.str + 1, 0};
        } else cur.count++;
        s.advance();
    }
    return out;
}

Array<s32> String::
find_line_offsets(String s) {
    Array<s32> out = Array<s32>::create();
    u8* start = s.str;
    while(s) {
        if(*s.str == '\n') {
            out.push(s32(s.str-start));
        }
        s.advance();
    }
    return out;
}

} // namespace amu