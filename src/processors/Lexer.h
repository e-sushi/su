/*
    Transforms an amu::Source into a collection of amu::Tokens
*/

#ifndef AMU_LEXER_H
#define AMU_LEXER_H

#include "representations/Token.h"
#include "representations/Source.h"
#include "storage/Array.h"
#include "processors/Processor.h"
#include "basic/Allocator.h"

namespace amu {

// a 'scope' determined completely by the lexical structure of the program
// that stores a list of indexes into the tokens array where the label 
// pattern was found as well as a pointer to the last LexicalScope.
// Tokens store a pointer to the LexicalScope in which they belong.
//
// This is an attempt to solve a problem with gathering enough information
// to determine whether or not a label is global or separate enough for 
// us 
//
// The indexes of in each array are offsets from the token that begins 
// the LexicalScope
struct LexicalScope {
	LexicalScope* last;
	
	// index into the tokens array this LexicalScope belongs to
	// indicating where it starts 
	u64 token_start;
	u64 token_end;

	Array<u64> labels;
	Array<u64> imports;
};


struct Lexer : public Processor {
	Code* code;
	Array<Token> tokens;
	

	// ~~~~ interface ~~~~


    static Lexer* create(Allocator* allocator, Code* code);

    void destroy();

    b32 run();

    void output(b32 human, String path);
};

} // namespace amu

#endif // AMU_LEXER_H
