/*
    Internal representation of a file that is to be compiled

    All files are compiled into modules, this just holds information relating directly to the file
*/

#ifndef AMU_SOURCE_H
#define AMU_SOURCE_H

#include "systems/Diagnostics.h"

namespace amu {

struct Module;
struct Code;

struct Source {
    FILE* file;
    
    DString path;
    String  name;
    String  front;
    String  ext;

    String buffer; // the loaded file

    // the module representing this Source
    Module* module;

    // representation of the actual code in this source file
    Code* code;

	// NOTE that the given path will be copied into a new DString so that we know we own it
	static Source*
	load(String path);

	void
	unload();

	static Source*
	lookup(String name);
};

} // namespace amu

#endif // AMU_SOURCE_H
