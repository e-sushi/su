#include "Unicode.h"
#include "Common.h"

namespace amu {
namespace unicode {

b32
is_continuation_byte(u8 byte){
	return ((byte & 0xC0) == 0x80);
}

// Returns the next codepoint and advance from the UTF-8 string `str`
DecodedCodepoint::DecodedCodepoint(u8* str){
	const u8 utf8_class[32] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5, };
	const u32 bitmask[10] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF };

	u8 byte = str[0];
	u8 byte_class = utf8_class[byte >> 3];
	switch(byte_class){
		case 1:{
			codepoint = byte;
		}break;
		case 2:{
			u8 next_byte = str[1];
			if(utf8_class[next_byte >> 3] == 0){
				codepoint  = (     byte & bitmask[4]) << 6;
				codepoint |= (next_byte & bitmask[5]);
				advance = 2;
			}
		}break;
		case 3:{
			u8 next_byte[2] = {str[1], str[2]};
			if(   (utf8_class[next_byte[0] >> 3] == 0)
			   && (utf8_class[next_byte[1] >> 3] == 0)){
				codepoint  = (        byte & bitmask[3]) << 12;
				codepoint |= (next_byte[0] & bitmask[5]) << 6;
				codepoint |= (next_byte[1] & bitmask[5]);
				advance = 3;
			}
		}break;
		case 4:{
			u8 next_byte[3] = {str[1], str[2], str[3]};
			if(   (utf8_class[next_byte[0] >> 3] == 0)
			   && (utf8_class[next_byte[1] >> 3] == 0)
			   && (utf8_class[next_byte[2] >> 3] == 0)){
				codepoint  = (        byte & bitmask[2]) << 18;
				codepoint |= (next_byte[0] & bitmask[5]) << 12;
				codepoint |= (next_byte[1] & bitmask[5]) << 6;
				codepoint |=  next_byte[2] & bitmask[5];
				advance = 4;
			}
		}break;
	}
}

}
}

