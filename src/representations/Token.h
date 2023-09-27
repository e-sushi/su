/*
    A Token represents the smallest piece of information we may have in a given file
*/

#ifndef AMU_TOKEN_H
#define AMU_TOKEN_H

#include "Source.h"
#include "Code.h"
#include "storage/String.h"
#include "storage/DString.h"

namespace amu{

namespace token {
// @genstrings(data/token_strings.generated, S/group_.*//, S/_$//)
enum kind : u32 { 
    null = 0,
    error = 0,                // when something doesnt make sense during lexing
    end_of_file,                // end of file

    identifier,  // function, variable and struct names                 
    
    //// literal ////
    group_literal,
    literal_float = group_literal,
    literal_integer,
    literal_character,
    literal_string,
    
    //// control ////
    group_control,
    semicolon = group_control, // ;
    open_brace,                // {
    close_brace,               // }
    open_paren,                // (
    close_paren,               // )
    open_square,               // [
    close_square,              // ]
    comma,                     // ,
    question_mark,             // ?
    colon,                     // :
    dot,                       // .
    range,                     // ..
    ellipsis,                  // ...
    at,                        // @
    pound,                     // #
    backtick,                  // `
    function_arrow,            // ->
    match_arrow,               // =>
    
    //// operators ////
    group_operator,
    plus = group_operator,      // +
    plus_equal,                 // +=
    minus,                      // -
    minus_equal,                // -=
    asterisk,                   // *
    asterisk_assignment,        // *=
    solidus,                    // /
    solidus_assignment,         // /=
    tilde,                      // ~
    tilde_assignment,           // ~=
    ampersand,                  // &
    ampersand_assignment,       // &=
    double_ampersand,           // && or 'and'
    vertical_line,              // |
    vertical_line_equals,       // |=
    logi_or,                    // || or 'or'
    caret,                      // ^
    caret_equal,                // ^=
    double_less_than,           // <<
    double_less_than_equal,     // <<=
    double_greater_than,        // >>
    double_greater_than_equal,  // >>=
    percent,                    // %
    percent_equal,              // %=
    equal,                      // =
    double_equal,               // ==
    explanation_mark,           // !
    explanation_mark_equal,     // !=
    less_than,                  // <
    less_than_equal,            // <=
    greater_than,               // >
    greater_than_equal,         // >=
    dollar,                     // $
    double_dollar,              // $$
    
    //// keywords ////
    group_keyword,
    return_ = group_keyword, // return
    if_,                     // if
    else_,                   // else
    for_,                    // for
    while_,                  // while 
    break_,                  // break
    continue_,               // continue
    defer_,                  // defer
    structdecl,              // struct
    moduledecl,              // module
    using_,                  // using
    switch_,                 // switch
    loop,                    // loop
    in,                      // in
    then,                    // then
    
    //// types  ////
    group_type,
    void_ = group_type, // void
    //NOTE(sushi) the order of these entries matter, primarily for type conversion reasons. see SemanticAnalyzer::init
    unsigned8,              // u8
    unsigned16,             // u16
    unsigned32,             // u32 
    unsigned64,             // u64 
    signed8,                // s8
    signed16,               // s16 
    signed32,               // s32 
    signed64,               // s64
    float32,                // f32 
    float64,                // f64 
    string,                 // str
    any,                    // any
    struct_,                // user defined type

    //// directives ////
    group_directive,
    directive_import = group_directive,
    directive_include,
    directive_internal,
    directive_run,
    directive_compiler_break,
    directive_print_type,
    directive_print_meta_type,
    directive_compiler_break_air_gen,
    directive_vm_break,
	directive_rand_int,

	// svar_file, // $file
	// svar_line, // $line
	// intrinsic printing function 
	sid_print, // $print
};

#include "data/token_strings.generated"

} // namespace token

struct Token {
    String raw;
    u64 hash;

    token::kind kind; 
    token::kind group;

    Code* code; 
    u64 l0, l1;
    u64 c0, c1;
    
    // TODO(sushi) replace this union with Literal and setup Lexer to detect 
    //             other literals such as the various sizes of scalars 
    union{
        f64 f64_val;
        s64 s64_val;
        u64 u64_val;
    };
};

FORCE_INLINE void 
to_string(DString* start, Token* t) { 
    start->append("Token<", t->code->identifier, ":", t->l0, ":", t->c0, " '", t->raw, "'>");
}

DString* 
to_string(Token* t) { 
    auto out = DString::create();
    to_string(out, t);
    return out; 
}

} // namespace amu

#endif // AMU_TOKEN_H
