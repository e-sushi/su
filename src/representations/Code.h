/*
	
	Representation of various parts of source that can be compiled independently.

*/

#ifndef AMU_CODE_H
#define AMU_CODE_H

#include "storage/Map.h"
#include "systems/Diagnostics.h"
#include "systems/Threading.h"
#include "Token.h"
#include "representations/AST.h"
#include "storage/Bump.h"

namespace amu {

struct Lexer;
struct Parser;
struct Sema;
struct GenTAC;
struct GenAIR;
struct VM;

struct Code : public ASTNode {
	enum class Kind {
		Unknown,
		Source, 
		Structure,
		Function,
		Macro,
	};

	Kind kind;

	enum class Stage {
		Newborn,
		Lex,
		Parse,
		Sema,
		TAC,
		AIR,
		VM,
	};
	
	Stage stage;

	// raw view of this code
	String raw; 

	// identifier for this code, only used for debugging
	String identifier;
	
	// Source this code belongs to. if this is 0, then this is VirtualCode
	Source* source = 0;

	// set true when this Code object fully represents compile time code
	b32 compile_time;

	// information from stages that this Code has been passed through
	Lexer* lexer = 0;
	Parser* parser = 0;
	Sema* sema = 0;
	GenTAC* tac_gen = 0;
	GenAIR* air_gen = 0;
	VM* machine = 0;

	// a Code object this one depends on 
	Code* dependency;
	
	TokenRange tokens;

	Mutex mtx;
	ConditionVariable cv;

	Bump allocator;

	
	// ~~~~~~ interface ~~~~~~~
	
	
	// results in an 'unbounded' Code object
	// the tokens array will start at 'start'
	// and end at the end of the given 'code's 
	// token array.
	static Code*
	from(Code* code, Token* start);
	
	static Code*
	from(Code* code, TokenRange range);

	// creates a Code object encompassing all tokens
	// that the given ASTNode covers, so make sure 
	// the node has its start and end pointers 
	// properly set.
	static Code*
	from(Code* code, ASTNode* node);
	
	// creates a new Code object encompassing the
	// entire given Source
	static Code*
	from(Source* source);

	void
	destroy();

	// returns true if this Code object is currently going through some
	// stage of processing
	b32 
	is_processing();
	
	TokenRange
	get_tokens();

	Array<Token>&
	get_token_array();

	void
	add_diagnostic(Diag d);
	
	// Attempts to send this Code object through each level of compilation
	// up to and including 'level'. If something fails during this process
	// false is returned.
	// Note that this doesn't clean up anything from any of the stages, as 
	// some of that information may be desired by whatever calls this.
	// This should be the only function ever used to process Code to some point
	// because we allow Code to be arbitrarily processed at any point. Only using
	// this function ensures that Code won't go through any stage more than once.
	// 
	// If this Code object is representing Source or a Module, then this will call
	// discretize_module() from the Parser instead of calling parse() like normal.
	// Then each new Code object will be processed asyncronously.
	b32
	process_to(Code::Stage stage);
	
	// same as process_to, but spawns a different thread to run on
	// and returns a future containing the result of processing
	Future<b32>
	process_to_async(Code::Stage stage);

	Future<b32>
	process_to_async_deferred(Future<void> f, Code::Stage stage);

	// same as process_to, but spawns a different thread and immediately
	// waits for it to finish
	b32
	process_to_wait(Code::Stage stage);
	
	// called by a different Code object (the dependent) that depends on this one having reached
	// some level of processing. If this code object has not reached the given level
	// then the caller will wait until it finishes that stage and is notified.
	// If a dependecy cycle is detected, false is returned.
	b32
	wait_until_level(Code::Stage stage, Code* dependent);
	
	// helper function called only by this Code object
	// just sets the state and optionally notifies the condition variable
	void
	change_state(Code::Stage stage, b32 notify = true);

	DString
	display();

	DString
	dump();
	
	Code() : ASTNode(ASTNode::Kind::Code) {};
};


struct TokenIterator {
	Code*  code; // code we are iterating
	Token* curt; // current token
	// Token* stop; // stop token 


	// ~~~~~~ interface ~~~~~~~


	TokenIterator() {}
	TokenIterator(Code* c);

	// returns the current token
	FORCE_INLINE Token* 
	current();

	// returns the kind of the current token
	FORCE_INLINE Token::Kind
	current_kind();

	// increments the iterator by one token and returns
	// the token arrived at
	// returns 0 if we're at the end 
	FORCE_INLINE Token* 
	increment();

	// decrements the iterator by one token and returns
	// the token arrived at
	// returns 0 if we're at the start
	FORCE_INLINE Token*
	decrement();

	// get the next token
	FORCE_INLINE Token* 
	next();

	// get the next token's kind
	// returns token::null if at the end 
	FORCE_INLINE Token::Kind
	next_kind();

	// get the previous token
	FORCE_INLINE Token*
	prev();

	// get the previous token's kind
	// returns token::null if at the beginning
	FORCE_INLINE Token::Kind
	prev_kind();

	// get the token 'n' steps ahead
	FORCE_INLINE Token*
	lookahead(u64 n);

	// get the token 'n' steps back
	FORCE_INLINE Token*
	lookback(u64 n);

	// when the iterator is at a Token that has a pair:
	// (, ", ', {, <, [
	// it will skip until it finds a matching pair
	FORCE_INLINE void
	skip_to_matching_pair();

	// skips until one of the given token::kinds are found
	template<typename... T> FORCE_INLINE void
	skip_until(T... args);

	// checks if the current token is of 'kind'
	FORCE_INLINE b32
	is(Token::Kind kind);

	// checks if the current token is of any of 'args'
	template<typename... T> FORCE_INLINE b32
	is_any(T... args);

	// checks if the next token is of 'kind'
	// returns false if at the end
	FORCE_INLINE b32
	next_is(Token::Kind kind);

	// checks if the previous token is of 'kind'
	// returns false if at the beginning
	FORCE_INLINE b32
	prev_is(Token::Kind kind);

	// displays the current line as well as a caret 
	// indicating where in the line we are 
	DString
	display_line();
};

void
to_string(DString& current, Code::Stage stage);

void
to_string(DString& current, Code* c);

} // namespace amu

#endif // AMU_CODE_H
