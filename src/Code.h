/*

    Interface for interacting with code
    This will provide methods for viewing, formatting, querying and manipulating plain text code using
    information supplied by other stages. 

    This may represent any level of code with any sort of information. It is a very abstract interface
    for passing around any amount and form of code.

    Code keeps track of the amount of information that is associated with the code it represents. 
*/

#ifndef AMU_CODE_H
#define AMU_CODE_H

#include "Validator.h"
#include "Generator.h"

namespace amu {

namespace code {
enum kind {
    unknown,
    token,
    range,
    label, // a label declaration and its entire entity
    structure,
    function,
    module,
    statement,
    expression,
    tuple,
    type,
    source, // represents an entire source file
};
} // namespace code

// information that would be provided from outside of a Code object
struct CodeContext {
    // the module this code would be residing in 
    Module* module;
};

struct Code {
    code::kind kind;
    // raw representation of this code
    String raw;
    
    Code* next;
    Code* prev;

    // if this Code was generated from another Code object, this points to that original
    // object. this is used when we generate Code from generic types and such and when we
    // perform formatting
    Code* base;

    // Source this code belongs to. if this is 0, then this is VirtualCode
    Source* source;

    // the highest level node of the source this Code represents
    // null if parsing has not yet been performed on this Code
    TNode* node;

    // information from stages that this Code has been passed through
    Lexer* lexer;
    Parser* parser;
    Validator* validator;
    Generator* generator;
};

// Code whose Tokens belong to some Source
struct SourceCode : public Code {
    View<Token> tokens;
};

// Code which is not represented by any given Source.
// This may be code that has been formatted, or it may be code that is generated 
// by the compiler. 
// It stores its own set of Tokens and Diagnostics
struct VirtualCode : public Code {
    String name; // unique identifier given to this virtual code
    DString str;
    Array<Token> tokens;
    Array<Diagnostic> diagnostics;
};

namespace code {

SourceCode*
from(Source* source);

VirtualCode*
from(String s);

// creates a Code object from start and end Tokens
Code*
from(Token* start, Token* end);

// create a Code object from an AST node
// if the TNode does not already mark its own start and end tokens, and 'resolve_start_end' is true
// we wil attempt to resolve these before giving the Code object
Code*
from(TNode* node, b32 resolve_start_end = false);

void
destroy(Code* code);

// transforms a Code object into a VirtualCode object
// if the given Code is already virtual, then the same object is returned
// this initializes a DString with the RString on the given Code

VirtualCode*
make_virtual(Code* code);

b32
is_virtual(Code* code);

b32
is_lexed(Code* code);

b32
is_parsed(Code* code);

String
name(Code* code);

View<Token>
get_tokens(Code* code);

// retrieve a reference to the array of tokens backing this Code
// if the Code is not Virtual, the Array comes from Source
// if it is, it comes from the local tokens array of VirtualCode
Array<Token>&
get_token_array(Code* code);

void
add_diagnostic(Code* code, Diagnostic d);

namespace virt {

} // namespace virtual

namespace util {

void
find_start_and_end(Code& code);

} // namespace util

namespace format {

// removes leading whitespace up until the min amount of whitespace over all lines
VirtualCode
remove_leading_whitespace(Code& code);

// changes the current indent width of the given Code to 'width'
VirtualCode
indent_width(Code& code, u32 width);

namespace token {

void 
replace(Code& code, Token* a, Token* b);

} // namespace token
} // namespace format

namespace lines {

struct Options {
    // remove leading tabs up until the min amount of tabs over all lines
    b32 remove_leading_whitespace : 1; 
    // display line numbers
    b32 line_numbers : 1; 
    // when line numbers have different lengths, right align them
    b32 right_align_line_numbers : 1; 
    // how many lines to gather before the given line
    u32 before; 
     // how many lines to gather after the given line
    u32 after; 
};

struct Lines {
    DString str;
    String line; // the line that this was created with
    Array<String> lines; // views into 'str', representing each gathered line
    Options opt;
    Token* token; // token originally used to retrieve these lines
};

// various methods for displaying multiple lines given some information
Lines get(Token* t, Options opt = {});
Lines get(TNode* n, Options opt = {});
template<typename T> Lines get(T* a, Options opt = {}); 

void  get(DString& start, Token* t, Options opt = {});
void  get(DString& start, TNode* n, Options opt = {});

} // namespace lines

// Code which stores a cursor as well as extra information for how to navigate the code
// struct TokenNavigator : public Code {

// };

// struct ASTNavigator : public Code {

// }

} // namespace code
} // namespace amu

#endif // AMU_CODE_H