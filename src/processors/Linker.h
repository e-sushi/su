#ifndef AMU_LINKER_H
#define AMU_LINKER_H

#include "processors/Processor.h"
#include "storage/String.h"

namespace amu{

struct Code;

struct Linker : Processor{
	enum Backend{
		LLVM,
		ELF,
		PE,
	};
	
	static Linker* create(Code* code, Backend backend);
	
	virtual void output(String path) = 0;
	
	// ~~~~ data ~~~~
	
	Code* code;
	Backend backend;
};

} // namespace amu

#endif // AMU_LINKER_H