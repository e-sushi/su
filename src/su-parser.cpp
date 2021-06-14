#include "su-parser.h"

//master token
//should probably redo this at some point I think, but i want to try doing it this way to see how it works
token curt;

#define token_next curt = tokens.next()
#define token_last curt = tokens.previous()

#define PARSE_FAIL(error)\
std::cout << "Error: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl;

#define PARSE_EXPECT(expected, token, error)\
if (token.type != expected) {PARSE_FAIL(token, error);}

#define EXPECT(tok_type)\
if(curt.type == tok_type)

//collection of flags/counters to help with error checking throughout the parsing process
struct {
	bool unaryf = 0;    //true if a unary op has been found
 	bool integerf = 0;  //true if an integer has been found
 	u32 expected_close_parens = 0; //inc when we find a ( and dec when we find a ), error if this isn't 0 in some cases
} syntax;

array<Expression> parse_expressions(array<token>& tokens) {
	array<Expression> expressions;

	token_next; //expect expression or semicolon
	EXPECT(tok_Semicolon) {
		if (syntax.unaryf) {
			//if this flag is set we didn't find an integer before ending the expressions
			PARSE_FAIL("expected an integer literal before end of statement");
		}
		token_last; //backup token since parse_statements checks for semicolons
		return expressions;
	}
	else EXPECT(tok_IntegerLiteral) {
		Expression e(curt.str, Expression_IntegerLiteral);
		syntax.unaryf = 0; //integer literal was found
		syntax.integerf = 1;
		e.expressions.add(parse_expressions(tokens));
		expressions.add(e);
		return expressions;
	} 
	else EXPECT(tok_BitwiseComplement) {
		if(syntax.integerf){ //early out if unary op is found after an integer
			PARSE_FAIL("unexpected unary operator following integer literal");
			return expressions;
		}
		Expression e(curt.str, Expression_UnaryOpBitComp);
		syntax.unaryf = 1; //set unary_flag to indicate that we are expecting an integer literal
		e.expressions.add(parse_expressions(tokens));
		expressions.add(e);
		return expressions;
	}
	else EXPECT(tok_LogicalNOT) {
		if(syntax.integerf){ //early out if unary op is found after an integer
			PARSE_FAIL("unexpected unary operator following integer literal");
			return expressions;
		}
		Expression e(curt.str, Expression_UnaryOpLogiNOT);
		syntax.unaryf = 1; //set unary_flag to indicate that we are expecting an integer literal
		e.expressions.add(parse_expressions(tokens));
		expressions.add(e);
		return expressions;
	}
	else EXPECT(tok_Negation) {
		if(syntax.integerf){ //early out if unary op is found after an integer
			PARSE_FAIL("unexpected unary operator following integer literal");
			return expressions;
		}
		Expression e(curt.str, Expression_UnaryOpNegate);
		syntax.unaryf = 1; //set unary_flag to indicate that we are expecting an integer literal
		e.expressions.add(parse_expressions(tokens));
		expressions.add(e);
		return expressions;
	}
	else if (syntax.unaryf) {
		//if we've reached this point and the unary_flag is set then there is no ; and no integer literal
		PARSE_FAIL("expected integer literal followed by ;");
	}
	else PARSE_FAIL("expected an expression or semicolon");

	return expressions;
}

array<Statement> parse_statements(array<token>& tokens) {
	array<Statement> statements;
	token_next; //expect return
	EXPECT(tok_Return) {

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

	return statements;
}

Function parse_function(array<token>& tokens) {
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

	return function;
}

Program suParser::parse(array<token>& tokens) {
	Program mother;

	Function function = parse_function(tokens);
	mother.functions.add(function);

	return mother;
}