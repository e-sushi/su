#include "backends/X64.h"
#include "processors/Assembler.h"

namespace amu{

Assembler* Assembler::
create(Code* code, Assembler::Backend backend){
	switch(backend){
		case Assembler::Backend::X64: return X64::create(code);
	}
	StaticAssert(false);
	return 0;
}

} // namespace amu