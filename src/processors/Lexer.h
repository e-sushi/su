/*
    Transforms an amu::Source into a collection of amu::Tokens
*/

#ifndef AMU_LEXER_H
#define AMU_LEXER_H

#include "representations/Token.h"
#include "representations/Source.h"
#include "storage/Array.h"

namespace amu {

struct Lexer {
    Code* code; 

    // indexes into code's Token Array that indicates possible global label declarations
    Array<spt> labels;


    static Lexer*
    create(Code* code);

    void
    destroy();

    void
    start();

    void
    output(b32 human, String path);
};

namespace lex {

// Lexer*
// create();

// void
// destroy(Lexer* lexer);

// // ask a Lexer to perform its analysis, storing information
// // in the provided Code object
// void
// execute(Code* code);

// // output the data emitted by the Lexer to the given path
// // human: true to emit a human readable format, false to emit binary data
// void
// output(Code* code, b32 human, String path);

} // namespace lexer
} // namespace amu

#endif // AMU_LEXER_H