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

array<AST*> parse_expressions(array<token>&tokens) {
	array<AST*> expressions;

	token_next; //expect integer literal
	EXPECT(tok_IntegerLiteral) {
		AST* integer = new AST();
		integer->type = AST_Constant;
		integer->tokens.add(curt);
		expressions.add(integer);

	} else PARSE_FAIL("expected integer literal");

	return expressions;
}

array<AST*> parse_statements(array<token>& tokens) {
	array<AST*> statements;
	token_next; //expect return
	EXPECT(tok_Return) {

		AST* retstate = new AST();
		retstate->type = AST_Statement;
		retstate->tokens.add(curt);

		//expect statments so gather them 
		array<AST*> expressions = parse_expressions(tokens);
		for (AST* e : expressions) retstate->children.add(e);

		statements.add(retstate);
		
		token_next; //expect semicolon
		EXPECT(tok_Semicolon) {

		} else PARSE_FAIL("expected ;"); 
		
	} else PARSE_FAIL("expected return statement");

	return statements;
}

AST* parse_function(array<token>& tokens) {
	AST* function = new AST();
	function->type = AST_Function;

	//experimental method for parsing, will definitly change later
	//need to come up with a nice sceme that doesn't nest expeections, probably just failing
	//when expect comes up false? 
	//https://norasandler.com/2017/11/29/Write-a-Compiler.html
	//some pseudo code here looks nice, but ill wait till later to try it 
	//EXPECT asks if the next tokens type matches a certain criteria and if it doesnt
	//we throw a parse fail
	token_next; //expect keyword
	EXPECT(tok_Keyword) {
		token_next; //expect function identifier
		EXPECT(tok_Identifier) {
			token_next; // expect (
			EXPECT(tok_OpenParen) {
				token_next; //expect )
				EXPECT(tok_CloseParen) {
					token_next; //expect {
					EXPECT(tok_OpenBrace) {
						//parse statements then add them to the function node
						array<AST*> statements = parse_statements(tokens);
						for (AST* statement : statements) function->children.add(statement);
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

AST suParser::parse(array<token>& tokens) {
	AST tree;
	tree.type = AST_Program;

	AST* function = parse_function(tokens);
	tree.children.add(function);

	
	

	return tree;
}