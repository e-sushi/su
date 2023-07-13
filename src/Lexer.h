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
    Source* source; // source file this Analyzer belongs to 

    Array<Token> tokens;
    Array<spt> global_labels;
    Array<spt> colons; // list of colons as indexes into 'tokens'
    Array<spt> structs; // list of 'struct' keywords as indexes into 'tokens'
    Array<spt> modules;
    Array<spt> funcarrows;

    struct {
        b32 failed; // set when the lexer fails in any way 
        f64 time; // time taken by the lexer
    } status;   
};

namespace lex {

Lexer
init(Source* source);

void
deinit(Lexer& lexer);

// ask a Lexer to perform its analysis
void
execute(Lexer& lexer);

// output the data emitted by the Lexer to the given path
void
output(Lexer& lexer, String path);

} // namespace lexer

} // namespace amu

#endif // AMU_LEXER_H