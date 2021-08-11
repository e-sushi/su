#pragma once
#ifndef SU_LEXER_H
#define SU_LEXER_H

#include "utils/string.h"
#include "utils/array.h"



enum token_type {
	tok_ERROR,			    // when something doesnt make sense during lexing
	tok_EOF,			    // end of file
	tok_Keyword,		    // int, float, etc.
	tok_Identifier,		    // function/variable names
	tok_OpenParen,		    // (
	tok_CloseParen,		    // )
	tok_OpenBrace,		    // {
	tok_CloseBrace,		    // }
	tok_Comma,			    // ,
	tok_Semicolon,		    // ;
	tok_Return,			    // return
	tok_IntegerLiteral,	    // 1
	tok_Assignment,         // =
	tok_Plus,               // +
	tok_Negation,		    // -
	tok_Multiplication,     // *
	tok_Division,           // /
	tok_LogicalNOT,         // !
	tok_BitwiseComplement,  // ~
	tok_LessThan,		    // <
	tok_GreaterThan,        // >
	tok_LessThanOrEqual,	// <=
	tok_GreaterThanOrEqual, // >=
	tok_AND,                // &&
	tok_BitAND,             // &
	tok_OR,                 // ||
	tok_BitOR,              // |
	tok_Equal,              // ==
	tok_NotEqual,           // !=
	tok_BitXOR,				// ^
	tok_BitShiftLeft,		// <<
	tok_BitShiftRight,		// >>
	tok_Modulo,             // %
	tok_QuestionMark,       // ?
	tok_Colon,				// :
	tok_If,					// if
	tok_Else,				// else

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
	token_type type;
	u32 line = 0;
	//string filename = "";
};

namespace suLexer {
	array<token> lex(FILE* file);
}

#endif