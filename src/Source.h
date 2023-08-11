/*
    Internal representation of a file that is to be compiled
    This is called 'Source' to avoid conflicting with deshi's File.

    All files are compiled into modules, this just holds information relating directly to the file
*/

#ifndef AMU_SOURCE_H
#define AMU_SOURCE_H

#include "Diagnostics.h"


namespace amu {

struct Lexer;
struct Parser;
struct Module;
struct Source {
    FILE* file;
    
    DString path; // NOTE(sushi) DString because we need to copy the path given by std::filesystem
    String name;
    String front;
    String ext;

    DString buffer; // the loaded file

    Array<Diagnostic> diagnostics; // messages associated with this file

    // the module representing this Source
    Module* module;

    Lexer* lexer;
    Parser* parser;
};

namespace source {

global Source*
load(String path);

global Source*
lookup(String name);

} // namespace source
} // namespace amu

#endif // AMU_SOURCE_H