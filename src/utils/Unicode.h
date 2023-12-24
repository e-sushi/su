/*

   Utils for dealing with unicode

*/


#ifndef AMU_UNICODE_H
#define AMU_UNICODE_H

#include "Common.h"

namespace amu {
namespace unicode {

struct DecodedCodepoint{
	u32 codepoint;
	u32 advance;
	
	DecodedCodepoint() {*this = {};}
	DecodedCodepoint(u8* str);
};

b32
is_continuation_byte(u8 byte);

} // namespace unicode
} // namespace amu

#endif
