#pragma once
#ifndef SU_PARSER_H
#define SU_PARSER_H

#include "utils/utility.h"
#include "su-lexer.h"

enum ASTType : u32 {
	AST_Program,
	AST_Function,
	AST_Statement,
	AST_Expression
};

enum FuncType : u32 {
	INT,
	FLOAT,
	DOUBLE
};

//abstract syntax tree
//this could probably just be Program and hold a vector of functions that holds a vector of statement, etc.
struct AST {
	ASTType type;

	//tokens relevant to the AST node, dunno if this is necessary yet
	array<token> tokens;
	array<AST*> children;
};

struct Function : public AST {
	string identifier = "";
	FuncType func_type;

	Function() {
		type = AST_Function;
	}

	Function(string identifier) {
		type = AST_Function;
		this->identifier = identifier;
	}

};

//im not sure if i want all these different type enums yet
enum StatementType : u32 {
	Statement_Return
};

struct Statement : public AST {
	StatementType statement_type;

	Statement(StatementType st) {
		type = AST_Statement;
		statement_type = st;
	}
};

enum ExpressionType : u32 {
	Expression_IntegerLiteral
};

struct Expression : public AST {
	string expstr;
	ExpressionType expression_type;
	
	Expression(string expstr, ExpressionType expression_type) {
		type = AST_Expression;
		this->expression_type = expression_type;
		this->expstr = expstr;
	}
};

namespace suParser {
	AST parse(array<token>& tokens);
}


#endif