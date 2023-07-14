/*
    A Token represents the smallest piece of information we may have in a given file
*/

#ifndef AMU_TOKEN_H
#define AMU_TOKEN_H

#include "kigu/common.h"
#include "Source.h"

namespace amu{

namespace token {
// @genstrings(src/data/token_strings.generated)
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
    increment,                  // ++
    plus_assignment,            // +=
    negation,                   // -
    negation_assignment,        // -=
    multiplication,             // *
    multiplication_assignment,  // *=
    division,                   // /
    division_assignment,        // /=
    bit_not,                    // ~
    bit_not_assignment,         // ~=
    bit_and,                    // &
    bit_and_assignment,         // &=
    logi_and,                   // &&
    bit_or,                     // |
    bit_or_assignment,          // |=
    logi_or,                    // ||
    bit_xor,                    // ^
    bit_xor_assignment,         // ^=
    bit_shift_left,             // <<
    bit_shift_left_assignment,  // <<=
    bit_shift_right,            // >>
    bit_shift_right_assignment, // >>=
    modulo,                     // %
    modulo_assignment,          // %=
    assignment,                 // =
    equal,                      // ==
    logical_not,                // !
    not_equal,                  // !=
    less_than,                  // <
    less_than_or_equal,         // <=
    greater_than,               // >
    greater_than_or_equal,      // >=
    dollar,                     // $
    
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
    using_,                   // using
    switch_,                 // switch
    loop,                    // loop
    
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
};

#include "data/token_strings.generated"

} // namespace token

struct Token {
    String raw;
    u64 hash;

    token::kind kind;
    token::kind group;

    Source* source; 
    u64 l0, l1;
    u64 c0, c1;

    u8* line_start;

    TNode* prescan_symbol; 

    union{
        f64 f64_val;
        s64 s64_val;
        u64 u64_val;
    };
};
} // namespace amu

#endif // AMU_TOKEN_H
