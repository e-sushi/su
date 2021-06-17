#include "su-parser.h"

//TODO remove returning arrays of expressions and pass them by reference and modify instead
//     or just use global arrays like we use a global string in assembling
//TODO optimize the tree by bypassing nodes for expressions
//	   like, if an expression goes straight to being an int, we don't need several layers of 
//     nodes determining that


// full backus naur stuff
// <program>            :: = <function>
// <function>           :: = "int" < id > "(" ")" "{" < statement > "}"
// <statement>          :: = "return" < exp > ";"
// <exp>                :: = <logical - and -exp>{ "||" < logical - and -exp > }
// <logical - and -exp> :: = <equality - exp>{ "&&" < equality - exp > }
// <equality - exp>     :: = <relational - exp>{ ("!=" | "==") < relational - exp > }
// <relational - exp>   :: = <additive - exp>{ ("<" | ">" | "<=" | ">=") < additive - exp > }
// <additive - exp>     :: = <term>{ ("+" | "-") < term > }
// <term>               :: = <factor>{ ("*" | "/") < factor > }
// <factor>             :: = "(" < exp > ")" | <unary_op> <factor> | <int>
// <unary_op>           :: = "!" | "~" | "-"


bool master_logger = true;

#define PARSEOUT(message)\
if(master_logger){ for(int i = 0; i < layer; i++)\
if(i % 2 == 0) std::cout << "|   ";\
else std::cout << "!   ";\
std::cout << message << std::endl;}

u32 layer = 0; //inc when we go into anything, dec when we come out

//master token
//should probably redo this at some point I think, but i want to try doing it this way to see how it works
token curt;

//These defines are mostly for conveinence and clarity as to what im doing
#define token_next curt = tokens.next()
#define token_last curt = tokens.previous()

#define token_peek tokens.next(0)
#define token_look_back(i) tokens.previous(i)

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
	PTE(tok_NotEqual,           Expression_BinaryOpNotEqual)
};


array<PTE> unaryOps{
	PTE(tok_BitwiseComplement, Expression_UnaryOpBitComp),
	PTE(tok_LogicalNOT,        Expression_UnaryOpLogiNOT),
	PTE(tok_Negation,          Expression_UnaryOpNegate),
};



array<Expression> parse_term(array<token>& tokens);
array<Expression> parse_expressions(array<token>& tokens);

// <factor> ::= "(" <exp> ")" | <unary_op> <factor> | <int>
array<Expression> parse_factor(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	
	PARSEOUT("factor:");


	token_next;
	switch (curt.type) {

		case tok_IntegerLiteral: {
			PARSEOUT("int literal:  " << curt.str);
			syntax.integerf = 1;
			expressions.add(Expression(curt.str, Expression_IntegerLiteral));
			layer--;
			return expressions;
		}break;

		case tok_OpenParen: {
			PARSEOUT("( OPEN");
			Expression e(curt.str, ExpressionGuard_Factor);
			e.expressions.add(parse_expressions(tokens));
			expressions.add(e);
			token_next;
			EXPECT(tok_CloseParen) {
				PARSEOUT(") CLOSE");
				layer--;
				return expressions;
			} else PARSE_FAIL("expected a closing parentheses");
		}break;

		default: {
			EXPECT_ONE_OF(unaryOps) {
				PARSEOUT("unary op " << ExTypeStrings[vfk(curt.type, unaryOps)]);
				Expression e(curt.str, vfk(curt.type, unaryOps));
				e.expressions.add(parse_factor(tokens));
				expressions.add(e);
				layer--;
				return expressions;
			} else PARSE_FAIL("unexpected token found in factor");
		}
	}

	layer--;
	return expressions;
}

// <term> ::= <factor> { ("*" | "/") <factor> }
array<Expression> parse_term(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<term>:");
	//all terms become factors
	Expression e(curt.str, ExpressionGuard_Factor);
	e.expressions.add(parse_factor(tokens));
	expressions.add(e);

	while (token_peek.type == tok_Multiplication || token_peek.type == tok_Division) {
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<term>:");
		expressions.add(Expression(curt.str, vfk(curt.type, binaryOps)));
		Expression e(curt.str, ExpressionGuard_Factor);
		e.expressions.add(parse_factor(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <additive-exp> ::= <term> { ("+" | "-") <term> }
array<Expression> parse_additive(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<additive>:");

	//decend down expression guards
	Expression e(curt.str, ExpressionGuard_Term);
	e.expressions.add(parse_term(tokens));
	expressions.add(e);

	while (token_peek.type == tok_Plus || token_peek.type == tok_Negation){
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<additive>:");

		//add operator expression
		expressions.add(Expression(curt.str, vfk(curt.type, binaryOps)));

		//decend down expression guards
		Expression e(curt.str, ExpressionGuard_Term);
		e.expressions.add(parse_term(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <relational-exp> ::= <additive-exp> { ("<" | ">" | "<=" | ">=") <additive-exp> }
array<Expression> parse_relational(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<relational>:");

	//decend down expression guards
	Expression e(curt.str, ExpressionGuard_Additive);
	e.expressions.add(parse_additive(tokens));
	expressions.add(e);

	while (
		token_peek.type == tok_LessThan ||
		token_peek.type == tok_GreaterThan ||
		token_peek.type == tok_LessThanOrEqual ||
		token_peek.type == tok_GreaterThanOrEqual) {

		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<relational>:");

		//add operator expression
		expressions.add(Expression(curt.str, vfk(curt.type, binaryOps)));

		//decend down expression guards
		Expression e(curt.str, ExpressionGuard_Additive);
		e.expressions.add(parse_additive(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <equality-exp> ::= <relational-exp> { ("!=" | "==") <relational-exp> }
array<Expression> parse_equality(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<equality>:");

	//decend down expression guards
	Expression e(curt.str, ExpressionGuard_Relational);
	e.expressions.add(parse_relational(tokens));
	expressions.add(e);

	while (token_peek.type == tok_NotEqual || token_peek.type == tok_Equal) {
		token_next;
		PARSEOUT("binary op " << ExTypeStrings[vfk(curt.type, binaryOps)]);
		PARSEOUT("<equality>:");

		//add operator expression
		expressions.add(Expression(curt.str, vfk(curt.type, binaryOps)));
		
		//decend down expression guards
		Expression e(curt.str, ExpressionGuard_Relational);
		e.expressions.add(parse_relational(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <logical-and-exp> ::= <equality-exp> { "&&" <equality-exp> }
array<Expression> parse_logical_and(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<logi AND>:");
	
	//decend down expression guards
	Expression e(curt.str, ExpressionGuard_Equality);
	e.expressions.add(parse_equality(tokens));
	expressions.add(e);

	while (token_peek.type == tok_AND) {
		token_next;
		PARSEOUT("binary op &&");
		PARSEOUT("<logi AND>:");

		//add operator expression
		expressions.add(Expression(curt.str, Expression_BinaryOpAND));
		
		//decend down expression guards
		Expression e(curt.str, ExpressionGuard_Equality);
		e.expressions.add(parse_equality(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <exp> ::= <logical-and-exp> { "||" <logical-and-exp> }
array<Expression> parse_expressions(array<token>& tokens) {
	layer++;
	array<Expression> expressions;
	PARSEOUT("<exp>:");
	
	//decend into expression guard hell
	Expression e(curt.str, ExpressionGuard_LogicalAND);
	e.expressions.add(parse_logical_and(tokens));
	expressions.add(e);

	while (token_peek.type == tok_OR) {
		token_next;
		PARSEOUT("binary op ||");
		PARSEOUT("<exp>:");

		//add operator expression
		expressions.add(Expression(curt.str, Expression_BinaryOpOR));
		
		//decend down expression guards
		Expression e(curt.str, ExpressionGuard_LogicalAND);
		e.expressions.add(parse_logical_and(tokens));
		expressions.add(e);
	}

	layer--;
	return expressions;
}

// <statement> ::= "return" <exp> ";"
array<Statement> parse_statements(array<token>& tokens) {
	layer++;
	array<Statement> statements;
	token_next; //expect return
	EXPECT(tok_Return) {
		PARSEOUT("return statement:");
		Statement retstate(Statement_Return);

		//expect expressions so gather them
		for (Expression e : parse_expressions(tokens)) 
			retstate.expressions.add(e);

		//reset syntax vars
		syntax = { 0, 0, 0 };

		statements.add(retstate);
		
		token_next; //expect semicolon
		EXPECT(tok_Semicolon) {
			return statements;
		} else PARSE_FAIL("expected ;"); 
		
	} else PARSE_FAIL("expected return statement");

	layer--;
	return statements;
}

// <function> ::= "int" <id> "(" ")" "{" <statement> "}"
Function parse_function(array<token>& tokens) {
	layer++;
	Function function;

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
			function.identifier = curt.str;
			PARSEOUT("Parse begin on function " << curt.str);
			token_next; // expect (
			EXPECT(tok_OpenParen) {
				token_next; //expect )
				EXPECT(tok_CloseParen) {
					token_next; //expect {
					EXPECT(tok_OpenBrace) {
						//parse statements then add them to the function node
						array<Statement> statements = parse_statements(tokens);
						for (Statement statement : statements) function.statements.add(statement);
						token_next;
						EXPECT(tok_CloseBrace) {

						} else PARSE_FAIL("expected }");
					} else PARSE_FAIL("expected {");
				} else PARSE_FAIL("expected )");
			} else PARSE_FAIL("expected (");
		} else PARSE_FAIL("invalid function identifier following keyword");
	} else PARSE_FAIL("expected a keyword (int, float, etc..) as first token");

	layer--;
	return function;
}

// <program> ::= <function>
void suParser::parse(array<token>& tokens, Program& mother) {
	//Program mother;

	PARSEOUT("Parse begin");

	Function function = parse_function(tokens);
	mother.functions.add(function);

	//return mother;
}