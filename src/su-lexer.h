#pragma once
#ifndef SU_LEXER_H
#define SU_LEXER_H

//#include "utils/string.h"
//#include "utils/array.h"

#include "utils/utility.h"


enum token_type {
	tok_ERROR,			// when something doesnt make sense during lexing
	tok_EOF,			// end of file
	tok_Keyword,		// int, float, etc.
	tok_Identifier,		// function/variable names
	tok_OpenParen,		// (
	tok_CloseParen,		// )
	tok_OpenBrace,		// {
	tok_CloseBrace,		// }
	tok_Comma,			// ,
	tok_Semicolon,		// ;
	tok_Return,			// return
	tok_IntegerLiteral,	// 1
	tok_Assignment,     // =
	tok_Plus,           // +
	tok_Minus,			// -
	tok_OpenAngle,		// <
	tok_CloseAngle      // >
};

static const char* tokenStrings[] = {
	"ERROR", "EOF", "Keyword", "Identifier", "Open Parentheses", "Close Parentheses",
	"Open Brace", "Close Brace", "Comma", "Semicolon", "Return", "Integer Literal",
	"Assignment", "Plus", "Minus", "Open Angle Bracket", "Close Angle Bracket"
};

struct token {
	string str = "";
	token_type type;
	u32 line = 0;
	string filename = "";
};

namespace suLexer {
	array<token> lex(FILE* file);
}

#endif