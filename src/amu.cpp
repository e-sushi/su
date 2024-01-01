/* amu

*/


#include "Common.h"
#include "util.h"

// #ifdef AMU_USE_NOTCURSES
#include "notcurses/notcurses.h"
// #endif

#include "utils/Time.h"
#include "basic/Memory.h"
// #include "basic/Node.h"
// #include "storage/View.h"
// #include "storage/Pool.h"
#include "storage/Array.h"
#include "storage/DString.h"
#include "storage/String.h"
#include "storage/Map.h"

#include "systems/Compiler.h"

int main(int argc, char* argv[]){
	using namespace amu;

	auto args = Array<String>::create(argc);
	forI(argc) {
		args.push(String::from(argv[i]));
	}
	
	compiler.init();
	defer { compiler.deinit(); };

	if(compiler.begin(args)) {
		return 0;
	} else {
		return 1;
	}
}
