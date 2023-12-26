#include "DString.h"
#include "utils/Unicode.h"
#include "systems/Messenger.h"

namespace amu{

using namespace unicode;

struct DStringHeader {
	s64 refs;
	s64 count;
	s64 space;
};

FORCE_INLINE DStringHeader*
get_header(void* ptr) {
	return ((DStringHeader*)ptr) - 1;
}

String::String(const DString& dstr) {
	auto header = get_header(dstr.ptr);
	str = (u8*)dstr.ptr;
	count = header->count;
}

DString::DString(String s) {
	s64 space = util::round_up_to(s.count, 8);
	auto header = (DStringHeader*)memory.allocate(sizeof(DStringHeader) + space);
	header->refs = 1;
	header->count = s.count;
	header->space = space;
	ptr = (u8*)(header + 1);
	memory.copy(ptr, s.str, header->count); 
}

DString::DString(const DString& x) {
	ptr = x.ptr;
	get_header(ptr)->refs += 1;
}

DString::~DString() {
	auto header = get_header(ptr);
	header->refs -= 1;
	if(!header->refs) {
		memory.free(header);
	}
}

DString DString::
copy() {
	return DString(String(*this));
}

void DString::
append(String s) {
	auto header = get_header(ptr);
    s64 offset = header->count;
	header->count += s.count;
	fit(header->count);
    memory.copy(ptr+offset, s.str, s.count); 
} 

void DString::
insert(u64 offset, String s) {
	auto header = get_header(ptr);
    if(s && offset <= header->count) {
        s64 required_space = header->count + s.count + 1;
		fit(required_space);
        memory.copy(ptr + offset + s.count, ptr + offset, ((header->count - offset)+1));
        memory.copy(ptr + offset, s.str, s.count);
        header->count += s.count;
    }
}

void DString::
prepend(String s) {
    insert(0, s);
}

u64 DString::
remove(u64 offset) {
	auto header = get_header(ptr);
    if((offset < header->count) && !is_continuation_byte(*(ptr+offset))) {
        DecodedCodepoint decoded = DecodedCodepoint(ptr+offset);
        memory.copy(ptr+offset, ptr+offset+decoded.advance, header->count - offset);
        header->count -= decoded.advance;
        return decoded.advance;
    }
    return 0;
}

u64 DString::
remove(u64 offset, u64 count) {
	auto header = get_header(ptr);
    // if offset is beyond a, or we are at a continuation byte at either ends of the range
    // return 0
    if(offset > header->count || 
        is_continuation_byte(*(ptr+offset)) || 
        is_continuation_byte(*(ptr+offset+count))) 
            return 0;
    
    u64 actual_remove = util::Min(header->count - offset, count);
    memory.copy(ptr+offset, ptr+offset+actual_remove, header->count-actual_remove+1);

    header->count -= actual_remove;
    return actual_remove;
}

void DString::
grow(u64 bytes) {
	auto header = get_header(ptr);
    if(bytes) {
        header->space = util::round_up_to(header->space + bytes, 8);
        ptr = (u8*)((DStringHeader*)memory.reallocate(header, sizeof(DStringHeader) + header->space) + 1);
    }
}

void DString::
fit(u64 bytes) {
	auto header = get_header(ptr);
	if(bytes > header->space) {
		grow(bytes - header->space);
	}
}

void DString::
indent(s64 n) {
    if(!n) return;
	auto header = get_header(ptr);
    if(n < 0) {
        NotImplemented; // :P
    } else {
        forI(n) {
            insert(0, " ");
        }
        u32 last = 0;
        while(1) {
            u32 nl = String{ptr+last+n+1, header->count-last-n-1}.find_first('\n');
            if(nl == -1) return;

            nl += last+n+2;

            if(nl == header->count) return;

            forI(n) {
                insert(nl, " ");
            }
            last = nl+n+1;
        }
    }
}

s64& DString::
space() {
	return get_header(ptr)->space;
}

s64& DString::
count() {
	return get_header(ptr)->count;
}

s64 DString::
available_space() {
	auto header = get_header(ptr);
	return header->space - header->count;
}

String DString::
get_string() {
	return String(*this);
}

#define ts(format)                                      \
	int n = snprintf(0, 0, format, y);                  \
	auto header = get_header(x.ptr);                    \
	s64 available_space = x.available_space();          \
	if(n > available_space) {                           \
		x.grow(n - available_space);                    \
	}                                                   \
	snprintf((char*)(x.ptr + x.count()), n+1, format, y); \
	x.count() += n;

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
