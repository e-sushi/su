/*
    Internal representation of a file that is to be compiled
    This is called 'Source' to avoid conflicting with deshi's File.

    All files are compiled into modules, this just holds information relating directly to the file
*/

#ifndef AMU_SOURCE_H
#define AMU_SOURCE_H

#include "Entity.h"
#include "core/file.h"

namespace amu {

struct Lexer;
struct Parser;
struct Source {
    File* file;
    DString buffer; // the loaded file

    // the module representing this Source
    Entity* module;

    Lexer* lexer;
    Parser* parser;
};

} // namespace amu

#endif // AMU_SOURCE_H