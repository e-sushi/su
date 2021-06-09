#include "su-lexer.h"
#include "../utils/misc.h"

char stopping_chars[NUMSTOPCHAR] = {
	';', ' ', '{',  '}', '\(', '\)', ','
};

string keywords[NUMKEYWORDS] = {
	"int"
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