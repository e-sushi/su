/*
    Transforms an amu::Source into a collection of amu::Tokens
*/

#ifndef AMU_LEXER_H
#define AMU_LEXER_H

#include "Token.h"
#include "Source.h"
#include "storage/Array.h"

namespace amu {

struct Lexer {
    Code* code; 

    struct {
        f64 time; // time taken by the lexer
    } status;   
};

namespace lex {

Lexer
init();

void
deinit(Lexer& lexer);

// ask a Lexer to perform its analysis, storing information
// in the provided Code object
void
execute(Code* code);

// output the data emitted by the Lexer to the given path
// binary = false - outputs a human readable format
void
output(Code* code, b32 human, String path);

} // namespace lexer

} // namespace amu

#endif // AMU_LEXER_H