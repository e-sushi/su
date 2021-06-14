#include "su-assembler.h"

string assemble_expression(Expression& exp) {
	string ASMBuff = "";

	//we have to start at the bottom of the expressions tree
	//to form asm code as far as I know right now
	if (exp.expressions.size() != 0) {
		for (Expression& e : exp.expressions) {
			ASMBuff += assemble_expression(e);
		}
	}

	switch (exp.expression_type) {
		case Expression_IntegerLiteral: {
			ASMBuff += "\nmov $" + exp.expstr + ", %rax";
		}break;
		case Expression_UnaryOpBitComp: {
			ASMBuff += "\nnot %rax     #Bitwise Complement ~";
		}break;
		case Expression_UnaryOpLogiNOT: {
			ASMBuff +=
				"\ncmp $0, %rax #Logical NOT !"
				"\nmov $0, %rax"
				"\nsete %al";
		}break;
		case Expression_UnaryOpNegate: {
			ASMBuff += "\nneg %rax     #Negation -";
		}break;
	}

	return ASMBuff;
}

string assemble_statement(Statement& statement) {
	string ASMBuff = "";

	switch (statement.statement_type) {
		case Statement_Return: {
			for (Expression& expression : statement.expressions) {
				ASMBuff += assemble_expression(expression);
				
			}
		}break;
	}

	return ASMBuff;
}

string assemble_function(Function& func) {
	string ASMBuff = "";

	//construct function label in asm
	ASMBuff += ".global " + func.identifier + "\n" + func.identifier + ":";
	ASMBuff += "\n"
		"pushq %rbp\n"
		"movq %rsp, %rbp";

	//construct function body
	for (Statement& statement : func.statements) {

		ASMBuff += assemble_statement(statement);

		
	}

	return ASMBuff;
}

string suAssembler::assemble(Program& program) {
	string ASMBuff = "";

	for (Function& func : program.functions) {
		ASMBuff += assemble_function(func);



		ASMBuff += "\npopq %rbp";
		ASMBuff += "\nret";
	}

	return ASMBuff;
}