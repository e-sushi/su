/*
 
	amu's Parser

	Notes
	-------

	When we're processing an entire source file or module, the strategy is to first find all labels within the lexical scope of it 
	and discretize the representing Code object into several child Code objects representing each independently process-able thing in 
	that lexical scope (functions, variables, imports, etc). This is done both to support using global labels anywhere and to make it 
	easy to parallelize Code objects and handle dependencies between them. 
	The parser does not automatically handle sending child Code objects down the processing pipeline in this case. I want to avoid this 
	because I believe it is better for Code objects to have direct control over sending each of its children down each pipeline in any 
	way they see fit. Originally what I did was discretize the Code objects, then sent each child down the entire pipeline (to AIR gen).
	I think it's better if Code objects have explicit control over when its children are processed and how far, though the one exception
	is when the Parser comes across a compile time expression. We segment this into its own Code object then process it all the way through
	the VM, but I think this is ok because there's no real way (or reason) to delegate this to the original Code object. 
	This design may end up sucking down the road, so if anyone else winds up working on this with me and thinks so, let me know.

*/

#ifndef AMU_PARSER_H
#define AMU_PARSER_H

#include "storage/Map.h"
#include "storage/Array.h"
#include "storage/String.h"
#include "representations/Label.h"
#include "representations/Code.h"

namespace amu {

struct Parser {
	Code* code;

	ASTNode* root;

	TableStack table;
	NodeStack node;

	code::TokenIterator token;


	// ~~~~~~ interface ~~~~~~~


	// creates a Parser for the given Code and returns it 
	static Parser*
	create(Code* code);

	void
	destroy();

	// begins parsing and returns true if successful
	// if the Code object represents Source or a Module then discretize_module()
	// is called and parsing ends. 
	// See notes above for why parsing a module/source file does not automatically parse 
	// its parts.
	b32
	parse();
	
	// discretizes a given module (or source file) into discrete
	// code objects based on the LexicalScope's marked labels. The new Code objects 
	// are attached to the current as children. This assumes a Module has not already
	// been created for the Code object and makes one itself. The token is also
	// assumed to be at the first token of the LexicalScope representing the module (or source file).
	b32
	discretize_module();

	// displays the current NodeStack 
	DString*
	display_stack();


	// actual stages of the Parser as methods so that we don't have to keep manually 
	// passing a Code object and TokenIterator around. These aren't meant to be 
	// manually accessed and I put them behind private just to keep them from polluting
	// Parser's namespace or whatever you'd call it in my IDE 
private:
	b32 start();
	b32 prescanned_label();
	b32 prescanned_function();
	b32 prescanned_type();
	b32 prescanned_var_decl();
	b32 struct_decl();
	b32 label();
	b32 label_in_tuple();
	b32 label_in_tuple_type();
	b32 label_in_for();
	b32 label_get();
	b32 label_group_after_comma();
	b32 label_after_colon();
	b32 tuple();
	b32 tuple_type();
	b32 expression();
	b32 access();
	b32 term();
	b32 additive();
	b32 bit_shift();
	b32 relational();
	b32 equality();
	b32 bit_and();
	b32 bit_xor();
	b32 bit_or();
	b32 logi_and();
	b32 logi_or();
	b32 range();
	b32 assignment();
	b32 conditional();
	b32 loop();
	b32 factor();
	b32 array_literal();
	b32 subscript();
	b32 block();
	b32 typeref();
	b32 reduce_builtin_type_to_typeref_expression();
	b32 reduce_literal_to_literal_expression();

	void node_start(ASTNode* node);
	void node_end(ASTNode* node);
};

namespace parser {

Parser*
create(Code* source);

void
destroy(Parser* parser);

void
parse(Code* code);

namespace stack {

void
push(Code* c, ASTNode* node);

// NOTE(sushi) this is somewhat dangerous cause you can accidentally pass something like 
//			   a function pointer to this and it will work
//			   but it's useful so i will keep it for now :)
//			   if our types inherited TNode, we could use std::is_base_of
//			   but i dont want to implement all of that rn
template<typename T> FORCE_INLINE void
push(Code* c, T* node) { push(c, (ASTNode*)node); }

ASTNode*
pop(Code* c);

ASTNode*
last(Code* c);

void
push_table(Code* c, LabelTable* table);

void
pop_table(Code* c);

LabelTable*
current_table(Code* c);

DString
display(Code* c);

} // namespace stack

namespace table {
// adds the given label to whatever table is currently active
void 
add(Code* c, String id, Label* l);
} // namespace table

namespace symbol {

Label*
search(Code* code, u64 hashed_id);

} // namespace symbol


} // namespace parser
} // namespace amu 

#endif // AMU_PARSER_H
