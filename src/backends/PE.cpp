#include "backends/PE.h"
#include "processors/Linker.h"
#include "representations/Code.h"
#include "storage/String.h"

namespace amu {

Linker* PE::
create(Code* code){
	PE* linker = (PE*)code->allocator.allocate(sizeof(PE));
	linker->code = code;
	linker->backend = Linker::Backend::PE;
	return linker;
}

void PE::
output(String path){
	
}

} // namespace amu