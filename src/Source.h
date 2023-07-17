/*
    Internal representation of a file that is to be compiled
    This is called 'Source' to avoid conflicting with deshi's File.

    All files are compiled into modules, this just holds information relating directly to the file
*/

#ifndef AMU_SOURCE_H
#define AMU_SOURCE_H

#include "Entity.h"
#include "core/file.h"
#include "Diagnostics.h"

namespace amu {

struct Lexer;
struct Parser;
struct Source {
    File* file;
    DString buffer; // the loaded file

    Array<Diagnostic> diagnostics; // messages associated with this file

    // the module representing this Source
    Module* module;

    Lexer* lexer;
    Parser* parser;
};

} // namespace amu

#endif // AMU_SOURCE_H