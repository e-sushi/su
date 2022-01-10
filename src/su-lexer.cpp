#include "su-lexer.h"
#include "utils/defines.h"
#include "utils/cstring.h"

enum LexerState_ {
	DiscernChar,
	ReadingString,

}; typedef u32 LexerState;

LexerState state = DiscernChar;

array<token> suLexer::lex(FILE* file) {
	array<token> tokens;
	char currChar = 0;
	char* buffptr = 0;
	u32 lines = 1;

	fseek(file, 0, SEEK_END);
	u32 size = ftell(file);
	buffptr = (char*)malloc(size);
	
	rewind(file);
	u32 read = fread(buffptr, 1, size, file);
	
	buffptr[read] = 0;

	cstring buff{ buffptr, read };

	char* chunk_start = 0;
#define chunksiz buff.str - chunk_start
#define chunkcmp chunk_start, chunksiz
#define chunkstr string(chunk_start, buff.str - chunk_start)

	while (buff.count) {
		switch (state) {
			case DiscernChar: {
				switch (buff[0]) {
					case ';': { tokens.add(token{ ";", Token_Semicolon,  lines }); advance(&buff); }break;
					case '{': { tokens.add(token{ "{", Token_OpenBrace,  lines }); advance(&buff); }break;
					case '}': { tokens.add(token{ "}", Token_CloseBrace, lines }); advance(&buff); }break;
					case '(': { tokens.add(token{ "(", Token_OpenParen,  lines }); advance(&buff); }break;
					case ')': { tokens.add(token{ ")", Token_CloseParen, lines }); advance(&buff); }break;
					case ',': { tokens.add(token{ ",", Token_Comma,      lines }); advance(&buff); }break;
					case '+': {
						if (buff[1] == '=') { tokens.add(token{ "+=", Token_PlusAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "+", Token_Plus, lines }); advance(&buff); }
					}break;
					case '-': {
						if (buff[1] == '=') { tokens.add(token{ "-=", Token_NegationAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "-", Token_Negation, lines }); advance(&buff); }
					}break;
					case '*': {
						if (buff[1] == '=') { tokens.add(token{ "*=", Token_MultiplicationAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "*", Token_Multiplication, lines }); advance(&buff); }
					}break;
					case '/': {
						if (buff[1] == '=') { tokens.add(token{ "/=", Token_DivisionAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "/", Token_Division, lines }); advance(&buff); }
					}break;
					case '~': {
						if (buff[1] == '=') { tokens.add(token{ "~=", Token_BitNOTAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "~", Token_BitNOT, lines }); advance(&buff); }
					}break;
					case '&': {
						if (buff[1] == '=') { tokens.add(token{ "&=", Token_BitANDAssignment, lines }); advance(&buff, 2); }
						else if (buff[1] == '&') { tokens.add(token{ "&&", Token_AND, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "&", Token_BitAND, lines }); advance(&buff); }
					}break;
					case '|': {
						if (buff[1] == '=') { tokens.add(token{ "|=", Token_BitORAssignment, lines }); advance(&buff, 2); }
						else if (buff[1] == '|') { tokens.add(token{ "||", Token_OR, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "|", Token_BitOR, lines }); advance(&buff); }
					}break;
					case '^': {
						if (buff[1] == '=') { tokens.add(token{ "^=", Token_BitXORAssignment, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "^", Token_BitXOR, lines }); advance(&buff); }
					}break;
					case '%': {
						if (buff[1] == '=') { tokens.add(token{ "%=", Token_Equal, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "%", Token_Modulo, lines }); advance(&buff); }
					}break;
					case '=': {
						if (buff[1] == '=') { tokens.add(token{ "==", Token_Equal, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "=", Token_Assignment, lines }); advance(&buff); }
					}break;
					case '!': {
						if (buff[1] == '=') { tokens.add(token{ "!=", Token_NotEqual, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "!", Token_LogicalNOT, lines }); advance(&buff); }
					}break;
					case '<': {
						if (buff[1] == '=') { tokens.add(token{ "<=", Token_LessThanOrEqual, lines }); advance(&buff, 2); }
						tokens.add(token{ "<", Token_LessThan, lines }); advance(&buff);
					}break;
					case '>': {
						if (buff[1] == '=') { tokens.add(token{ ">=", Token_GreaterThanOrEqual, lines }); advance(&buff, 2); }
						else { tokens.add(token{ ">", Token_GreaterThan, lines }); advance(&buff); }
					}break;
					case '?': { tokens.add(token{ "", Token_QuestionMark, lines }); advance(&buff); }break;
					case ':': { tokens.add(token{ "", Token_Colon, lines }); advance(&buff); }break;
					case '\n': { lines++; advance(&buff); }break;
					case '\t':
					case ' ': { advance(&buff); }break;
					default: { //if this char is none of the above cases we start looking for a worded token
						state = ReadingString;
						chunk_start = buff.str;
						advance(&buff);
					}break;
				}
			}break;
				
			case ReadingString: {
				switch (buff[0]) {
					case ';': case ' ': case '{': case '}': case '(': case ')':
					case ',': case '+': case '*': case '/': case '-': case '<': 
					case '=': case '!': case '~': case '&': case '|': case '\n':
					case '%': case ':': case '?': case '>': case '^': case '\t': {
						if (isalpha(*chunk_start)) {
							if      (!strncmp("return", chunk_start, 6)) tokens.add(token{ "return", Token_Return,     lines });
							else if (!strncmp("s32",    chunk_start, 3)) tokens.add(token{ "s32",    Token_Signed32,   lines });
							else if (!strncmp("s64",    chunk_start, 3)) tokens.add(token{ "s64",    Token_Signed64,   lines });
							else if (!strncmp("u32",    chunk_start, 3)) tokens.add(token{ "u32",    Token_Unsigned32, lines });
							else if (!strncmp("u64",    chunk_start, 3)) tokens.add(token{ "u64",    Token_Unsigned64, lines });
							else if (!strncmp("f32",    chunk_start, 3)) tokens.add(token{ "f32",    Token_Float32,    lines });
							else if (!strncmp("f64",    chunk_start, 3)) tokens.add(token{ "f64",    Token_Float64,    lines });
							else if (!strncmp("if",     chunk_start, 2)) tokens.add(token{ "if",     Token_If,         lines });
							else if (!strncmp("else",   chunk_start, 4)) tokens.add(token{ "else",   Token_Else,       lines });
							else tokens.add(token{ chunkstr, Token_Identifier, lines });
						}
						else {
							b32 valid = 1;
							forI(buff.str - chunk_start) if (!isnumber(chunk_start[i])) valid = 0;
							if (valid) tokens.add(token{ chunkstr, Token_Literal, lines });
							else tokens.add(token{ chunkstr, Token_ERROR, lines }); // TODO error out of the program here
						}
						
						if (buff[0] == '\n') lines++;
						state = DiscernChar;

					}break;
					default: { advance(&buff); }break;
				}
			}break;
		}
	}
	tokens.add(token{ "", Token_EOF, lines });
	return tokens;
}