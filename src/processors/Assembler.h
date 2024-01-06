#ifndef AMU_ASSEMBLER_H
#define AMU_ASSEMBLER_H

#include "processors/Processor.h"
#include "storage/String.h"

namespace amu{

struct Code;

struct Assembler : Processor{
	enum Backend{
		LLVM,
		X64,
	};
	
	static Assembler* create(Code* code, Backend backend);
	
	virtual void run() = 0;
	
	virtual void output(String path, b32 human_readable) = 0;
	
	// ~~~~ data ~~~~
	
	Code* code;
	Backend backend;
};

} // namespace amu

#endif // AMU_ASSEMBLER_H