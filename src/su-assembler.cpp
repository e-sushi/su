#include "su-assembler.h"

string ASMBuff = "";
bool allow_comments = true;
bool print_as_we_go = true;

//this may not be necessary and could just be a state enum, there only ever seems to be one flag set for each layer
struct Flags {
	b32 add  = 0;
	b32 sub  = 0;
	b32 mult = 0;
	b32 divi = 0;

	b32 AND            = 0;
	b32 bitAND         = 0;
	b32 OR             = 0;
	b32 bitOR          = 0;
	b32 less           = 0;
	b32 greater        = 0;
	b32 less_eq        = 0;
	b32 greater_eq     = 0;
	b32 equal          = 0;
	b32 not_equal      = 0;
	b32 modulo         = 0;
	b32 bitXOR         = 0;
	b32 bitshift_left  = 0;
	b32 bitshift_right = 0;


	b32 logi_not = 0;
	b32 bit_comp = 0;
	b32 negate   = 0;

	b32 factor_eval = 0;
	b32 term_eval   = 0;
};

//increment when a label is made so we can generate unique names
u32 label_count = 0;

inline void addASMLine(string asmLine, string comment = "") {
	

	if (print_as_we_go) {
		string out = "";

		if (asmLine[asmLine.size - 1] != ':' && asmLine[0] != '.') {
			out += "\n    " + asmLine;
		}
		else {
			out += "\n\n" + asmLine;
		}

		if (allow_comments && comment.size > 0) {
			if (asmLine.size > 18) {
				out += " # " + comment;
			}
			else {
				for (int i = 0; i < 18 - asmLine.size; i++) out += " ";
				out += "# " + comment;
			}
		}

		std::cout << out;
		ASMBuff += out;
	}
	else {
		//check if we're adding an instruction and tab if we are
		if (asmLine[asmLine.size - 1] != ':' && asmLine[0] != '.') {
			ASMBuff += "\n    " + asmLine;
		}
		else {
			ASMBuff += "\n\n" + asmLine;
		}

		if (allow_comments && comment.size > 0) {
			for (int i = 0; i < 16 - asmLine.size; i++) ASMBuff += " ";
			ASMBuff += "# " + comment;
		}
	}
	
}

void assemble_expressions(array<Expression>& expressions) {
	assert(expressions.size() != 0); "assemble_expression was passed an empty array";
	Flags flags;

	u32 label_num_on_enter = label_count;

	for (Expression& exp : expressions) {
		switch (exp.expression_type) {



			////////////////////////
			////				////
			////     Guards     ////
			////				////
			////////////////////////



			case ExpressionGuard_LogicalAND: {
				assemble_expressions(exp.expressions);
				//peek to see if there's an OR ahead
				if (expressions.peek().expression_type == Expression_BinaryOpOR) {
					//if there is we must check if the last result was true
					string label_num = itos(label_count);
					string end_label_num = itos(label_num_on_enter);
					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
					addASMLine("je    _ORLabel" + label_num);
					addASMLine("mov   $1,   %rax", "we didn't jump so last result was true");
					addASMLine("jmp   _ORend" + end_label_num);
					addASMLine("_ORLabel" + label_num + ":");
					label_count++;
				}
				else if(expressions.lookback().expression_type == Expression_BinaryOpOR) {
					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
					string label_num = itos(label_count);
					string end_label_num = itos(label_num_on_enter);
					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was true");
					addASMLine("setne %al");
					addASMLine("_ORend" + end_label_num + ":");
				}
			}break;

			case ExpressionGuard_BitOR: {
				assemble_expressions(exp.expressions);
				
				//peek to see if there's an AND ahead
				if (expressions.peek().expression_type == Expression_BinaryOpAND) {
					//if there is we must check if the last result was true
					string label_num = itos(label_count);
					string end_label_num = itos(label_num_on_enter);
					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
					addASMLine("jne   _ANDLabel" + label_num);
					addASMLine("jmp   _ANDend" + end_label_num);
					addASMLine("_ANDLabel" + label_num + ":");
					label_count++;
				}
				else if (expressions.lookback().expression_type == Expression_BinaryOpAND) {
					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
					string label_num = itos(label_count);
					string end_label_num = itos(label_num_on_enter);
					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was false");
					addASMLine("setne %al");
					addASMLine("_ANDend" + end_label_num + ":");
				}
			}break;

			case ExpressionGuard_BitXOR: {
				assemble_expressions(exp.expressions);
				if (flags.bitOR) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise OR");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("or    %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitOR = 0;
				}

			}break;

			case ExpressionGuard_BitAND: {
				assemble_expressions(exp.expressions);
				if (flags.bitXOR) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise XOR");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("xor   %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitXOR = 0;
				}
			}break;

			case ExpressionGuard_Equality: {
				assemble_expressions(exp.expressions);
				if (flags.bitAND) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise and");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("and   %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitAND = 0;
				}
			}break;

			case ExpressionGuard_Relational: {
				assemble_expressions(exp.expressions);
				if (flags.equal) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform equality check");
					addASMLine("mov   $0,   %rax");
					addASMLine("sete  %al");
					flags.equal = 0;
				}
				else if(flags.not_equal) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform equality check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setne %al");
					flags.not_equal = 0;
				}
			}break;

			case ExpressionGuard_BitShift: {
				assemble_expressions(exp.expressions);
				if (flags.less) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform less than check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setl  %al");
					flags.less = 0;
				}
				else if (flags.less_eq) { 
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform less than eq check");
					addASMLine("mov   $0,   %eax");
					addASMLine("setle %al");
					flags.less_eq = 0;
				}
				else if (flags.greater) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform greater than check");
					addASMLine("mov   $0,   %eax");
					addASMLine("setg  %al");
					flags.greater = 0;
				}
				else if (flags.greater_eq) {
					addASMLine("pop   %rcx",       "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform greater than eq check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setge %al");
					flags.greater_eq = 0;
				}
			}break;

			case ExpressionGuard_Additive: {
				assemble_expressions(exp.expressions);
				if (flags.bitshift_left) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for left bitshift");
					addASMLine("pop   %rax",       "retrieve stored from stack");
					addASMLine("shl   %cl, %rax", "left bitshift %rax by %rcx");
					flags.bitshift_left = 0;
				}
				else if (flags.bitshift_right) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for left bitshift");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("shr   %cl, %rax", "left bitshift %rax by %rcx");
					flags.bitshift_right = 0;
				}
				
			}break;

			case ExpressionGuard_Term: {
				assemble_expressions(exp.expressions);
				if (flags.add) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("add   %rcx, %rax", "add, store result in %rax");
					flags.add = 0;
				}
				else if (flags.sub) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for subtraction");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("sub   %rcx, %rax", "sub, store result in %rax");
					flags.sub = 0;
				}
			}break;

			case ExpressionGuard_Factor: {
				assemble_expressions(exp.expressions);
				if (flags.mult) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("imul  %rcx, %rax", "signed multiply, store result in %rax");
					flags.mult = 0;
				}
				else if (flags.divi) {
					addASMLine("mov   %rax, %rcx", "swap %rax and %rcx for division");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("cqto", "convert quad in %rax to octo in %rdx:%rax");
					addASMLine("idiv  %rcx", "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
					flags.divi = 0;
				}
				else if (flags.modulo) {
					addASMLine("mov   %rax, %rcx", "swap %rax and %rcx for division");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("cqto", "convert quad in %rax to octo in %rdx:%rax");
					addASMLine("idiv  %rcx", "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
					addASMLine("mov   %rdx, %rax", "move remainder into %rax");
					flags.modulo = 0;
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
				flags.AND = 1;

			}break;

			case Expression_BinaryOpBitAND: {
				addASMLine("push  %rax", "store %rax for bit and");
				flags.bitAND = 1;
			}break;

			case Expression_BinaryOpOR: {
				flags.OR= 1;

			}break;

			case Expression_BinaryOpBitOR: {
				addASMLine("push  %rax", "store %rax for bit or");
				flags.bitOR = 1;
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

			case Expression_BinaryOpModulo: {
				addASMLine("push  %rax", "store %rax for modulo");
				flags.modulo = 1;
			}break;

			case Expression_BinaryOpXOR: {
				addASMLine("push  %rax", "store %rax for xor");
				flags.bitXOR = 1;
			}break;

			case Expression_BinaryOpBitShiftLeft: {
				addASMLine("push  %rax", "store %rax for left bitshift");
				flags.bitshift_left = 1;
			}break;

			case Expression_BinaryOpBitShiftRight: {
				addASMLine("push  %rax", "store %rax for right bitshift");
				flags.bitshift_right = 1;
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
				addASMLine("cmp   $0,   %rax", "perform logical not");
				addASMLine("mov   $0,   %rax");
				addASMLine("sete  %al");
			}break;

			case Expression_UnaryOpNegate: {
				assemble_expressions(exp.expressions);
				addASMLine("neg   %rax", "perform negation");
			}break;


			////////////////////////
			////				////
			////    Literals    ////
			////				////
			////////////////////////

			case Expression_IntegerLiteral: {
				addASMLine("mov   $" + exp.expstr + ",%rax", "move integer literal into %rax");
			}break;
		}
		expressions.next();
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