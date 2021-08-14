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
#define Token_next curt = tokens.next()
#define Token_last curt = tokens.prev()

#define Token_peek tokens.peek(0)
#define Token_look_back(i) tokens.lookback(i)

#define ParseFail(error)\
std::cout << "\nError: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl;

#define Expect(Token_Type)\
if(curt.type == Token_Type) {

#define ExpectOneOf(exp_type)\
if(exp_type.has(curt.type)) {

#define ElseExpect(Token_Type)\
} else if (curt.type == Token_Type) {


#define ExpectFail(error)\
} else { ParseFail(error); }

#define ExpectFailCode(failcode)\
} else { failcode }

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
	{Token_BitwiseComplement, Expression_UnaryOpBitComp},
	{Token_LogicalNOT,        Expression_UnaryOpLogiNOT},
	{Token_Negation,          Expression_UnaryOpNegate},
};



void parse_term(array<Expression>* expressions);
void parse_expressions(array<Expression>* expressions);

// <factor> :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
void parse_factor(array<Expression>* expressions) {
	layer++;
	PrettyPrint("factor");
	expressions->add(Expression(curt.str, ExpressionGuard_Factor));

	switch (curt.type) {

		case Token_Literal: {
			PrettyPrint("int literal:  " << curt.str);
			syntax.integerf = 1;
			expressions->last->expressions.add(Expression(curt.str, Expression_IntegerLiteral));
			layer--;
			return;
		}break;

		case Token_OpenParen: {
			PrettyPrint("( OPEN");
			expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
			
			Token_next;
			parse_expressions(&expressions->last->expressions);
			
			Token_next;
			Expect(Token_CloseParen) 
				PrettyPrint(") CLOSE");
				layer--;
				return;
			ExpectFail("expected a )")
		}break;

		case Token_Identifier: {
			PrettyPrint("identifier: " << curt.str);
			expressions->add(Expression(curt.str, Expression_IdentifierRHS));
			layer--;
			return;
		}break;

		default: {
			ExpectOneOf(unaryOps)
				PrettyPrint("unary op " << ExTypeStrings[*unaryOps.at(curt.type)]);
				Token_next;
				expressions->add(Expression(curt.str, *unaryOps.at(curt.type)));
				parse_factor(&expressions->last->expressions);
				layer--;
				return;
			ExpectFail("unexpected token found in factor");
		}
	}

	layer--;
}

// <term> :: = <factor> { ("*" | "/" | "%") <factor> }
void parse_term(array<Expression>* expressions) {
	layer++;
	PrettyPrint("term");

	//all terms become factors
	expressions->add(Expression(curt.str, ExpressionGuard_Term));
	parse_factor(&expressions->last->expressions);

	while (
		Token_peek.type == Token_Multiplication || Token_peek.type == Token_Division ||
		Token_peek.type == Token_Modulo) {
		
		Token_next;
		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
		PrettyPrint("term");

		//add operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		expressions->add(Expression(curt.str, ExpressionGuard_Term));
		Token_next;
		parse_factor(&expressions->last->expressions);
	}

	layer--;
}

// <additive-exp> ::= <term> { ("+" | "-") <term> }
void parse_additive(array<Expression>* expressions) {
	layer++;
	string name = "additive";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_Additive));
	parse_term(&expressions->last->expressions);

	while (Token_peek.type == Token_Plus || Token_peek.type == Token_Negation) {
		Token_next;
		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
		PrettyPrint(name);
		
		//add operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_Additive));
		Token_next;
		parse_term(&expressions->last->expressions);
	}

	layer--;
}

// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
void parse_bitshift(array<Expression>* expressions) {
	layer++;
	string name = "bitshift";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
	parse_additive(&expressions->last->expressions);

	while (Token_peek.type == Token_BitShiftRight || Token_peek.type == Token_BitShiftLeft) {
		Token_next;
		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
		Token_next;
		parse_additive(&expressions->last->expressions);
	}

	layer--;
}

// <relational> :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
void parse_relational(array<Expression>* expressions) {
	layer++;
	string name = "relational";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_Relational));
	parse_bitshift(&expressions->last->expressions);

	while (
		Token_peek.type == Token_LessThan ||
		Token_peek.type == Token_GreaterThan ||
		Token_peek.type == Token_LessThanOrEqual ||
		Token_peek.type == Token_GreaterThanOrEqual) {

		Token_next; 
		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
		
		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_Relational));
		Token_next;
		parse_bitshift(&expressions->last->expressions);
	}

	layer--;
}

// <equality> ::= <relational> { ("!=" | "==") <relational> }
void parse_equality(array<Expression>* expressions) {
	layer++;
	string name = "equality";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_Equality));
	parse_relational(&expressions->last->expressions);

	while (Token_peek.type == Token_NotEqual || Token_peek.type == Token_Equal) {
		Token_next; 
		PrettyPrint("binary op " << ExTypeStrings[*binaryOps.at(curt.type)]);
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
		
		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_Equality));
		Token_next;
		parse_relational(&expressions->last->expressions);
	}

	layer--;
}

// <bitwise and> :: = <equality> { "&" <equality> }
void parse_bitwise_and(array<Expression>* expressions) {
	layer++;
	string name = "bit AND";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
	parse_equality(&expressions->last->expressions);

	while (Token_peek.type == Token_BitAND) {
		Token_next;
		PrettyPrint("binary op &");
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitAND));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
		Token_next;
		parse_equality(&expressions->last->expressions);
	}

	layer--;
}

// <bitwise xor> :: = <bitwise and> { "^" <bitwise and> }
void parse_bitwise_xor(array<Expression>* expressions) {
	layer++;
	string name = "bit XOR";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
	parse_bitwise_and(&expressions->last->expressions);

	while (Token_peek.type == Token_BitXOR) {
		Token_next;
		PrettyPrint("binary op ^");
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpXOR));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
		Token_next;
		parse_bitwise_and(&expressions->last->expressions);
	}

	layer--;
}

//<bitwise or> :: = <bitwise xor> { "|" <bitwise xor> }
void parse_bitwise_or(array<Expression>* expressions) {
	layer++;
	string name = "bit OR";
	PrettyPrint(name);

	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
	parse_bitwise_xor(&expressions->last->expressions);

	while (Token_peek.type == Token_BitOR) {
		Token_next;
		PrettyPrint("binary op |");
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitOR));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
		Token_next;
		parse_bitwise_xor(&expressions->last->expressions);

	}

	layer--;
}

// <logical and> :: = <bitwise or> { "&&" <bitwise or> } 
void parse_logical_and(array<Expression>* expressions) {
	layer++;
	string name = "logi AND";
	PrettyPrint(name);
	
	//decend down expression guards
	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
	parse_bitwise_or(&expressions->last->expressions);

	while (Token_peek.type == Token_AND) {
		Token_next;
		PrettyPrint("binary op &&");
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpAND));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
		Token_next;
		parse_bitwise_or(&expressions->last->expressions);
	}

	layer--;
}

// <logical or> :: = <logical and> { "||" <logical and> }
void parse_logical_or(array<Expression>* expressions) {
	layer++;
	string name = "logi OR";
	PrettyPrint(name);

	//decend into expression guard hell
	expressions->add(Expression(curt.str, ExpressionGuard_LogicalOR));
	parse_logical_and(&expressions->last->expressions);

	while (Token_peek.type == Token_OR) {
		Token_next;
		PrettyPrint("binary op ||");
		PrettyPrint(name);

		//add operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpOR));

		//decend down expression guards
		expressions->add(Expression(curt.str, ExpressionGuard_LogicalOR));
		Token_next;
		parse_logical_and(&expressions->last->expressions);
	}

	layer--;
}

// <conditional> ::= <logical or> [ "?" <exp> ":" <conditional> ]
void parse_conditional(array<Expression>* expressions) {
	layer++;
	PrettyPrint("conditional");

	expressions->add(Expression(curt.str, ExpressionGuard_Conditional));
	parse_logical_or(&expressions->last->expressions);

	if (Token_peek.type == Token_QuestionMark) {
		Token_next; 
		PrettyPrint("ternary conditional");
		expressions->add(Expression(curt.str, ExpressionGuard_Assignment));
		Token_next;
		parse_expressions(&expressions->last->expressions);
		Token_next;
		Expect(Token_Colon) {
			expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
			Token_next;
			parse_conditional(&expressions->last->expressions);
		}
		ExpectFail("expected : for ternary conditional")
	}


	layer--;
}

// <exp> :: = <id> "=" <exp> | <conditional>
void parse_expressions(array<Expression>* expressions) {
	layer++;
	string name = "exp";
	 
	switch (curt.type) {
		case Token_Identifier: {
			PrettyPrint("~id " << curt.str << " exp:");
			if(Token_peek.type == Token_Assignment) {
				expressions->add(Expression(curt.str, Expression_IdentifierLHS));
				Token_next; 
				layer++;
				PrettyPrint("var assignment");
				expressions->add(Expression(curt.str, Expression_BinaryOpAssignment));
				
				expressions->add(Expression(curt.str, ExpressionGuard_Assignment));
				Token_next;
				parse_expressions(&expressions->last->expressions);
				layer--;
			} 
			else {
				expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
				parse_conditional(&expressions->last->expressions);
			}
		}break;

		//I think if it's anything else it should be parsed normally
		default: {
			expressions->add(Expression(curt.str, ExpressionGuard_HEAD));
			parse_conditional(&expressions->last->expressions);
		}break;
	}
	layer--;
}

// <declaration> :: = "int" <id> [ = <exp> ] ";"
void parse_declaration(Declaration* declaration, BlockItem* bi = nullptr) {
	layer++;

	Expect(Token_Identifier) {
		//Declaration* decl = new Declaration();
		declaration->identifier = curt.str;
		declaration->type = Decl_Int;
		PrettyPrint("declaration of var '" << declaration->identifier << "' of type int");

		Token_next;

		Expect(Token_Assignment) {
			layer++;
			PrettyPrint("variable assignment");
			Token_next;
			declaration->expressions.add(Expression(curt.str, ExpressionGuard_Assignment));
			parse_expressions(&declaration->expressions.last->expressions);
			declaration->initialized = true;
			layer--;
			Token_next;

			Expect(Token_Semicolon);
			//if (bi) bi.declaration = decl;
			ExpectFail("expected a ;");
		}
		ElseExpect(Token_Semicolon) {
			layer++;
			PrettyPrint("no assignment");
			layer--;
			//add expression guard for assignment regardless of if there is one so we can default to 0 in assembly 
			declaration->expressions.add(Expression(curt.str, ExpressionGuard_Assignment));
			//if (bi) bi.declaration = decl;
		}
		ExpectFail("expected a ;")
	}
	ExpectFail("expected an identifier after type keyword");


	layer--;
}

// <statement> :: = "return" <exp> ";" | 
//                  <exp> ";" | 
//                  "if" "(" <exp> ")" <statement> [ "else" <statement> ] 
void parse_statement(Statement* statement, BlockItem* bi = nullptr) {
	layer++;

	switch (curt.type) {

		//both if and else tokens are handled here!
		//if an else token is found outside of this then it has no preceeding if statement
		//if and else are added to a Conditional Statement
		case Token_If: {
			PrettyPrint("if statement");
			if(statement->type != Statement_Conditional)
				statement->type = Statement_Conditional;
		
			Token_next; // if -> (
			Expect(Token_OpenParen) {
				Token_next; //( -> exp
				//create actual if statement and parse it's expressions
				statement->statements.add(Statement(Statement_If));
				Statement* is = statement->statements.last;
				parse_expressions(&is->expressions);
				if (is->expressions.size() == 0) ParseFail("missing expression for if statement");
				
				Token_next; // exp -> )
				Expect(Token_CloseParen) {
					if (Token_peek.type == Token_Keyword) { ParseFail("invalid attempt to declare a variable in non-scoped if statement"); }
					else {
						Token_next; // ) -> statement
						
						//parse for if statement's statements
						is->statements.add(Statement(Statement_Conditional));
						parse_statement(is->statements.last);
						Expect(Token_Semicolon) {
							//check for optional else
							if (Token_peek.type == Token_Else) {
								Token_next; // ; -> else
								PrettyPrint("else statement");
								statement->statements.add(Statement(Statement_Else));
								Statement* es = statement->statements.last;
								es->statements.add(Statement(Statement_Conditional));
								Token_next; // else -> whatever follows
								parse_statement(es->statements.last);
							}
						} ExpectFail("expected a ;");
					}
				} ExpectFail("missing ) for if statement");
			} ExpectFail("expected ( after if");
		}break;

		case Token_Else: {
			ParseFail("else statement found without preceeding if statement");
		}break;

		case Token_Literal:
		case Token_Identifier: {
			PrettyPrint("exp statement:");
			if (statement->type != Statement_Expression)
				statement->type = Statement_Expression;
			parse_expressions(&statement->expressions);
			Token_next;
			Expect(Token_Semicolon) {
				//if (bi) bi.statement = smt;
			} ExpectFail("expected a ;");
		}break;

		case Token_Return: {
			PrettyPrint("return statement:");
			//Statement* smt = new Statement(Statement_Return);
			if (statement->type != Statement_Return)
				statement->type = Statement_Return;
			Token_next;
			parse_expressions(&statement->expressions);
			Token_next;
			Expect(Token_Semicolon) {
				//if (bi) bi.statement = smt;
			} ExpectFail("expected a ;");
		}break;

	}
	layer--;
}

// <function> :: = "int" <id> "(" ")" "{" { <block item> } "}"
void parse_function(array<Function>* functions) {
	layer++;

	//Expect asks if the next tokens type matches a certain criteria and if it doesnt
	//we throw a parse fail
	Token_next;                                      //expect keyword                                 
	Expect(Token_Keyword) { Token_next;                //expect function identifier
		Expect(Token_Identifier) {        
			PrettyPrint("Parse begin on function " << curt.str);
			functions->add(Function(curt.str));
			Function* function = functions->last;
			Token_next;                              // expect (
			Expect(Token_OpenParen) { Token_next;      // expect )
				Expect(Token_CloseParen) { Token_next; // expect {
					Expect(Token_OpenBrace) {
						while (Token_peek.type != Token_CloseBrace) {
							BlockItem bi = BlockItem();
							if (Token_peek.type == Token_Keyword) {
								// if we find a keyword then we are declaring a variable
								Token_next; Token_next;
								bi.is_declaration = 1;
								functions->last->blockitems.add(bi);
								parse_declaration(&function->blockitems.last->declaration, function->blockitems.last);
							}
							else{
								//else we must be doing some kind of statement 
								Token_next;
								functions->last->blockitems.add(bi);
								parse_statement(&function->blockitems.last->statement, function->blockitems.last);
							}
							if (Token_peek.type == Token_EOF) {
								ParseFail("EOF reached before closing function");
								goto function_fail;
							}
						}
						Token_next;
						Expect(Token_CloseBrace) {
						} ExpectFail("expected }");
					} ExpectFail("expected {");
				} ExpectFail("expected )");
			} ExpectFail("expected (");
		} ExpectFail("invalid function identifier following keyword");
	} ExpectFail("expected a keyword (int, float, etc..) as first token");

function_fail:
	layer--;
}

// <program> ::= <function>
void suParser::parse(array<token>& tokens_in, Program& mother) {
	//Program mother;

	tokens = tokens_in;
	curt = tokens[0];

	PrettyPrint("Parse begin");

	parse_function(&mother.functions);

	//return mother;
}