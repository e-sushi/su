#include "su-lexer.h"
#include "utils/misc.h"

array<char> stopping_chars{
	';', ' ', '{',  '}', '\(', '\)', ','
};

array<string> keywords{
	"int", "return"
};

array<token> suLexer::lex(FILE* file) {
	array<token> tokens(MAXTOK);
	char currChar = 0;
	string buff = "";

	int tokcount = 0;
	while (buff.size + 1 < MAXBUFF) {
		currChar = fgetc(file);
		//check that our current character isn't any 'stopping' characters
		if (is_in(currChar, stopping_chars)) {

			//store both the stopping character and previous buffer as tokens
			//also decide what type of token it is
			if (buff[0]) {
				token t;
				t.str = buff;

				//tokens[tokcount].str = buff;

				if (is_in(buff, keywords))
					t.type = tok_IntegerLiteral;

				tokens.add(t);

				tokcount++;
			}
			if (currChar != ' ') {
				token t;
				t.str = currChar;
				switch (currChar) {
					case ';':  t.type = tok_Semicolon; break;
					case '{':  t.type = tok_OpenBrace; break;
					case '}':  t.type = tok_CloseBrace; break;
					case '\(': t.type = tok_OpenParen; break;
					case '\)': t.type = tok_CloseParen; break;
					case ',':  t.type = tok_Comma; break;

				}
				tokens.add(t);
			}
			buff.clear();
		}
		else if (currChar == EOF) { break; }
		else if(currChar != '\t' && currChar != '\n') {
			//if not then keep adding to buffer if character is not a newline or tab
			buff += currChar;
		}
	}

	return tokens;
}