#include "su-parser.h"

#include "utils/map.h"

//TODO remove returning arrays of expressions and pass them by reference and modify instead
//     or just use global arrays like we use a global string in assembling
//TODO optimize the tree by bypassing nodes for expressions
//	   like, if an expression goes straight to being an int, we don't need several layers of 
//     nodes determining that



// full backus naur stuff
// <program>       :: = <function>
// <function>      :: = "int" <id> "(" ")" "{" { <block item> } "}"
// <block item>    :: = <statement> | <declaration>
// <declaration>   :: = "int" <id> [ = <exp> ] ";"
// <statement>     :: = "return" <exp> ";" | <exp> ";" | "if" "(" <exp> ")" <statement> [ "else" <statement> ] 
// <exp>           :: = <id> "=" <exp> | <conditional>
// <conditional>   :: = <logical or> [ "?" <exp> ":" <conditional> ]
// <logical or>    :: = <logical and> { "||" <logical and> } 
// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
// <bitwise and>   :: = <equality> { "&" <equality> }
// <equality>      :: = <relational> { ("!=" | "==") <relational> }
// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
// <additive>      :: = <term> { ("+" | "-") <term> }
// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
// <unary>         :: = "!" | "~" | "-"


//this file is meant to represent the above, with each function representing a stage on the left hand side
//the functions are upside down though, so I don't have to have an excessive amount of forward declarations at the top



bool master_logger = true;

#define PrettyPrint(message)\
if(master_logger){ for(int i = 0; i < layer; i++)\
if(i % 2 == 0) std::cout << "|   ";\
else std::cout << "!   ";\
std::cout << "~" << message << std::endl;}

u32 layer = 0; //inc when we go into anything, dec when we come out

//master token
token curt;
array<token> tokens;

//master fail flag, set when something happens that prevents from building properly
bool epic_fail_n_we_must_gtfo = false;

//These defines are mostly for conveinence and clarity as to what im doing
//#define token_next() curt = tokens.next()
#define token_last curt = tokens.prev()

void token_next(u32 count = 1) {
	curt = tokens.next(count);
}

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)

#define next_match(tok) (tokens.peek().type == tok)
#define curr_atch(tok) (curt.type == tok)

#define ParseFail(error)\
std::cout << "\nError: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl;

#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectOneOf(exp_type)\
if(exp_type.has(curt.type)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 


#define ExpectFail(error)\
 else { ParseFail(error); }

#define ExpectFailCode(failcode)\
 else { failcode }

//collection of flags/counters to help with error checking throughout the parsing process
struct {
	bool unaryf = 0;    //true if a unary op has been found
	bool binaryf = 0;   //true if a binary op has been found
 	bool integerf = 0;  //true if an integer has been found

	bool keyword_type = 0; //true if a type specifier keyword has been found
} syntax;

local map<Token_Type, ExpressionType> binaryOps{
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
	{Token_BitXOR,		       ExpressionGuard_BitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
};

local map<Token_Type, ExpressionType> unaryOps{
	{Token_BitNOT,     Expression_UnaryOpBitComp},
	{Token_LogicalNOT, Expression_UnaryOpLogiNOT},
	{Token_Negation,   Expression_UnaryOpNegate},
};

local array<Token_Type> typeTokens{
	Token_Signed32,
	Token_Signed64,
	Token_Unsigned32,
	Token_Unsigned64,
	Token_Float32,
	Token_Float64,
};

template<class... T>
inline b32 check_signature(T... in) {
	int i = 0;
	return ((tokens.peek(i++).type == in) && ...);
}

//void parse_term(array<Expression>* expressions);
//void parse_expressions(array<Expression>* expressions);
//
//// <factor> :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
//void parse_factor(array<Expression>* expressions) {
//	layer++;
//	PrettyPrint("factor");
//	expressions->add(Expression(curt.str, ExpressionGuard_Factor));
//
//	switch (curt.type) {
//
//		case Token_Literal: {
//			PrettyPrint("int literal:  " << curt.str);
//			syntax.integerf = 1;
//			expressions->last->expressions.add(Expression(curt.str, Expression_IntegerLiteral));
//			layer--;
//			return;
//		}break;
//
//		case Token_OpenParen: {
//			PrettyPrint("( OPEN");
//			expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
//			
//			token_next();
//			parse_expressions(&expressions->last->expressions);
//			
//			token_next();
//			Expect(Token_CloseParen) 
//				PrettyPrint(") CLOSE");
//				layer--;
//				return;
//			ExpectFail("Expected a )")
//		}break;
//
//		case Token_Identifier: {
//			PrettyPrint("identifier: " << curt.str);
//			expressions->add(Expression(curt.str, Expression_IdentifierRHS));
//			layer--;
//			return;
//		}break;
//
//		default: {
//			ExpectOneOf(unaryOps)
//				PrettyPrint("unary op " << ExTypeStrings[*unaryOps.at(curt.type)]);
//				token_next();
//				expressions->add(Expression(curt.str, *unaryOps.at(curt.type)));
//				parse_factor(&expressions->last->expressions);
//				layer--;
//				return;
//			ExpectFail("unExpected token found in factor");
//		}
//	}
//
//	layer--;
//}
//
//// <term> :: = <factor> { ("*" | "/" | "%") <factor> }
//void parse_term(array<Expression>* expressions) {
//	layer++;
//	PrettyPrint("term");
//
//	//all terms become factors
//	expressions->add(Expression(curt.str, ExpressionGuard_Term));
//	parse_factor(&expressions->last->expressions);
//
//	while (
//		token_peek.type == Token_Multiplication || token_peek.type == Token_Division ||
//		token_peek.type == Token_Modulo) {
//		
//		token_next();
//		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
//		PrettyPrint("term");
//
//		//add operator expression
//		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
//
//		expressions->add(Expression(curt.str, ExpressionGuard_Term));
//		token_next();
//		parse_factor(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <additive-exp> ::= <term> { ("+" | "-") <term> }
//void parse_additive(array<Expression>* expressions) {
//	layer++;
//	string name = "additive";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_Additive));
//	parse_term(&expressions->last->expressions);
//
//	while (token_peek.type == Token_Plus || token_peek.type == Token_Negation) {
//		token_next();
//		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
//		PrettyPrint(name);
//		
//		//add operator expression
//		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_Additive));
//		token_next();
//		parse_term(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
//void parse_bitshift(array<Expression>* expressions) {
//	layer++;
//	string name = "bitshift";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
//	parse_additive(&expressions->last->expressions);
//
//	while (token_peek.type == Token_BitShiftRight || token_peek.type == Token_BitShiftLeft) {
//		token_next();
//		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
//		token_next();
//		parse_additive(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <relational> :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
//void parse_relational(array<Expression>* expressions) {
//	layer++;
//	string name = "relational";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_Relational));
//	parse_bitshift(&expressions->last->expressions);
//
//	while (
//		token_peek.type == Token_LessThan ||
//		token_peek.type == Token_GreaterThan ||
//		token_peek.type == Token_LessThanOrEqual ||
//		token_peek.type == Token_GreaterThanOrEqual) {
//
//		token_next(); 
//		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
//		
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_Relational));
//		token_next();
//		parse_bitshift(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <equality> ::= <relational> { ("!=" | "==") <relational> }
//void parse_equality(array<Expression>* expressions) {
//	layer++;
//	string name = "equality";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_Equality));
//	parse_relational(&expressions->last->expressions);
//
//	while (token_peek.type == Token_NotEqual || token_peek.type == Token_Equal) {
//		token_next(); 
//		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
//		
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_Equality));
//		token_next();
//		parse_relational(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <bitwise and> :: = <equality> { "&" <equality> }
//void parse_bitwise_and(array<Expression>* expressions) {
//	layer++;
//	string name = "bit AND";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
//	parse_equality(&expressions->last->expressions);
//
//	while (token_peek.type == Token_BitAND) {
//		token_next();
//		PrettyPrint("binary op &");
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, Expression_BinaryOpBitAND));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
//		token_next();
//		parse_equality(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <bitwise xor> :: = <bitwise and> { "^" <bitwise and> }
//void parse_bitwise_xor(array<Expression>* expressions) {
//	layer++;
//	string name = "bit XOR";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
//	parse_bitwise_and(&expressions->last->expressions);
//
//	while (token_peek.type == Token_BitXOR) {
//		token_next();
//		PrettyPrint("binary op ^");
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, Expression_BinaryOpXOR));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
//		token_next();
//		parse_bitwise_and(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
////<bitwise or> :: = <bitwise xor> { "|" <bitwise xor> }
//void parse_bitwise_or(array<Expression>* expressions) {
//	layer++;
//	string name = "bit OR";
//	PrettyPrint(name);
//
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
//	parse_bitwise_xor(&expressions->last->expressions);
//
//	while (token_peek.type == Token_BitOR) {
//		token_next();
//		PrettyPrint("binary op |");
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, Expression_BinaryOpBitOR));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
//		token_next();
//		parse_bitwise_xor(&expressions->last->expressions);
//
//	}
//
//	layer--;
//}
//
//// <logical and> :: = <bitwise or> { "&&" <bitwise or> } 
//void parse_logical_and(array<Expression>* expressions) {
//	layer++;
//	string name = "logi AND";
//	PrettyPrint(name);
//	
//	//decend down expression guards
//	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
//	parse_bitwise_or(&expressions->last->expressions);
//
//	while (token_peek.type == Token_AND) {
//		token_next();
//		PrettyPrint("binary op &&");
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, Expression_BinaryOpAND));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
//		token_next();
//		parse_bitwise_or(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <logical or> :: = <logical and> { "||" <logical and> }
//void parse_logical_or(array<Expression>* expressions) {
//	layer++;
//	string name = "logi OR";
//	PrettyPrint(name);
//
//	//decend into expression guard hell
//	expressions->add(Expression(curt.str, ExpressionGuard_LogicalOR));
//	parse_logical_and(&expressions->last->expressions);
//
//	while (token_peek.type == Token_OR) {
//		token_next();
//		PrettyPrint("binary op ||");
//		PrettyPrint(name);
//
//		//add operator expression
//		expressions->add(Expression(curt.str, Expression_BinaryOpOR));
//
//		//decend down expression guards
//		expressions->add(Expression(curt.str, ExpressionGuard_LogicalOR));
//		token_next();
//		parse_logical_and(&expressions->last->expressions);
//	}
//
//	layer--;
//}
//
//// <conditional> ::= <logical or> [ "?" <exp> ":" <conditional> ]
//void parse_conditional(array<Expression>* expressions) {
//	layer++;
//	PrettyPrint("conditional");
//
//	expressions->add(Expression(curt.str, ExpressionGuard_Conditional));
//	parse_logical_or(&expressions->last->expressions);
//
//	if (token_peek.type == Token_QuestionMark) {
//		token_next();
//		PrettyPrint("ternary conditional");
//		expressions->add(Expression(curt.str, Expression_TernaryConditional));
//		token_next();
//		parse_expressions(&expressions->last->expressions);
//		token_next();
//		Expect(Token_Colon) {
//			expressions->add(Expression(curt.str, ExpressionGuard_Conditional));
//			token_next();
//			parse_conditional(&expressions->last->expressions);
//		}
//		ExpectFail("Expected : for ternary conditional")
//	}
//
//
//	layer--;
//}
//
//// <exp> :: = <id> "=" <exp> | <conditional>
//void parse_expressions(array<Expression>* expressions) {
//	layer++;
//	string name = "exp";
//	 
//	switch (curt.type) {
//		case Token_Identifier: {
//			PrettyPrint("~id " << curt.str << " exp:");
//			if(token_peek.type == Token_Assignment) {
//				expressions->add(Expression(curt.str, Expression_IdentifierLHS));
//				token_next(); 
//				layer++;
//				PrettyPrint("var assignment");
//				expressions->add(Expression(curt.str, Expression_BinaryOpAssignment));
//				
//				expressions->add(Expression(curt.str, ExpressionGuard_Assignment));
//				token_next();
//				parse_expressions(&expressions->last->expressions);
//				layer--;
//			} 
//			else {
//				expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
//				parse_conditional(&expressions->last->expressions);
//			}
//		}break;
//
//		//I think if it's anything else it should be parsed normally
//		default: {
//			expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
//			parse_conditional(&expressions->last->expressions);
//		}break;
//	}
//	layer--;
//}
//
//// <declaration> :: = "int" <id> [ = <exp> ] ";"
//void parse_declaration(Declaration* declaration, BlockItem* bi = nullptr) {
//	layer++;
//
//	Expect(Token_Identifier) {
//		//Declaration* decl = new Declaration();
//		declaration->identifier = curt.str;
//		declaration->type = Decl_Int;
//		PrettyPrint("declaration of var '" << declaration->identifier << "' of type int");
//
//		token_next();
//
//		Expect(Token_Assignment) {
//			layer++;
//			PrettyPrint("variable assignment");
//			token_next();
//			declaration->expressions.add(Expression(curt.str, ExpressionGuard_Assignment));
//			parse_expressions(&declaration->expressions.last->expressions);
//			declaration->initialized = true;
//			layer--;
//			token_next();
//
//			Expect(Token_Semicolon);
//			//if (bi) bi.declaration = decl;
//			ExpectFail("Expected a ;");
//		}
//		ElseExpect(Token_Semicolon) {
//			layer++;
//			PrettyPrint("no assignment");
//			layer--;
//			//add expression guard for assignment regardless of if there is one so we can default to 0 in assembly 
//			declaration->expressions.add(Expression(curt.str, ExpressionGuard_Assignment));
//			//if (bi) bi.declaration = decl;
//		}
//		ExpectFail("Expected a ;")
//	}
//	ExpectFail("Expected an identifier after type keyword");
//
//
//	layer--;
//}
//
//// <statement> :: = "return" <exp> ";" | 
////                  <exp> ";" | 
////                  "if" "(" <exp> ")" <statement> [ "else" <statement> ] 
//void parse_statement(Statement* statement, BlockItem* bi = nullptr) {
//	layer++;
//
//	switch (curt.type) {
//
//		//both if and else tokens are handled here!
//		//if an else token is found outside of this then it has no preceeding if statement
//		//if and else are added to a Conditional Statement to allow easy placing of if labels
//		case Token_If: {
//			PrettyPrint("if statement");
//			if(statement->type != Statement_Conditional)
//				statement->type = Statement_Conditional;
//		
//			token_next(); // if -> (
//			Expect(Token_OpenParen) {
//				token_next(); //( -> exp
//				//create actual if statement and parse it's expressions
//				statement->statements.add(Statement(Statement_If));
//				Statement* is = statement->statements.last;
//				parse_expressions(&is->expressions);
//				if (is->expressions.size() == 0) ParseFail("missing expression for if statement");
//				
//				token_next(); // exp -> )
//				Expect(Token_CloseParen) {
//					if (token_peek.type == Token_Unsigned32) { ParseFail("invalid attempt to declare a variable in non-scoped if statement"); }
//					else {
//						token_next(); // ) -> statement
//						
//						//parse for if statement's statements
//						is->statements.add(Statement(Statement_Conditional));
//						parse_statement(is->statements.last);
//						Expect(Token_Semicolon) {
//							//check for optional else
//							if (token_peek.type == Token_Else) {
//								token_next(); // ; -> else
//								PrettyPrint("else statement");
//								statement->statements.add(Statement(Statement_Else));
//								Statement* es = statement->statements.last;
//								es->statements.add(Statement(Statement_Conditional));
//								token_next(); // else -> whatever follows
//								parse_statement(es->statements.last);
//							}
//						} ExpectFail("Expected a ;");
//					}
//				} ExpectFail("missing ) for if statement");
//			} ExpectFail("Expected ( after if");
//		}break;
//
//		case Token_Else: {
//			ParseFail("else statement found without preceeding if statement");
//		}break;
//
//		case Token_Literal:
//		case Token_Identifier: {
//			PrettyPrint("exp statement:");
//			if (statement->type != Statement_Expression)
//				statement->type = Statement_Expression;
//			parse_expressions(&statement->expressions);
//			token_next();
//			Expect(Token_Semicolon) {
//				//if (bi) bi.statement = smt;
//			} ExpectFail("Expected a ;");
//		}break;
//
//		case Token_Return: {
//			PrettyPrint("return statement:");
//			//Statement* smt = new Statement(Statement_Return);
//			if (statement->type != Statement_Return)
//				statement->type = Statement_Return;
//			token_next();
//			parse_expressions(&statement->expressions);
//			token_next();
//			Expect(Token_Semicolon) {
//				//if (bi) bi.statement = smt;
//			} ExpectFail("Expected a ;");
//		}break;
//
//	}
//	layer--;
//}
//
//// <function> :: = "int" <id> "(" ")" "{" { <block item> } "}"
//void parse_function(array<Function>* functions) {
//	layer++;
//
//	//Expect asks if the next tokens type matches a certain criteria and if it doesnt
//	//we throw a parse fail
//	token_next();                                      //Expect keyword                                 
//	Expect(Token_Unsigned32) { token_next();                //Expect function identifier
//		Expect(Token_Identifier) {        
//			PrettyPrint("Parse begin on function " << curt.str);
//			functions->add(Function(curt.str));
//			Function* function = functions->last;
//			token_next();                              // Expect (
//			Expect(Token_OpenParen) { token_next();      // Expect )
//				Expect(Token_CloseParen) { token_next(); // Expect {
//					Expect(Token_OpenBrace) {
//						while (token_peek.type != Token_CloseBrace) {
//							BlockItem bi = BlockItem();
//							if (token_peek.type == Token_Unsigned32) {
//								// if we find a keyword then we are declaring a variable
//								token_next(); token_next();
//								bi.is_declaration = 1;
//								functions->last->blockitems.add(bi);
//								parse_declaration(&function->blockitems.last->declaration, function->blockitems.last);
//							}
//							else{
//								//else we must be doing some kind of statement 
//								token_next();
//								functions->last->blockitems.add(bi);
//								parse_statement(&function->blockitems.last->statement, function->blockitems.last);
//							}
//							if (token_peek.type == Token_EOF) {
//								ParseFail("EOF reached before closing function");
//								goto function_fail;
//							}
//						}
//						token_next();
//						Expect(Token_CloseBrace) {
//						} ExpectFail("Expected }");
//					} ExpectFail("Expected {");
//				} ExpectFail("Expected )");
//			} ExpectFail("Expected (");
//		} ExpectFail("invalid function identifier following keyword");
//	} ExpectFail("Expected a keyword (int, float, etc..) as first token");
//
//function_fail:
//	layer--;
//}

enum ParseState {
	psGlobal,      	// <program>       :: = <function>
	psFunction,		// <function>      :: = "int" <id> "(" ")" "{" { <block item> } "}"
	psBlockItem,   	// <block item>    :: = <statement> | <declaration>
	psDeclaration,	// <declaration>   :: = "int" <id> [ = <exp> ] ";"
	psStatement,	// <statement>     :: = "return" <exp> ";" | <exp> ";" | "if" "(" <exp> ")" <statement> [ "else" <statement> ] | "{" { <block-item> } "}
	psExpression,	// <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional,	// <conditional>   :: = <logical or> [ "?" <exp> ":" <conditional> ]
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
	psFactor,		// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
	psUnary,        // <unary>         :: = "!" | "~" | "-"
}; 

//working vars
Program*     program;
Function*    function;
BlockItem*   blockitem;
Statement*   statement;
Declaration* declaration;
Expression*  expression;

//stacks
array<Function*>    functions;
array<BlockItem*>   blockitems;
array<Statement*>   statements;
array<Declaration*> declarations;
array<Expression*>  expressions;

enum FlagBits {

}; typedef u32 Flags;

inline void push_function() { functions.add(function); }
inline void pop_function() { function = *functions.last; functions.pop(); }

inline void new_function(string name, b32 set = 1) {
	program->functions.add(Function(name));
	function = program->functions.last;
}

inline void push_block_item() { blockitems.add(blockitem); }
inline void pop_block_item() { blockitem = *blockitems.last; blockitems.pop(); }

inline void new_block_item(b32 set = 1) {
	if (blockitem) blockitems.add(blockitem);
	if(set) function->blockitems.add(BlockItem());
	blockitem = function->blockitems.last;
}

inline void push_declaration() { declarations.add(declaration); }
inline void pop_declaration() { declaration = *declarations.last; declarations.pop(); }

inline void new_declaration(string identifier, Token_Type type, b32 set = 1) {
	blockitem->declaration = Declaration{ type,identifier };
	if(set) declaration = &blockitem->declaration;
	blockitem->is_declaration = 1;
}

inline void push_statement() { if (statement) statements.add(statement); }
inline void pop_statement()  { statement = *statements.last; statements.pop(); }

inline void new_statement_on_block_item(string str, StatementType type, b32 set = 1) {
	blockitem->statement = Statement(str, type);
	if(set) statement = &blockitem->statement;
}

inline void new_statement_on_statement(string str, StatementType type, b32 set = 1) {
	statement->statements.add(Statement(str, type));
	if(set) statement = (*statements.last)->statements.last;
}

inline void push_expression() { if (expression) expressions.add(expression); }
inline void pop_expression()  { expression = *expressions.last; expressions.pop(); }

inline void new_expression_on_declaration(string str, ExpressionType type, b32 set = 1) {
	declaration->expressions.add(Expression(str, type));
	if(set) expression = declaration->expressions.last;
}

inline void new_expression_on_expression(string str, ExpressionType type, b32 set = 1) {
	expression->expressions.add(Expression(str, type));
	if(set) expression = expression->expressions.last;
}



#define EarlyOut goto emergency_exit
void parser(ParseState state) {
	layer++;
	switch (state) {

		case psGlobal: { //////////////////////////////////////////////////////////////////////
			while (!next_match(Token_EOF)) {
				ExpectOneOf(typeTokens) {
					token_next();
					Expect(Token_Identifier){
						if (next_match(Token_OpenParen)) parser(psFunction);
						
					}
					//else if(check_signature(Token_Identifier, Token_))
					Expect(Token_Identifier) {
						token_next();
						Expect(Token_OpenParen) {

						}
					}
				}ExpectFail("yeah i dont know right now");
			}
		}break;

		case psFunction: { ////////////////////////////////////////////////////////////////////
			push_function();
			token_next();
			Expect(Token_OpenParen) { 
				new_function(curt.str);
				token_next();
				//function parameter checking goes here
				Expect(Token_CloseParen) {
					token_next();
					Expect(Token_OpenBrace) {
						while (!next_match(Token_CloseBrace)) {
							token_next();
							parser(psBlockItem);

							if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); EarlyOut; }
						}
					}ExpectFail("expected {");
				}ExpectFail("expected )");
			}ExpectFail("expected (");
			pop_function();
		}break;

		case psBlockItem: {////////////////////////////////////////////////////////////////////
			push_block_item();
			new_block_item();
			ExpectOneOf(typeTokens) {
				parser(psDeclaration); 
			}
			else {
				
				
			}
			pop_block_item();
		}break;

		case psDeclaration: {///////////////////////////////////////////////////////////////////
			push_declaration();
			Token_Type type = curt.type;
			token_next();
			Expect(Token_Identifier) {
				new_declaration(curt.str, type);
				token_next();
				Expect(Token_Assignment) {
					new_expression_on_declaration(curt.str, ExpressionGuard_Assignment);
					parser(psExpression);
					declaration->initialized = true;
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ; after variable assignment")
				}
				ElseExpect(Token_Semicolon) {
					//add expression guard for assignment regardless of if there is one so we can default to 0 in assembly 
					new_expression_on_declaration(curt.str, ExpressionGuard_Assignment);
				}
				ExpectFail("expected a semicolon or assignment after variable declaration");
			}
			pop_declaration();
		}break;


		case psStatement: {
			push_statement();
			token_next();
			switch (curt.type) {
				case Token_If: {
					
					token_next();

				}break;
			}
			pop_statement();
		}break;

		case psExpression: {///////////////////////////////////////////////////////////////////
			push_expression();
			token_next();
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						new_expression_on_expression(curt.str, Expression_IdentifierLHS, 0);
						token_next();
						new_expression_on_expression(curt.str, Expression_BinaryOpAssignment, 0);
						new_expression_on_expression(curt.str, ExpressionGuard_Assignment);
						token_next();
						parser(psExpression);
					}
					else {
						new_expression_on_expression(curt.str, ExpressionGuard_HEAD);
						parser(psConditional);
					}
				}break;
				default: {
					new_expression_on_expression(curt.str, ExpressionGuard_HEAD);
					parser(psConditional);
				}break;

			}
			pop_expression();
		}break;

		case psConditional: {///////////////////////////////////////////////////////////////////

		}break;
	}

emergency_exit:
	layer--;
}

// <program> ::= <function>
void suParser::parse(array<token>& tokens_in, Program& mother) {
	//Program mother;

	tokens = tokens_in;
	curt = tokens[0];
	program = &mother;

	PrettyPrint("Parse begin");
	

	parser(psGlobal);


	//return mother;
}