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

} // namespace amu

#endif // AMU_LEXER_H