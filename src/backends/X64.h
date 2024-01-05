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
		u64 value;
	};
	
	static Assembler* create(Code* code);
	
	void run();
	
	void output(String path);
	
	// ~~~~ data ~~~~
	
	StackValue stack[256];
	u32 stack_count;
	u8* binary;
	u32 binary_count;
	u32 binary_space;
};

} // namespace amu

#endif // AMU_X64_H