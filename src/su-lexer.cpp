enum LexerState_ {
	DiscernChar,
	ReadingString,
	ReadingStringLiteral,
}; typedef u32 LexerState;

LexerState state = DiscernChar;


b32 suLexer::lex(const string& file, array<token>& tokens) {
	cstring buff{file.str, file.count};
	u32 lines = 1;
	
	b32 readingstring = 0;
	char* chunk_start = 0;
#define chunksiz buff.str - chunk_start
#define chunkcmp chunk_start, chunksiz
#define chunkstr string(chunk_start, buff.str - chunk_start)
	
	while (buff.count) {
		switch (state) {
			case DiscernChar: {
				switch (buff[0]) {
					case ';': { tokens.add(token{ ";", Token_Semicolon,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case '{': { tokens.add(token{ "{", Token_OpenBrace,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case '}': { tokens.add(token{ "}", Token_CloseBrace,   Token_ControlCharacter, lines }); advance(&buff); }break;
					case '(': { tokens.add(token{ "(", Token_OpenParen,    Token_ControlCharacter, lines }); advance(&buff); }break;
					case ')': { tokens.add(token{ ")", Token_CloseParen,   Token_ControlCharacter, lines }); advance(&buff); }break;
					case ',': { tokens.add(token{ ",", Token_Comma,        Token_ControlCharacter, lines }); advance(&buff); }break;
					case '?': { tokens.add(token{ "?", Token_QuestionMark, Token_ControlCharacter, lines }); advance(&buff); }break;
					case ':': { tokens.add(token{ ",", Token_Colon,        Token_ControlCharacter, lines }); advance(&buff); }break;
					case '.': { tokens.add(token{ ".", Token_Dot,          Token_ControlCharacter, lines }); advance(&buff); }break;
					
					case '+': {
						if (buff[1] == '=') { tokens.add(token{ "+=", Token_PlusAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '+') { tokens.add(token{ "++", Token_Increment, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "+", Token_Plus, Token_Operator, lines }); advance(&buff); }
					}break;
					case '-': {
						if (buff[1] == '=') { tokens.add(token{ "-=", Token_NegationAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '-') { tokens.add(token{ "--", Token_Decrememnt, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "-", Token_Negation, Token_Operator, lines }); advance(&buff); }
					}break;
					case '*': {
						if (buff[1] == '=') { tokens.add(token{ "*=", Token_MultiplicationAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "*", Token_Multiplication, Token_Operator, lines }); advance(&buff); }
					}break;
					case '/': {
						if (buff[1] == '=') { tokens.add(token{ "/=", Token_DivisionAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "/", Token_Division, Token_Operator, lines }); advance(&buff); }
					}break;
					case '~': {
						if (buff[1] == '=') { tokens.add(token{ "~=", Token_BitNOTAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "~", Token_BitNOT, Token_Operator, lines }); advance(&buff); }
					}break;
					case '&': {
						if (buff[1] == '=') { tokens.add(token{ "&=", Token_BitANDAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '&') { tokens.add(token{ "&&", Token_AND, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "&", Token_BitAND, Token_Operator, lines }); advance(&buff); }
					}break;
					case '|': {
						if (buff[1] == '=') { tokens.add(token{ "|=", Token_BitORAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '|') { tokens.add(token{ "||", Token_OR, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "|", Token_BitOR, Token_Operator, lines }); advance(&buff); }
					}break;
					case '^': {
						if (buff[1] == '=') { tokens.add(token{ "^=", Token_BitXORAssignment, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "^", Token_BitXOR, Token_Operator, lines }); advance(&buff); }
					}break;
					case '%': {
						if (buff[1] == '=') { tokens.add(token{ "%=", Token_Equal, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "%", Token_Modulo, Token_Operator, lines }); advance(&buff); }
					}break;
					case '=': {
						if (buff[1] == '=') { tokens.add(token{ "==", Token_Equal, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "=", Token_Assignment, Token_Operator, lines }); advance(&buff); }
					}break;
					case '!': {
						if (buff[1] == '=') { tokens.add(token{ "!=", Token_NotEqual, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "!", Token_LogicalNOT, Token_Operator, lines }); advance(&buff); }
					}break;
					case '<': {
						if (buff[1] == '=') { tokens.add(token{ "<=", Token_LessThanOrEqual, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '<') { tokens.add(token{ "<<", Token_BitShiftLeft, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ "<", Token_LessThan, Token_Operator, lines }); advance(&buff); }
					}break;
					case '>': {
						if (buff[1] == '=') { tokens.add(token{ ">=", Token_GreaterThanOrEqual, Token_Operator, lines }); advance(&buff, 2); }
						else if (buff[1] == '>') { tokens.add(token{ ">>", Token_BitShiftRight, Token_Operator, lines }); advance(&buff, 2); }
						else { tokens.add(token{ ">", Token_GreaterThan, Token_Operator, lines }); advance(&buff); }
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
						if (isalpha(*chunk_start)) {
							if      (!strncmp("return", chunk_start, 6)) tokens.add(token{ "return", Token_Return,     Token_ControlKeyword, lines });
							else if (!strncmp("if",     chunk_start, 2)) tokens.add(token{ "if",     Token_If,         Token_ControlKeyword, lines });
							else if (!strncmp("else",   chunk_start, 4)) tokens.add(token{ "else",   Token_Else,       Token_ControlKeyword, lines });
							else if (!strncmp("for",    chunk_start, 3)) tokens.add(token{ "for",    Token_For,        Token_ControlKeyword, lines });
							else if (!strncmp("while",  chunk_start, 5)) tokens.add(token{ "while",  Token_While,      Token_ControlKeyword, lines });
							else if (!strncmp("struct", chunk_start, 4)) tokens.add(token{ "struct", Token_Struct,     Token_ControlKeyword, lines });
							else if (!strncmp("s32",    chunk_start, 3)) tokens.add(token{ "s32",    Token_Signed32,   Token_Typename,       lines });
							else if (!strncmp("s64",    chunk_start, 3)) tokens.add(token{ "s64",    Token_Signed64,   Token_Typename,       lines });
							else if (!strncmp("u32",    chunk_start, 3)) tokens.add(token{ "u32",    Token_Unsigned32, Token_Typename,       lines });
							else if (!strncmp("u64",    chunk_start, 3)) tokens.add(token{ "u64",    Token_Unsigned64, Token_Typename,       lines });
							else if (!strncmp("f32",    chunk_start, 3)) tokens.add(token{ "f32",    Token_Float32,    Token_Typename,       lines });
							else if (!strncmp("f64",    chunk_start, 3)) tokens.add(token{ "f64",    Token_Float64,    Token_Typename,       lines });
							else if (!strncmp("any",    chunk_start, 3)) tokens.add(token{ "any",    Token_Any,        Token_Typename,       lines });
							else if (!strncmp("str",    chunk_start, 3)) tokens.add(token{ "any",    Token_String,     Token_Typename,       lines });
							else tokens.add(token{ chunkstr, Token_Identifier, Token_Identifier, lines });
						}
						else {
							b32 valid = 1;
							b32 isfloat = 0;
							forI(buff.str - chunk_start) { if (!isnumber(chunk_start[i])) valid = 0; if (chunk_start[i] == '.') isfloat = 1; }
							if (valid) tokens.add(token{ chunkstr, (isfloat ? Token_LiteralFloat : Token_LiteralInteger), Token_Literal, lines });
							else {
								//tokens.add(token{ chunkstr, Token_ERROR, lines });
								//PRINTLN("TOKEN ERROR ON TOKEN " << chunkstr);
							}
							// TODO error out of the program here
						}

						if (buff[0] == '\n') lines++;
						state = DiscernChar;
					}break;

					case '.': {
						if (isalpha(*chunk_start)) {
							//must be member access period (i hope)
							//TODO maybe add checks here that its not a keyword
							tokens.add(token{ chunkstr, Token_Identifier, Token_Identifier, lines });
							tokens.add(token{ ".", Token_Dot, Token_ControlCharacter, lines });
							advance(&buff);
						}
						else {}//float case handled above
					}break;

					default: { advance(&buff); }break;
				}
			}break;

			//TODO setup escape character stuff
			case ReadingStringLiteral: {
				switch (buff[0]) {
					case '"': {
						tokens.add(token{ chunkstr, Token_LiteralString, Token_Literal, lines });
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
	tokens.add(token{ "", Token_EOF, Token_EOF, lines });
	return true;
}