#ifndef AMU_X64_H
#define AMU_X64_H

#include "Common.h"
#include "processors/Assembler.h"
#include "storage/String.h"

namespace amu{

struct Code;

struct X64 : Assembler{
	struct StackValue{
		u32 type;
		u32 reg; // register + flags
		union{
			u8  _u8;
			u16 _u16;
			u32 _u32;
			u64 _u64;
			s8  _s8;
			s16 _s16;
			s32 _s32;
			s64 _s64;
			f64 _f64;
			f32 _f32;
		};
	};
	
	static Assembler* create(Code* code);
	
	void run();
	
	void output(String path, b32 human_readable);
	
	// ~~~~ data ~~~~
	
	StackValue stack[256];
	StackValue* stack_top;
	u8* binary;
	u32 binary_count;
	u32 binary_space;
};

} // namespace amu

#endif // AMU_X64_H