/*
    Type representing some contiguous source code

    This is currently achieved by just holding two Tokens, one representing end and one representing 
    an inclusive end.
*/

#include "Token.h"

namespace amu {

struct Code {
    Token start;
    Token end;
};

namespace code {

Code
create(Token start, Token end) {
    return Code{start, end};
}

} // namespace code
} // namespace amu