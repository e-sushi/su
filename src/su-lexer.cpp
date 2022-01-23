enum LexerState_ {
	DiscernChar,
	ReadingString,
	ReadingStringLiteral,
}; typedef u32 LexerState;

LexerState state = DiscernChar;

array<u32> idindexes;
array<cstring> structnames;

b32 suLexer::lex(const string& file) {
	cstring buff{file.str, file.count};
	u32 lines = 1;
	b32 readingstring = 0;
	b32 grab_structname = 0;
	char* chunk_start = 0;
#define chunksiz buff.str - chunk_start
#define chunkcmp chunk_start, chunksiz
#define chunkstr cstring{chunk_start, u32(buff.str - chunk_start)}
#define cstr(count) cstring{chunk_start, count}

	while (buff.count) {
		switch (state) {
			case DiscernChar: {
				switch (buff[0]) {
					case ';': { lexer.tokens.add(token{ cstr(1), Token_Semicolon,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case '{': { lexer.tokens.add(token{ cstr(1), Token_OpenBrace,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case '}': { lexer.tokens.add(token{ cstr(1), Token_CloseBrace,   Token_ControlCharacter, lines }); advance(&buff); }break;
					case '(': { lexer.tokens.add(token{ cstr(1), Token_OpenParen,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case ')': { lexer.tokens.add(token{ cstr(1), Token_CloseParen,   Token_ControlCharacter, lines }); advance(&buff); }break;
					case ',': { lexer.tokens.add(token{ cstr(1), Token_Comma,        Token_ControlCharacter, lines }); advance(&buff); }break;
					case '?': { lexer.tokens.add(token{ cstr(1), Token_QuestionMark, Token_ControlCharacter, lines }); advance(&buff); }break;
					case ':': { lexer.tokens.add(token{ cstr(1), Token_Colon,        Token_ControlCharacter, lines }); advance(&buff); }break;
					case '.': { lexer.tokens.add(token{ cstr(1), Token_Dot,          Token_ControlCharacter, lines }); advance(&buff); }break;
					
					case '+': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2),Token_PlusAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '+') { lexer.tokens.add(token{ cstr(2),Token_Increment, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1),Token_Plus, Token_Operator, lines }); advance(&buff); }
					}break;
					case '-': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_NegationAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '-') { lexer.tokens.add(token{ cstr(2), Token_Decrememnt, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1), Token_Negation, Token_Operator, lines }); advance(&buff); }
					}break;
					case '*': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_MultiplicationAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_Multiplication, Token_Operator, lines }); advance(&buff); }
					}break;
					case '/': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_DivisionAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_Division, Token_Operator, lines }); advance(&buff); }
					}break;
					case '~': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_BitNOTAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_BitNOT, Token_Operator, lines }); advance(&buff); }
					}break;
					case '&': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_BitANDAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '&') { lexer.tokens.add(token{ cstr(2), Token_AND, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1), Token_BitAND, Token_Operator, lines }); advance(&buff); }
					}break;
					case '|': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_BitORAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '|') { lexer.tokens.add(token{ cstr(2), Token_OR, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1), Token_BitOR, Token_Operator, lines }); advance(&buff); }
					}break;
					case '^': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_BitXORAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_BitXOR, Token_Operator, lines }); advance(&buff); }
					}break;
					case '%': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_Equal, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_Modulo, Token_Operator, lines }); advance(&buff); }
					}break;
					case '=': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_Equal, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_Assignment, Token_Operator, lines }); advance(&buff); }
					}break;
					case '!': {
						if (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_NotEqual, Token_Operator, lines }); advance(&buff, 2); }
						else                { lexer.tokens.add(token{ cstr(1), Token_LogicalNOT, Token_Operator, lines }); advance(&buff); }
					}break;
					case '<': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_LessThanOrEqual, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '<') { lexer.tokens.add(token{ cstr(2), Token_BitShiftLeft, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1), Token_LessThan, Token_Operator, lines }); advance(&buff); }
					}break;
					case '>': {
						if      (buff[1] == '=') { lexer.tokens.add(token{ cstr(2), Token_GreaterThanOrEqual, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '>') { lexer.tokens.add(token{ cstr(2), Token_BitShiftRight, Token_Operator, lines }); advance(&buff, 2); }
						else                     { lexer.tokens.add(token{ cstr(1), Token_GreaterThan, Token_Operator, lines }); advance(&buff); }
					}break;
					case '\n': { lines++; advance(&buff); }break;
					case '\t':
					case ' ': { advance(&buff); }break;
					case '"': {
						state = ReadingStringLiteral;
						advance(&buff);
						chunk_start = buff.str;
					}break;
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
					case '=': case '!': case '~': case '&': case '|': case '\t':
					case '%': case ':': case '?': case '>': case '^': case '\n': {
						state = DiscernChar;
						if (isalpha(*chunk_start)) {
							if      (!strncmp("return",   chunk_start, 6)) { lexer.tokens.add(token{ cstr(6), Token_Return,     Token_ControlKeyword, lines });}
							else if (!strncmp("if",       chunk_start, 2)) { lexer.tokens.add(token{ cstr(2), Token_If,         Token_ControlKeyword, lines });}
							else if (!strncmp("else",     chunk_start, 4)) { lexer.tokens.add(token{ cstr(4), Token_Else,       Token_ControlKeyword, lines });}
							else if (!strncmp("for",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_For,        Token_ControlKeyword, lines });}
							else if (!strncmp("while",    chunk_start, 5)) { lexer.tokens.add(token{ cstr(5), Token_While,      Token_ControlKeyword, lines });}
							else if (!strncmp("break",    chunk_start, 5)) { lexer.tokens.add(token{ cstr(5), Token_Break,      Token_ControlKeyword, lines });}
							else if (!strncmp("continue", chunk_start, 8)) { lexer.tokens.add(token{ cstr(8), Token_Continue,   Token_ControlKeyword, lines });}
							else if (!strncmp("struct",   chunk_start, 6)) { lexer.tokens.add(token{ cstr(6), Token_StructDecl, Token_ControlKeyword, lines }); lexer.struct_decl.add(lexer.tokens.count-1); grab_structname = 1;}
							else if (!strncmp("void",     chunk_start, 4)) { lexer.tokens.add(token{ cstr(4), Token_Void,       Token_Typename,       lines });}
							else if (!strncmp("s32",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Signed32,   Token_Typename,       lines });}
							else if (!strncmp("s64",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Signed64,   Token_Typename,       lines });}
							else if (!strncmp("u32",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Unsigned32, Token_Typename,       lines });}
							else if (!strncmp("u64",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Unsigned64, Token_Typename,       lines });}
							else if (!strncmp("f32",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Float32,    Token_Typename,       lines });}
							else if (!strncmp("f64",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Float64,    Token_Typename,       lines });}
							else if (!strncmp("any",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_Any,        Token_Typename,       lines });}
							else if (!strncmp("str",      chunk_start, 3)) { lexer.tokens.add(token{ cstr(3), Token_String,     Token_Typename,       lines });}
							else { 
								lexer.tokens.add(token{ chunkstr, Token_Identifier, Token_Identifier, lines });
								if (grab_structname) {
									structnames.add(chunkstr);
									grab_structname = 0;
								}
								else {
									idindexes.add(lexer.tokens.count - 1);
								}
							}
						}
						else {
							b32 valid = 1;
							b32 isfloat = 0;
							forI(buff.str - chunk_start) { 
								if (chunk_start[i] == '.')
									isfloat = 1;
								else if (!isnumber(chunk_start[i])) 
									valid = 0; 
								 }
							if (valid) { 
								lexer.tokens.add(token{ chunkstr, (isfloat ? Token_LiteralFloat : Token_LiteralInteger), Token_Literal, lines });
								if (isfloat) {
									lexer.tokens.last->float32 = stod(chunkstr);
								}
								else {
									lexer.tokens.last->integer = stoi(chunkstr);
								}
							}
							else {
								//lexer.tokens.add(token{ chunkstr, Token_ERROR, lines });
								//PRINTLN("TOKEN ERROR ON TOKEN " << chunkstr);
							}
							// TODO error out of the program here
						}

						if (buff[0] == '\n') {
							lines++;
						}
						
					}break;

					case '.': {
						if (isalpha(*chunk_start)) {
							//must be member access period (i hope)
							//TODO maybe add checks here that its not a keyword
							lexer.tokens.add(token{ chunkstr, Token_Identifier, Token_Identifier, lines });
							lexer.tokens.add(token{ cstr_lit("."), Token_Dot, Token_ControlCharacter, lines });
							advance(&buff);
						}
						else { advance(&buff); }//float case handled above
					}break;

					default: { advance(&buff); }break;
				}
			}break;

			//TODO setup escape character stuff
			case ReadingStringLiteral: {
				switch (buff[0]) {
					case '"': {
						lexer.tokens.add(token{ chunkstr, Token_LiteralString, Token_Literal, lines });
						state = DiscernChar;
						advance(&buff);
					}break;
					case '\n': {
						PRINTLN("unexpected \\n before \" on line " << lines);
						return false;
					}break;
					default: { advance(&buff); }break;
				}
			}break;
		}
	}
	lexer.tokens.add(token{ cstr_lit(""), Token_EOF, Token_EOF, lines});
	
	//iterate over all found identifiers and figure out if they match any known struct names
	//so parser knows when a struct name is being used as a type
	//TODO find a way to have this only add global declarations of structs/functions/vars
	forI(idindexes.count) {
		token& t = lexer.tokens[idindexes[i]];
		b32 isstruct = 0;
		for (cstring& str : structnames) {
			if (equals(str, t.str)) {
				t.type = Token_Struct;
				t.group = Token_Typename;
				isstruct = 1;
				break;
			}
		}
		if (!isstruct && match_any(lexer.tokens[idindexes[i]-1].group, Token_Typename)) {
			lexer.func_decl.add(idindexes[i] - 1);
		}
	}

	return true;
}