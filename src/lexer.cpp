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
#include "core/logger.h"
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


	//keywords
const u64 kh_return     = str8_static_hash64(str8_static_t("return"));
const u64 kh_if         = str8_static_hash64(str8_static_t("if"));
const u64 kh_else       = str8_static_hash64(str8_static_t("else"));
const u64 kh_for        = str8_static_hash64(str8_static_t("for"));
const u64 kh_while      = str8_static_hash64(str8_static_t("while"));
const u64 kh_break      = str8_static_hash64(str8_static_t("break"));
const u64 kh_continue   = str8_static_hash64(str8_static_t("continue"));
const u64 kh_defer      = str8_static_hash64(str8_static_t("defer"));
const u64 kh_struct     = str8_static_hash64(str8_static_t("struct"));
const u64 kh_this       = str8_static_hash64(str8_static_t("this"));
const u64 kh_void       = str8_static_hash64(str8_static_t("void"));
const u64 kh_s8         = str8_static_hash64(str8_static_t("s8"));
const u64 kh_s16        = str8_static_hash64(str8_static_t("s16"));
const u64 kh_s32        = str8_static_hash64(str8_static_t("s32"));
const u64 kh_s64        = str8_static_hash64(str8_static_t("s64"));
const u64 kh_u8         = str8_static_hash64(str8_static_t("u8"));
const u64 kh_u16        = str8_static_hash64(str8_static_t("u16"));
const u64 kh_u32        = str8_static_hash64(str8_static_t("u32"));
const u64 kh_u64        = str8_static_hash64(str8_static_t("u64"));
const u64 kh_f32        = str8_static_hash64(str8_static_t("f32"));
const u64 kh_f64        = str8_static_hash64(str8_static_t("f64"));
const u64 kh_str        = str8_static_hash64(str8_static_t("str"));
const u64 kh_any        = str8_static_hash64(str8_static_t("any"));

//directives
const u64 kh_import     = str8_static_hash64(str8_static_t("import"));
const u64 kh_internal   = str8_static_hash64(str8_static_t("internal"));
const u64 kh_run        = str8_static_hash64(str8_static_t("run"));

local Type
token_is_keyword_or_identifier(str8 raw){DPZoneScoped;
	u64 a = str8_hash64(raw);
	switch(a){
		case kh_return:   return Token_Return;
		case kh_if:       return Token_If;
		case kh_else:     return Token_Else;
		case kh_for:      return Token_For;
		case kh_while:    return Token_While;
		case kh_break:    return Token_Break;
		case kh_continue: return Token_Continue;
		case kh_defer:    return Token_Defer;
		case kh_struct:   return Token_StructDecl;
		case kh_this:     return Token_This;
		case kh_void:     return Token_Void;
		case kh_s8:       return Token_Signed8;
		case kh_s16:      return Token_Signed16;
		case kh_s32:      return Token_Signed32;
		case kh_s64:      return Token_Signed64;
		case kh_u8:       return Token_Unsigned8;
		case kh_u16:      return Token_Unsigned16;
		case kh_u32:      return Token_Unsigned32;
		case kh_u64:      return Token_Unsigned64;
		case kh_f32:      return Token_Float32;
		case kh_f64:      return Token_Float64;
		case kh_str:      return Token_String;
		case kh_any:      return Token_Any;
	}
    return Token_Identifier;
}

local Type
token_is_directive_or_identifier(str8 raw){DPZoneScoped;
	u64 a = str8_hash64(raw);
	switch(a){
		case kh_import:   return Token_Directive_Import;
		case kh_internal: return Token_Directive_Internal;
		case kh_run:      return Token_Directive_Run;
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

LexedFile* Lexer::lex(str8 buffer){DPZoneScoped;
	Stopwatch lex_time = start_stopwatch();
	suLog(1, "Lexing...");

	str8 stream = buffer;
	u32 line_num = 1;
	u32 line_col = 1;
	u8* line_start = stream.str;

	u32 scope_depth = 0;
	u32 internal_scope_depth = -1; //set to the scope of an internal directive if it is scoped

	//need to track so that varaibles declared in stuff like for loops or functions
	//arent considered global and arent exported
	u32 paren_depth = 0; 

	suLog(2, "Beginning lex");
	while(stream){
		Token token={0};
		token.file = lexfile->file->name;
		token.l0 = line_num;
		token.c0 = line_col;
		token.scope_depth = scope_depth;
		token.idx = lexfile->tokens.count;
		token.raw.str = stream.str;
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
	 			lexfile->tokens.add(token);
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
				lexfile->tokens.add(token);
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
				lexfile->tokens.add(token);
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
			CASE1(':', Token_Colon);
			CASE1('.', Token_Dot);
			CASE1('@', Token_At);
			CASE1('#', Token_Pound);
			CASE1('`', Token_Backtick);

			case '{':{ //NOTE special for scope tracking and internals 
				token.type = Token_OpenBrace;
				//NOTE(sushi) internal directive scopes do affect scope level
				if(lexfile->tokens.count && lexfile->tokens[lexfile->tokens.count-1].type == Token_Directive_Internal){
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
						lex_error(token, "Multi-line comment has no ending */ token.");
						return 0;
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

					switch(token.type){
						case Token_Void:                    
						case Token_Signed8:                 
						case Token_Signed16:                
						case Token_Signed32:                
						case Token_Signed64:                
						case Token_Unsigned8:               
						case Token_Unsigned16:              
						case Token_Unsigned32:              
						case Token_Unsigned64:              
						case Token_Float32:                 
						case Token_Float64:                 
						case Token_String:                  
						case Token_Any:{
							//if this token is a type we can check if its a declaration and 
							//if it is, determine what kind of declaration it is 						
							if(lexfile->tokens.count && lexfile->tokens[lexfile->tokens.count-1].type == Token_Colon){
								//we know its a declaration, but we need to know if its a variable or a function
								Token* curt = &lexfile->tokens[lexfile->tokens.count-1];
								if(lexfile->tokens[lexfile->tokens.count-2].type == Token_CloseParen){
									//find where the function actually starts
									while(curt->type!=Token_OpenParen) curt--;
									curt--;

									if(!scope_depth && !paren_depth){
										lexfile->global_identifiers.add(curt->raw, {curt->raw, curt, Identifier_Function, 0});
									}else{
										lexfile->global_identifiers.add(curt->raw, {curt->raw, curt, Identifier_Function, 0});
									}
								}else{
									curt--;
									if(!scope_depth && !paren_depth){
										lexfile->global_identifiers.add(curt->raw, {curt->raw, &lexfile->tokens[lexfile->tokens.count-1], Identifier_Variable, 0});
									}else{
										lexfile->local_identifiers.add(curt->raw, {curt->raw, &lexfile->tokens[lexfile->tokens.count-1], Identifier_Variable, 0});
									}
								}
							}
						}break;        

						case Token_StructDecl:{
							//in this case we know its a declaration because thats the only reason to use this keyword
							Token* curt = &lexfile->tokens[lexfile->tokens.count-2];
							if(!scope_depth && !paren_depth){
								lexfile->global_identifiers.add(curt->raw, {curt->raw, &lexfile->tokens[lexfile->tokens.count-2], Identifier_Structure, 0});
							}else{
								lexfile->local_identifiers.add(curt->raw, {curt->raw, &lexfile->tokens[lexfile->tokens.count-2], Identifier_Structure, 0});
							}
						}
					}

					if(lexfile->tokens.count && lexfile->tokens[lexfile->tokens.count-1].type == Token_Pound){
						Type type = token_is_directive_or_identifier(token.raw);
						if(type == Token_Identifier){
							lex_error(token, "Invalid directive following #. Directive was '", token.raw, "'");
							type = Token_ERROR;
						}else{
							switch(type){
								case Token_Directive_Import:{
									lexfile->preprocessor.imports.add(lexfile->tokens.count);
								}break;
								case Token_Directive_Internal:{
									lexfile->preprocessor.internals.add(lexfile->tokens.count);
								}break;
								case Token_Directive_Run:{
									lexfile->preprocessor.runs.add(lexfile->tokens.count);
								}break;
							}

							token.type = type; 
						}
					}
				}else{
					lex_error(token, "Invalid token '",str8{stream.str, decoded_codepoint_from_utf8(stream.str, 4).advance},"'.");
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
			lexfile->tokens.add(token);
		}
		
	}

	lexfile->tokens.add(Token{Token_EOF, Token_EOF});	

	//display debug information
	if(globals.verbosity > 3){
		if(lexfile->global_identifiers.count)
			suLog(4, "Global identifiers:");
		for(Identifier id : lexfile->global_identifiers)
			suLog(4, "  ", id.alias);
		if(lexfile->local_identifiers.count)
			suLog(4, "Local identifiers");
		for(Identifier id : lexfile->local_identifiers)
			suLog(4, "  ", id.alias);
	}

	suLog(1, VTS_GreenFg, "Finished lexing in ", peek_stopwatch(lex_time), " ms", VTS_Default);
	logger_pop_indent();
	return lexfile;
}

#undef LINE_COLUMN
#undef CASE3
#undef CASE2
#undef CASE1
#undef CASEW