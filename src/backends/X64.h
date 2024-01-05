#ifndef AMU_X64_H
#define AMU_X64_H

#include "Common.h"
#include "processors/Assembler.h"
#include "storage/Array.h"
#include "storage/String.h"

namespace amu{

struct Code;

struct X64 : Assembler{
	static Assembler* create(Code* code);
	
	void run();
	
	void output(String path);
	
	// ~~~~ data ~~~~
	
	Array<u8> binary;
};

} // namespace amu

#endif // AMU_X64_H