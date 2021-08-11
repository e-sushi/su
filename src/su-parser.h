#pragma once
#ifndef SU_PARSER_H
#define SU_PARSER_H

#include "su-lexer.h"


	//////////////////////////////////////
	//// Abstract Syntax Tree Structs ////
	//////////////////////////////////////



enum ExpressionType : u32 {
	Expression_IdentifierLHS,
	Expression_IdentifierRHS,

	//Types
	Expression_IntegerLiteral,

	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,

	//Binary Operators
	Expression_BinaryOpPlus,
	Expression_BinaryOpMinus,
	Expression_BinaryOpMultiply,
	Expression_BinaryOpDivision,
	Expression_BinaryOpAND,
	Expression_BinaryOpBitAND,
	Expression_BinaryOpOR,
	Expression_BinaryOpBitOR,
	Expression_BinaryOpLessThan,
	Expression_BinaryOpGreaterThan,
	Expression_BinaryOpLessThanOrEqual,
	Expression_BinaryOpGreaterThanOrEqual,
	Expression_BinaryOpEqual,
	Expression_BinaryOpNotEqual,
	Expression_BinaryOpModulo,
	Expression_BinaryOpXOR,
	Expression_BinaryOpBitShiftLeft,
	Expression_BinaryOpBitShiftRight,
	Expression_BinaryOpAssignment,

	//Expression Guards
	ExpressionGuard_Assignment,
	ExpressionGuard_Conditional,
	ExpressionGuard_LogicalOR,
	ExpressionGuard_LogicalAND,
	ExpressionGuard_BitOR,
	ExpressionGuard_BitXOR,
	ExpressionGuard_BitAND,
	ExpressionGuard_Equality,
	ExpressionGuard_Relational,
	ExpressionGuard_BitShift,
	ExpressionGuard_Additive,
	ExpressionGuard_Term,
	ExpressionGuard_Factor,
};

static const char* ExTypeStrings[] = {
	"IdentifierLHS",
	"IdentifierRHS",

	"IntegerLiteral",

	"~",
	"!",
	"-",

	"+",
	"-",
	"*",
	"/",
	"&&",
	"&",
	"||",
	"|",
	"<",
	">",
	"<=",
	">=",
	"==",
	"!=",
	"%",
	"^",
	"<<",
	">>",
	"=",

	"assignment",
	"logical and",
	"logical or",
	"bit or",
	"bit xor",
	"bit and",
	"equality",
	"relational",
	"bit shift",
	"additive",
	"term",
	"factor",
};

struct Expression {
	string expstr;
	ExpressionType type;

	array<Expression> expressions;

	Expression(string expstr, ExpressionType type) {
		this->type = type;
		this->expstr = expstr;
	}
};

enum StatementType : u32 {
	Statement_Unknown,
	Statement_Return,
	Statement_Expression,
	Statement_Declaration,
	Statement_IfConditional,
	Statement_ElseConditional,
	Statement_ConditionalStatement,

};

struct Statement {
	StatementType type = Statement_Unknown;


	array<Expression> expressions;
	array<Statement> statements;

	Statement() {};

	Statement(StatementType st) {
		type = st;
	}

	Statement(string vid, StatementType st) {
		type = st;
	}
};



enum DeclType : u32 {
	Decl_Int,
	Decl_Float,
	Decl_Double,
	Decl_User_Defined
};

struct Declaration {
	DeclType type;
	string identifier = "";
	bool initialized = false;
	array<Expression> expressions;
};

struct BlockItem {
	bool is_declaration = 0;

	Declaration* declaration;
	Statement* statement;
};

enum FuncType : u32 {
	INT,
	FLOAT,
	DOUBLE
};

struct Function {
	FuncType type;
	string identifier = "";

	array<BlockItem> blockitems;

	Function() {}

	Function(string identifier) {
		this->identifier = identifier;
	}

};

struct Program {
	array<Function> functions;
};


namespace suParser {
	void parse(array<token>& tokens, Program& mother);
}


#endif