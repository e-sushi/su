#pragma once
#ifndef SU_LEXER_H
#define SU_LEXER_H

#include "utils/string.h"
#include "utils/array.h"



enum Token_Type {
    Token_ERROR,              // when something doesnt make sense during lexing
    Token_EOF,                // end of file
    Token_Keyword,            // int, float, etc.
    Token_Identifier,         // function/variable names
    Token_OpenParen,          // (
    Token_CloseParen,         // )
    Token_OpenBrace,          // {
    Token_CloseBrace,         // }
    Token_Comma,              // ,
    Token_Semicolon,          // ;
    Token_Return,             // return
    Token_Literal,            // 1, soon to be any literal
    Token_Signed32,           // s32,
    Token_Signed64,           // s64, 
    Token_Unsigned32,         // u32,
    Token_Unsigned64,         // u64,
    Token_Float32,            // f32, float
    Token_Float64,            // f64, double
    Token_Assignment,         // =
    Token_Plus,               // +
    Token_Negation,           // -
    Token_Multiplication,     // *
    Token_Division,           // /
    Token_LogicalNOT,         // !
    Token_BitwiseComplement,  // ~
    Token_LessThan,           // <
    Token_GreaterThan,        // >
    Token_LessThanOrEqual,    // <=
    Token_GreaterThanOrEqual, // >=
    Token_AND,                // &&
    Token_BitAND,             // &
    Token_OR,                 // ||
    Token_BitOR,              // |
    Token_Equal,              // ==
    Token_NotEqual,           // !=
    Token_BitXOR,             // ^
    Token_BitShiftLeft,       // <<
    Token_BitShiftRight,      // >>
    Token_Modulo,             // %
    Token_QuestionMark,       // ?
    Token_Colon,              // :
    Token_If,                 // if
    Token_Else,               // else

};

static const char* tokenStrings[] = {
	"ERROR", "EOF", "Keyword", "Identifier", "Open Parentheses", "Close Parentheses",
	"Open Brace", "Close Brace", "Comma", "Semicolon", "Return", "Integer Literal",
	"Assignment", "Plus", "Negation", "Multiplication", "Division", "Logical NOT", 
	"Bitwise Compliment", "Less Than", "Greater Than", "Less Than Or Equal To",
	"Greater Than Or Equal To", "AND", "BitAND", "OR", "BitOR", "Equal", "Not Equal",
	"XOR", "Bit Shift Left", "Bit Shift Right", "Modulo", "Question Mark", "Colon",
	"if Statement", "else Statement"
};

struct token {
	string str;
	Token_Type type;
	u32 line = 0;
	//string filename = "";
};

namespace suLexer {
	array<token> lex(FILE* file);
}

#endif