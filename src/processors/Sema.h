/*

    Semantic analyzer 

	Sema manages its own stack of tables and nodes. 
	Functions expect the nodes they work with to be the last things on the NodeStack
	and functions do *not* remove these elements from the stack. Only the function who
	added them to the stack may remove them. 
	TODO(sushi) explaing why this is important when we need to modify the AST

	This may be incredibly inefficient, especially since now the stack is being managed
	on the heap, but it works for now and I will just optimize it later.

*/

#ifndef AMU_VALIDATOR_H
#define AMU_VALIDATOR_H

namespace amu {

struct LabelTable;

struct Sema {
	Code* code;

	TableStack tstack;
	NodeStack  nstack;


	// ~~~~ interface ~~~~


	static Sema*
	create(Code* code);

	void
	destroy();

	b32
	start();

private:
	b32 module();
	b32 label();
	b32 expr();
	b32 access();
	b32 typeref();
	b32 typedef_();
	b32 call();
	b32 function();
	b32 func_arg_tuple();
	b32 func_ret();
	b32 block();
};

} // namespace amu 

#endif // AMU_VALIDATOR_H
