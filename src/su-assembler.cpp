#include "su-assembler.h"

string ASMBuff = "";
bool allow_comments = true;
bool print_as_we_go = true;

//this may not be necessary and could just be a state enum, there only ever seems to be one flag set
struct Flags {
	b32 add  = 0;
	b32 sub  = 0;
	b32 mult = 0;
	b32 divi = 0;

	b32 AND = 0;
	b32 OR = 0;
	b32 less = 0;
	b32 greater = 0;
	b32 less_eq = 0;
	b32 greater_eq = 0;
	b32 equal = 0;
	b32 not_equal = 0;

	b32 logi_not = 0;
	b32 bit_comp = 0;
	b32 negate = 0;

	b32 factor_eval = 0;
	b32 term_eval   = 0;
};


inline void addASMLine(string asmLine, string comment = "") {
	

	if (print_as_we_go) {
		string out = "\n" + asmLine;

		if (allow_comments && comment.size > 0) {
			for (int i = 0; i < 18 - asmLine.size; i++) out += " ";
			out += "# " + comment;
		}

		std::cout << out;
		ASMBuff += out;
	}
	else {
		ASMBuff += "\n" + asmLine;

		if (allow_comments && comment.size > 0) {
			for (int i = 0; i < 16 - asmLine.size; i++) ASMBuff += " ";
			ASMBuff += "# " + comment;
		}
	}
	
}

void assemble_expressions(array<Expression>& expressions) {
	assert(expressions.size() != 0); "assemble_expression was passed an empty array";
	Flags flags;

	for (Expression& exp : expressions) {
		switch (exp.expression_type) {



			////////////////////////
			////				////
			////     Guards     ////
			////				////
			////////////////////////



			case ExpressionGuard_Term: {
				assemble_expressions(exp.expressions);
				//when the term exits we can then do addition/subtraction stuff
				if (flags.add) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("add   %rcx, %rax", "add, store result in %rax");
					flags.add = 0;
				}
				else if (flags.sub) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for subtraction");
					addASMLine("pop   %rax",       "retrieve stored from stack");
					addASMLine("sub   %rcx, %rax", "sub, store result in %rax");
					flags.sub = 0;
				}
			}break;

			case ExpressionGuard_Factor: {
				assemble_expressions(exp.expressions);
				//when the factor exits we can then do multiplication/division stuff
				if (flags.mult) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("imul  %rcx, %rax", "signed multiply, store result in %rax");
					flags.mult = 0;
				}
				else if (flags.divi) {
					addASMLine("mov   %rax, %rcx", "swap %rax and %rcx for division");
					addASMLine("pop   %rax",       "retrieve stored from stack");
					addASMLine("cqto",             "convert quad in %rax to octo in %rdx:%rax");
					addASMLine("idiv  %rcx",       "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
					flags.divi = 0;
				}
			}break;

			case ExpressionGuard_LogicalAND: {
				assemble_expressions(exp.expressions);
			}break;

			case ExpressionGuard_Equality: {
				assemble_expressions(exp.expressions);
				
			}break;

			case ExpressionGuard_Relational: {
				assemble_expressions(exp.expressions);
				if (flags.equal) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform equality check");
					addASMLine("mov   $0, %eax");
					addASMLine("sete  %al");
					flags.equal = 0;
				}
				else if(flags.not_equal) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform equality check");
					addASMLine("mov   $0, %eax");
					addASMLine("setne %al");
					flags.not_equal = 0;
				}
			}break;

			case ExpressionGuard_Additive: {
				assemble_expressions(exp.expressions);
				if (flags.less) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform less than check");
					addASMLine("mov   $0, %eax");
					addASMLine("setl  %al");
					flags.less = 0;
				}
				else if (flags.less_eq) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform less than eq check");
					addASMLine("mov   $0, %eax");
					addASMLine("setle %al");
					flags.less_eq = 0;
				}
				else if (flags.greater) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform greater than check");
					addASMLine("mov   $0, %eax");
					addASMLine("setg  %al");
					flags.greater = 0;
				}
				else if (flags.greater_eq) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %eax, %ecx", "perform greater than eq check");
					addASMLine("mov   $0, %eax");
					addASMLine("setge %al");
					flags.greater_eq = 0;
				}
			}break;



			////////////////////////
			////				////
			////   Binary Ops   ////
			////				////
			////////////////////////



			case Expression_BinaryOpPlus: {
				addASMLine("push  %rax", "store %rax for addition");
				flags.add = 1;
			}break;

			case Expression_BinaryOpMinus: {
				addASMLine("push  %rax", "store %rax for subtraction");
				flags.sub = 1;
			}break;

			case Expression_BinaryOpMultiply: {
				addASMLine("push  %rax", "store %rax for multiplication");
				flags.mult = 1;
			}break;

			case Expression_BinaryOpDivision: {
				addASMLine("push  %rax", "store %rax for division");
				flags.divi = 1;
			}break;

			case Expression_BinaryOpAND: {
				addASMLine("push  %rax", "store %rax for AND");

			}break;

			case Expression_BinaryOpOR: {
				addASMLine("push  %rax", "store %rax for equal check");

			}break;

			case Expression_BinaryOpLessThan: {
				addASMLine("push  %rax", "store %rax for < check");
				flags.less = 1;
			}break;

			case Expression_BinaryOpGreaterThan: {
				addASMLine("push  %rax", "store %rax for > check");
				flags.greater = 1;
			}break;

			case Expression_BinaryOpLessThanOrEqual: {
				addASMLine("push  %rax", "store %rax for <= check");
				flags.less_eq = 1;
			}break;

			case Expression_BinaryOpGreaterThanOrEqual: {
				addASMLine("push  %rax", "store %rax for >= check");
				flags.greater_eq = 1;

			}break;

			case Expression_BinaryOpEqual: {
				addASMLine("push  %rax", "store %rax for equal check");
				flags.equal = 1;
			}break;

			case Expression_BinaryOpNotEqual: {
				addASMLine("push  %rax", "store %rax for not equal check");
				flags.not_equal = 1;
			}break;



			////////////////////////
			////				////
			////   Unary  Ops   ////
			////				////
			////////////////////////



			case Expression_UnaryOpBitComp: {
				assemble_expressions(exp.expressions);
				addASMLine("not  %rax", "perform bitwise complement");
			}break;

			case Expression_UnaryOpLogiNOT: {
				assemble_expressions(exp.expressions);
				addASMLine("cmp   $0, %rax", "perform logical not");
				addASMLine("mov   $0, %rax");
				addASMLine("sete  %al");
			}break;

			case Expression_UnaryOpNegate: {
				assemble_expressions(exp.expressions);
				addASMLine("neg   %rax", "perform negation");
			}break;

			case Expression_IntegerLiteral: {
				addASMLine("mov   $" + exp.expstr + ", %rax", "move integer literal into %rax");
			}break;
		}
	}
}

void assemble_statement(Statement& statement) {
	switch (statement.statement_type) {
		case Statement_Return: {
			assemble_expressions(statement.expressions);
			addASMLine("ret");
		}break;
	}
}

void assemble_function(Function& func) {
	//construct function label in asm
	addASMLine(".global " + func.identifier);
	addASMLine(func.identifier + ":");

	//construct function body
	for (Statement& statement : func.statements) {
		assemble_statement(statement);
	}

}

string suAssembler::assemble(Program& program) {
	for (Function& func : program.functions) {
		assemble_function(func);
	}

	return ASMBuff;
}