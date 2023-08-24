/*
    Internal representation of a file that is to be compiled

    All files are compiled into modules, this just holds information relating directly to the file
*/

#ifndef AMU_SOURCE_H
#define AMU_SOURCE_H

#include "Diagnostics.h"

namespace amu {

struct Lexer;
struct Parser;
struct Module;
struct Code;
struct Source {
    FILE* file;
    
    String path;
    String name;
    String front;
    String ext;

    String buffer; // the loaded file

    Array<Token> tokens;

    Array<Diagnostic> diagnostics; // messages associated with this file

    // the module representing this Source
    Module* module;

    // representation of the actual code in this source file
    Code* code;
};

namespace source {

global Source*
load(String path);

global void
unload(Source* source);

global Source*
lookup(String name);

} // namespace source
} // namespace amu

#endif // AMU_SOURCE_H