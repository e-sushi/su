#ifdef FIX_MY_IDE_PLEASE
#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/node.h"

#define DESHI_DISABLE_IMGUI
#include "core/logger."
#include "core/platform.h"
#include "core/file.h"
#include "core/threading.h"

#include <stdio.h>
#include <stdlib.h>

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/hash.h"
#include "types.h"
#endif


constexpr u8 lexer_char_to_digit[256] = { //convert ascii characters to digits
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0, //0-47
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, //48-57 (0-9)
    0,0,0,0,0, 0,0, //58-64
    10, 11, 12, 13, 14, 15, //65-70 (A-F)
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0, //71-96
    10, 11, 12, 13, 14, 15, //97-102 (a-f)
};

//!ref: https://github.com/pervognsen/bitwise/blob/master/ion/lex.c
#define CASEW(c1,t1) \
if(equals(cstr_lit(c1), raw)) return t1

#define CASE1(c1,t1) \
case c1:{ 	         \
token.type = t1;     \
stream_next;    \
}break;

#define CASE2(c1,t1, c2,t2) \
case c1:{        \
token.type = t1;            \
stream_next;                  \
if(*stream.str == c2){          \
token.type = t2;            \
stream_next;           \
}                           \
}break;

#define CASE3(c1,t1, c2,t2, c3,t3) \
case c1:{                          \
token.type = t1;                   \
stream_next;                  \
if      (*stream.str == c3){                \
token.type = t3;                   \
stream_next;                  \
}else if(*stream.str == c2){                \
token.type = t2;                   \
stream_next;                  \
}                                  \
}break;

#define lex_error(token, ...)\
LogE("lexer", token.file, "(",token.l0,",",token.c0,"): ", __VA_ARGS__)

#define lex_warn(token, ...)\
LogW("lexer", token.file, "(",token.l0,",",token.c0,"): ", __VA_ARGS__)

#define str8case(str) str8_static_hash64(str8_static_t(str))

local Type
token_is_keyword_or_identifier(str8 raw){DPZoneScoped;
	u64 a = str8_hash64(raw);
	switch(a){
		case str8case("return"):   return Token_Return;
		case str8case("if"):       return Token_If;
		case str8case("else"):     return Token_Else;
		case str8case("for"):      return Token_For;
		case str8case("while"):    return Token_While;
		case str8case("break"):    return Token_Break;
		case str8case("continue"): return Token_Continue;
		case str8case("defer"):    return Token_Defer;
		case str8case("struct"):   return Token_StructDecl;
		case str8case("this"):     return Token_This;
		case str8case("void"):     return Token_Void;
		case str8case("s8"):       return Token_Signed8;
		case str8case("s16"):      return Token_Signed16;
		case str8case("s32"):      return Token_Signed32;
		case str8case("s64"):      return Token_Signed64;
		case str8case("u8"):       return Token_Unsigned8;
		case str8case("u16"):      return Token_Unsigned16;
		case str8case("u32"):      return Token_Unsigned32;
		case str8case("u64"):      return Token_Unsigned64;
		case str8case("f32"):      return Token_Float32;
		case str8case("f64"):      return Token_Float64;
		case str8case("str"):      return Token_String;
		case str8case("any"):      return Token_Any;
		case str8case("as"):       return Token_As;
		case str8case("using"):    return Token_Using;
	}
    return Token_Identifier;
}

//NOTE(sushi) this is separate from the keyword checker because we dont want to reserve these keywords unless they follow our directive
local Type
token_is_directive_or_identifier(str8 raw){DPZoneScoped;
	u64 a = str8_hash64(raw);
	switch(a){
		case str8case("import"):   return Token_Directive_Import;
		case str8case("internal"): return Token_Directive_Internal;
		case str8case("run"):      return Token_Directive_Run;
	}
	return Token_Identifier;
}

b32 is_white_space(u32 codepoint){
	switch(codepoint){
		case '\t': case '\n': case '\v': case '\f':  case '\r':
		case ' ': case 133: case 160: case 5760: case 8192:
		case 8193: case 8194: case 8195: case 8196: case 8197:
		case 8198: case 8199: case 8200: case 8201: case 8202:
		case 8232: case 8239: case 8287: case 12288: return true;
		default: return false;
	}
}

b32 is_digit(u32 codepoint){
	switch(codepoint){
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			return true;
		default:
			return false;
	}
}

//any unicode codepoint that is not within the ascii range, is not whitespace, or is an alphanumeric character from 
//ascii is an identifier character
b32 is_identifier_char(u32 codepoint){
	if(isalnum(codepoint))        return true;
	if(is_white_space(codepoint)) return false;
	if(codepoint == '_')          return true;
	if(codepoint > 127)           return true;
	return false;
}

#define stream_decode {                                                \
	DecodedCodepoint __dc = decoded_codepoint_from_utf8(stream.str, 4);\
	cp = __dc.codepoint;                                               \
	adv = __dc.advance;                                                \
}                                                 

#define stream_next { str8_advance(&stream); line_col++; } 

void Lexer::lex(){DPZoneScoped;
	suLogger& logger = sufile->logger; 
	logger.log(1, "Lexing...");

	Stopwatch lex_time = start_stopwatch();
	
	str8 stream = sufile->file_buffer;
	u32 line_num = 1;
	u32 line_col = 1;
	u8* line_start = stream.str;

	u32 scope_depth = 0;
	u32 internal_scope_depth = -1; //set to the scope of an internal directive if it is scoped

	//need to track so that variables declared in stuff like for loops or functions
	//arent considered global and arent exported
	u32 paren_depth = 0; 

	logger.log(2, "Beginning lex");
	while(stream){
		Token token={0};
		token.file = sufile->file->name;
		token.l0 = line_num;
		token.c0 = line_col;
		token.line_start = line_start;
		token.scope_depth = scope_depth;
		token.idx = sufile->lexer.tokens.count;
		token.raw.str = stream.str;
		if(!paren_depth && !scope_depth) token.is_global = 1;
		switch(decoded_codepoint_from_utf8(stream.str, 4).codepoint){
			case '\t': case '\n': case '\v': case '\f':  case '\r':
			case ' ': case 133: case 160: case 5760: case 8192:
			case 8193: case 8194: case 8195: case 8196: case 8197:
			case 8198: case 8199: case 8200: case 8201: case 8202:
			case 8232: case 8239: case 8287: case 12288:{
				while(is_white_space(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
					if(*stream.str == '\n'){
						line_start = stream.str;
						line_num++;
						line_col = 0;
					};
					stream_next;
				}
			}continue;

			case '0': case '1': case '2': case '3': case '4': 
			case '5': case '6': case '7': case '8': case '9':{
				while(is_digit(*stream.str) || *stream.str == '_') stream_next;
				if(*stream.str == '.' || *stream.str == 'e' || *stream.str == 'E'){
					stream_next;
					while(isdigit(*stream.str)){ stream_next; } //skip to non-digit
					token.raw.count = stream.str - token.raw.str;
					token.type        = Token_LiteralFloat;
					token.f64_val = stod(token.raw); 
				}else if(*stream.str == 'x' || *stream.str == 'X'){
					stream_next;
					while(isxdigit(*stream.str)){ stream_next; } //skip to non-hexdigit
					token.raw.count = stream.str - token.raw.str;
					token.type      = Token_LiteralInteger;
					token.s64_val   = stolli(token.raw);
				}else{
					token.raw.count = stream.str - token.raw.str;
					token.type      = Token_LiteralInteger;
					token.s64_val   = stolli(token.raw); 
				}

				token.l1 = line_num;
	 			token.c1 = line_col;
	 			sufile->lexer.tokens.add(token);
			}continue;

			case '\'':{
				token.type  = Token_LiteralCharacter;
				token.group = TokenGroup_Literal;
				stream_next;
				
				while(*stream.str != '\'') {//skip until closing single quotes
					stream_next; 
					if(*stream.str == 0){
						lex_error(token, "Unexpected EOF when lexing single quotes");
					}
				}
				
				token.l1 = line_num;
				token.c1 = line_col;
				token.raw.count = stream.str - (++token.raw.str); //dont include the single quotes
				sufile->lexer.tokens.add(token);
				stream_next;
			}continue; //skip token creation b/c we did it manually

			case '"':{
				token.type  = Token_LiteralString;
				token.group = TokenGroup_Literal;
				stream_next;
				
				while(*stream.str != '"') stream_next; //skip until closing double quotes
				
				token.l1 = line_num;
				token.c1 = line_col;
				token.raw.count = stream.str - (++token.raw.str); //dont include the double quotes
				sufile->lexer.tokens.add(token);
				stream_next;
			}continue; //skip token creation b/c we did it manually
			
			CASE1(';', Token_Semicolon);
			case '(':{ 
				token.type = Token_OpenParen;
				paren_depth++;
				stream_next;
			}break;
			case ')':{ 
				token.type = Token_CloseParen; 
				paren_depth--;
				stream_next;
			}break;

			CASE1('[', Token_OpenSquare);
			CASE1(']', Token_CloseSquare);
			CASE1(',', Token_Comma);
			CASE1('?', Token_QuestionMark);
			case ':':{ //NOTE special for declarations
				token.type = Token_Colon; 
				sufile->lexer.declarations.add(sufile->lexer.tokens.count);
				stream_next; 
			}break;
			CASE1('.', Token_Dot);
			CASE1('@', Token_At);
			CASE1('#', Token_Pound);
			CASE1('`', Token_Backtick);

			case '{':{ //NOTE special for scope tracking and internals 
				token.type = Token_OpenBrace;
				//NOTE(sushi) internal directive scopes do affect scope level
				if(sufile->lexer.tokens.count && sufile->lexer.tokens[sufile->lexer.tokens.count-1].type == Token_Directive_Internal){
					internal_scope_depth = scope_depth;
				}else{
					scope_depth++;
				}
				stream_next;
			}break;
			
			case '}':{ //NOTE special for scope tracking and internals
				token.type = Token_CloseBrace;
				if(scope_depth == internal_scope_depth){
					internal_scope_depth = -1;
				}else{
					scope_depth--;
				}
				stream_next;
			}break;
			
			//// @operators ////
			CASE3('+', Token_Plus,            '=', Token_PlusAssignment,      '+', Token_Increment);
			CASE3('-', Token_Negation,        '=', Token_NegationAssignment,  '-', Token_Decrement);
			CASE2('*', Token_Multiplication,  '=', Token_MultiplicationAssignment);
			CASE2('%', Token_Modulo,          '=', Token_ModuloAssignment);
			CASE2('~', Token_BitNOT,          '=', Token_BitNOTAssignment);
			CASE3('&', Token_BitAND,          '=', Token_BitANDAssignment,    '&', Token_AND);
			CASE3('|', Token_BitOR,           '=', Token_BitORAssignment,     '|', Token_OR);
			CASE2('^', Token_BitXOR,          '=', Token_BitXORAssignment);
			CASE2('=', Token_Assignment,      '=', Token_Equal);
			CASE2('!', Token_LogicalNOT,      '=', Token_NotEqual);

			case '/':{ //NOTE special because of comments
				token.type = Token_Division;
				stream_next;
				if(*stream.str == '='){
					token.type = Token_DivisionAssignment;
					stream_next;
				}else if(*stream.str == '/'){
					while(stream && *stream.str != '\n') stream_next; //skip single line comment
					continue; //skip token creation
				}else if(*stream.str == '*'){
					while((stream.count > 1) && !(stream.str[0] == '*' && stream.str[1] == '/')){ stream_next; } //skip multiline comment
					if(stream.count <= 1 && *(stream.str-1) != '/' && *(stream.str-2) != '*'){
						logger.error(&token, "Multi-line comment has no ending */ token.");
						return;
					}
					stream_next; stream_next;
					continue; //skip token creation
				}
			}break;
		
			case '<':{ //NOTE special because of bitshift assignment
				token.type = Token_LessThan;
				stream_next;
				if      (*stream.str == '='){
					token.type = Token_LessThanOrEqual;
					stream_next;
				}else if(*stream.str == '<'){
					token.type = Token_BitShiftLeft;
					stream_next;
					if(*stream.str == '='){
						token.type = Token_BitShiftLeftAssignment;
						stream_next;
					}
				}
			}break;
			
			case '>':{ //NOTE special because of bitshift assignment
				token.type = Token_GreaterThan;
				stream_next;
				if      (*stream.str == '='){
					token.type = Token_GreaterThanOrEqual;
					stream_next;
				}else if(*stream.str == '>'){
					token.type = Token_BitShiftRight;
					stream_next;
					if(*stream.str == '='){
						token.type = Token_BitShiftRightAssignment;
						stream_next;
					}
				}
			}break;
			
			default:{
				if(is_identifier_char(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
				  	while(is_identifier_char(decoded_codepoint_from_utf8(stream.str, 4).codepoint)) 
						stream_next; //skip until we find a non-identifier char

                	token.raw.count = stream.str - token.raw.str;
                	token.type = token_is_keyword_or_identifier(token.raw);

					if(sufile->lexer.tokens.count && sufile->lexer.tokens[sufile->lexer.tokens.count-1].type == Token_Pound){
						Type type = token_is_directive_or_identifier(token.raw);
						if(type == Token_Identifier){
							logger.error(&token, "Invalid directive following #. Directive was '", token.raw, "'");
							type = Token_ERROR;
						}else{
							switch(type){
								case Token_Directive_Import:{
									sufile->lexer.imports.add(sufile->lexer.tokens.count);
								}break;
								case Token_Directive_Internal:{
									sufile->lexer.internals.add(sufile->lexer.tokens.count);
								}break;
								case Token_Directive_Run:{
									sufile->lexer.runs.add(sufile->lexer.tokens.count);
								}break;
							}
							token.type = type; 
						}
					}
				}else{
					logger.error(&token, "Invalid token '",str8{stream.str, decoded_codepoint_from_utf8(stream.str, 4).advance},"'.");
					token.type = Token_ERROR;
					stream_next;
				}
			}break;
		}

			//set token's group
		if(token.type == Token_Identifier){
			token.group = TokenGroup_Identifier;
		}else if(token.type >= Token_LiteralFloat && token.type <= Token_LiteralString){
			token.group = TokenGroup_Literal;
		}else if(token.type >= Token_Semicolon && token.type <= Token_Backtick){
			token.group = TokenGroup_Control;
		}else if(token.type >= Token_Plus && token.type <= Token_GreaterThanOrEqual){
			token.group = TokenGroup_Operator;
		}else if(token.type >= Token_Return && token.type <= Token_StructDecl){
			token.group = TokenGroup_Keyword;
		}else if(token.type >= Token_Void && token.type <= Token_Struct){
			token.group = TokenGroup_Type;
		}else if(token.type >= Token_Directive_Import && token.type <= Token_Directive_Run){
			token.group = TokenGroup_Directive;
		}
		
		//set token end
		if(token.type != Token_ERROR){
			token.l1 = line_num;
			token.c1 = line_col;
			token.raw.count = stream.str - token.raw.str;
			sufile->lexer.tokens.add(token);
		}
	}

	sufile->lexer.tokens.add(Token{Token_EOF, Token_EOF});	
	logger.log(1, "Finished lexing in ", peek_stopwatch(lex_time), " ms", VTS_Default);
}

#undef LINE_COLUMN
#undef CASE3
#undef CASE2
#undef CASE1
#undef CASEW