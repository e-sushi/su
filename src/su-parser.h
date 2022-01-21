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
	"idLHS",
	"idRHS",
	
	"literal",
	
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
	Node node;
};
#define ExpressionFromNode(node_ptr) ((Expression*)((u8*)(node_ptr) - OffsetOfMember(Expression,node)))

enum StatementType : u32 {
	Statement_Unknown,
	Statement_Return,
	Statement_Expression,
	Statement_Declaration,
	Statement_Conditional,
	Statement_If,
	Statement_Else,
	Statement_Scope,
};

struct Statement {
	StatementType type = Statement_Unknown;
	Node node;
};
#define StatementFromNode(node_ptr) ((Statement*)((u8*)(node_ptr) - OffsetOfMember(Statement,node)))

struct Declaration {
	Token_Type type;
	string identifier = "";
	b32 initialized = false;
	Node node;
};
#define DeclarationFromNode(node_ptr) ((Declaration*)((u8*)(node_ptr) - OffsetOfMember(Declaration,node)))

//probably doesnt need to be a struct
struct Scope {
	Node node;
};
#define ScopeFromNode(node_ptr) ((Scope*)((u8*)(node_ptr) - OffsetOfMember(Scope,node)))

struct Function {
	string identifier = "";
	DataType type;
	Node node;
};
#define FunctionFromNode(node_ptr) ((Function*)((u8*)(node_ptr) - OffsetOfMember(Function,node)))

struct Program {
	Node node;
};

namespace suParser {
	b32 parse(array<token>& tokens, Program& mother);
}

#endif //SU_PARSER_H