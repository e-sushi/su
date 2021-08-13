#include "su-assembler.h"
#include "utils/map.h"

string ASMBuff = "";
bool allow_comments = 1;
bool print_as_we_go = 1;

//this may not be necessary and could just be a state enum, there only ever seems to be one flag set for each layer
struct Flags {
	bool add  = 0;
	bool sub  = 0;
	bool mult = 0;
	bool divi = 0;

	bool AND            = 0;
	bool bitAND         = 0;
	bool OR             = 0;
	bool bitOR          = 0;
	bool less           = 0;
	bool greater        = 0;
	bool less_eq        = 0;
	bool greater_eq     = 0;
	bool equal          = 0;
	bool not_equal      = 0;
	bool modulo         = 0;
	bool bitXOR         = 0;
	bool bitshift_left  = 0;
	bool bitshift_right = 0;


	bool logi_not = 0;
	bool bit_comp = 0;
	bool negate   = 0;

	bool factor_eval = 0;
	bool term_eval   = 0;

	string var_offset = "";
	bool var_assignment = 1;
};

//increment when a label is made so we can generate unique names
struct {
	u32 OR_labels     = 0;
	u32 OR_end_labels = 0;

	u32 AND_labels     = 0;
	u32 AND_end_labels = 0;

	u32 if_labels     = 0;
	u32 if_end_labels = 0;
	u32 else_labels   = 0;
}labels;


//variable map for keeping track of variable names and their position on the stack
map<string, u32> var_map;

#define ExprFail(error)\
std::cout << "\n\nError: " << error << "\n caused by expression '" << exp.expstr << "'" << std::endl;

inline void addASMLine(string asmLine, string comment = "") {
	

	if (print_as_we_go) {
		string out = "";

		if (asmLine[asmLine.size - 1] != ':' && asmLine[0] != '.') {
			out += "\n    " + asmLine;
		}
		else {
			out += "\n" + asmLine;
		}

		if (allow_comments && comment.size > 0) {
			if (asmLine.size > 25) {
				out += " # " + comment;
			}
			else {
				for (int i = 0; i < 25 - asmLine.size; i++) out += " ";
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

//current statement
Statement* smt;
//current declaration
Declaration* decl;
void assemble_expressions(array<Expression>& expressions) {
	Assert(expressions.size() != 0, "assemble_expression was passed an empty array");
	Flags flags;

	//u32 label_num_on_enter = label_count;

	for (int i = 0; i < expressions.size(); i++) {
		Expression exp = expressions[i];
		switch (exp.type) {


	
			////////////////////////
			////				////
			////     Guards     ////
			////				////
			////////////////////////
	
	
	
			case ExpressionGuard_Assignment: {
				//TODO this needs to look prettier later
				if (decl) {
					if (var_map.at(decl->identifier)) { ExprFail("attempt to redeclare a variable"); }
					else {
						if (exp.expressions.size() != 0) {
							var_map.add(decl->identifier, (var_map.count + 1) * 8);
							assemble_expressions(exp.expressions);
							addASMLine("push  %rax", "save value of variable '" + decl->identifier + "' on the stack");
						}
						else {
							//case where we declare a variable but dont assign an expression to it
							//default to 0
							var_map.add(decl->identifier, (var_map.count + 1) * 8);
							addASMLine("mov   $0,  %rax", "default var value to 0");
							addASMLine("push  %rax", "save value of variable '" + decl->identifier + "' on the stack");
						}
					}
				}
				else{
					assemble_expressions(exp.expressions);
					addASMLine("mov   %rax, -" + flags.var_offset + "(%rbp)", "store result into specified variable");
				}
				
			}break;

			case ExpressionGuard_HEAD: {
				assemble_expressions(exp.expressions);
			}break;

			case ExpressionGuard_Conditional: {
				assemble_expressions(exp.expressions);
				//if (smt && smt->type == Statement_Conditional) {
				//	string label_num = string::toStr(label_count);
				//	string end_label_num = string::toStr(label_num_on_enter);
				//	
				//	
				//}

			}break;
	
			case ExpressionGuard_LogicalOR: {
				assemble_expressions(exp.expressions);
				//peek to see if there's an OR ahead
				if (i < expressions.size() - 1 && expressions[i + 1].type == Expression_BinaryOpOR) {
					//if there is we must check if the last result was true
					string label_num = string::toStr(labels.OR_labels);
					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
					addASMLine("je    _ORLabel" + label_num);
					addASMLine("mov   $1,   %rax", "we didn't jump so last result was true");
					addASMLine("jmp   _ORend" + string::toStr(labels.OR_end_labels));
					addASMLine("_ORLabel" + label_num + ":");
					labels.OR_labels++;
				}
				else if (i > expressions.size() + 1 && expressions[i - 1].type == Expression_BinaryOpOR) {
					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was true");
					addASMLine("setne %al");
					addASMLine("_ORend" + string::toStr(labels.OR_end_labels) + ":");
					labels.OR_end_labels++;
				}
			}break;
	
			case ExpressionGuard_LogicalAND: {
				assemble_expressions(exp.expressions);
				//peek to see if there's an AND ahead
				if (i < expressions.size() - 1 && expressions[i + 1].type == Expression_BinaryOpAND) {
					//if there is we must check if the last result was true
					string label_num = string::toStr(labels.AND_labels);
					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
					addASMLine("jne   _ANDLabel" + label_num);
					addASMLine("jmp   _ANDend" + string::toStr(labels.AND_end_labels));
					addASMLine("_ANDLabel" + label_num + ":");
					flags.AND = true;
					labels.AND_labels++;
				}
				else if (flags.AND) {//(i > expressions.size() + 1 && expressions[i - 1]->type == Expression_BinaryOpAND) {
					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was false");
					addASMLine("setne %al");
					addASMLine("_ANDend" + string::toStr(labels.AND_end_labels) + ":");
					labels.AND_end_labels++;
				}
				
			}break;
	
			case ExpressionGuard_BitOR: {
				assemble_expressions(exp.expressions);
				if (flags.bitOR) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise OR");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("or    %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitOR = 0;
				}
				
			}break;
	
			case ExpressionGuard_BitXOR: {
				assemble_expressions(exp.expressions);
				if (flags.bitXOR) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise XOR");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("xor   %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitXOR = 0;
				}
	
			}break;
	
			case ExpressionGuard_BitAND: {
				assemble_expressions(exp.expressions);
				if (flags.bitAND) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise and");
					addASMLine("pop   %rax", "retrieve stored from stack");
					addASMLine("and   %rcx, %rax", "bitwise and %rax with %rcx");
					flags.bitAND = 0;
				}
			}break;
	
			case ExpressionGuard_Equality: {
				assemble_expressions(exp.expressions);
				if (flags.equal) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform equality check");
					addASMLine("mov   $0,   %rax");
					addASMLine("sete  %al");
					flags.equal = 0;
				}
				else if (flags.not_equal) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform equality check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setne %al");
					flags.not_equal = 0;
				}
			}break;
	
			case ExpressionGuard_Relational: {
				assemble_expressions(exp.expressions);
				if (flags.less) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform less than check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setl  %al");
					flags.less = 0;
				}
				else if (flags.less_eq) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform less than eq check");
					addASMLine("mov   $0,   %eax");
					addASMLine("setle %al");
					flags.less_eq = 0;
				}
				else if (flags.greater) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform greater than check");
					addASMLine("mov   $0,   %eax");
					addASMLine("setg  %al");
					flags.greater = 0;
				}
				else if (flags.greater_eq) {
					addASMLine("pop   %rcx", "retrieve stored from stack");
					addASMLine("cmp   %rax, %rcx", "perform greater than eq check");
					addASMLine("mov   $0,   %rax");
					addASMLine("setge %al");
					flags.greater_eq = 0;
				}
			}break;
	
			case ExpressionGuard_BitShift: {
				assemble_expressions(exp.expressions);
				if (flags.bitshift_left) {
					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for left bitshift");
					addASMLine("pop   %rax", "retrieve stored from stack");
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
	
			case ExpressionGuard_Additive: {
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
	
			case ExpressionGuard_Term: {
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
	
			case ExpressionGuard_Factor: {
				assemble_expressions(exp.expressions);
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
	
			case Expression_BinaryOpAssignment: {
				flags.var_assignment = 1;
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
	
	
	
			////////////////////////
			////				////
			////   Identifier   ////
			////				////
			////////////////////////
	
	
	
			case Expression_IdentifierRHS: {
				try {
					flags.var_offset = string::toStr(*var_map.at(exp.expstr));
					addASMLine("mov   -" + flags.var_offset + "(%rbp), %rax", "store variable '" + exp.expstr + "' value into %rax for use in an expression");
				}
				catch (...) { ExprFail("attempt to reference an undeclared variable"); }
			}break;
	
			case Expression_IdentifierLHS: {
				try {
					flags.var_offset = string::toStr(*var_map.at(exp.expstr));
					if (smt->type == Statement_Return) {
					   // addASMLine("mov   -" + flags.var_offset + "(%rbp), %rax", "store variable's value into %rax for use in an expression");
					}
					
				}
				catch (...) { ExprFail("attempt to reference an undeclared variable"); }
			}break;
		}
		//expressions.next();
	}
}

//keeps track of certain info used by certain kinds of statements

struct {
	bool if_started = 0;
	string if_end_label_num = ""; //the label a chain of if statements jump to if one of their expressions is true
	array<u32> if_label_stack; //the labels an if statement jumps to if its exp is false
} statement_state;

//TODO clean up how if/else labels are kept track of here
bool returned = false;

void assemble_statement(Statement* statement) {
	smt = statement;

	bool master_if = false;

	switch (statement->type) {
		case Statement_If: {
			//assemble if statement's expression
			assemble_expressions(statement->expressions);
			addASMLine("cmp   $0,   %rax", "check if result was false for if statement");
			addASMLine("je    _IfEndLabel" + string::toStr(*statement_state.if_label_stack.last));

			//an if statement should only have one statement to assemble
			assemble_statement(&statement->statements[0]);
			
			
		}break;

		case Statement_Else: {
			//an else should only have one statement
			assemble_statement(&statement->statements[0]);
		}break;

		case Statement_Conditional: {
			//if there are no statements then we are just dealing with expressions
			if (statement->statements.count == 0) {
				assemble_expressions(statement->expressions);
			}
			//if we have only one statement, it must be an if
			else if(statement->statements.count == 1) {
				//push if's end label num to the stack
				statement_state.if_label_stack.add(labels.if_end_labels++);

				//statement_state.if_end_label_num = string::toStr(labels.if_end_labels);
				assemble_statement(&statement->statements[0]);

				//add if end label for assembled if
				addASMLine("_IfEndLabel" + string::toStr(*statement_state.if_label_stack.last) + ":");

				statement_state.if_label_stack.pop();
			}
			//if we have 2 statements we must have an if and an else
			else if (statement->statements.count == 2) {
				if (!statement_state.if_started) {
					//set state to know that we have started an if statement and have a final end label
					statement_state.if_started = 1;
					statement_state.if_end_label_num = string::toStr(labels.if_end_labels++);
					statement_state.if_label_stack.add(labels.if_end_labels++);
					master_if = 1;
				}


				statement_state.if_label_stack.add(labels.if_end_labels++);

				//assmeble if statement
				assemble_statement(&statement->statements[0]);

				//jump to final if statement if expression was true and we are in a chain of if elses
				addASMLine("jmp   _IfEndLabel" + string::toStr(*statement_state.if_label_stack.first));

				//add if end label for assembled if
				addASMLine("_IfEndLabel" + string::toStr(*statement_state.if_label_stack.last) + ":");

				statement_state.if_label_stack.pop();

				//assemble else statement
				assemble_statement(&statement->statements[1]);

				//add final if exit label if we started the if else chain
				if (master_if) {
					addASMLine("_IfEndLabel" + string::toStr(*statement_state.if_label_stack.first) + ":");
					master_if = 0;
					statement_state.if_started = 0;
				}
			}
			else {
				//ExprFail("A conditional statement found more than 2 statements attched to it!")
			}
			

		}break;

		case Statement_Expression: {
			assemble_expressions(statement->expressions);
		}break;

		case Statement_Return: {
			assemble_expressions(statement->expressions);
			returned = true;
			//I might want this to just be in assemble_function, but i saw a suggestion to do it here so we'll see
			addASMLine("mov   %rbp, %rsp", "restore %rsp of caller");
			addASMLine("pop   %rbp", "retore old %rbp");
			addASMLine("ret\n");
		}break;
	}



	smt = nullptr;
}

void assemble_declaration(Declaration* declaration) {
	decl = declaration;
	if (declaration->expressions.size() != 0) {
		assemble_expressions(declaration->expressions);
	}
	decl = nullptr;
}

void assemble_function(Function* func) {
	//construct function label in asm
	addASMLine(".global " + func->identifier);
	addASMLine(func->identifier + ":");


	//construct function body
	addASMLine("push  %rbp",       "save old stack frame base");
	addASMLine("mov   %rsp, %rbp", "current top of stack is now bottom of new stack frame");
	for (BlockItem& block_item : func->blockitems) {
		if (block_item.is_declaration) {
			assemble_declaration(&block_item.declaration);
		}
		else {
			assemble_statement(&block_item.statement);
		}
	}
	if (!returned) {
		addASMLine("mov   $0, %rax",   "no return statement was found so return 0 by default");
		addASMLine("mov   %rbp, %rsp", "restore %rsp of caller");
		addASMLine("pop   %rbp",       "retore old %rbp");
		addASMLine("ret");
	}
}

string suAssembler::assemble(Program& program) {
	for (Function& func : program.functions) {
		assemble_function(&func);
	}

	return ASMBuff;
}