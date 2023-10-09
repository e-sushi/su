/*
    Transforms an amu::Source into a collection of amu::Tokens
*/

#ifndef AMU_LEXER_H
#define AMU_LEXER_H

#include "representations/Token.h"
#include "representations/Source.h"
#include "storage/Array.h"

namespace amu {

// a 'scope' determined completely by the lexical structure of the program
// that stores a list of indexes into the tokens array where the label 
// pattern was found as well as a pointer to the last LexicalScope.
// Tokens store a pointer to the LexicalScope in which they belong.
//
// This is an attempt to solve a problem with gathering enough information
// to determine whether or not a label is global or separate enough for 
// us 
struct LexicalScope {
	LexicalScope* last;

	Array<u64> labels;
	Array<u64> imports;
};

struct Lexer {
    Code* code; 

    // indexes into code's Token Array that indicates possible global label declarations
    Array<spt> labels;
	
	Future<b32> fut;

	
	// ~~~~ interface ~~~~


    static Lexer*
    create(Code* code);

    void
    destroy();

    b32
    start();

    void
    output(b32 human, String path);
};

} // namespace amu

#endif // AMU_LEXER_H
