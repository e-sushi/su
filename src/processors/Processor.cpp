#include "Processor.h"
#include "systems/Messenger.h"
#include "systems/Compiler.h"

namespace amu {

void Processor::
destroy() {
	TRACE(code, name, " is being destroyed.");
	diag_stack.destroy();
}

void Processor::
start() {
	TRACE()
	start = util::stopwatch::start();
}

}
