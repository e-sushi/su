#include "backends/PE.h"
#include "processors/Linker.h"

namespace amu{

Linker* Linker::
create(Code* code, Linker::Backend backend){
	switch(backend){
		case Linker::Backend::PE: return PE::create(code);
	}
	StaticAssert(false);
	return 0;
}

} // namespace amu