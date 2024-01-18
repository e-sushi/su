#ifndef AMU_TOKENITERATOR_H
#define AMU_TOKENITERATOR_H

#include "representations/Token.h"

namespace amu {

struct Code;

struct TokenIterator {
	Code*  code; // code we are iterating
	Token* curt; // current token


	// ~~~~~~ interface ~~~~~~~


	TokenIterator() {}
	TokenIterator(Code* c);

	// returns the current token
	Token* current();

	// returns the kind of the current token
	Token::Kind current_kind();

	// increments the iterator by one token and returns
	// the token arrived at
	// returns 0 if we're at the end 
	Token* increment();

	// decrements the iterator by one token and returns
	// the token arrived at
	// returns 0 if we're at the start
	Token* decrement();

	// get the next token
	Token* next();

	// get the next token's kind
	// returns token::null if at the end 
	Token::Kind next_kind();

	// get the previous token
	Token* prev();

	// get the previous token's kind
	// returns token::null if at the beginning
	Token::Kind prev_kind();

	// get the token 'n' steps ahead
	Token* lookahead(u64 n);

	// get the token 'n' steps back
	Token* lookback(u64 n);

	// when the iterator is at a Token that has a pair:
	// (, ", ', {, <, [
	// it will skip until it finds a matching pair
	void skip_to_matching_pair();

	// skips until one of the given token::kinds are found
	template<typename... T> 
	void skip_until(T... args);

	// checks if the current token is of 'kind'
	b32 is(Token::Kind kind);

	// checks if the current token is of any of 'args'
	template<typename... T> 
	b32 is_any(T... args);

	// checks if the next token is of 'kind'
	// returns false if at the end
	b32 next_is(Token::Kind kind);

	// checks if the previous token is of 'kind'
	// returns false if at the beginning
	b32 prev_is(Token::Kind kind);

	// displays the current line as well as a caret 
	// indicating where in the line we are 
	DString display_line();
};

} // namespace amu

#endif
