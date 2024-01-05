#include "DString.h"
#include "utils/Unicode.h"
#include "systems/Compiler.h"
#include "systems/Messenger.h"
#include "Bump.h"

namespace amu{

using namespace unicode;

Bump allocator;

void DString::
init_allocator() {
	allocator.init();
}

void DString::
deinit_allocator() {
	allocator.deinit();
}

String::String(const DString& dstr) {
	str = dstr.str;
	count = dstr.count;
}

DString::DString(String s) {
	str = (u8*)allocator.allocate(s.count);
	count = s.count;
	space = util::round_up_to(count, 8);
}

DString DString::
copy() {
	return DString(String(*this));
}

void DString::
append(String s) {
    s64 offset = count;
	count += s.count;
	fit(count);
    memory.copy(str+offset, s.str, s.count); 
} 

void DString::
insert(u64 offset, String s) {
    if(s && offset <= count) {
        s64 required_space = count + s.count + 1;
		fit(required_space);
        memory.copy(str + offset + s.count, str + offset, ((count - offset)+1));
        memory.copy(str + offset, s.str, s.count);
        count += s.count;
    }
}

void DString::
prepend(String s) {
    insert(0, s);
}

u64 DString::
remove(u64 offset) {
    if((offset < count) && !is_continuation_byte(*(str+offset))) {
        DecodedCodepoint decoded = DecodedCodepoint(str+offset);
        memory.copy(str+offset, str+offset+decoded.advance, count - offset);
        count -= decoded.advance;
        return decoded.advance;
    }
    return 0;
}

u64 DString::
remove(u64 offset, u64 count) {
    // if offset is beyond a, or we are at a continuation byte at either ends of the range
    // return 0
    if(offset > count || 
        is_continuation_byte(*(str+offset)) || 
        is_continuation_byte(*(str+offset+count))) 
            return 0;
    
    u64 actual_remove = util::Min(count - offset, count);
    memory.copy(str+offset, str+offset+actual_remove, count-actual_remove+1);

    count -= actual_remove;
    return actual_remove;
}

void DString::
grow(u64 bytes) {
    if(bytes) {
        space = util::round_up_to(space + bytes, 8);
		str = (u8*)allocator.reallocate(str, count, space);
    }
}

void DString::
fit(u64 bytes) {
	if(bytes > space) {
		grow(bytes - space);
	}
}

void DString::
indent(s64 n) {
    if(!n) return;
    if(n < 0) {
        NotImplemented; // :P
    } else {
        forI(n) insert(0, " ");

        u32 last = 0;
        while(1) {
            u32 nl = String{str+last+n+1, count-last-n-1}.find_first('\n');
            if(nl == -1) return;

            nl += last+n+2;

            if(nl == count) return;

            forI(n) {
                insert(nl, " ");
            }
            last = nl+n+1;
        }
    }
}

String DString::
get_string() {
	return String(*this);
}

s64 DString::
available_space() {
	return space - count;
}

#define ts(format)                                      \
	int n = snprintf(0, 0, format, y);                  \
	s64 available_space = x.available_space();          \
	if(n > available_space) {                           \
		x.grow(n - available_space);                    \
	}                                                   \
	snprintf((char*)(x.str + x.count), n+1, format, y); \
	x.count += n;

void to_string(DString& x, const char* y)   { x.append(String::from(y)); }
void to_string(DString& x, const String y)  { x.append(y); }
void to_string(DString& x, const DString y) { x.append(String(y)); }
void to_string(DString& x, const u8 y)      { ts("%hhu"); }
void to_string(DString& x, const u16 y)     { ts("%hu"); }
void to_string(DString& x, const u32 y)     { ts("%u"); }
void to_string(DString& x, const u64 y)     { ts("%llu"); }
void to_string(DString& x, const s8 y)      { ts("%hhi"); }
void to_string(DString& x, const s16 y)     { ts("%hi"); }
void to_string(DString& x, const s32 y)     { ts("%i"); }
void to_string(DString& x, const s64 y)     { ts("%lli"); }
void to_string(DString& x, const f32 y)     { ts("%f"); }
void to_string(DString& x, const f64 y)     { ts("%f"); }
void to_string(DString& x, const void* y)   { ts("%p"); }

} // namespace amu
