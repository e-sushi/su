#ifndef AMU_PE_H
#define AMU_PE_H

#include "processors/Linker.h"
#include "storage/String.h"

namespace amu{

struct PE : Linker{
	static Linker* create(Code* code);
	
	void output(String path);
	
	// ~~~~ data ~~~~
	
	
};

} // namespace amu

#endif // AMU_PE_H