
//array<token>& tokens = lexer.tokens;

b32 parse_failed = false;

#define ParseFail(...)\
{logE("parser", __VA_ARGS__, "\n caused by token '", TokenTypes_Names[curt.type], "' on line ", curt.l0); parse_failed = true;}

#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectGroup(Token_Type)\
if(curt.group == Token_Type) 

#define ExpectOneOf(...)\
if(match_any(curt.type, __VA_ARGS__)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 

#define ElseExpectGroup(Token_Type)\
else if(curt.group == Token_Type) 

#define ExpectSignature(...) if(check_signature(__VA_ARGS__))
#define ElseExpectSignature(...)  else if(check_signature(__VA_ARGS__))

#define ExpectFail(...)\
else { ParseFail(__VA_ARGS__); }

#define ExpectFailCode(failcode, error)\
else { ParseFail(error); failcode }

#define expstr(type) ExTypeStrings[type]

local map<TokenType, ExpressionType> tokToExp{
	{Token_Multiplication,     Expression_BinaryOpMultiply},
	{Token_Division,           Expression_BinaryOpDivision},
	{Token_Negation,           Expression_BinaryOpMinus},
	{Token_Plus,               Expression_BinaryOpPlus},
	{Token_AND,                Expression_BinaryOpAND},
	{Token_OR,                 Expression_BinaryOpOR},
	{Token_LessThan,           Expression_BinaryOpLessThan},
	{Token_GreaterThan,        Expression_BinaryOpGreaterThan},
	{Token_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{Token_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{Token_Equal,              Expression_BinaryOpEqual},
	{Token_NotEqual,           Expression_BinaryOpNotEqual},
	{Token_BitAND,             Expression_BinaryOpBitAND},
	{Token_BitOR,              Expression_BinaryOpBitOR},
	{Token_BitXOR,             Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
	{Token_BitNOT,             Expression_UnaryOpBitComp},
	{Token_LogicalNOT,         Expression_UnaryOpLogiNOT},
	{Token_Negation,           Expression_UnaryOpNegate},
};

map<cstring, Node*> knownFuncs; 
map<cstring, Node*> knownVars;  
map<cstring, Node*> knownStructs;

Node ParserDebugTree; //keeps a stack trace of the parser if enabled so it can be output later
Node* ParserDebugTreeCurrent = &ParserDebugTree;
Arena ParserDebugArena;

inline DataType dataTypeFromToken(TokenType type) {
	switch (type) {
		case Token_Void      : {return DataType_Void;}
		case Token_Signed8   : {return DataType_Signed8;}  
		case Token_Signed16  : {return DataType_Signed16; }
		case Token_Signed32  : {return DataType_Signed32;}     
		case Token_Signed64  : {return DataType_Signed64;}    
		case Token_Unsigned8 : {return DataType_Unsigned8;}  
		case Token_Unsigned16: {return DataType_Unsigned16;}  
		case Token_Unsigned32: {return DataType_Unsigned32;}
		case Token_Unsigned64: {return DataType_Unsigned64; }
		case Token_Float32   : {return DataType_Float32;}     
		case Token_Float64   : {return DataType_Float64;}   
		case Token_String    : {return DataType_String;}    
		case Token_Any       : {return DataType_Any;}
		case Token_Struct    : {return DataType_Structure;}    
		default: {PRINTLN("given token type is not a data type"); return DataType_Void;}
	}
}

inline upt dataTypeSizes(DataType type) {
	switch (type) {
		case DataType_Void       : {return    0;}
		case DataType_Signed8    : {return    1;}
		case DataType_Signed16   : {return    2;}
		case DataType_Signed32	 : {return    4;}
		case DataType_Signed64	 : {return    8;}
		case DataType_Unsigned8	 : {return    1;}
		case DataType_Unsigned16 : {return    2;}
		case DataType_Unsigned32 : {return    4;}
		case DataType_Unsigned64 : {return    8;}
		case DataType_Float32	 : {return    4;}
		case DataType_Float64	 : {return    8;}
		case DataType_String	 : {return    0;} //TODO size of string
		case DataType_Any		 : {return    0;} //most likely determined at compile or runtime
		case DataType_Structure	 : {return npos;} //should always be determined at compile time
	}
}
#define DebugPrintConversions(type1, type2) log("DebugConversionPrint", STRINGIZE(type1), " was converted to ", STRINGIZE(type2))

#define ConvertWarnIfOverflow(actualtype, type1, type2) \
actualtype og = sec->type1; sec->type2 = og; \
if (og != sec->type2) { \
log_warning_custom(WC_Overflow, int(pri->token_start->file.count), pri->token_start->file.str, pri->token_start->l0, pri->token_start->c0, \
toStr(" Value went from '",og,"' to '",sec->type2,"'.\n").str, STRINGIZE(type1), STRINGIZE(type2)); \
} DebugPrintConversions(type1, type2)


#define ConvertGroup(a,b) \
switch (pri->datatype) { \
case DataType_Signed8:    { ConvertWarnIfOverflow(a, b,    int8); return true; } \
case DataType_Signed16:   { ConvertWarnIfOverflow(a, b,   int16); return true; } \
case DataType_Signed32:   { ConvertWarnIfOverflow(a, b,   int32); return true; } \
case DataType_Signed64:   { ConvertWarnIfOverflow(a, b,   int64); return true; } \
case DataType_Unsigned8:  { ConvertWarnIfOverflow(a, b,   uint8); return true; } \
case DataType_Unsigned16: { ConvertWarnIfOverflow(a, b,  uint16); return true; } \
case DataType_Unsigned32: { ConvertWarnIfOverflow(a, b,  uint32); return true; } \
case DataType_Unsigned64: { ConvertWarnIfOverflow(a, b,  uint64); return true; } \
case DataType_Float32:    { ConvertWarnIfOverflow(a, b, float32); return true; } \
case DataType_Float64:    { ConvertWarnIfOverflow(a, b, float64); return true; } \
case DataType_Structure:  { NotImplemented; } \
} (void)0

//secn is casted to match prin if possible
b32 type_check(Node* prin, Node* secn) {
	Expression* pri = ExpressionFromNode(prin);
	Expression* sec = ExpressionFromNode(secn);
	
	switch (sec->datatype) {
		case DataType_Signed8:    { ConvertGroup( s8,    int8); }break;
		case DataType_Signed16:   { ConvertGroup(s16,   int16); }break;
		case DataType_Signed32:   { ConvertGroup(s32,   int32); }break;
		case DataType_Signed64:   { ConvertGroup(s64,   int64); }break;
		case DataType_Unsigned8:  { ConvertGroup( u8,   uint8); }break;
		case DataType_Unsigned16: { ConvertGroup(u16,  uint16); }break;
		case DataType_Unsigned32: { ConvertGroup(u32,  uint32); }break;
		case DataType_Unsigned64: { ConvertGroup(u64,  uint64); }break;
		case DataType_Float32:    { ConvertGroup(f32, float32); }break;
		case DataType_Float64:    { ConvertGroup(f64, float64); }break;
		case DataType_Structure:  { NotImplemented; }break;
	}
	return false;
}

void set_expression_type_from_declaration(Node* exp, Node* decl) {
	Declaration* d = DeclarationFromNode(decl);
	Expression* e = ExpressionFromNode(exp);
	switch (d->type) {
		case DataType_Void       : {                                 e->datatype=DataType_Void;      }break;
		case DataType_Signed8    : {e->int8        = d->int8;        e->datatype=DataType_Signed8;   }break;
		case DataType_Signed16   : {e->int16       = d->int16;       e->datatype=DataType_Signed16;  }break;
		case DataType_Signed32   : {e->int32       = d->int32;       e->datatype=DataType_Signed32;  }break;
		case DataType_Signed64   : {e->int64       = d->int64;       e->datatype=DataType_Signed64;  }break;
		case DataType_Unsigned8  : {e->uint8       = d->uint8;       e->datatype=DataType_Unsigned8; }break;
		case DataType_Unsigned16 : {e->uint16      = d->uint16;      e->datatype=DataType_Unsigned16;}break;
		case DataType_Unsigned32 : {e->uint32      = d->uint32;      e->datatype=DataType_Unsigned32;}break;
		case DataType_Unsigned64 : {e->uint64      = d->uint64;      e->datatype=DataType_Unsigned64;}break;
		case DataType_Float32    : {e->float32     = d->float32;     e->datatype=DataType_Float32;   }break;
		case DataType_Float64    : {e->float64     = d->float64;     e->datatype=DataType_Float64;   }break;
		case DataType_String     : {e->str         = d->str;         e->datatype=DataType_String;    }break; 
		case DataType_Structure  : {e->struct_type = d->struct_type; e->datatype=DataType_Structure; }break;  
		case DataType_Any        : { /*TODO any type stuff*/ }break;
	}
}

enum ParseState_ {
	stNone        = 0,
	stInFunction  = 1 << 0,
	stInForLoop   = 1 << 1,
	stInWhileLoop = 1 << 2, 
}; typedef u32 ParseState;
ParseState pState = stNone;

#define StateSet(flag)    AddFlag(pState, flag)
#define StateUnset(flag)  RemoveFlag(pState, flag)
#define StateHas(flag)    HasFlag(pState, flag)
#define StateHasAll(flag) HasAllFlags(pState, flag)

template<typename T>
T compile_time_binop(ExpressionType type, T a, T b) {
	switch (type) {
		case Expression_BinaryOpPlus:               {return a +  b;}break;
		case Expression_BinaryOpMinus:              {return a -  b;}break;
		case Expression_BinaryOpMultiply:           {return a *  b;}break;
		case Expression_BinaryOpDivision:           {return a /  b;}break;
		case Expression_BinaryOpAND:                {return a && b;}break;
		case Expression_BinaryOpBitAND:             {return a &  b;}break;
		case Expression_BinaryOpOR:                 {return a || b;}break;
		case Expression_BinaryOpBitOR:              {return a |  b;}break;
		case Expression_BinaryOpLessThan:           {return a <  b;}break;
		case Expression_BinaryOpGreaterThan:        {return a >  b;}break;
		case Expression_BinaryOpLessThanOrEqual:    {return a <= b;}break;
		case Expression_BinaryOpGreaterThanOrEqual: {return a >= b;}break;
		case Expression_BinaryOpEqual:              {return a == b;}break;
		case Expression_BinaryOpNotEqual:           {return a != b;}break;
		case Expression_BinaryOpModulo:             {return a %  b;}break;
		case Expression_BinaryOpBitXOR:             {return a ^  b;}break;
		case Expression_BinaryOpBitShiftLeft:       {return a << b;}break;
		case Expression_BinaryOpBitShiftRight:      {return a >> b;}break;
	}
	return a;
}



//node is the parent of the binop operation, ret is what was returned from outside of this function
//this function generalizes the binop parsing done throughout define
//TODO add a way to choose left vs right assoviativity maybe



#define Define(stage, node) define(stage, node)//debug_define(stage, node)

const char* stagestr[] = {
	"Global",
	"Struct",
	"Function",
	"Scope",
	"Declaration",
	"Statement",
	"Expression",
	"Conditional",
	"LogicalOR",
	"LogicalAND",
	"BitwiseOR",
	"BitwiseXOR",
	"BitwiseAND",
	"Equality",
	"Relational",
	"Bitshift",
	"Additive",
	"Term",
	"Factor",
};

const char* nodeTypeStrs[] = {
	"Program",
	"Structure",
	"Function",
	"Scope",
	"Declaration",
	"Statement",
	"Expression",
};

Node* declare(Node* node, NodeType type);
Node* define(ParseStage stage, Node* node);
auto debug_define = [&](ParseStage stage, Node* node) -> Node* {
	if (!ParserDebugArena.data) ParserDebugArena.init(Kilobytes(1));
	
	Node* nu = (Node*)ParserDebugArena.add(Node());
	nu->comment = toStr(
						"Define called\n",
						"stage: ", stagestr[stage], "\\l",
						"node :\\l",
						"   type: ", nodeTypeStrs[node->type], "\\l"
						"	com:  ", node->comment, "\\l"
						);
	
	insert_last(ParserDebugTreeCurrent, nu);
	ParserDebugTreeCurrent = nu;
	
	Node* ret = define(stage, node);
	
	nu = (Node*)ParserDebugArena.add(Node());
	nu->comment = toStr(
						"Define returned\n",
						"stage: ", stagestr[stage], "\\l",
						"node :\\l",
						"\ttype: ", (ret ? nodeTypeStrs[ret->type]: "null"), "\\l",
						"\tcom:  ", (ret ? ret->comment : "null"), "\\l"
						);
	
	insert_last(ParserDebugTreeCurrent, nu);
	ParserDebugTreeCurrent = nu;
	
	return ret;
	
};

//declares a node of a given type and returns it 
Node* Parser::declare(Node* node, NodeType type) {
	//TODO overloaded functions have different signatures
	switch (type) {
		case NodeType_Function: {
			//TODO check for redefinition
			string* funclabel = (string*)arena.add(string());
			ExpectGroup(TokenGroup_Type) {
				DataType dtype = dataTypeFromToken(curt.type);
				token_next();
				Expect(Token_Identifier) {
					Node* me = new_function(curt.raw, toStr("func: ", dataTypeStrs[dtype], " ", curt.raw));
					Function* func = parser.function;
					func->token_start = tokens->iter-1;
					insert_last(node, me);
					function->type = dtype;
					*funclabel = toStr(curt.raw, "@", dataTypeStrs[dtype], "@");
					token_next();
					Expect(Token_OpenParen) {
						while (next_match_group(TokenGroup_Type)) {
							token_next();
							Declaration* ret = DeclarationFromNode(declare(me, NodeType_Declaration));
							*funclabel += toStr(dataTypeStrs[ret->type], ",");
							function->positional_args++;
							while (!next_match(Token_Comma, Token_CloseParen)) token_next();
							token_next();
						}
						//not really necessary, so remove if u want
						if (funclabel->endsWith("@")) *funclabel += "void";
						else if (funclabel->endsWith(",")) (*funclabel)--;
						//HACK
						if (next_match(Token_CloseParen)) token_next();
						Expect(Token_CloseParen) { 
							knownFuncs.add(function->identifier, me);
							function->token_idx = currTokIdx() + 1;
							function->internal_label = cstring{ funclabel->str, funclabel->count };
						}
						ExpectFail("expected a ) for func decl ", function->identifier);
						func->token_end = tokens->iter+1;
						return me;
					}ExpectFail("expected ( for function declaration of ", function->identifier);
				}ExpectFail("expected identifier for function declaration");
			}ExpectFail("expected typename for function declaration");
		}break;
		
		case NodeType_Structure: {
			Expect(Token_StructDecl) {
				token_next();
				Expect(Token_Identifier) {
					Node* me = new_structure(curt.raw, toStr("struct: ", curt.raw));
					Struct* strct = parser.structure;
					strct->token_start = tokens->iter-1;
					insert_last(node, me);
					token_next();
					structure->token_idx = currTokIdx();
					Expect(Token_OpenBrace) {
						while (match_any(tokens->peek().group, TokenGroup_Type)) {
							token_next();
							DataType dtype = dataTypeFromToken(curt.type);
							token_next();
							Expect(Token_Identifier) {
								cstring id = curt.raw;
								token_next();
								Expect(Token_OpenParen) {
									Node* f;
									if (knownFuncs.has(id)) {
										f = *knownFuncs.at(id);
										change_parent(me, f);
										knownFuncs.remove(id); //TODO do optimized swap remove instead
										while (!next_match(Token_CloseParen)) token_next();
										token_next(); token_next();
									}
									else {
										token_prev(2);
										f = declare(me, NodeType_Function);
										token_next();
									}
									u32 open_count = 1;
									Expect(Token_OpenBrace) {
										//TODO this can go wrong with certain kind of syntax errors 
										//TODO maybe theres a better way to do this?
										while (open_count) {
											if      (next_match(Token_OpenBrace))  open_count++;
											else if (next_match(Token_CloseBrace)) open_count--;
											else if (next_match(Token_EOF)) ParseFail("unexpected EOF while parsing function ", structure->identifier, "::", FunctionFromNode(f)->identifier);
											token_next();
										}
										Expect(Token_CloseBrace) { 
											Function* func = FunctionFromNode(f);
											parser.structure->member_funcs.add(id, func); 
											func->token_end = tokens->iter;
										}
									}
								}
								else ExpectOneOf(Token_Semicolon, Token_Assignment) {
									token_prev(2);
									declare(me, NodeType_Declaration);
									structure->member_vars.add(curt.raw, declaration);
									//eat any possible default var stuff
									while (!next_match(Token_Semicolon)) token_next();
									token_next();
								}		
							}
						}
						token_next();
						Expect(Token_CloseBrace) {
							token_next();
							Expect(Token_Semicolon) { 
								knownStructs.add(structure->identifier, me); 
								strct->token_end = tokens->iter;
								return me; 
							}
							ExpectFail("expected ; for struct decl ", structure->identifier);
						}ExpectFail("expected } for struct decl ", structure->identifier);
					}ExpectFail("expected{after struct identifier ", structure->identifier);
				}ExpectFail("expected an identifier for struct decl");
			}ExpectFail("expected 'struct' keyword for struct declaration (somehow 'declare' was called without this?)");
		}break;
		
		case NodeType_Declaration: {
			ExpectGroup(TokenGroup_Type) {
				DataType dtype = dataTypeFromToken(curt.type);
				cstring type_id = curt.raw;
				token_next();
				Expect(Token_Identifier) {
					string typestr;
					if (dtype == DataType_Structure) typestr = type_id;
					else typestr = dataTypeStrs[dtype];
					Node* var = new_declaration(curt.raw, dtype, toStr("var: ", typestr, " ", curt.raw));
					declaration->token_idx = currTokIdx();
					declaration->type_id = type_id;
					change_parent(node, var);
					knownVars.add(curt.raw, var);
					return var;
				}
			}
		}break;
	}
	return 0;
}

Node* Parser::define(ParseStage stage, Node* node) {
	if (parse_failed) return 0;
	
	switch (stage) {
		
		case psGlobal: { ////////////////////////////////////////////////////////////////////// @Global
			while (!(curt.type == Token_EOF || next_match(Token_EOF))) {
				if (parse_failed) return 0;
				ExpectGroup(TokenGroup_Type) {
					ExpectSignature(1, Token_Identifier, Token_OpenParen) {
						Define(psFunction, node);
					}
				}ExpectFail("yeah i dont know right now");
				token_next();
			}
		}break;
		
		case psStruct: {
			if (node->type == NodeType_Structure){
				structure = StructFromNode(node), setTokenIdx(structure->token_idx);
			}
			else {
				declare(node, NodeType_Structure);
			}
			
			for (Declaration* v : structure->member_vars) {
				Define(psDeclaration, &v->node);
			}
			for (Function* f : structure->member_funcs) {
				Define(psFunction, &f->node);
			}
			Expect(Token_Semicolon) { return 0; /*empty struct*/ }
			while (!next_match(Token_CloseBrace)) token_next();
			token_next(2);
			Expect(Token_Semicolon) { token_next(); }
			ExpectFail("expected ; after struct definition");
			
		}break;
		
		case psFunction: { //////////////////////////////////////////////////////////////////// @Function
			StateSet(stInFunction);
			if (node->type == NodeType_Function) {
				Function* f = FunctionFromNode(node);
				
				for (Declaration* d : f->args) {
					if (Define(psDeclaration, &d->node)) {
						f->positional_args--;
					}
				}
				token_next();
				if(curt.type != Token_OpenBrace)
					token_next(2);
				
				setTokenIdx(f->token_idx);
				Expect(Token_OpenBrace) {
					Define(psScope, node);
				}ExpectFail("expected {");
			}
			else {
				declare(node, NodeType_Function);
				Define(psFunction, node);
			}
			StateUnset(stInFunction);
			
		}break;
		
		case psScope: { /////////////////////////////////////////////////////////////////////// @Scope
			Node* me = new_scope("scope");
			Scope* scpe = scope;
			scpe->token_start = tokens->iter;
			insert_last(node, &scope->node);
			while (!next_match(Token_CloseBrace)) {
				if(scope->has_return_statement){
					log_warning(WC_Unreachable_Code_After_Return, (&curt));
					if(globals.warnings_as_errors) break;
				}
				
				token_next();
				ExpectGroup(TokenGroup_Type) {
					Define(psDeclaration, me);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("missing ; after declaration assignment")
				}
				ElseExpect(Token_OpenBrace) {
					Scope* scope = this->scope;
					Define(psScope, me);
					this->scope = scope;
				}
				else {
					Define(psStatement, me);
				}
				if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); return 0; }
			}
			token_next();
			scpe->token_end = tokens->iter;
		}break;
		
		case psDeclaration: {////////////////////////////////////////////////////////////////// @Declaration
			if (node->type == NodeType_Declaration)
				declaration = DeclarationFromNode(node), setTokenIdx(declaration->token_idx);
			else 
				declare(node, NodeType_Declaration);
			
			if (declaration->type == DataType_Structure) {
				if (!knownStructs.has(declaration->type_id)) { ParseFail("variable declared with unknown struct '", declaration->type_id, "'"); return 0; }
				declaration->struct_type = StructFromNode(*knownStructs.at(declaration->type_id));
			}
			
			if (next_match(Token_Assignment)) {
				Node* ret = Define(psExpression, &declaration->node);
				return ret;
			}
			
		}break;
		
		case psStatement: {//////////////////////////////////////////////////////////////////// @Statement
			switch (curt.type) {
				case Token_If: {
					Node* ifno = new_statement(Statement_Conditional, "if statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, ifno);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for if statement"); break; }
						Define(psExpression, ifno);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								Define(psStatement, ifno); //scope
								Expect(Token_CloseBrace){ }ExpectFail("expected a }");
								stmt->token_end = tokens->iter;
							}
							else {
								ExpectGroup(TokenGroup_Type) { ParseFail("can't declare a declaration in an unscoped if statement"); return 0; } //TODO there's no reason this cant be valid, just warn when it happens
								Define(psStatement, ifno);
								Expect(Token_Semicolon){ }ExpectFail("expected a ;");
								stmt->token_end = tokens->iter;
							}
							if (next_match(Token_Else)) {
								token_next();
								Define(psStatement, ifno);
							}
						}ExpectFail("expected )");
					}ExpectFail("expected (");
				}break;
				
				case Token_Else: {
					new_statement(Statement_Else, "else statement");
					Statement* stmt = statement;
					stmt->token_start = tokens->iter;
					insert_last(node, &statement->node);
					token_next();
					define(psStatement, &statement->node);
					stmt->token_end = tokens->iter;
				}break;
				
				case Token_For: {
					StateSet(stInForLoop);
					Node* me = new_statement(Statement_For, "for statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for for statement"); return 0; }
						
						//initial clause
						ExpectGroup(TokenGroup_Type) {
							//we are declaring a var for this for loop
							declare(me, NodeType_Declaration);
							define(psDeclaration, &declaration->node);
							token_next();
						}
						else {
							Define(psExpression, me);
							token_next();
						}
						
						//control expression
						Expect(Token_Semicolon) {
							token_next(); 
							if(!next_match(Token_Semicolon)){
								Define(psExpression, me);
								token_next();
							}else{ //NOTE auto-insert literal 1 if no control expression
								Node* var = new_expression(cstr_lit("1"), Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", 1));
								expression->datatype = DataType_Signed32;
								expression->int32 = 1;
								insert_last(node, var);
							}
							
							//post expression
							Expect(Token_Semicolon) {
								if(!next_match(Token_CloseParen)){ //NOTE allow for no post expression
									token_next();
									Define(psExpression, me);
									token_next();
								}
							}ExpectFail("missing second ; in for statement");
						}ExpectFail("missing first ; in for statement");
						
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								Define(psStatement, me);
								stmt->token_end = tokens->iter;
							}
							else {
								ExpectGroup(TokenGroup_Type) { ParseFail("can't declare a declaration in an unscoped for statement"); return 0; }
								Define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
								stmt->token_end = tokens->iter;
							}
						}ExpectFail("expected ) for for loop");
					}ExpectFail("expected ( after for");
					StateUnset(stInForLoop);
				}break;
				
				case Token_While: {
					StateSet(stInWhileLoop);
					Node* me = new_statement(Statement_While, "while statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for while statement"); return 0; }
						ExpectGroup(TokenGroup_Type) { ParseFail("declaration not allowed for while condition"); return 0; }
						Define(psExpression, me);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								Define(psStatement, me);
								stmt->token_end = tokens->iter;
							}
							else {
								ExpectGroup(TokenGroup_Type) { ParseFail("can't declare a declaration in an unscoped for statement"); return 0; }
								Define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
								stmt->token_end = tokens->iter;
							}
						}ExpectFail("expected ) for while");
					}ExpectFail("expected ( after while");
					StateUnset(stInWhileLoop);
				}break;
				
				case Token_Break: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("break not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Break, "break statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, me);
					token_next();
					stmt->token_end = tokens->iter;
				}break;
				
				case Token_Continue: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("continue not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Continue, "continue statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, me);
					token_next();
					stmt->token_end = tokens->iter;
				}break;
				
				case Token_Return: {
					scope->has_return_statement = true;
					new_statement(Statement_Return, "return statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					insert_last(node, &statement->node);
					token_next();
					Define(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ;");
					stmt->token_end = tokens->iter;
				}break;
				
				case Token_OpenBrace: {
					Define(psScope, node);
				}break;
				
				case Token_StructDecl: {
					Node* me = new_statement(Statement_Struct, "struct statement");
					Statement* stmt = parser.statement;
					stmt->token_start = tokens->iter;
					Define(psStruct, me);
					insert_last(node, me);
					stmt->token_end = tokens->iter-1;
				}break;
				
				case Token_Semicolon: {
					//eat multiple semicolons
				}break;
				
				default: {
					new_statement(Statement_Expression, "exp statement");
					Statement* stmt = statement;
					stmt->token_start = tokens->iter;
					insert_last(node, &statement->node);
					Define(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("Expected a ;");
					stmt->token_end = tokens->iter;
				}break;
			}
		}break;
		
		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (Node** n = knownVars.at(curt.raw); n) {
						Node* id = new_expression(curt.raw, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", curt.raw));
						set_expression_type_from_declaration(id, *n);
						if(next_match(Token_Assignment)) {
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next(2);
							Node* ret = Define(psExpression, me);
							type_check(id, ret);
							change_parent(me, ret);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_PlusAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpPlus);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_NegationAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpMinus);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_MultiplicationAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpMultiply);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_DivisionAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpDivision);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_BitANDAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpBitAND);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_BitORAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpBitOR);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_BitXORAssignment)) {
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpBitXOR);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_BitShiftLeftAssignment )){
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpBitShiftLeft);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else if(next_match(Token_BitShiftRightAssignment)){
							token_next();
							Node* me = new_expression(curt.raw, Expression_BinaryOpAssignment);
							change_parent(me, id);
							token_next();
							Node* plus = new_expression(curt.raw, Expression_BinaryOpBitShiftRight);
							Node* id2 = new_expression(ExpressionFromNode(id)->str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", ExpressionFromNode(id)->str));
							change_parent(me, plus);
							change_parent(plus, id2);
							Node* ret = Define(psExpression, plus);
							insert_last(node, me);
							return me;
						}
						else {
							new_expression(curt.raw, Expression_IdentifierLHS);
							Node* ret = Define(psConditional, &expression->node);
							if (!ret) return 0;
							insert_last(node, ret);
							return ret;
						}
					}
					else { ParseFail("unknown var '", curt.raw, " referenced"); return 0; }
				}break;
				
				case Token_Semicolon:{
					//do nothing
				}break;
				
				default: {
					Node* ret = Define(psConditional, node);
					return ret;
				}break;
			}
		}break;
		
		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			Expect(Token_If) {
				Node* me = new_expression(curt.raw, Expression_TernaryConditional,  "if exp");
				insert_last(node, me);
				token_next();
				Expect(Token_OpenParen) {
					token_next();
					Define(psExpression, me);
					token_next();
					Expect(Token_CloseParen) {
						token_next();
						Define(psExpression, me);
						token_next();
						Expect(Token_Else) {
							token_next();
							Define(psExpression, me);
							return me;
						}ExpectFail("conditional if's are required to have an else");
					}ExpectFail("expected ) for if expression")
				}ExpectFail("expected ( for if expression")
			}
			else {
				return Define(psLogicalOR, node);
			}
		}break;
		
		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			Node* ret = Define(psLogicalAND, node);
			if (!next_match(Token_OR))
				return ret;
			return binopParse(node, ret, psLogicalAND, Token_OR);
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			Node* ret = Define(psBitwiseOR, node);
			if (!next_match(Token_AND))
				return ret;
			return binopParse(node, ret, psBitwiseOR);
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			Node* ret = Define(psBitwiseXOR, node);
			if (!next_match(Token_BitOR))
				return ret;
			return binopParse(node, ret, psBitwiseXOR, Token_BitOR);
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			Node* ret = Define(psBitwiseAND, node);
			if (!next_match(Token_BitXOR))
				return ret;
			return binopParse(node, ret, psBitwiseAND, Token_BitXOR);
		}break;
		
		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			Node* ret = Define(psEquality, node);
			if (!next_match(Token_BitAND))
				return ret;
			return binopParse(node, ret, psEquality, Token_BitAND);
		}break;
		
		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			Node* ret = Define(psRelational, node);
			if (!next_match(Token_NotEqual, Token_Equal))
				return ret;
			return binopParse(node, ret, psRelational, Token_NotEqual, Token_Equal);
		}break;
		
		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			Node* ret = Define(psBitshift, node);
			if (!next_match(Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual))
				return ret;
			return binopParse(node, ret, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
		}break;
		
		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			Node* ret = Define(psAdditive, node);
			if (!next_match(Token_BitShiftLeft, Token_BitShiftRight))
				return ret;
			return binopParse(node, ret, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
		}break;
		
		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			Node* ret = Define(psTerm, node);
			if (!next_match(Token_Plus, Token_Negation))
				return ret;
			return binopParse(node, ret, psTerm, Token_Plus, Token_Negation);
		}break;
		
		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			Node* ret = Define(psFactor, node);
			if (!next_match(Token_Multiplication, Token_Division, Token_Modulo))
				return ret;
			return binopParse(node, ret, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
		}break;
		
		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			switch (curt.type) {
				
				//TODO implicitly change types here when applicable, or do that where they're returned
				case Token_LiteralFloat: {
					Node* var = new_expression(cstring{}, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.float_value));
					expression->datatype = DataType_Float32;
					expression->float32 = curt.float_value; //TODO detect f64
					insert_last(node, &expression->node);
					return var;
				}break;
				case Token_LiteralInteger: {
					Node* var = new_expression(cstring{}, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.int_value));
					expression->datatype = DataType_Signed64;
					expression->int64 = curt.int_value;
					insert_last(node, &expression->node);
					return var;
				}break;
				
				case Token_LiteralString: {
					Node* var = new_expression(curt.raw, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " \"", curt.raw, "\""));
					expression->datatype = DataType_String;
					expression->str = curt.raw;
					insert_last(node, &expression->node);
					return var;
				}break;
				
				case Token_OpenParen: {
					token_next();
					Node* ret = Define(psExpression, &expression->node);
					change_parent(node, ret);
					token_next();
					Expect(Token_CloseParen) { return ret; }
					ExpectFail("expected a )");
				}break;
				
				case Token_Identifier: {
					if (next_match(Token_OpenParen)) {
						if (!knownFuncs.has(curt.raw)) { ParseFail(toStr("unknown function ", curt.raw, " referenced")); return 0; }
						Node* me = new_expression(curt.raw, Expression_Function_Call, toStr(ExTypeStrings[Expression_Function_Call], " ", curt.raw));
						insert_last(node, me);
						Function* f = FunctionFromNode(*knownFuncs.at(curt.raw));
						expression->datatype = f->type;
						token_next();
						
						//to order arguments correctly 
						array<Node*> expsend;
						array<cstring> named_args;
						
						expsend.resize(f->args.count);
						
						u32 positional_args_given = 0;
						while (!next_match(Token_CloseParen)) {
							token_next();
							b32 namedarg = 0;
							Expect(Token_Identifier) {
								cstring id = curt.raw;
								token_next();
								Expect(Token_Assignment) {
									namedarg = 1;
									u32 argidx = f->args.findkey(id);
									if (argidx == npos)   { ParseFail("unidentified named argument, ", id, " (or you wanted to use ==?)"); return 0; }
									if (expsend[argidx]) { ParseFail("attempt to use named arg on argument who has already been set: ", id); return 0; }
									token_next();
									expsend[argidx] = Define(psExpression, node);
									if (!expsend[argidx]) return 0;
									token_next();
								}
								else {
									token_prev();
								}
							}
							if (!namedarg) {
								//TODO handle too many args given
								//check if a positional arg was already filled by a named arg
								while (expsend[positional_args_given]) positional_args_given++;
								expsend[positional_args_given] = Define(psExpression, node);
								if (!expsend[positional_args_given++]) return 0;
								token_next();
							}
							
							Expect(Token_Comma){}
							ElseExpect(Token_CloseParen) { break; }
							ExpectFail("no , separating function arguments");
						}
						
						if (positional_args_given < f->positional_args) {
							ParseFail("func ", f->identifier, " requires ", f->positional_args, " positional args but ", (positional_args_given ? "only " : ""), positional_args_given, (positional_args_given == 1 ? " was " : " were "), "given.");
							logE("parser", "missing args are: ");
							for (u32 i = 0; i < f->positional_args; i++) {
								if(!expsend[i])
									logE("parser", f->args[i]->identifier, " of type ", dataTypeStrs[f->args[i]->type]);
							}
							return 0;
						}
						
						for (u32 i = 0; i < expsend.count; i++) {
							if (!expsend[i]) {
								insert_last(me, f->args[i]->node.first_child->last_child);
							}
							else {
								change_parent(me, expsend[i]);
							}
						}
						
						Expect(Token_CloseParen) {}
						return me;
					}
					else if (next_match(Token_Dot)) {
						//member access
						if (Node** var = knownVars.at(curt.raw); var) { 
							Node* me = new_expression(curt.raw, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.raw));
							ExpressionFromNode(me)->struct_type = DeclarationFromNode(*var)->struct_type;
							token_next();
							return Define(psFactor, me);
						} else { ParseFail("attempt to access a member of an undeclared variable"); return 0; }
					}
					else {
						if (!knownVars.has(curt.raw)) { ParseFail("unknown var '", curt.raw, "' referenced"); return 0; }
						Node* var = new_expression(curt.raw, Expression_IdentifierRHS, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.raw));
						insert_last(node, var);
						set_expression_type_from_declaration(var, *knownVars.at(curt.raw));
						if (next_match(Token_Increment, Token_Decrement)) {
							token_next();
							new_expression(curt.raw, (curt.type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix));
							insert_last(node, &expression->node);
							change_parent(&expression->node, var);
							var = &expression->node;
						}
						return var;
					}
					
				}break;
				
				case Token_If: {
					return Define(psConditional, &expression->node);
				}break;
				
				case Token_Increment: {
					new_expression(curt.raw, Expression_IncrementPrefix);
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						Define(psFactor, &expression->node);
					}ExpectFail("'++' needs l-value");
					return ret;
				}break;
				
				case Token_Decrement: {
					new_expression(curt.raw, Expression_DecrementPrefix);
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						Define(psFactor, &expression->node);
					}ExpectFail("'--' needs l-value");
					return ret;
				}break;
				
				case Token_Semicolon: {
					return node;
				}break;
				
				case Token_Negation: {
					new_expression(curt.raw, Expression_UnaryOpNegate);
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Define(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_LogicalNOT: {
					new_expression(curt.raw, Expression_UnaryOpLogiNOT);
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Define(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_BitNOT: {
					new_expression(curt.raw, Expression_UnaryOpBitComp);
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Define(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_Dot: {
					Expression* nodeexp = ExpressionFromNode(node);
					Struct* nodestruct = nodeexp->struct_type;
					Node* me = new_expression(curt.raw, Expression_BinaryOpMemberAccess);
					token_next();
					Expect(Token_Identifier) {
						if (!nodeexp->struct_type->member_vars.has(curt.raw)) { ParseFail("attempt to access undefined member of var ", (*tokens)[currTokIdx() - 2].raw); return 0; }
						Node* me2 = new_expression(curt.raw, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.raw));
						nodeexp = parser.expression;
						nodeexp->struct_type = (*nodestruct->member_vars.at(curt.raw))->struct_type;
						nodestruct = nodeexp->struct_type;
						change_parent(me, node);
						change_parent(me, me2);
						while (next_match(Token_Dot)) {
							token_next();
							me2 = new_expression(curt.raw, Expression_BinaryOpMemberAccess);
							token_next();
							Expect(Token_Identifier) {
								if (!nodestruct->member_vars.has(curt.raw)) { ParseFail("attempt to access undefined member of var ", (*tokens)[currTokIdx() - 2].raw); return 0; }
								Node* next = new_expression(curt.raw, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.raw));
								nodeexp = parser.expression;
								nodeexp->struct_type = (*nodestruct->member_vars.at(curt.raw))->struct_type;
								nodestruct = nodeexp->struct_type;
								change_parent(me2, me);
								change_parent(me2, next);
								me = me2;
							}ExpectFail("expected identifier following member access dot");
						}
					}ExpectFail("expected identifier following member access dot");
					return me;
				}break;
				
				case Token_This: {
					//TODO when we set up pointers
				}break;
				
				
				default: {
					ParseFail("unexpected token found in factor");
				}break;
			}
		}break;
	}
	return 0;
}

b32 Parser::parse_program(Program& mother) {
	arena.init(Kilobytes(10));
	mother.node.comment = "program";
	tokens = &preprocessor.tokens;
	
	for (u32 i : preprocessor.func_decl) {
		setTokenIdx(i);
		declare(&mother.node, NodeType_Function);
	}
	if (parse_failed) return false;
	for (u32 i : preprocessor.struct_decl) {
		setTokenIdx(i);
		declare(&mother.node, NodeType_Structure);
	}
	if (parse_failed) return false;
	
	for (Node* n : knownStructs) {
		Define(psStruct, n);
	}
	if (parse_failed) return false;
	
	for (Node* n : knownFuncs) {
		Define(psFunction, n);
	}
	if (parse_failed) return false;
	
	return true;
}