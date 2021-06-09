#pragma once
#ifndef SU_LEXER_H
#define SU_LEXER_H

//#include "utils/string.h"
//#include "utils/array.h"

#include "utils/utility.h"

#define NUMSTOPCHAR 7
#define NUMKEYWORDS 1

#define MAXBUFF 255
#define MAXTOK 500



enum token_type {
	tok_EOF,
	tok_Keyword,
	tok_Identifier,
	tok_OpenParen,
	tok_CloseParen,
	tok_OpenBrace,
	tok_CloseBrace,
	tok_Comma,
	tok_Semicolon,
	tok_Return,
	tok_IntegerLiteral
};

struct token {
	string str;
	token_type type;

	//token(const token& t) {
	//	str = string(str);
	//	type = t.type;
	//}

};

namespace suLexer {
	array<token> lex(FILE* file);
}

#endif