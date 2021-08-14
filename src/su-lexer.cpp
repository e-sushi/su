#include "su-lexer.h"
#include "utils/defines.h"

#define MAXBUFF 255

array<char> stopping_chars{
	';', ' ', '{',  '}', '\(', '\)', 
	',', '+', '*', '/', '-', '<', '>', 
	'=', '!', '~', '\n', '&', '|', '^',
	'%', ':', '?'
};

array<string> keywords{
	"int", "return", "if", "else"
};

array<token> suLexer::lex(FILE* file) {
	array<token> tokens;
	char currChar = 0;
	string buff = "";
	u32 lines = 1;
	
	//TODO get rid of maxbuff here
	while (buff.size + 1 < MAXBUFF) {
		currChar = fgetc(file);
		//check that our current character isn't any 'stopping' characters
		if (stopping_chars.has(currChar)) {

			//store both the stopping character and previous buffer as tokens
			//also decide what type of token it is
			if (buff[0]) {
				token t;
				t.str = buff;
				//check if token is a keyword
				if (keywords.has(buff)) {
					if      (buff == "return") t.type = Token_Return;
					else if (buff == "int")    t.type = Token_Keyword;
					else if (buff == "if")     t.type = Token_If;
					else if (buff == "else")   t.type = Token_Else;
				}
				//if its not then it could be a number of other things
				else {
					//TODO make this cleaner
					if (isalpha(buff[0])) {
						//if it begins with a letter it must be an identifier
						//for now
						t.type = Token_Identifier;
					}
					else if (isdigit(buff[0])) {
						//check if its a digit, then verify that the rest are digits
						bool error = false;
						for (int i = 0; i < buff.size; i++) {
							if (!isdigit(buff[i])) error = true;
						}
						if (error) t.type = Token_ERROR;
						else t.type = Token_Literal;
					}
				}
				t.line = lines;
				tokens.add(t);
			}

			//check what our stopping character is 
			if (currChar != ' ' && currChar != '\n') {
				token t;
				t.str = currChar;
				switch (currChar) {
					case ';':  t.type = Token_Semicolon;         break;
					case '{':  t.type = Token_OpenBrace;         break;
					case '}':  t.type = Token_CloseBrace;        break;
					case '\(': t.type = Token_OpenParen;         break;
					case '\)': t.type = Token_CloseParen;        break;
					case ',':  t.type = Token_Comma;             break;
					case '+':  t.type = Token_Plus;              break;
					case '-':  t.type = Token_Negation;          break;
					case '*':  t.type = Token_Multiplication;    break;
					case '/':  t.type = Token_Division;          break;
					case '~':  t.type = Token_BitwiseComplement; break;
					case '%':  t.type = Token_Modulo;            break;
					case '^':  t.type = Token_BitXOR;            break;
					case '?':  t.type = Token_QuestionMark;      break;
					case ':':  t.type = Token_Colon;             break;
					
					case '&': {
						if (fgetc(file) == '&') {
							t.type = Token_AND;
							t.str += '&';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_BitAND;
						}
					}break;

					case '|': {
						if (fgetc(file) == '|') {
							t.type = Token_OR;
							t.str += '|';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_BitOR;
						}
					}break;

					case '!': {
						if (fgetc(file) == '=') {
							t.type = Token_NotEqual;
							t.str += '=';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_LogicalNOT;
						}
					}break;

					case '=': {
						if (fgetc(file) == '=') {
							t.type = Token_Equal;
							t.str += '=';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_Assignment;
						}
					}break;

					case '>': {
						char c = fgetc(file);
						if (c == '=') {
							t.type = Token_GreaterThanOrEqual; 
							t.str += '=';
						}
						else if (c == '>') {
							t.type = Token_BitShiftRight;
							t.str += '>';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_GreaterThan;
						}
					}break;

					case '<': {
						char c = fgetc(file);
						if (c == '=') {
							t.type = Token_LessThanOrEqual;
							t.str += '=';
						}
						else if (c == '<') {
							t.type = Token_BitShiftLeft;
							t.str += '<';
						}
						else {
							fseek(file, -1, SEEK_CUR);
							t.type = Token_LessThan;
						}
					}break;
				}
				t.line = lines;
				tokens.add(t);
			}
			buff.clear();
		}
		else if (currChar == EOF) { 
			token t;
			t.str = "End of File";
			t.type = Token_EOF;
			t.line = lines;
			tokens.add(t);
			break;
		}
		else if (currChar != '\t' && currChar != '\n') {
			//if not then keep adding to buffer if character is not a newline or tab
			buff += currChar;
		}
		if (currChar == '\n') lines++;
	}

	return tokens;
}