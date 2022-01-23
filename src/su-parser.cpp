//master token
token curt;
array<token> tokens;

b32 parse_failed = false;

//These defines are mostly for conveinence and clarity as to what im doing
//#define token_next() curt = tokens.next()
#define token_last curt = tokens.prev()

void token_next(u32 count = 1) {
	curt = tokens.next(count);
}

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)
#define curr_atch(tok) (curt.type == tok)

#define ParseFail(error)\
(std::cout << "\nError: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl,  parse_failed = true)

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

#define ExpectFail(error)\
else { ParseFail(error); /*DebugBreakpoint;*/ } //TODO make it so this breakpoint only happens in debug mode or whatever

#define ExpectFailCode(failcode)\
else { failcode }

local map<Token_Type, ExpressionType> tokToExp{
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
	{Token_BitXOR,		       Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
	{Token_BitNOT,             Expression_UnaryOpBitComp},
	{Token_LogicalNOT,         Expression_UnaryOpLogiNOT},
	{Token_Negation,           Expression_UnaryOpNegate},
};

local map<const char*, Function*> knownFuncs;
local map<const char*, Variable> knownVars;

DataType dataTypeFromToken(Token_Type type) {
	switch (type) {
		case Token_Void      : {return DataType_Void;}
		case Token_Signed8   : {return DataType_Signed8;}        
		case Token_Signed32  : {return DataType_Signed32;}     
		case Token_Signed64  : {return DataType_Signed64;}    
		case Token_Unsigned8 : {return DataType_Unsigned8;}  
		case Token_Unsigned32: {return DataType_Unsigned32;}
		case Token_Unsigned64: {return DataType_Unsigned64; }
		case Token_Float32   : {return DataType_Float32;}     
		case Token_Float64   : {return DataType_Float64;}   
		case Token_String    : {return DataType_String;}    
		case Token_Any       : {return DataType_Any;}
		case Token_Struct    : {return DataType_Structure;}    
		default: {PRINTLN("given token type is not a data type"); }
	}
}

template<class... T>
inline b32 check_signature(u32 offset, T... in) {
	return ((tokens.peek(offset++).type == in) && ...);
}

template<typename... T, typename A> inline b32
match_any(A tested, T... in) {
	return((tested == in) || ...);
}

template<typename... T> inline b32
next_match(T... in) {
	return ((tokens.peek(1).type == in) || ...);
}

Function*    function;
Scope*       scope;
Declaration* declaration;
Statement*   statement;
Expression*  expression;

Arena arena;

inline Node* new_function(string& identifier, const string& node_str = "") {
	function = (Function*)arena.add(Function());
	function->identifier = identifier;
	function->node.type    = NodeType_Function;
	function->node.comment = node_str;
	return &function->node;
}

inline Node* new_scope(const string& node_str = "") {
	scope = (Scope*)arena.add(Scope());
	scope->node.type    = NodeType_Scope;
	scope->node.comment = node_str;
	return &scope->node;
}

inline Node* new_declaration(const string& node_str = "") {
	declaration = (Declaration*)arena.add(Declaration());
	declaration->node.type    = NodeType_Declaration;
	declaration->node.comment = node_str;
	return &declaration->node;
}

inline Node* new_statement(StatementType type, const string& node_str = ""){
	statement = (Statement*)arena.add(Statement());
	statement->type = type;
	statement->node.type    = NodeType_Statement;
	statement->node.comment = node_str;
	return &statement->node;
}

inline Node* new_expression(string& str, ExpressionType type, const string& node_str = "") {
	expression = (Expression*)arena.add(Expression());
	expression->expstr = str;
	expression->type   = type;
	expression->node.type    = NodeType_Expression;
	expression->node.comment = node_str;
	return &expression->node;
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

enum ParseStage {
	psGlobal,      	// <program>       :: = { <function> }
	psFunction,		// <function>      :: = <type> <id> "(" [ <declaration> {"," <declaration> } ] ")" <scope>
	psScope,        // <scope>         :: = "{" { (<declaration> | <statement> | <scope>) } "}"
	psDeclaration,	// <declaration>   :: = <type> <id> [ = <exp> ] ";"
	psStatement,	// <statement>     :: = "return" <exp> ";" | <exp> ";" | <scope> 
	                //                      | "if" "(" <exp> ")" <statement> [ "else" <statement> ]
					//                      | "for" "(" [<exp>] ";" [<exp>] ";" [<exp>] ")" <statement>
					//                      | "for" "(" <declaration> [<exp>] ";" [<exp>] ")" <statement>
					//                      | "while" "(" <exp> ")" <statement>
					//                      | "break" [<integer>] ";" 
					//                      | "continue" ";"
	psExpression,	// <exp>           :: = <id> "=" <exp> | <conditional> | <funccall>
					// <funccall>      :: = <id> "(" [ <exp> {"," <exp>} ] ")"
	psConditional,	// <conditional>   :: = <logical or> | "if" "(" <exp> ")" <exp> "else" <exp> 
	psLogicalOR,	// <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,	// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,	// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,	// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,	// <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,		// <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,	// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,		// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,		// <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,			// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,		// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal> | <id> | <incdec> <id> | <id> <incdec> | "if"
                    // <literal>       :: = <integer> | <float> | <string>
					// <float>         :: = { <integer> } "." <integer> { <integer> }
					// <string>        :: = """ { <char> } """
					// <integer>       :: = (1|2|3|4|5|6|7|8|9|0) 
					// <char>          :: = you know what chars are
					// <type>          :: = (u8|u32|u64|s8|s32|s64|f32|f64|str|any)
					// <incdec>        :: = "++" | "--"
                    // <unary>         :: = "!" | "~" | "-"
}; 

template<typename... T>
Node* binopParse(Node* node, Node* ret, ParseStage next_stage, T... tokcheck) {
	token_next();
	Node* me = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
	change_parent(me, ret);
	insert_last(node, me);
	token_next();
	ret = parser(next_stage, me);

	while (next_match(tokcheck...)) {
		token_next();
		Node* me2 = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
		token_next();
		ret = parser(next_stage, node);
		change_parent(me2, me);
		change_parent(me2, ret);
		insert_last(node, me2);
		me = me2;
	}
	return me;
}

Node* parser(ParseStage state, Node* node) {
	if (parse_failed) return 0;
	
	switch (state) {
		
		case psGlobal: { ////////////////////////////////////////////////////////////////////// @Global
			while (!(curt.type == Token_EOF || next_match(Token_EOF))) {
				if (parse_failed) return 0;


				ExpectGroup(Token_Typename) {
					ExpectSignature(1, Token_Identifier, Token_OpenParen) {
						parser(psFunction, node);
					}
					Expect(Token_Identifier){
						if (next_match(Token_OpenParen)) {
							
							
						}
					}
				}ExpectFail("yeah i dont know right now");
				token_next();
			}
		}break;
		
		case psFunction: { //////////////////////////////////////////////////////////////////// @Function
			StateSet(stInFunction);
			DataType type = dataTypeFromToken(curt.type);
			token_next();
			Expect(Token_Identifier) {
				Node* me = new_function(curt.str, toStr("func ", dataTypeStrs[type], " ", curt.str));
				insert_last(node, me);
				function->type = type;
				token_next();
				Expect(Token_OpenParen) {
					token_next();
					ExpectGroup(Token_Typename) {
						parser(psDeclaration, me);
						ExpectGroup(Token_Typename) { ParseFail("no , separating function parameters"); }
						function->args.add(declaration->type);
						while (next_match(Token_Comma)) {
							token_next(); token_next();
							parser(psDeclaration, me);
							function->args.add(declaration->type);
							ExpectGroup(Token_Typename) { ParseFail("no , separating function parameters"); }
						}
						token_next();
					}
					Expect(Token_Identifier) { ParseFail("untyped identifier in function declaration's arguments"); }
					Expect(Token_CloseParen) {
						knownFuncs.add(function->identifier.str, function);
						token_next();
						Expect(Token_OpenBrace) {
							parser(psScope, me);
						}ExpectFail("expected {");
					}ExpectFail("expected )");
				}ExpectFail("expected (");
			}
			StateUnset(stInFunction);
		}break;
		
		case psScope: { /////////////////////////////////////////////////////////////////////// @Scope
			Node* me = new_scope("scope");
			insert_last(node, &scope->node);
			while (!next_match(Token_CloseBrace)) {
				token_next();
				ExpectGroup(Token_Typename) {
					parser(psDeclaration, &declaration->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("missing ; after variable assignment")
				}
				ElseExpect(Token_OpenBrace) {
					parser(psScope, me);
				}
				else {
					parser(psStatement, me);
				}
				if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); return 0; }
			}
			token_next();
		}break;
		
		case psDeclaration: {////////////////////////////////////////////////////////////////// @Declaration
			DataType type = dataTypeFromToken(curt.type);
			token_next();
			new_declaration(toStr("decl ", dataTypeStrs[type], " ", curt.str));
			insert_last(node, &declaration->node);
			Expect(Token_Identifier) {
				declaration->identifier = curt.str;
				declaration->type = type;
				if(next_match(Token_Assignment)) {
					token_next();
					new_expression(curt.str, ExpressionGuard_Assignment, ExTypeStrings[ExpressionGuard_Assignment]);
					token_next();
					insert_last(node, &expression->node);
					Node* ret = parser(psExpression, &expression->node);
					return ret;
				}
			}
		}break;
		
		case psStatement: {//////////////////////////////////////////////////////////////////// @Statement
			switch (curt.type) {
				case Token_If: {
					Node* ifno = new_statement(Statement_Conditional, "if statement");
					insert_last(node, ifno);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for if statement"); break; }
						parser(psExpression, ifno);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								parser(psStatement, ifno);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped if statement"); return 0; }
								parser(psStatement, ifno);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
							if (next_match(Token_Else)) {
								token_next();
								parser(psStatement, ifno);
							}
						}ExpectFail("expected )");
					}ExpectFail("expected (");
				}break;
				
				case Token_Else: {
					new_statement(Statement_Conditional, "else statement");
					insert_last(node, &statement->node);
					token_next();
					parser(psStatement, &statement->node);
				}break;

				case Token_For: {
					StateSet(stInForLoop);
					Node* me = new_statement(Statement_For, "for statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for for statement"); return 0; }
						ExpectGroup(Token_Typename) {
							//we are declaring a var for this for loop
							new_declaration("decl " + tokens.peek().str);
							insert_last(me, &declaration->node);
							parser(psDeclaration, &declaration->node);
						}
						else {
							parser(psExpression, me);
							token_next();
						}
						Expect(Token_Semicolon) {
							token_next(); 
							parser(psExpression, me);
							token_next();
							Expect(Token_Semicolon) {
								token_next();
								parser(psExpression, me);
								token_next();
							}ExpectFail("missing second ; in for statement");
						}ExpectFail("missing first ; in for statement");
						
						
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								parser(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped for statement"); return 0; }
								parser(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for for loop");
					}ExpectFail("expected ( after for");
					StateUnset(stInForLoop);
				}break;

				case Token_While: {
					StateSet(stInWhileLoop);
					Node* me = new_statement(Statement_While, "while statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for while statement"); return 0; }
						ExpectGroup(Token_Typename) { ParseFail("declaration not allowed for while condition"); return 0; }
						parser(psExpression, me);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								parser(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped for statement"); return 0; }
								parser(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for while");
					}ExpectFail("expected ( after while");
					StateUnset(stInWhileLoop);
				}break;

				case Token_Break: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("break not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Break, "break statement");
					insert_last(node, me);
					token_next();
				}break;

				case Token_Continue: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("continue not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Continue, "continue statement");
					insert_last(node, me);
					token_next();
				}break;
				
				case Token_Return: {
					new_statement(Statement_Return, "return statement");
					insert_last(node, &statement->node);
					token_next();
					parser(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ;");
					
				}break;
				
				case Token_OpenBrace: {
					parser(psScope, node);
				}break;

				case Token_Semicolon: {
					//eat multiple semicolons
				}break;

				default: {
					new_statement(Statement_Expression, "exp statement");
					insert_last(node, &statement->node);
					parser(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("Expected a ;");
				}break;
			}
		}break;
		
		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						new_expression(curt.str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", curt.str));     
						insert_last(node, &expression->node);
						token_next();
						new_expression(curt.str, Expression_BinaryOpAssignment, ExTypeStrings[Expression_BinaryOpAssignment]);
						insert_last(node, &expression->node);
						token_next();
						new_expression(curt.str, ExpressionGuard_Assignment);
						
						Node* ret = parser(psExpression, &expression->node);
						insert_last(node, ret);
						return ret;
					}
					else if (next_match(Token_OpenParen)) {
						if (!knownFuncs.has(curt.str.str)) { ParseFail(toStr("unknown function ", curt.str, " referenced")); return 0; }
						Node* me = new_expression(curt.str, Expression_Function_Call, toStr(ExTypeStrings[Expression_Function_Call], " ", curt.str));
						insert_last(node, me);
						Function* callee = *knownFuncs.at(curt.str.str);
						token_next(); token_next();
						if (callee->args.count > 0) {
							//Expect(Token_Identifier) {
							// This will be for doing func(arg = blah,...)
							//}

							forI(callee->args.count) {
								parser(psExpression, me);
								token_next();
								if (i != callee->args.count - 1) {
									Expect(Token_CloseParen) { ParseFail(toStr("Not enough arguments provided for func ", callee->identifier)); return 0; }
									Expect(Token_Comma) { token_next(); }
									ExpectFail("no , between function arguments");
								}
							}
							Expect(Token_CloseParen) { }
							ExpectFail(toStr("expected ) after function call to ", callee->identifier));

							//TODO list what required arguments are missing 
							//ExpectFail(toStr("expected an identifier or literal as function arg to ", callee->identifier));
						}
						else {
							Expect(Token_CloseParen) {}
							ExpectFail(toStr("expected ) on function call to ", callee->identifier));
						}
					}
					else {
						new_expression(curt.str, ExpressionGuard_HEAD);
						Node* ret = parser(psConditional, &expression->node);
						insert_last(node, ret);
						return ret;
					}
				}break;

				default: {
					new_expression(curt.str, ExpressionGuard_HEAD);
					Node* ret = parser(psConditional, node);
					return ret;
				}break;
			}
		}break;
		
		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			Expect(Token_If) {
				Node* me = new_expression(curt.str, Expression_TernaryConditional,  "if exp");
				insert_last(node, me);
				token_next();
				Expect(Token_OpenParen) {
					token_next();
					parser(psExpression, me);
					token_next();
					Expect(Token_CloseParen) {
						token_next();
						parser(psExpression, me);
						token_next();
						Expect(Token_Else) {
							token_next();
							parser(psExpression, me);
							return me;
						}ExpectFail("conditional if's are required to have an else");
					}ExpectFail("expected ) for if expression")
				}ExpectFail("expected ( for if expression")
			}
			else {
				return parser(psLogicalOR, node);
			}
		}break;
		
		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			Node* ret = parser(psLogicalAND, node);
			if (!next_match(Token_OR))
				return ret;
			return binopParse(node, ret, psLogicalAND, Token_OR);
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			Node* ret = parser(psBitwiseOR, node);
			if (!next_match(Token_AND))
				return ret;
			return binopParse(node, ret, psBitwiseOR);
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			Node* ret = parser(psBitwiseXOR, node);
			if (!next_match(Token_BitOR))
				return ret;
			return binopParse(node, ret, psBitwiseXOR, Token_BitOR);
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			Node* ret = parser(psBitwiseAND, node);
			if (!next_match(Token_BitXOR))
				return ret;
			return binopParse(node, ret, psBitwiseAND, Token_BitXOR);
		}break;
		
		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			Node* ret = parser(psEquality, node);
			if (!next_match(Token_BitAND))
				return ret;
			return binopParse(node, ret, psEquality, Token_BitAND);
		}break;
		
		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			Node* ret = parser(psRelational, node);
			if (!next_match(Token_NotEqual, Token_Equal))
				return ret;
			return binopParse(node, ret, psRelational, Token_NotEqual, Token_Equal);
		}break;
		
		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			Node* ret = parser(psBitshift, node);
			if (!next_match(Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual))
				return ret;
			return binopParse(node, ret, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
		}break;
		
		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			Node* ret = parser(psAdditive, node);
			if (!next_match(Token_BitShiftLeft, Token_BitShiftRight))
				return ret;
			return binopParse(node, ret, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
		}break;
		
		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			Node* ret = parser(psTerm, node);
			if (!next_match(Token_Plus, Token_Negation))
				return ret;
			return binopParse(node, ret, psTerm, Token_Plus, Token_Negation);
		}break;
		
		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			Node* ret = parser(psFactor, node);
			if (!next_match(Token_Multiplication, Token_Division, Token_Modulo))
				return ret;
			return binopParse(node, ret, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
		}break;
		
		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			switch (curt.type) {
				
				
				case Token_LiteralFloat:
				case Token_LiteralInteger: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					insert_last(node, &expression->node);
					return var;
				}break;

				case Token_LiteralString: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " \"", curt.str, "\""));
					insert_last(node, &expression->node);
					return var;
				}break;
				
				case Token_OpenParen: {
					token_next();
					Node* ret = parser(psExpression, &expression->node);
					change_parent(node, ret);
					token_next();
					Expect(Token_CloseParen) { return ret; }
					ExpectFail("expected a )");
				}break;
				
				case Token_Identifier: {
					Node* var = new_expression(curt.str, Expression_IdentifierRHS, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.str));
					insert_last(node, var);
					if (next_match(Token_Increment, Token_Decrememnt)) {
						token_next();
						new_expression(curt.str, (curt.type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix), (curt.type == Token_Increment ? "++ post" : "-- post"));
						insert_last(node, &expression->node);
						change_parent(&expression->node, var);
						var = &expression->node;
					}
					return var;
				}break;

				case Token_If: {
					return parser(psConditional, &expression->node);
				}break;

				case Token_Increment: {
					new_expression(curt.str, Expression_IncrementPrefix, "++ pre");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						parser(psFactor, &expression->node);
					}ExpectFail("'++' needs l-value");
					return ret;
				}break;

				case Token_Decrememnt: {
					new_expression(curt.str, Expression_DecrementPrefix, "-- pre");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						parser(psFactor, &expression->node);
					}ExpectFail("'--' needs l-value");
					return ret;
				}break;
				
				case Token_Semicolon: {
					return node;
				}break;

				case Token_Negation: {
					new_expression(curt.str, Expression_UnaryOpNegate, "-");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;

				case Token_LogicalNOT: {
					new_expression(curt.str, Expression_UnaryOpLogiNOT, "!");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;

				case Token_BitNOT: {
					new_expression(curt.str, Expression_UnaryOpBitComp, "~");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;

				default: {
					ParseFail("unexpected token found in factor");
				}break;
			}
		}break;
	}
	return 0;
}

// <program> ::= <function>
b32 suParser::parse(array<token>& tokens_in, Program& mother) {
	arena.init(Kilobytes(10));
	
	tokens = tokens_in;
	curt = tokens[0];

	
	mother.node.comment = "program";
	
	parser(psGlobal, &mother.node);
	
	return 0;//parse_failed;
}