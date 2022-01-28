struct Assembler{
	//settings
	b32 write_comments = true;
	u32 padding_width = 2;
	u32 instruction_width = 8;
	u32 args_width = 16;
	
	//state
	string output;
	
	b32 failed = false;
	b32 function_returned = false;
	b32 function_error = false; //TODO statement as well
	
	s32 if_nesting = 0;   //s32 so we dont underflow on errors
	s32 loop_nesting = 0; //^
	
	u32 label_counter = 1;
	//char label[16] = ".L?";
	
	Declaration* active_decl = 0;
	u32 stack_offset = 0;
	map<string,u32> var_map; //TODO use cstring, allow for local shadowing, use an arena
} assembler;


////////////////
//// @utils //// //NOTE assembly is in AT&T syntax: instr src,dest #comment
////////////////
//TODO setup args for function / start function / end function

local FORCE_INLINE void asm_pure(const char* str)   { assembler.output += str; }
local FORCE_INLINE void asm_pure(const string& str) { assembler.output += str; }
local FORCE_INLINE void asm_label(const char* str)  { assembler.output += str; assembler.output += ":\n"; }
local FORCE_INLINE void asm_label(const string& str){ assembler.output += str; assembler.output += ":\n"; }

local void
asm_instruction(const char* instruction, const char* comment){  //'pad instruction # comment\n'
    upt len_instruction = Max(strlen(instruction), upt(assembler.instruction_width + assembler.args_width + 1));
    char* str = 0;
	upt count = 0;
    if(comment && assembler.write_comments){
		count = assembler.padding_width + len_instruction + 2 + strlen(comment) + 1;
		assembler.output.reserve(assembler.output.count + count);
		str   = assembler.output.str + assembler.output.count;
        sprintf(str, "%-*s%-*s# %s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, comment);
    }else{
		count = assembler.padding_width + len_instruction + 1;
		assembler.output.reserve(assembler.output.count + count);
		str   = assembler.output.str + assembler.output.count;
        sprintf(str, "%-*s%-*s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction);
    }
	assembler.output.count += count;
}

local void
asm_instruction(const char* instruction, const char* args, const char* comment){  //'pad instruction args # comment\n'
    upt len_instruction = Max(strlen(instruction), upt(assembler.instruction_width));
    upt len_args = Max(strlen(args), upt(assembler.args_width));
    char* str = 0;
	upt count = 0;
    if(comment && assembler.write_comments){
		count = assembler.padding_width + len_instruction + len_args + 2 + strlen(comment) + 2;
		assembler.output.reserve(assembler.output.count + count);
		str   = assembler.output.str + assembler.output.count;
        sprintf(str, "%-*s%-*s %-*s# %s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, (int)len_args, args, comment);
    }else{
		count = assembler.padding_width + len_instruction + len_args + 2;
		assembler.output.reserve(assembler.output.count + count);
		str   = assembler.output.str + assembler.output.count;
        sprintf(str, "%-*s%-*s %-*s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, (int)len_args, args);
    }
    assembler.output.count += count;
}

local FORCE_INLINE void
asm_push_stack(u32 reg, const char* comment = 0){
	asm_instruction("pushq", registers_x64[reg], (comment) ? comment : "push register onto stack");
}

local FORCE_INLINE void
asm_pop_stack(u32 reg, const char* comment = 0){
	asm_instruction("popq",  registers_x64[reg], (comment) ? comment : "pop stack into register");
}

/////////////////////
//// @assembling ////
/////////////////////
void assemble_expression(Expression* expr);

FORCE_INLINE void assemble_binop_children(Expression* expr){
	Assert(expr->node.child_count == 2, "Binary operators must have two child nodes");
	//NOTE right then left because our AST grows left, so we want to push values to be used later onto the stack first
	assemble_expression(ExpressionFromNode(expr->node.last_child));
	asm_push_stack(Register_RAX);
	assemble_expression(ExpressionFromNode(expr->node.first_child));
}

local void
assemble_expression(Expression* expr){
	//if(assembler.write_comments) asm_pure(toStr("# ", expr->node.comment, "\n"));
	if(assembler.function_error) return;
	
	switch(expr->type){
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Identifiers
		case Expression_IdentifierLHS:{
			u32* offset = assembler.var_map.at(expr->expstr);
			if(offset){
				if(assembler.active_decl){
					asm_instruction("movq", toStr("%rax,-",*offset,"(%rbp)").str, toStr("copy the value at %rax in var ", assembler.active_decl->identifier).str);
				}else{
					asm_instruction("movq", toStr("%rax,-",*offset,"(%rbp)").str, "copy the value at %rax in a var");
				}
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", expr->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		case Expression_IdentifierRHS:{
			u32* offset = assembler.var_map.at(expr->expstr);
			if(offset){
				asm_instruction("movq", toStr("-",*offset,"(%rbp),%rax").str, toStr("copy value of var ",expr->expstr," in %rax").str);
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", expr->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Ternary
		case Expression_TernaryConditional:{
			Assert(expr->node.child_count == 3, "Expression_TernaryConditional must have three child nodes");
			
			string skip_if_label = toStr(".L",assembler.label_counter++);
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("cmpq", "$0,%rax",       "compare %rax against zero");
			asm_instruction("jz", skip_if_label.str, "jump over if body if false");
			
			//if true
			assemble_expression(ExpressionFromNode(expr->node.first_child->next));
			string skip_else_label = toStr(".L",assembler.label_counter++);
			asm_instruction("jmp", skip_else_label.str, "jump over else body if true");
			
			//if false
			asm_label(skip_if_label);
			assemble_expression(ExpressionFromNode(expr->node.last_child));
			
			//return to normal
			asm_label(skip_else_label);
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Literals
		case Expression_Literal:{
			Assert(expr->node.child_count == 0, "Expression_Literal must not have any child nodes");
			
			//string args = toStr("$",expr->integer_literal.value,",%rax");
			string args = "$" + to_string(expr->int64) + ",%rax";
			asm_instruction("movq", args.str, "copy integer literal into %rax");
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Unary Operators
		case Expression_UnaryOpBitComp:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpBitComp must have only one child node");
			
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("notq", "%rax", "perform bitwise complement");
		}break;
		
		case Expression_UnaryOpLogiNOT:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpLogiNOT must have only one child node");
			
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("cmpq", "$0,%rax", "perform logical not");
			asm_instruction("movq", "$0,%rax", "");
			asm_instruction("sete", "%al",     "");
		}break;
		
		case Expression_UnaryOpNegate:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpNegate must have only one child node");
			
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("negq", "%rax", "perform artihmetic negation");
		}break;
		
		case Expression_IncrementPrefix:{
			Assert(expr->node.child_count == 1, "Expression_IncrementPrefix must have only one child node");
			Assert(ExpressionFromNode(expr->node.first_child)->type == Expression_IdentifierRHS, "Expression_IncrementPrefix child must be Expression_IdentifierRHS");
			
			Expression* var = ExpressionFromNode(expr->node.first_child);
			u32* offset = assembler.var_map.at(var->expstr);
			if(offset){
				asm_instruction("incq", toStr("-",*offset,"(%rbp)").str,      toStr("increment value of var ",var->expstr).str);
				asm_instruction("movq", toStr("-",*offset,"(%rbp),%rax").str, toStr("copy value of var ",var->expstr," in %rax").str);
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", var->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		case Expression_IncrementPostfix:{
			Assert(expr->node.child_count == 1, "Expression_IncrementPostfix must have only one child node");
			Assert(ExpressionFromNode(expr->node.first_child)->type == Expression_IdentifierRHS, "Expression_IncrementPostfix child must be Expression_IdentifierRHS");
			
			Expression* var = ExpressionFromNode(expr->node.first_child);
			u32* offset = assembler.var_map.at(var->expstr);
			if(offset){
				asm_instruction("movq", toStr("-",*offset,"(%rbp),%rax").str, toStr("copy value of var ",var->expstr," in %rax").str);
				asm_instruction("incq", toStr("-",*offset,"(%rbp)").str,      toStr("increment value of var ",var->expstr).str);
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", var->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		case Expression_DecrementPrefix:{
			Assert(expr->node.child_count == 1, "Expression_DecrementPrefix must have only one child node");
			Assert(ExpressionFromNode(expr->node.first_child)->type == Expression_IdentifierRHS, "Expression_DecrementPrefix child must be Expression_IdentifierRHS");
			
			Expression* var = ExpressionFromNode(expr->node.first_child);
			u32* offset = assembler.var_map.at(var->expstr);
			if(offset){
				asm_instruction("decq", toStr("-",*offset,"(%rbp)").str,      toStr("decrement value of var ",var->expstr).str);
				asm_instruction("movq", toStr("-",*offset,"(%rbp),%rax").str, toStr("copy value of var ",var->expstr," in %rax").str);
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", var->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		case Expression_DecrementPostfix:{
			Assert(expr->node.child_count == 1, "Expression_DecrementPostfix must have only one child node");
			Assert(ExpressionFromNode(expr->node.first_child)->type == Expression_IdentifierRHS, "Expression_DecrementPostfix child must be Expression_IdentifierRHS");
			
			Expression* var = ExpressionFromNode(expr->node.first_child);
			u32* offset = assembler.var_map.at(var->expstr);
			if(offset){
				asm_instruction("movq", toStr("-",*offset,"(%rbp),%rax").str, toStr("copy value of var ",var->expstr," in %rax").str);
				asm_instruction("decq", toStr("-",*offset,"(%rbp)").str, toStr("decrement value of var ",var->expstr).str);
			}else{
				logfE("assembler", "Attempted to reference an undeclared variable: %s", var->expstr.str);
				assembler.function_error = true;
			}
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Binary Operators
		case Expression_BinaryOpOR:{
			//TODO: optimization, check for false/true literals
			Assert(expr->node.child_count == 2, "Expression_BinaryOpOR must have two child nodes");
			
			string label = toStr(".L",assembler.label_counter++);
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("cmpq", "$0,%rax", "check %rax for logical OR");
			asm_instruction("jnz", label.str,  "jump over right side of logical OR if true");
			assemble_expression(ExpressionFromNode(expr->node.last_child));
			asm_label(label);
		}break;
		
		case Expression_BinaryOpAND:{
			//TODO: optimization, check for false/true literals
			Assert(expr->node.child_count == 2, "Expression_BinaryOpAND must have two child nodes");
			
			string label = toStr(".L",assembler.label_counter++);
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("cmpq", "$0,%rax", "check %rax for logical AND");
			asm_instruction("jz", label.str,   "jump over right side of logical AND if false");
			assemble_expression(ExpressionFromNode(expr->node.last_child));
			asm_label(label);
		}break;
		
		case Expression_BinaryOpBitOR:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("orq",  "%rdx,%rax", "bitwise or %rax with %rdx");
		}break;
		
		case Expression_BinaryOpBitXOR:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("xorq", "%rdx,%rax", "bitwise xor %rax with %rdx");
		}break;
		
		case Expression_BinaryOpBitAND:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("andq", "%rdx,%rax", "bitwise and %rax with %rdx");
		}break;
		
		case Expression_BinaryOpEqual:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq", "%rax,%rdx", "perform comparison, %rdx == %rax");
			asm_instruction("sete", "%al",       "set %al if equal");
		}break;
		
		case Expression_BinaryOpNotEqual:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq",  "%rax,%rdx", "perform comparison, %rdx != %rax");
			asm_instruction("setne", "%al",       "set %al if not equal");
		}break;
		
		case Expression_BinaryOpLessThan:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq", "%rdx,%rax", "perform comparison, %rdx < %rax");
			asm_instruction("setl", "%al",       "set %al if less than");
		}break;
		
		case Expression_BinaryOpLessThanOrEqual:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq",  "%rdx,%rax", "perform comparison, %rdx <= %rax");
			asm_instruction("setle", "%al",       "set %al if less than/equal");
		}break;
		
		case Expression_BinaryOpGreaterThan:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq", "%rdx,%rax", "perform comparison, %rdx > %rax");
			asm_instruction("setg", "%al",       "set %al if greater than");
		}break;
		
		case Expression_BinaryOpGreaterThanOrEqual:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("cmpq",  "%rdx,%rax", "perform comparison, %rdx >= %rax");
			asm_instruction("setge", "%al",       "set %al if greater than/equal");
		}break;
		
		case Expression_BinaryOpBitShiftLeft:{
			assemble_binop_children(expr);
			
			asm_instruction("movq", "%rax,%rdx", "copy %rax into %rdx for bitshift left");
			asm_pop_stack(Register_RAX);
			asm_instruction("shlq", "%cl,%rax",  "bitshift left %rax by %rdx");
		}break;
		
		case Expression_BinaryOpBitShiftRight:{
			assemble_binop_children(expr);
			
			asm_instruction("movq", "%rax,%rdx", "copy %rax into %rdx for bitshift right");
			asm_pop_stack(Register_RAX);
			asm_instruction("shrq", "%cl,%rax",  "bitshift right %rax by %rdx");
		}break;
		
		case Expression_BinaryOpPlus:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("addq", "%rdx,%rax", "add, store result in %rax");
		}break;
		
		case Expression_BinaryOpMinus:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("subq", "%rdx,%rax", "subtract %rax - %rdx, store result in %rax");
		}break;
		
		case Expression_BinaryOpMultiply:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RDX);
			asm_instruction("imulq", "%rdx,%rax", "signed multiply, store result in %rax");
		}break;
		
		case Expression_BinaryOpDivision:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RCX);
			asm_instruction("cqto",          "sign extend %rax into %rdx:%rax");
			asm_instruction("idivq", "%rcx", "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
		}break;
		
		case Expression_BinaryOpModulo:{
			assemble_binop_children(expr);
			
			asm_pop_stack(Register_RCX);
			asm_instruction("cqto",               "sign extend %rax into %rdx:%rax");
			asm_instruction("idivq", "%rcx",      "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
			asm_instruction("movq",  "%rdx,%rax", "copy remainder from %rdx into %rax");
		}break;
		
		case Expression_BinaryOpAssignment:{
			Assert(expr->node.child_count == 2, "Expression_BinaryOpAssignment must have only two child nodes");
			
			assemble_expression(ExpressionFromNode(expr->node.last_child));
			assemble_expression(ExpressionFromNode(expr->node.first_child));
		}break;
		
		default:{ NotImplemented; assembler.failed = true; }break;
	}
}

local void assemble_scope(Scope* scope);
local void
assemble_statement(Statement* stmt){
	if(assembler.function_error) return;
	
    switch(stmt->type){
        case Statement_Return:{
			for_node(stmt->node.first_child) assemble_expression(ExpressionFromNode(it));
			
			asm_instruction("leave", "restore the previous stack pointer and base pointers (end scope)");
            asm_instruction("ret",   "return code pointer back to func call site");
            assembler.function_returned = true;
        }break;
		
		case Statement_Expression:{
			for_node(stmt->node.first_child) assemble_expression(ExpressionFromNode(it));
		}break;
		
		case Statement_Conditional:{
			Assert(stmt->node.child_count >= 1, "Statement_Conditional must have at least 1 child node");
			
			assembler.if_nesting += 1;
			
			//first arg is boolean expression
			string skip_else_label;
			string skip_if_label = toStr(".L",assembler.label_counter++);
			assemble_expression(ExpressionFromNode(stmt->node.first_child));
			asm_instruction("cmpq", "$0,%rax",       "compare %rax against zero");
			asm_instruction("jz", skip_if_label.str, "jump over if body if false");
			
			//if body
			b32 else_encountered = false;
			for(Node* it = stmt->node.first_child->next; it != 0; it = it->next){ //start from second child
				if(it->type == NodeType_Statement){
					Statement* child = StatementFromNode(it);
					if(child->type == Statement_Else){
						else_encountered = true;
						skip_else_label = toStr(".L",assembler.label_counter++);
						asm_instruction("jmp", skip_else_label.str, "jump over else body if true");
						break; //don't assemble it yet, we will do that below
					}
					
					assemble_statement(child);
				}else if(it->type == NodeType_Scope){
					assemble_scope(ScopeFromNode(it));
				}else{ NotImplemented; assembler.failed = true; }
			}
			
			asm_label(skip_if_label);
			//else body
			if(else_encountered){
				Assert(stmt->node.last_child->type == NodeType_Statement && StatementFromNode(stmt->node.last_child)->type == Statement_Else, "If there is an else statement as a child node to an if statement, it must be the last node");
				assemble_statement(StatementFromNode(stmt->node.last_child));
				asm_label(skip_else_label);
			}else{
				assembler.if_nesting -= 1;
			}
		}break;
		
		case Statement_Else:{
			if(assembler.if_nesting <= 0){
				logfE("assembler", "Else statement has no matching if statement"); //TODO catch this in the parser
				assembler.function_error = true;
				return;
			}
			
			for_node(stmt->node.first_child) assemble_statement(StatementFromNode(it));
			assembler.if_nesting -= 1;
		}break;
		
		case Statement_For:{
			log("assembler", "Statement_For not implemented yet");
		}break;
		
		case Statement_While:{
			log("assembler", "Statement_While not implemented yet");
		}break;
		
		case Statement_Break:{
			log("assembler", "Statement_Break not implemented yet");
		}break;
		
		case Statement_Continue:{
			log("assembler", "Statement_Continue not implemented yet");
		}break;
		
		case Statement_Struct:{
			log("assembler", "Statement_Struct not implemented yet");
		}break;
		
		default:{ NotImplemented; assembler.failed = true; }break;
    }
}

local void
assemble_declaration(Declaration* decl){
	if(assembler.function_error) return;
	
	//NOTE a structure size of 'npos' means that it got past parser without being defined properly
	//TODO get type size from declaration
	//TODO need sizing information overall to call the correct registers
	if(decl->type != DataType_Signed32){
		logfE("assembler", "Declaration '%s' declared with type '%s' which is unhandled currently", decl->identifier.str, dataTypeStrs[decl->type]);
		return;
	}
	
	assembler.stack_offset += 8; //sizeof(s64) == 4
	assembler.var_map.add(decl->identifier, assembler.stack_offset);
	
	assembler.active_decl = decl;
	asm_instruction("pushq", "$0", toStr("init var ", decl->identifier, " to zero").str);
	for_node(decl->node.first_child) assemble_expression(ExpressionFromNode(it));
	assembler.active_decl = 0;
}

local void
assemble_scope(Scope* scope){
	if(assembler.function_error) return;
	
	for_node(scope->node.first_child){
		switch(it->type){
			case NodeType_Declaration: assemble_declaration(DeclarationFromNode(it)); break;
			case NodeType_Statement: assemble_statement(StatementFromNode(it)); break;
			case NodeType_Scope: assemble_scope(ScopeFromNode(it)); break;
			default:{ NotImplemented; assembler.failed = true; }break;
		}
	}
}

local void
assemble_function(Function* func){
	//TODO func either has to have a return statement or an else with a return statement
	if(func->node.child_count == 0) return;
	Assert(func->node.child_count == 1 && func->node.first_child->type == NodeType_Scope, "A function only has one child and it has to be a scope if its a definition");
	
    assembler.function_returned = false;
    
	asm_label(func->identifier);
	asm_instruction("pushq", "%rbp",      "save base pointer to stack (start function)");
    asm_instruction("movq",  "%rsp,%rbp", "save the current stack pointer as the base pointer");
	
	for_node(ScopeFromNode(func->node.first_child)->node.first_child){ //NOTE manually handle scope to check for returns
		switch(it->type){
			case NodeType_Declaration:{
				assemble_declaration(DeclarationFromNode(it));
			}break;
			case NodeType_Statement:{
				assemble_statement(StatementFromNode(it));
			}break;
			case NodeType_Scope:{
				assemble_scope(ScopeFromNode(it));
			}break;
			default:{ NotImplemented; assembler.failed = true; }break;
		}
	}
	
    if(func->type != DataType_Void && !assembler.function_returned && !equals(func->identifier, cstr_lit("main"))){
		log_warning(WC_Not_All_Paths_Return, func->token_start->file.count, func->token_start->file.str, func->token_start->line_start, func->token_start->col_start, func->identifier.count, func->identifier.str);
		if(globals.warnings_as_errors) assembler.function_error = true;
		
		asm_instruction("mov", "$0,%rax", "no return statement was found so return 0 by default");
		asm_instruction("leave",          "restore the previous stack pointer and base pointers");
        asm_instruction("ret",            "return code pointer back to func call site (end function)");
    }
	
	if(assembler.function_error){
		assembler.failed = true;
		assembler.function_error = false;
	}
}

u32 assemble_program(Program& program, string& assembly) {
    assembler.output.reserve(1024);
    string filename(program.filename); filename = "\"" + filename + "\"";
    asm_instruction(".file", filename.str, "start of this file");
    asm_instruction(".text",               "start of code section");
    asm_instruction(".globl", "main", "marks the function 'main' as being global"); //TODO add entry point to Program
    
	for_node(program.node.first_child){
		assemble_function(FunctionFromNode(it));
	}
	
	if(assembler.failed){
		logfE("assembler", "Program '' failed to compile due to internal errors");
		return 1;
	}
	
	assembly = assembler.output;
	return EC_Success;
}