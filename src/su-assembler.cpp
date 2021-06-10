#include "su-assembler.h"


string suAssembler::assemble(AST& program) {
	string ASMBuff = "";



	for (AST* function : program.children) {
		assert(function->type == AST_Function); "A node was found to be something other than a function when it shouldn't be";
		Function* func = (Function*)function;
		//construct function label in asm
		//TODO make overloads for adding const char[] to our string
		ASMBuff += ".global ";
		ASMBuff += func->identifier;
		ASMBuff += "\n";
		ASMBuff += func->identifier;
		ASMBuff += ":\n";
		
		//construct function body
		for (AST* statement : function->children) {
			assert(statement->type == AST_Statement); "A node was found to be something other than a statement inside of a function";
			Statement* st = (Statement*)statement;

			switch (st->statement_type) {
				case Statement_Return: {
					int retint = 0;
					for (AST* expression : st->children) {
						Expression* exp = (Expression*)expression;
						ASMBuff += "mov ";
						ASMBuff += "$";
						ASMBuff += exp->expstr;
						ASMBuff += ", %rax\nret";
					}
					
				}break;
			}
		
		}

	
	}

	return ASMBuff;
}