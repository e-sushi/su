#pragma once
#ifndef SU_PARSER_H
#define SU_PARSER_H

#include "su-lexer.h"
#include "su-types.h"


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
	Expression_BinaryOpBitXOR,
	Expression_BinaryOpBitShiftLeft,
	Expression_BinaryOpBitShiftRight,
	Expression_BinaryOpAssignment,

	//Special ternary conditional expression type
	Expression_TernaryConditional,

	//Expression Guards
	ExpressionGuard_Assignment,
	ExpressionGuard_HEAD, //to align expression guards correctly with their evaluations
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

	"tern cond",

	"assignment",
	"head",
	"conditional",
	"logical or",
	"logical and",
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

	//array<Expression> expressions;

	Node node;

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
	Statement_Conditional,
	Statement_If,
	Statement_Else,
};

struct BlockItem;
struct Statement {
	StatementType type = Statement_Unknown;

	//array<Expression> expressions;
	//array<Statement>  statements;
	//array<BlockItem*> compound;

	Node node;

	Statement() {};

	Statement(StatementType st) {
		type = st;
	}

	Statement(string vid, StatementType st) {
		type = st;
	}
};

struct Declaration {
	Token_Type type;
	string identifier = "";
	b32 initialized = false;
	//array<Expression> expressions;
	Node node;
};

struct BlockItem {
	b32 is_declaration = 0;
	Node node; //NOTE this node is a singleton and is either a statement or a declaration

	//Declaration declaration;
	//Statement statement;
	//Node declaration; //NOTE these nodes are singletons!
	//Node statement; 
};

struct Function {
	string identifier = "";
	DataType type;

	Node node;

	Function() {}

	Function(string identifier) {
		this->identifier = identifier;
	}

};

struct Program {
	//array<Function> functions;
	Node node;
};

namespace suParser {
	b32 parse(array<token>& tokens, Program& mother);
}


#endif //SU_PARSER_H