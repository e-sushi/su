/*
    Transforms an amu::Source into a collection of amu::Tokens
*/

#include "Token.h"
#include "Source.h"
#include "storage/Array.h"

namespace amu {

struct LexicalAnalyzer {
    Source* source; // source file this Analyzer belongs to 

    Array<Token> tokens;
    Array<spt> colons; // list of colons foundas indexes into 'tokens'  

    struct {
        b32 failed; // set when the lexer fails in any way 


        f64 time; // time taken by the lexer
    } status;   
};

namespace lex {

LexicalAnalyzer
init(Source* source);

void
deinit(LexicalAnalyzer& lexer);

// ask a LexicalAnalyzer to perform its analysis
void
analyze(LexicalAnalyzer& lexer);

} // namespace lexer

} // namespace amu