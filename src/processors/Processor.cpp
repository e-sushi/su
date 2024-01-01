#include "Processor.h"
#include "systems/Messenger.h"
#include "systems/Compiler.h"

namespace amu {

void Processor::
init(String name_) {
	name = name_;
	diag_stack = Array<Diag>::create(8);
}

void Processor::
deinit() {
	diag_stack.destroy();
}

void Processor::
start() {
	start_time = Time::Point::now();
	DEBUG(MessageSender(), "Processor '", name, "' started");
}

void Processor::
end() {
	end_time = Time::Point::now();
	DEBUG(MessageSender(), name, " finished in ", (end_time - start_time).pretty());
}

void Processor::
push_diag(Diag diag) {
	diag_stack.push(diag);
}

Diag& Processor::
last_diag() {
	return diag_stack[-1];
}

Diag Processor::
pop_diag() {
	return diag_stack.pop();
}

}
