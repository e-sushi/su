#pragma once
#ifndef SU_LEXER_H
#define SU_LEXER_H

//#include "utils/string.h"
//#include "utils/array.h"

#include "utils/utility.h"


enum token_type {
	tok_ERROR,			  // when something doesnt make sense during lexing
	tok_EOF,			  // end of file
	tok_Keyword,		  // int, float, etc.
	tok_Identifier,		  // function/variable names
	tok_OpenParen,		  // (
	tok_CloseParen,		  // )
	tok_OpenBrace,		  // {
	tok_CloseBrace,		  // }
	tok_Comma,			  // ,
	tok_Semicolon,		  // ;
	tok_Return,			  // return
	tok_IntegerLiteral,	  // 1
	tok_Assignment,       // =
	tok_Plus,             // +
	tok_Negation,		  // -
	tok_Multiplication,   // *
	tok_Division,         // /
	tok_OpenAngle,		  // <
	tok_CloseAngle,       // >
	tok_LogicalNOT,       // !
	tok_BitwiseComplement // ~
};

static const char* tokenStrings[] = {
	"ERROR", "EOF", "Keyword", "Identifier", "Open Parentheses", "Close Parentheses",
	"Open Brace", "Close Brace", "Comma", "Semicolon", "Return", "Integer Literal",
	"Assignment", "Plus", "Negation", "Multiplication", "Division", "Open Angle Bracket", 
	"Close Angle Bracket", "Logical NOT", "Bitwise Compliment"
};

struct token {
	char head[4] = { 'H', 'e', 'a', 'd' };
	string str;
	token_type type;
	u32 line = 0;
	char tail[4] = { 'T', 'a', 'i', 'l' };
	//string filename = "";
};

namespace suLexer {
	array<token> lex(FILE* file);
}

#endif