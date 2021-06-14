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


	//////////////////////////////////////
	//// Abstract Syntax Tree Structs ////
	//////////////////////////////////////



enum ExpressionType : u32 {
	Expression_IntegerLiteral,
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate
};

struct Expression {
	string expstr;
	ExpressionType expression_type;
	ASTType type = AST_Expression;

	array<Expression> expressions;

	Expression(string expstr, ExpressionType expression_type) {
		this->expression_type = expression_type;
		this->expstr = expstr;
	}
};

//im not sure if i want all these different type enums yet
enum StatementType : u32 {
	Statement_Return
};

struct Statement {
	StatementType statement_type;
	ASTType type = AST_Statement;

	array<Expression> expressions;

	Statement(StatementType st) {
		statement_type = st;
	}
};

enum FuncType : u32 {
	INT,
	FLOAT,
	DOUBLE
};

struct Function {
	string identifier = "";
	FuncType func_type;

	ASTType type = AST_Function;

	array<Statement> statements;

	Function() {
		type = AST_Function;
	}

	Function(string identifier) {
		type = AST_Function;
		this->identifier = identifier;
	}

};

//abstract syntax tree
//this could probably just be Program and hold a vector of functions that holds a vector of statement, etc.
struct Program {
	ASTType type = AST_Program;

	array<Function> functions;
};




namespace suParser {
	Program parse(array<token>& tokens);
}


#endif