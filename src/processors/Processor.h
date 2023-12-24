/*

 	Basic functionalities shared by each processor.

*/

#ifndef AMU_PROCESSOR_H
#define AMU_PROCESSOR_H

#include "Common.h"
#include "util.h"
#include "storage/String.h"
#include "storage/Array.h"
#include "systems/Diagnostics.h"

namespace amu {

struct Processor {
	String name;
	util::Stopwatch time_start;
	util::Stopwatch time_end;

	Array<Diag> diag_stack;
	
	Processor(String name) : name(name) {
		diag_stack = Array<Diag>::create();
	}

	void
	destroy();

	void
	start();

	void
	end();

	void
	push_diag(Diag diag);

	Diag&
	last_diag();

	Diag
	pop_diag();
};


}


#endif
