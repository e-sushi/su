/*
    A Token represents the smallest piece of information we may have in a given file
*/

#ifndef AMU_TOKEN_H
#define AMU_TOKEN_H

#include "kigu/common.h"
#include "Source.h"

namespace amu{

struct Token {
    String raw;
    u64 hash;

    Source* source; 
    u64 l0, l1;
    u64 c0, c1;

    u8* line_start;

    union{
        f64 f64_val;
        s64 s64_val;
        u64 u64_val;
    };

    enum Type {
        Null = 0,
        ERROR = 0,                // when something doesnt make sense during lexing
        EndOfFile,                // end of file
        
        Group_Identifier,
        Identifier = Group_Identifier,  // function, variable and struct names                 
        
        //// literal ////
        Group_Literal,
        LiteralFloat = Group_Literal,
        LiteralInteger,
        LiteralCharacter,
        LiteralString,
        
        //// control ////
        Group_Control,
        Semicolon = Group_Control, // ;
        OpenBrace,                      // {
        CloseBrace,                     // }
        OpenParen,                      // (
        CloseParen,                     // )
        OpenSquare,                     // [
        CloseSquare,                    // ]
        Comma,                          // ,
        QuestionMark,                   // ?
        Colon,                          // :
        Dot,                            // .
        At,                             // @
        Pound,                          // #
        Backtick,                       // `
        FunctionArrow,                  // ->
        
        //// operators ////
        Group_Operator,
        Plus = Group_Operator, // +
        Increment,                  // ++
        PlusAssignment,             // +=
        Negation,                   // -
        Decrement,                  // --
        NegationAssignment,         // -=
        Multiplication,             // *
        MultiplicationAssignment,   // *=
        Division,                   // /
        DivisionAssignment,         // /=
        BitNOT,                     // ~
        BitNOTAssignment,           // ~=
        BitAND,                     // &
        BitANDAssignment,           // &=
        AND,                        // &&
        BitOR,                      // |
        BitORAssignment,            // |=
        OR,                         // ||
        BitXOR,                     // ^
        BitXORAssignment,           // ^=
        BitShiftLeft,               // <<
        BitShiftLeftAssignment,     // <<=
        BitShiftRight,              // >>
        BitShiftRightAssignment,    // >>=
        Modulo,                     // %
        ModuloAssignment,           // %=
        Assignment,                 // =
        Equal,                      // ==
        LogicalNOT,                 // !
        NotEqual,                   // !=
        LessThan,                   // <
        LessThanOrEqual,            // <=
        GreaterThan,                // >
        GreaterThanOrEqual,         // >=
        Dollar,                     // $
        
        //// keywords ////
        Group_Keyword,
        Return = Group_Keyword, // return
        If,                          // if
        Else,                        // else
        For,                         // for
        While,                       // while 
        Break,                       // break
        Continue,                    // continue
        Defer,                       // defer
        StructDecl,                  // struct
        ModuleDecl,                  // module
        This,                        // this
        Using,                       // using
        As,                          // as
        Operator,                    // operator
        
        //// types  ////
        Group_Type,
        Void = Group_Type, // void
        //NOTE(sushi) the order of these entries matter, primarily for type conversion reasons. see SemanticAnalyzer::init
        Unsigned8,              // u8
        Unsigned16,             // u16
        Unsigned32,             // u32 
        Unsigned64,             // u64 
        Signed8,                // s8
        Signed16,               // s16 
        Signed32,               // s32 
        Signed64,               // s64
        Float32,                // f32 
        Float64,                // f64 
        String,                 // str
        Any,                    // any
        Struct,                 // user defined type

        //aliases, plus one extra for indicating pointers
        DataType_Void       = Void,
        DataType_Unsigned8  = Unsigned8,
        DataType_Unsigned16 = Unsigned16,
        DataType_Unsigned32 = Unsigned32,
        DataType_Unsigned64 = Unsigned64,
        DataType_Signed8    = Signed8,
        DataType_Signed16   = Signed16,
        DataType_Signed32   = Signed32,
        DataType_Signed64   = Signed64,
        DataType_Float32    = Float32,
        DataType_Float64    = Float64,
        DataType_String     = String,
        DataType_Any        = Any,
        DataType_Struct     = Struct,
        DataType_Ptr,

        //// directives ////
        Group_Directive,
        Directive_Import = Group_Directive,
        Directive_Include,
        Directive_Internal,
        Directive_Run,
    };
    Type type;
};

} // namespace amu

#endif // AMU_TOKEN_H
