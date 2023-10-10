/*

	Interface for interacting with code
	This will provide methods for viewing, formatting, querying and manipulating plain text code using
	information supplied by other stages. 

	This may represent any level of code with any sort of information. It is a very abstract interface
	for passing around any amount and form of code.

	Code keeps track of the amount of information that is associated with the code it represents.

	A Code object may be unbounded in that it doesn't know where it ends. This is necessary for Code 
	objects that haven't finished parsing. The end token of an unbounded Code object will point at 
	the last token of whatever token array it was created from.

	Note that currently there is no real way to determine if a Code object is unbounded or not. I'm not
	sure if it's necessary or not yet.
*/

#ifndef AMU_CODE_H
#define AMU_CODE_H

#include "systems/Diagnostics.h"
#include "systems/Threading.h"

namespace amu {

struct Lexer;
struct Parser;
struct Sema;
struct GenTAC;
struct GenAIR;
struct VM;
struct SourceCode;
struct VirtualCode;

namespace token {
enum kind : u32;
} // namespace token


namespace code {
// @genstrings(data/code_strings.generated)
enum kind {
	unknown,
	token,
	range,
	label, // a label declaration and its entire entity
	structure,
	function,
	module,
	statement,
	expression,
	tuple,
	typedef_,
	source, // represents an entire source file
	func_def_head,
	var_decl,
};

#include "data/code_strings.generated"

// TODO(sushi) im not sure if the post states are really necessary
// @genstrings(data/code_state_strings.generated)
enum state  {
	newborn, 
	in_lex,
	post_lex,
	in_parse,
	post_parse,
	in_sema,
	post_sema,
	in_tacgen,
	post_tacgen,
	in_airgen,
	post_airgen,	
	in_vm,
	post_vm,

	failed, // set when this Code object cannot complete processing
};

#include "data/code_state_strings.generated"

// the level of compilation a given Code object has been through
// this is really just a helper for specifying the correct state to
// bring a Code object to since writing process_to(code::post_lex)
// looks somewhat weirder than process_to(code::lex);
enum level {
	none,
	lex = post_lex,
	parse = post_parse,
	sema = post_sema,
	tac = post_tacgen,
	air = post_airgen,
	vm = post_vm,
};

} // namespace code



struct Code : public ASTNode {
	code::kind kind;
	code::level level;
	// raw representation of this code
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
	
	// this Code object's state
	// atmoic because we only do simple operations on it
	// and don't need to keep an entire lock for it around 
	std::atomic<code::state> state;
	Mutex mtx;
	ConditionVariable cv;

	
	// ~~~~~~ interface ~~~~~~~
	
	
	// results in an 'unbounded' Code object
	// the tokens array will start at 'start'
	// and end at the end of the given 'code's 
	// token array.
	static Code*
	from(Code* code, Token* start);
	
	static Code*
	from(Code* code, Token* start, Token* end);

	// creates a Code object encompassing all tokens
	// that the given ASTNode covers, so make sure 
	// the node has its start and end pointers 
	// properly set.
	static Code*
	from(Code* code, ASTNode* node);
	
	// creates a new Code object encompassing the
	// entire given Source
	static SourceCode*
	from(Source* source);

	// creates a new Code object. Note that it will
	// not be lexed.
	static VirtualCode*
	from(String s);

	void
	destroy();

	VirtualCode*
	make_virtual();

	b32
	is_virtual();

	// returns true if this Code object is currently going through some
	// stage of processing
	b32 
	is_processing();

	View<Token>
	get_tokens();

	Array<Token>&
	get_token_array();

	void
	add_diagnostic(Diagnostic d);
	
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
	process_to(code::level level);
	
	// same as process_to, but spawns a different thread to run on
	// and returns a future containing the result of processing
	Future<b32>
	process_to_async(code::level level);

	Future<b32>
	process_to_async_deferred(Future<void> f, code::level level);

	// same as process_to, but spawns a different thread and immediately
	// waits for it to finish
	b32
	process_to_wait(code::level level);
	
	// called by a different Code object (the dependent) that depends on this one having reached
	// some level of processing. If this code object has not reached the given level
	// then the caller will wait until it finishes that stage and is notified.
	// If a dependecy cycle is detected, false is returned.
	b32
	wait_until_level(code::level level, Code* dependent);
	
	// helper function called only by this Code object
	// just sets the state and optionally notifies the condition variable
	void
	change_state(code::state state, b32 notify = true);

	DString*
	display() = 0;

	DString*
	dump() = 0;

	Code() : state(code::newborn), ASTNode(ast::code) {}
	Code(code::kind k) : kind(k), state(code::newborn), ASTNode(ast::code) {}
};

template<> inline b32 Base::
is<Code>() { return is<ASTNode>() && as<ASTNode>()->kind == ast::code; }

template<> inline b32 Base::
is(code::kind k) { return is<Code>() && as<Code>()->kind == k; }

// Code whose Tokens belong to some Source
struct SourceCode : public Code {
	View<Token> tokens;

	
	// ~~~~~~ interface ~~~~~~~


	DString*
	display();

	DString*
	dump();

	SourceCode() : Code(code::unknown) {}
};

// Code which is not represented by any given Source.
// This may be code that has been formatted, or it may be code that is generated 
// by the compiler. 
// It stores its own set of Tokens and Diagnostics
struct VirtualCode : public Code {
	DString* str;
	Array<Token> tokens;
	Array<Diagnostic> diagnostics;


	// ~~~~~~ interface ~~~~~~~


	DString*
	display();

	DString*
	dump();
	

	VirtualCode() : Code(code::unknown) {}
};

namespace code {


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
	FORCE_INLINE token::kind
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
	FORCE_INLINE token::kind
	next_kind();

	// get the previous token
	FORCE_INLINE Token*
	prev();

	// get the previous token's kind
	// returns token::null if at the beginning
	FORCE_INLINE token::kind
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
	is(u32 kind);

	// checks if the current token is of any of 'args'
	template<typename... T> FORCE_INLINE b32
	is_any(T... args);

	// checks if the next token is of 'kind'
	// returns false if at the end
	FORCE_INLINE b32
	next_is(u32 kind);

	// checks if the previous token is of 'kind'
	// returns false if at the beginning
	FORCE_INLINE b32
	prev_is(u32 kind);

	// displays the current line as well as a caret 
	// indicating where in the line we are 
	DString*
	display_line();
};


namespace virt {

} // namespace virtual

namespace util {

void
find_start_and_end(Code& code);

} // namespace util

namespace format {

// removes leading whitespace up until the min amount of whitespace over all lines
VirtualCode
remove_leading_whitespace(Code& code);

// changes the current indent width of the given Code to 'width'
VirtualCode
indent_width(Code& code, u32 width);

namespace token {

void 
replace(Code& code, Token* a, Token* b);

} // namespace token
} // namespace format

namespace lines {

struct Options {
	// remove leading tabs up until the min amount of tabs over all lines
	// this doesn't take into account mixed spaces and tabs
	b32 remove_leading_whitespace;
	// display line numbers
	b32 line_numbers; 
	// when line numbers have different lengths, right align them
	b32 right_align_line_numbers; 
	// how many lines to gather before the given line
	u32 before; 
	 // how many lines to gather after the given line
	u32 after; 
};

struct Lines {
	DString* str;
	String line; // the line that this was created with
	Array<String> lines; // views into 'str', representing each gathered line
	Options opt;
	Token* token; // token originally used to retrieve these lines
};

// various methods for displaying multiple lines given some information
Lines get(Token* t, Options opt = {});
Lines get(TNode* n, Options opt = {});
template<typename T> Lines get(T* a, Options opt = {}); 

void  get(DString* start, Token* t, Options opt = {});
void  get(DString* start, TNode* n, Options opt = {});

void normalize_whitespace(Lines& lines);
void remove_leading_whitespace(Lines& lines);

} // namespace lines

// Code which stores a cursor as well as extra information for how to navigate the code
// struct TokenNavigator : public Code {

// };

// struct ASTNavigator : public Code {

// }

} // namespace code

void
to_string(DString* current, Code* c);

} // namespace amu

#endif // AMU_CODE_H
