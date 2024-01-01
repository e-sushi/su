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

#include "utils/Time.h"

namespace amu {

struct Processor {
	String name;
	Time::Point start_time;
	Time::Point end_time;

	Array<Diag> diag_stack;
	
	void 
	init(String name);

	void
	deinit();

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

} // namespace amu


#endif
