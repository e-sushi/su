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
case c1:{ZoneScoped;          \
token.type = t1; \
stream++;        \
}break;

#define CASE2(c1,t1, c2,t2) \
case c1:{ZoneScoped;      \
token.type = t1;        \
stream++;               \
if(*stream == c2){      \
token.type = t2;      \
stream++;             \
}                       \
}break;

#define CASE3(c1,t1, c2,t2, c3,t3) \
case c1:{ ZoneScoped;                       \
token.type = t1;               \
stream++;                      \
if      (*stream == c3){       \
token.type = t3;             \
stream++;                    \
}else if(*stream == c2){       \
token.type = t2;             \
stream++;                    \
}                              \
}break;

#define LINE_COLUMN ((stream.str - line_start)+1)

#define lexer_report_error(token, fmt, ...)\
printf("%.*s(%d,%d): Error: " fmt, int(token.file.count), token.file.str, token.l0, token.c0, ##__VA_ARGS__)

//TODO maybe speed this up with hashing/layering



enum keyword_hashes{
	kh_return     = 2085702151,
	kh_if         = 1545789136,
	kh_else       = 3419725518,
	kh_for        = 856556216,
	kh_while      = 152936142,
	kh_break      = 2718502296,
	kh_continue   = 2910489658,
	kh_defer      = 1815187539,
	kh_struct     = 3948182702,
	kh_this       = 683897725,
	kh_void       = 1243292671,
	kh_s8         = 1325482204,
	kh_s16        = 3315202241,
	kh_s32        = 1760205235,
	kh_s64        = 4080335092,
	kh_u8         = 1291926966,
	kh_u16        = 3348757479,
	kh_u32        = 1860870949,
	kh_u64        = 4046779854,
	kh_f32        = 2078979996,
	kh_f64        = 3761560331,
	kh_str        = 802133312,
	kh_any        = 1488259997,
};

local Token_Type
token_is_keyword_or_identifier(cstring raw){
	ZoneScoped;
	u32 a = hash<cstring>()(raw);
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

b32 lex_file(cstring filename, const string& file){
	ZoneScoped;
	lexer.file_index.add(filename);
	LexedFile& lfile = lexer.file_index[filename];
	
	array<u32> identifier_indexes;
	array<u32> global_identifiers;
	array<cstring> struct_names;
	
	cstring stream{file.str, file.count};
	u32 line_number = 1;
	u32 line_count_non_blank = 0; //TODO count number of non-comment and non-empty lines
	u32 scope_number = 0;
	char* line_start = stream.str;
	while(stream && *stream != '\0'){
		ZoneScoped;
		//set token start
		Token token{};
		token.file = filename;
		token.l0   = line_number;
		token.c0   = LINE_COLUMN;
		token.raw.str = stream.str;
		
		switch(*stream){
			//// @whitespace ////
			case ' ': case '\n': case '\r': case '\t': case '\v':{
				ZoneScoped;
				while(isspace(*stream)){
					if(*stream == '\n'){
						line_start = stream.str;
						line_number++;
					}
					stream++;
				}
			}continue; //skip token creation
			
			//// @literals ////
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':{
				ZoneScoped;
				while(isdigit(*stream) || *stream == '_'){ stream++; } //skip to non-digit (excluding underscore)
				if(*stream == '.' || *stream == 'e' || *stream == 'E'){
					stream++;
					while(isdigit(*stream)){ stream++; } //skip to non-digit
					token.raw.count = stream.str - token.raw.str;
					token.type        = Token_LiteralFloat;
					token.float_value = stod(token.raw); //NOTE this replaces token.raw since they are unioned
				}else if(*stream == 'x' || *stream == 'X'){
					stream++;
					while(isxdigit(*stream)){ stream++; } //skip to non-hexdigit
					token.raw.count = stream.str - token.raw.str;
					token.type = Token_LiteralInteger;
					token.int_value = b16tou64(token.raw); //NOTE this replaces token.raw since they are unioned
				}else{
					token.raw.count = stream.str - token.raw.str;
					token.type      = Token_LiteralInteger;
					token.int_value = b10tou64(token.raw); //NOTE this replaces token.raw since they are unioned
				}
				
				token.l1 = line_number;
				token.c1 = LINE_COLUMN;
				lfile.tokens.add(token);
			}continue; //skip token creation b/c we did it manually
			
			case '\'':{
				ZoneScoped;
				token.type  = Token_LiteralCharacter;
				token.group = TokenGroup_Literal;
				stream++;
				
				while(stream && *stream != '\''){ stream++; } //skip until closing single quotes
				
				token.l1 = line_number;
				token.c1 = LINE_COLUMN;
				token.raw.count = stream.str - (++token.raw.str); //dont include the single quotes
				lfile.tokens.add(token);
				stream++;
			}continue; //skip token creation b/c we did it manually
			
			case '"':{
				ZoneScoped;
				token.type  = Token_LiteralString;
				token.group = TokenGroup_Literal;
				stream++;
				
				while(stream && *stream != '"'){ stream++; } //skip until closing double quotes
				
				token.l1 = line_number;
				token.c1 = LINE_COLUMN;
				token.raw.count = stream.str - (++token.raw.str); //dont include the double quotes
				lfile.tokens.add(token);
				stream++;
			}continue; //skip token creation b/c we did it manually
			
			//// @letters //// (keywords and identifiers)
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
            case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case '_':{
				ZoneScoped;
                while(isalnum(*stream) || *stream == '_'){ stream++; } //skip to non-alphanumeric
				
                token.raw.count = stream.str - token.raw.str;
                token.type = token_is_keyword_or_identifier(token.raw);
				
				if(lfile.tokens.count && lfile.tokens[lfile.tokens.count-1].type == Token_StructDecl){
					struct_names.add(token.raw);
				}
				if(token.type == Token_Identifier){
					identifier_indexes.add(lfile.tokens.count);
					if(scope_number == 0){
						global_identifiers.add(lfile.tokens.count);
					}
				}
            }break;
			
			//// @control ////
			CASE1(';', Token_Semicolon);
			CASE1('(', Token_OpenParen);
			CASE1(')', Token_CloseParen);
			CASE1('[', Token_OpenSquare);
			CASE1(']', Token_CloseSquare);
			CASE1(',', Token_Comma);
			CASE1('?', Token_QuestionMark);
			CASE1(':', Token_Colon);
			CASE1('.', Token_Dot);
			CASE1('@', Token_At);
			CASE1('#', Token_Pound);
			CASE1('`', Token_Backtick);
			
			case '{':{ //NOTE special for scope tracking
			ZoneScoped;
				token.type = Token_OpenBrace;
				scope_number++;
				stream++;
			}break;
			
			case '}':{ //NOTE special for scope tracking
			ZoneScoped;
				token.type = Token_CloseBrace;
				scope_number--;
				stream++;
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
			ZoneScoped;
				token.type = Token_Division;
				stream++;
				if(*stream == '='){
					token.type = Token_DivisionAssignment;
					stream++;
				}else if(*stream == '/'){
					while(stream && *stream != '\n'){ stream++; } //skip single line comment
					continue; //skip token creation
				}else if(*stream == '*'){
					while((stream.count > 1) && !(stream[0] == '*' && stream[1] == '/')){ stream++; } //skip multiline comment
					if(stream.count <= 1 && stream[-1] != '/' && stream[-2] != '*'){
						lexer_report_error(token, "Multi-line comment has no ending */ token.");
						return false;
					}
					stream++; stream++;
					continue; //skip token creation
				}
			}break;
			
			case '<':{ //NOTE special because of bitshift assignment
			ZoneScoped;
				token.type = Token_LessThan;
				stream++;
				if      (*stream == '='){
					token.type = Token_LessThanOrEqual;
					stream++;
				}else if(*stream == '<'){
					token.type = Token_BitShiftLeft;
					stream++;
					if(*stream == '='){
						token.type = Token_BitShiftLeftAssignment;
						stream++;
					}
				}
			}break;
			
			case '>':{ //NOTE special because of bitshift assignment
			ZoneScoped;
				token.type = Token_GreaterThan;
				stream++;
				if      (*stream == '='){
					token.type = Token_GreaterThanOrEqual;
					stream++;
				}else if(*stream == '>'){
					token.type = Token_BitShiftRight;
					stream++;
					if(*stream == '='){
						token.type = Token_BitShiftRightAssignment;
						stream++;
					}
				}
			}break;
			
			default:{
				ZoneScoped;
				logE("lexer", "Invalid token '",*stream,"' at ",filename,"(",line_number,",",LINE_COLUMN,").");
				token.type = Token_ERROR;
				stream++;
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
		}
		
		//set token end
		if(token.type != Token_ERROR){
			token.l1 = line_number;
			token.c1 = LINE_COLUMN;
			token.raw.count = stream.str - token.raw.str;
			lfile.tokens.add(token);
		}
	}
	
	lfile.tokens.add(Token{Token_EOF, Token_EOF});
	
	//not very helpful
	if(scope_number) logE("lexer", "unbalanced {} somewhere in code");
	
	//iterate over all found identifiers and figure out if they match any known struct names
	//so parser knows when a struct name is being used as a type
	//TODO find a good way to only add identifiers that are in the global scope 
	forI(Max(identifier_indexes.count, global_identifiers.count)){
		b32 isstruct = 0;
		if(i < identifier_indexes.count && lfile.tokens[identifier_indexes[i] - 1].type != Token_StructDecl){
			Token& t = lfile.tokens[identifier_indexes[i]];
			b32 isstruct = 0;
			for (cstring& str : struct_names){
				if(equals(str, t.raw)){
					t.type = Token_Struct;
					t.group = TokenGroup_Type;
					isstruct = 1;
					break;
				}
			}
			
		}
		if(i < global_identifiers.count){
			if(lfile.tokens[global_identifiers[i] - 1].type == Token_StructDecl){
				lfile.struct_decl.add(global_identifiers[i] - 1);
			}else if(match_any(lfile.tokens[global_identifiers[i] - 1].group, TokenGroup_Type)){
				if(lfile.tokens[global_identifiers[i] + 1].type == Token_OpenParen){
					lfile.func_decl.add(global_identifiers[i] - 1);
				}else{
					lfile.var_decl.add(global_identifiers[i] - 1);
				}
			} 
		}
		
	}
	
	return true;
}

#undef LINE_COLUMN
#undef CASE3
#undef CASE2
#undef CASE1
#undef CASEW