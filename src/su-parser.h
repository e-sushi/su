#pragma once
#ifndef SU_PARSER_H
#define SU_PARSER_H

#include "utils/utility.h"
#include "su-lexer.h"

enum ASTType {
	AST_Program,
	AST_Function,
	AST_Statement,
	AST_Constant
};

enum FuncType {
	INT,
	FLOAT,
	DOUBLE
};

//abstract syntax tree
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

};

struct Statement : public AST {



};

namespace suParser {
	AST parse(array<token>& tokens);
}


#endif