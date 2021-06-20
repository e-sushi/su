#include "su-parser.h"

//TODO remove returning arrays of expressions and pass them by reference and modify instead
//     or just use global arrays like we use a global string in assembling
//TODO optimize the tree by bypassing nodes for expressions
//	   like, if an expression goes straight to being an int, we don't need several layers of 
//     nodes determining that


// full backus naur stuff
// <program>       :: = <function>
// <function>      :: = "int" <id> "(" ")" "{" <statement> "}"
// <statement>     :: = "return" <exp> ";"
// <exp>           :: = <logical and> { "||" <logical and> }
// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
// <bitwise and>   :: = <equality> { "&" <equality> }
// <equality>      :: = <relational> { ("!=" | "==") <relational> }
// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
// <additive>      :: = <term> { ("+" | "-") <term> }
// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int>
// <unary>         :: = "!" | "~" | "-"

bool master_logger = false;

#define PARSEOUT(message)\
if(master_logger){ for(int i = 0; i < layer; i++)\
if(i % 2 == 0) std::cout << "|   ";\
else std::cout << "!   ";\
std::cout << message << std::endl;}

u32 layer = 0; //inc when we go into anything, dec when we come out

//master token
//should probably redo this at some point I think, but i want to try doing it this way to see how it works
token curt;
array<token> tokens;

//Expression gexp;

//These defines are mostly for conveinence and clarity as to what im doing
#define token_next curt = tokens.next()
#define token_last curt = tokens.prev()

#define token_peek tokens.peek(0)
#define token_look_back(i) tokens.lookback(i)

#define PARSE_FAIL(error)\
std::cout << "Error: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl;

#define EXPECT(tok_type)\
if(curt.type == tok_type)

#define EXPECT_ONE_OF(exp_type)\
if(is_in(curt.type, exp_type))

//collection of flags/counters to help with error checking throughout the parsing process
struct {
	bool unaryf = 0;    //true if a unary op has been found
	bool binaryf = 0;   //true if a binary op has been found
 	bool integerf = 0;  //true if an integer has been found
} syntax;

//operator "maps" until i make an actual map struct
#define PTE pair<token_type, ExpressionType>
array<PTE> binaryOps{
	PTE(tok_Multiplication,     Expression_BinaryOpMultiply),
	PTE(tok_Division,           Expression_BinaryOpDivision),
	PTE(tok_Negation,           Expression_BinaryOpMinus),
	PTE(tok_Plus,               Expression_BinaryOpPlus),
	PTE(tok_AND,                Expression_BinaryOpAND),
	PTE(tok_OR,                 Expression_BinaryOpOR),
	PTE(tok_LessThan,           Expression_BinaryOpLessThan),
	PTE(tok_GreaterThan,        Expression_BinaryOpGreaterThan),
	PTE(tok_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual),
	PTE(tok_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual),
	PTE(tok_Equal,              Expression_BinaryOpEqual),
	PTE(tok_NotEqual,           Expression_BinaryOpNotEqual),
	PTE(tok_BitAND,             Expression_BinaryOpBitAND),
	PTE(tok_BitOR,              Expression_BinaryOpBitOR),
	PTE(tok_BitXOR,		        ExpressionGuard_BitXOR),
	PTE(tok_BitShiftLeft,       Expression_BinaryOpBitShiftLeft),
	PTE(tok_BitShiftRight,      Expression_BinaryOpBitShiftRight),
	PTE(tok_Modulo,             Expression_BinaryOpModulo),
};


array<PTE> unaryOps{
	PTE(tok_BitwiseComplement, Expression_UnaryOpBitComp),
	PTE(tok_LogicalNOT,        Expression_UnaryOpLogiNOT),
	PTE(tok_Negation,          Expression_UnaryOpNegate),
};



void parse_term(array<Expression*>* expressions);
void parse_expressions(array<Expression*>* expressions);

// <factor> ::= "(" <exp> ")" | <unary> <factor> | <int>
void parse_factor(array<Expression*>* expressions) {
	layer++;
	string name = "factor";
	PARSEOUT("<" << name << ">:");

	token_next;
	switch (curt.type) {

		case tok_IntegerLiteral: {
			PARSEOUT("int literal:  " << curt.str);
			syntax.integerf = 1;
			expressions->add_anon(new Expression(curt.str, Expression_IntegerLiteral));
			layer--;
			return;
		}break;

		case tok_OpenParen: {
			PARSEOUT("( OPEN");
			Expression* e = new Expression(curt.str, ExpressionGuard_Factor);
			parse_expressions(&e->expressions);
			expressions->add(e);
			token_next;
			EXPECT(tok_CloseParen) {
				PARSEOUT(") CLOSE");
				layer--;
				return;
			} else PARSE_FAIL("expected a closing parentheses");
		}break;

		default: {
			EXPECT_ONE_OF(unaryOps) {
				PARSEOUT("unary op " << ExTypeStrings[vfk(curt.type, unaryOps)]);
				Expression* e = new Expression(curt.str, vfk(curt.type, unaryOps));
				parse_factor(&e->expressions);
				expressions->add(e);
				layer--;
				return;
			} else PARSE_FAIL("unexpected token found in factor");
		}
	}

	layer--;
}

// <term> :: = <factor> { ("*" | "/" | "%") <factor> }
void parse_term(array<Expression*>* expressions) {
	layer++;
	string name = "term";
	PARSEOUT("<" << name << ">:");

	//all terms become factors
	Expression* e = new Expression(curt.str, ExpressionGuard_Factor);
	parse_factor(&e->expressions);
	expressions->add(e);

	while (
		token_peek.type == tok_Multiplication || token_peek.type == tok_Division ||
		token_peek.type == tok_Modulo) {
		
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, vfk(curt.type, binaryOps)));

		Expression* e = new Expression(curt.str, ExpressionGuard_Factor);
		parse_factor(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <additive-exp> ::= <term> { ("+" | "-") <term> }
void parse_additive(array<Expression*>* expressions) {
	layer++;
	string name = "additive";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_Term);
	parse_term(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_Plus || token_peek.type == tok_Negation){
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, vfk(curt.type, binaryOps)));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_Term);
		parse_term(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
void parse_bitshift(array<Expression*>* expressions) {
	layer++;
	string name = "bitshift";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_Additive);
	parse_additive(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_BitShiftRight || token_peek.type == tok_BitShiftLeft) {

		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, vfk(curt.type, binaryOps)));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_Additive);
		parse_additive(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <relational> :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
void parse_relational(array<Expression*>* expressions) {
	layer++;
	string name = "relational";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_BitShift);
	parse_bitshift(&e->expressions);
	expressions->add(e);

	while (
		token_peek.type == tok_LessThan ||
		token_peek.type == tok_GreaterThan ||
		token_peek.type == tok_LessThanOrEqual ||
		token_peek.type == tok_GreaterThanOrEqual) {

		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, vfk(curt.type, binaryOps)));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_BitShift);
		parse_bitshift(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <equality> ::= <relational> { ("!=" | "==") <relational> }
void parse_equality(array<Expression*>* expressions) {
	layer++;
	string name = "equality";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_Relational);
	parse_relational(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_NotEqual || token_peek.type == tok_Equal) {
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, vfk(curt.type, binaryOps)));
		
		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_Relational);
		parse_relational(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <bitwise and> :: = <equality> { "&" <equality> }
void parse_bitwise_and(array<Expression*>* expressions) {
	layer++;
	string name = "bit AND";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_Equality);
	parse_equality(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_BitAND) {
		token_next;
		PARSEOUT("binary op &");
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, Expression_BinaryOpBitAND));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_Equality);
		parse_equality(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <bitwise xor> :: = <bitwise and> { "^" <bitwise and> }
void parse_bitwise_xor(array<Expression*>* expressions) {
	layer++;
	string name = "bit XOR";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_BitAND);
	parse_bitwise_and(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_BitXOR) {
		token_next;
		PARSEOUT("binary op ^");
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, Expression_BinaryOpXOR));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_BitAND);
		parse_bitwise_and(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

//<bitwise or> :: = <bitwise xor> { "|" <bitwise xor> }
void parse_bitwise_or(array<Expression*>* expressions) {
	layer++;
	string name = "bit OR";
	PARSEOUT("<" << name << ">:");

	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_BitXOR);
	parse_bitwise_xor(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_BitOR) {
		token_next;
		PARSEOUT("binary op |");
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, Expression_BinaryOpBitOR));

		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_BitXOR);
		parse_bitwise_xor(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <logical and> :: = <bitwise or> { "&&" <bitwise or> } 
void parse_logical_and(array<Expression*>* expressions) {
	layer++;
	string name = "logi AND";
	PARSEOUT("<" << name << ">:");
	
	//decend down expression guards
	Expression* e = new Expression(curt.str, ExpressionGuard_BitOR);
	parse_bitwise_or(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_AND) {
		token_next;
		PARSEOUT("binary op &&");
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, Expression_BinaryOpAND));
		
		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_BitOR);
		parse_bitwise_or(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <exp> ::= <logical-and-exp> { "||" <logical-and-exp> }
void parse_expressions(array<Expression*>* expressions) {
	layer++;
	string name = "exp";
	PARSEOUT("<" << name << ">:");
	
	//decend into expression guard hell
	Expression* e = new Expression(curt.str, ExpressionGuard_LogicalAND);
	parse_logical_and(&e->expressions);
	expressions->add(e);

	while (token_peek.type == tok_OR) {
		token_next;
		PARSEOUT("binary op ||");
		PARSEOUT("<" << name << ">:");

		//add operator expression
		expressions->add_anon(new Expression(curt.str, Expression_BinaryOpOR));
		
		//decend down expression guards
		Expression* e = new Expression(curt.str, ExpressionGuard_LogicalAND);
		parse_logical_and(&e->expressions);
		expressions->add(e);
	}

	layer--;
}

// <statement> ::= "return" <exp> ";"
void parse_statements(array<Statement*>& statements) {
	layer++;

	token_next; //expect return
	EXPECT(tok_Return) {
		PARSEOUT("return statement:");
		Statement* retstate = new Statement(Statement_Return);

		//expect expressions so gather them
		parse_expressions(&retstate->expressions);

		//reset syntax vars
		syntax = { 0, 0, 0 };

		statements.add(retstate);
		
		token_next; //expect semicolon
		EXPECT(tok_Semicolon) {

		} else PARSE_FAIL("expected ;"); 
		
	} else PARSE_FAIL("expected return statement");

	layer--;
}

// <function> ::= "int" <id> "(" ")" "{" <statement> "}"
void parse_function(array<Function*>& functions) {
	layer++;
	Function* function = new Function();

	//experimental method for parsing, will definitly change later
	//need to come up with a nice sceme that doesn't nest expeections, probably just failing
	//when expect comes up false? 
	//https://norasandler.com/2017/11/29/Write-a-Compiler.html
	//some pseudo code here looks nice, but ill wait till later to try it 
	// 
	// 
	//EXPECT asks if the next tokens type matches a certain criteria and if it doesnt
	//we throw a parse fail
	token_next; //expect keyword
	EXPECT(tok_Keyword) {
		token_next; //expect function identifier
		EXPECT(tok_Identifier) {
			function->identifier = curt.str;
			PARSEOUT("Parse begin on function " << curt.str);
			token_next; // expect (
			EXPECT(tok_OpenParen) {
				token_next; //expect )
				EXPECT(tok_CloseParen) {
					token_next; //expect {
					EXPECT(tok_OpenBrace) {
						parse_statements(function->statements);
						token_next;
						EXPECT(tok_CloseBrace) {
							functions.add(function);
						} else PARSE_FAIL("expected }");
					} else PARSE_FAIL("expected {");
				} else PARSE_FAIL("expected )");
			} else PARSE_FAIL("expected (");
		} else PARSE_FAIL("invalid function identifier following keyword");
	} else PARSE_FAIL("expected a keyword (int, float, etc..) as first token");

	layer--;
}

// <program> ::= <function>
void suParser::parse(array<token>& tokens_in, Program& mother) {
	//Program mother;

	tokens = tokens_in;

	PARSEOUT("Parse begin");

	parse_function(mother.functions);

	//return mother;
}