#include <iostream>
#include <math.h>
#include "utils/string.h"
#include "utils/array.h"

#define MAXBUFF 255
#define MAXTOK 500

#define NUMSTOPCHAR 7
#define NUMKEYWORDS 1

#define arraysize(array) (sizeof(array)/sizeof(array[0]))

char stopping_chars[NUMSTOPCHAR] = {
	';', ' ', '{',  '}', '\(', '\)', ','
};

string keywords[NUMKEYWORDS] = {
	"int"
};

template<class T>
int is_in(T& c, T array[]) {
	int size = sizeof(array);
	int size2 = sizeof(T);
	for (int i = 0; i < size; i++) {
		if (c == array[i]) return 1;
	}
	return 0;
}

enum token_type {
	tok_EOF,
	tok_Keyword,
	tok_Identifier,
	tok_OpenParen,
	tok_CloseParen, 
	tok_OpenBrace,
	tok_CloseBrace,
	tok_Semicolon,
	tok_Return,
	tok_IntegerLiteral
};

struct token {
	string token;
	token_type type;
};

token* begin_parse() {
	return nullptr;
}

int main() {
	FILE* file = fopen("main.su", "r");
	if (!file) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}
	string buff = "";
	char currChar = 0;

	array<token> tokens(MAXTOK);

	int tokcount = 0;
	while (buff.size + 1 < MAXBUFF) {
		currChar = fgetc(file);
		//check that our current character isn't any 'stopping' characters
		if (is_in(currChar, stopping_chars)) {

			//store both the stopping character and previous buffer as tokens
			//also decide what type of token it is
			if (buff[0]) {
				tokens[tokcount].token = buff;
				//memcpy(tokens[tokcount].token, buff, MAXBUFF);
				//string buffer(buff);
				if (is_in(buff, keywords))
					tokens[tokcount].type = tok_Keyword;
				tokcount++;
			}
			if (currChar != ' ') {
				//memset(tokens[tokcount].token, 0, MAXBUFF);
				//tokens[tokcount++].token[0] = currChar;
				tokens[tokcount++].token = string(currChar);
			}
			//memset(buff, 0, sizeof(buff));
			buff.clear();

		}
		else if (currChar == EOF) {
			//if (buff[0]) memcpy(tokens[tokcount++].token, buff, MAXBUFF);
			//tokens[tokencount++].token = EOF
			break;
		}
		else {
			//if not then keep adding to buffer if character is not a newline or tab
			
			//if(currChar != '\t' && currChar != '\n')
			//	buff[i++] = currChar;
			buff += currChar;
		}
	}

	token_type currtype;

	//int iter = 0;
	//while (tokens[iter].token[0]) {
	//	printf("%s|", tokens[iter].token);
	//	iter++;
	//}
}