#pragma once
#ifndef SU_TYPES_H
#define SU_TYPES_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/string.h"
#include "ctype.h"

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Compile Options
enum ReturnCode {
	ReturnCode_Success                = 0,
	ReturnCode_No_File_Passed         = 1,
	ReturnCode_File_Not_Found         = 2,
	ReturnCode_File_Locked            = 3,
	ReturnCode_File_Invalid_Extension = 4,
	ReturnCode_Invalid_Argument       = 5,
	ReturnCode_Lexer_Failed           = 6,
	ReturnCode_Preprocessor_Failed    = 7,
	ReturnCode_Parser_Failed          = 8,
	ReturnCode_Assembler_Failed       = 9,
};

enum OSOut {
	OSOut_Windows,
	OSOut_Linux,
	OSOut_OSX,
};

struct {
	u32 warning_level = 1;
	b32 verbose_print      = false;
	b32 supress_warnings   = false;
	b32 supress_messages   = false;
	b32 warnings_as_errors = false;
	OSOut osout = OSOut_Windows;
} globals;

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Nodes
enum NodeType : u32 {
	NodeType_Program,
	NodeType_Structure,
	NodeType_Function,
	NodeType_Variable,
	NodeType_Scope,
	NodeType_Statement,
	NodeType_Expression,
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Registers
enum Registers{
	Register_NULL,
	
	Register0_64,  Register0_32,  Register0_16,  Register0_8,
	Register1_64,  Register1_32,  Register1_16,  Register1_8,
	Register2_64,  Register2_32,  Register2_16,  Register2_8,
	Register3_64,  Register3_32,  Register3_16,  Register3_8,
	Register4_64,  Register4_32,  Register4_16,  Register4_8,
	Register5_64,  Register5_32,  Register5_16,  Register5_8,
	Register6_64,  Register6_32,  Register6_16,  Register6_8,
	Register7_64,  Register7_32,  Register7_16,  Register7_8,
	Register8_64,  Register8_32,  Register8_16,  Register8_8,
	Register9_64,  Register9_32,  Register9_16,  Register9_8,
	Register10_64, Register10_32, Register10_16, Register10_8,
	Register11_64, Register11_32, Register11_16, Register11_8,
	Register12_64, Register12_32, Register12_16, Register12_8,
	Register13_64, Register13_32, Register13_16, Register13_8,
	Register14_64, Register14_32, Register14_16, Register14_8,
	Register15_64, Register15_32, Register15_16, Register15_8,
	
	//x64 names
	Register_RAX = Register0_64,  Register_EAX  = Register0_32,  Register_AX   = Register0_16,  Register_AL   = Register0_8,
	Register_RDX = Register1_64,  Register_EDX  = Register1_32,  Register_DX   = Register1_16,  Register_DL   = Register1_8,
	Register_RCX = Register2_64,  Register_ECX  = Register2_32,  Register_CX   = Register2_16,  Register_CL   = Register2_8,
	Register_RBX = Register3_64,  Register_EBX  = Register3_32,  Register_BX   = Register3_16,  Register_BL   = Register3_8,
	Register_RSI = Register4_64,  Register_ESI  = Register4_32,  Register_SI   = Register4_16,  Register_SIL  = Register4_8,
	Register_RDI = Register5_64,  Register_EDI  = Register5_32,  Register_DI   = Register5_16,  Register_DIL  = Register5_8,
	Register_RSP = Register6_64,  Register_ESP  = Register6_32,  Register_SP   = Register6_16,  Register_SPL  = Register6_8,
	Register_RBP = Register7_64,  Register_EBP  = Register7_32,  Register_BP   = Register7_16,  Register_BPL  = Register7_8,
	Register_R8  = Register8_64,  Register_R8D  = Register8_32,  Register_R8W  = Register8_16,  Register_R8B  = Register8_8,
	Register_R9  = Register9_64,  Register_R9D  = Register9_32,  Register_R9W  = Register9_16,  Register_R9B  = Register9_8,
	Register_R10 = Register10_64, Register_R10D = Register10_32, Register_R10W = Register10_16, Register_R10B = Register10_8,
	Register_R11 = Register11_64, Register_R11D = Register11_32, Register_R11W = Register11_16, Register_R11B = Register11_8,
	Register_R12 = Register12_64, Register_R12D = Register12_32, Register_R12W = Register12_16, Register_R12B = Register12_8,
	Register_R13 = Register13_64, Register_R13D = Register13_32, Register_R13W = Register13_16, Register_R13B = Register13_8,
	Register_R14 = Register14_64, Register_R14D = Register14_32, Register_R14W = Register14_16, Register_R14B = Register14_8,
	Register_R15 = Register15_64, Register_R15D = Register15_32, Register_R15W = Register15_16, Register_R15B = Register15_8,
	
	//usage
	Register_FunctionReturn     = Register_RAX,
	Register_BasePointer        = Register_RBP,
	Register_StackPointer       = Register_RSP,
	Register_FunctionParameter0 = Register_RDI,
	Register_FunctionParameter1 = Register_RSI,
	Register_FunctionParameter2 = Register_RDX,
	Register_FunctionParameter3 = Register_RCX,
	Register_FunctionParameter4 = Register_R8,
	Register_FunctionParameter5 = Register_R9,
};

global const char* registers_x64[] = {
	"%null",
	"%rax", "%eax",  "%ax",   "%al",
	"%rdx", "%edx",  "%dx",   "%dl",
	"%rcx", "%ecx",  "%cx",   "%cl",
	"%rbx", "%ebx",  "%bx",   "%bl",
	"%rsi", "%esi",  "%si",   "%sil",
	"%rdi", "%edi",  "%di",   "%dil",
	"%rsp", "%esp",  "%sp",   "%spl",
	"%rbp", "%ebp",  "%bp",   "%bpl",
	"%r8",  "%r8d",  "%r8w",  "%r8b",
	"%r9",  "%r9d",  "%r9w",  "%r9b",
	"%r10", "%r10d", "%r10w", "%r10b",
	"%r11", "%r11d", "%r11w", "%r11b",
	"%r12", "%r12d", "%r12w", "%r12b",
	"%r13", "%r13d", "%r13w", "%r13b",
	"%r14", "%r14d", "%r14w", "%r14b",
	"%r15", "%r15d", "%r15w", "%r15b",
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Builtin Types
typedef u32 DataType; enum { 
	DataType_NotTyped,
	DataType_Void,       // void
	DataType_Implicit,   // implicitly typed
	DataType_Signed8,    // s8
	DataType_Signed16,   // s16
	DataType_Signed32,   // s32 
	DataType_Signed64,   // s64
	DataType_Unsigned8,  // u8
	DataType_Unsigned16, // u16
	DataType_Unsigned32, // u32 
	DataType_Unsigned64, // u64 
	DataType_Float32,    // f32 
	DataType_Float64,    // f64 
	DataType_String,     // str
	DataType_Pointer,    // type*
	DataType_Any,
	DataType_Structure,  // data type of types and functions
}; 

const char* dataTypeStrs[] = {
	"notype",
	"void",
	"impl",  
	"s8",  
	"s16",
	"s32",  
	"s64",  
	"u8", 
	"u16",
	"u32",
	"u64",
	"f32",   
	"f64",   
	"str",    
	"ptr",   
	"any",
	"struct", 
}; 


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Lexer
enum Token_Type : u32{
	Token_Null = 0,
	Token_ERROR = 0,                // when something doesnt make sense during lexing
	Token_EOF,                      // end of file
	
	Token_Identifier,               // function/variable names                 
	
	//// literal ////
	Token_LiteralFloat,
	Token_LiteralInteger,
	Token_LiteralCharacter,
	Token_LiteralString,
	
	//// control ////
	Token_Semicolon,                // ;
	Token_OpenBrace,                // {
	Token_CloseBrace,               // }
	Token_OpenParen,                // (
	Token_CloseParen,               // )
	Token_OpenSquare,               // [
	Token_CloseSquare,              // ]
	Token_Comma,                    // ,
	Token_QuestionMark,             // ?
	Token_Colon,                    // :
	Token_Dot,                      // .
	Token_At,                       // @
	Token_Pound,                    // #
	Token_Backtick,                 // `
	
	//// operators ////
	Token_Plus,                     // +
	Token_Increment,                // ++
	Token_PlusAssignment,           // +=
	Token_Negation,                 // -
	Token_Decrement,                // --
	Token_NegationAssignment,       // -=
	Token_Multiplication,           // *
	Token_MultiplicationAssignment, // *=
	Token_Division,                 // /
	Token_DivisionAssignment,       // /=
	Token_BitNOT,                   // ~
	Token_BitNOTAssignment,         // ~=
	Token_BitAND,                   // &
	Token_BitANDAssignment,         // &=
	Token_AND,                      // &&
	Token_BitOR,                    // |
	Token_BitORAssignment,          // |=
	Token_OR,                       // ||
	Token_BitXOR,                   // ^
	Token_BitXORAssignment,         // ^=
	Token_BitShiftLeft,             // <<
	Token_BitShiftLeftAssignment,   // <<=
	Token_BitShiftRight,            // >>
	Token_BitShiftRightAssignment,  // >>=
	Token_Modulo,                   // %
	Token_ModuloAssignment,         // %=
	Token_Assignment,               // =
	Token_Equal,                    // ==
	Token_LogicalNOT,               // !
	Token_NotEqual,                 // !=
	Token_LessThan,                 // <
	Token_LessThanOrEqual,          // <=
	Token_GreaterThan,              // >
	Token_GreaterThanOrEqual,       // >=
	
	//// keywords ////
	Token_Return,                   // return
	Token_If,                       // if
	Token_Else,                     // else
	Token_For,                      // for
	Token_While,                    // while 
	Token_Break,                    // break
	Token_Continue,                 // continue
	Token_Defer,                    // defer
	Token_StructDecl,               // struct
	Token_This,                     // this
	
	//// types  ////
	Token_Void,                     // void
	Token_Signed8,                  // s8
	Token_Signed16,                 // s16 
	Token_Signed32,                 // s32 
	Token_Signed64,                 // s64
	Token_Unsigned8,                // u8
	Token_Unsigned16,               // u16
	Token_Unsigned32,               // u32 
	Token_Unsigned64,               // u64 
	Token_Float32,                  // f32 
	Token_Float64,                  // f64 
	Token_String,                   // str
	Token_Any,                      // any
	Token_Struct,                   // user defined type


	//// directives ////
	Token_Directive_Import,
	Token_Directive_Internal,
	Token_Directive_Run,

}; //typedef u32 Token_Type;

#define NAME(code) STRINGIZE(code)
const char* TokenTypes_Names[] = {
	"null/error",
	NAME(Token_EOF),
	NAME(Token_Identifier),
	
	NAME(Token_LiteralFloat),
	NAME(Token_LiteralInteger),
	NAME(Token_LiteralCharacter),
	NAME(Token_LiteralString),
	
	NAME(Token_Semicolon),
	NAME(Token_OpenBrace),
	NAME(Token_CloseBrace),
	NAME(Token_OpenParen),
	NAME(Token_CloseParen),
	NAME(Token_OpenSquare),
	NAME(Token_CloseSquare),
	NAME(Token_Comma),
	NAME(Token_QuestionMark),
	NAME(Token_Colon),
	NAME(Token_Dot),
	NAME(Token_At),
	NAME(Token_Pound),
	NAME(Token_Backtick),
	
	NAME(Token_Plus),
	NAME(Token_Increment),
	NAME(Token_PlusAssignment),
	NAME(Token_Negation),
	NAME(Token_Decrement),
	NAME(Token_NegationAssignment),
	NAME(Token_Multiplication),
	NAME(Token_MultiplicationAssignment),
	NAME(Token_Division),
	NAME(Token_DivisionAssignment),
	NAME(Token_BitNOT),
	NAME(Token_BitNOTAssignment),
	NAME(Token_BitAND),
	NAME(Token_BitANDAssignment),
	NAME(Token_AND),
	NAME(Token_BitOR),
	NAME(Token_BitORAssignment),
	NAME(Token_OR),
	NAME(Token_BitXOR),
	NAME(Token_BitXORAssignment),
	NAME(Token_BitShiftLeft),
	NAME(Token_BitShiftLeftAssignment),
	NAME(Token_BitShiftRight),
	NAME(Token_BitShiftRightAssignment),
	NAME(Token_Modulo),
	NAME(Token_ModuloAssignment),
	NAME(Token_Assignment),
	NAME(Token_Equal),
	NAME(Token_LogicalNOT),
	NAME(Token_NotEqual),
	NAME(Token_LessThan),
	NAME(Token_LessThanOrEqual),
	NAME(Token_GreaterThan),
	NAME(Token_GreaterThanOrEqual),
	
	NAME(Token_Return),
	NAME(Token_If),
	NAME(Token_Else),
	NAME(Token_For),
	NAME(Token_While),
	NAME(Token_Break),
	NAME(Token_Continue),
	NAME(Token_Defer),
	NAME(Token_StructDecl),
	NAME(Token_This),
	
	NAME(Token_Void),
	NAME(Token_Signed8),
	NAME(Token_Signed16),
	NAME(Token_Signed32),
	NAME(Token_Signed64),
	NAME(Token_Unsigned8),
	NAME(Token_Unsigned16),
	NAME(Token_Unsigned32),
	NAME(Token_Unsigned64),
	NAME(Token_Float32),
	NAME(Token_Float64),
	NAME(Token_String),
	NAME(Token_Any),
	NAME(Token_Struct),
};
#undef NAME

enum TokenGroups{
	TokenGroup_NULL,
	TokenGroup_Identifier,
	TokenGroup_Literal,
	TokenGroup_Control,
	TokenGroup_Operator,
	TokenGroup_Keyword,
	TokenGroup_Type,
}; typedef u32 TokenGroup;

struct Token {
	Token_Type  type;
	TokenGroup group;
	str8 raw; 
	
	str8 file;
	u32 l0, l1;
	u32 c0, c1;

	union{
		f64 f64_val;
		s64 s64_val;
		u64 u64_val;
	};
};




//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Abstract Syntax Tree 
enum ExpressionType : u32 {
	Expression_IdentifierLHS,
	Expression_IdentifierRHS,
	
	Expression_Function_Call,
	
	//Special ternary conditional expression type
	Expression_TernaryConditional,
	
	//Types
	Expression_Literal,
	
	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,
	Expression_IncrementPrefix,
	Expression_IncrementPostfix,
	Expression_DecrementPrefix,
	Expression_DecrementPostfix,
	
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
	Expression_BinaryOpMemberAccess,
};

static const char* ExTypeStrings[] = {
	"idLHS: ",
	"idRHS: ",
	
	"fcall: ",
	
	"tern: ",
	
	"literal: ",
	
	"~",
	"!",
	"-",
	"++ pre",
	"++ post",
	"-- pre",
	"-- post",
	
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
	"accessor",
};

struct Struct;
struct Expression {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	cstring expstr;
	ExpressionType type;
	DataType datatype;
	Struct* struct_type;
	union {
		f32 float32;
		f64 float64;
		s8  int8;
		s16 int16;
		s32 int32;
		s64 int64;
		u8  uint8;
		u16 uint16;
		u32 uint32;
		u64 uint64;
		cstring str;
	};
};
#define ExpressionFromNode(x) CastFromMember(Expression, node, x)

enum StatementType : u32 {
	Statement_Unknown,
	Statement_Return,
	Statement_Expression,
	Statement_Declaration,
	Statement_Conditional,
	Statement_Else,
	Statement_For,
	Statement_While,
	Statement_Break,
	Statement_Continue,
	Statement_Struct,
};

struct Statement {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	StatementType type = Statement_Unknown;
};
#define StatementFromNode(x) CastFromMember(Statement, node, x)

struct Scope {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	b32 has_return_statement = false;
};
#define ScopeFromNode(x) CastFromMember(Scope, node, x)

enum{
	decl_function,
	decl_variable,
	decl_structure,
};


struct Declaration {
	TNode node;
	Token* token_start;
	Token* token_end;
	u64 token_idx = 0;
	
	Type type;
	str8 identifier;

};
#define DeclarationFromNode(x) CastFromMember(Declaration, node, x)

struct Function {
	Declaration decl;
	str8 internal_label;
	DataType type;
	u32 positional_args = 0;
	map<cstring, Declaration*> args;
	//TODO do this with a binary tree sort of thing instead later
	array<Function*> overloads;
};
#define FunctionFromDeclaration(x) CastFromMember(Function, decl, x)
#define FunctionFromNode(x) FunctionFromDeclaration(DeclarationFromNode(x))

struct Struct {
	Declaration decl;
	
	map<cstring, Declaration*> members;
};
#define StructFromDeclaration(x) CastFromMember(Struct, decl, x)
#define StructFromNode(x) StructFromDeclaration(DeclarationFromNode(x))

struct Variable{
	Declaration decl;

	Struct* struct_data;
	union {
		f32 float32;
		f64 float64;
		s8  int8;
		s16 int16;
		s32 int32;
		s64 int64;
		u8  uint8;
		u16 uint16;
		u32 uint32;
		u64 uint64;
		str8 str;
	};
};
#define VariableFromDeclaration(x) CastFromMember(Variable, decl, x)
#define VariableFromNode(x) VariableFromDeclaration(DeclarationFromNode(x))

struct Program {
	Node node;
	str8 filename;
	//TODO add entrypoint string
};

enum ParseStage {
	psGlobal,      // <program>       :: = { ( <function> | <struct> ) }
	psStruct,      // <struct>        :: = "struct" <id> "{" { ( <declaration> ";" | <function> ) } "}" [<id>] ";"
	psFunction,    // <function>      :: = <type> ( [ "[" [<integer>] "]" ] | "*" ) <id> "(" [ <declaration> {"," <declaration> } ] ")" <scope>
	psScope,       // <scope>         :: = "{" { (<declaration> | <statement> | <scope>) } "}"
	psDeclaration, // <declaration>   :: = <type> ( [ "[" [<integer>] "]" ] | "*" ) <id> [ = <exp> ]
	psStatement,   // <statement>     :: = "return" <exp> ";" | <exp> ";" | <scope> 
	//                                   | "if" "(" <exp> ")" <statement> | <scope> [ "else" <statement> | <scope> ]
	//                                   | "for" "(" [<exp>] ";" [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "for" "(" <declaration> ";" [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "while" "(" <exp> ")" <statement>
	//                                   | "break" [<integer>] ";" 
	//                                   | "continue" ";"
	//                                   | <struct>
	psExpression,  // <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional, // <conditional>   :: = <logical or> | "if" "(" <exp> ")" <exp> "else" <exp> 
	psLogicalOR,   // <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,  // <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,   // <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,  // <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,  // <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,    // <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,  // <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,    // <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,    // <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,        // <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,      // <factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal> | <id> | <incdec> <id> | <id> <incdec> |  <funccall> | <memberaccess> | "if"
	//                <funccall>      :: = < id> "("[( <exp> | <id> = <exp> ) {"," ( <exp> | <id> = <exp> ) }] ")"
	//                <literal>       :: = <integer> | <float> | <string>
	//                <memberaccess>  :: = <id> "." <id> { "." <id> }
	//                <float>         :: = { <integer> } "." <integer> { <integer> }
	//                <string>        :: = """ { <char> } """
	//                <integer>       :: = (1|2|3|4|5|6|7|8|9|0) 
	//                <char>          :: = you know what chars are
	//                <type>          :: = (u8|u32|u64|s8|s32|s64|f32|f64|str|any)
	//                <incdec>        :: = "++" | "--"
	//                <unary>         :: = "!" | "~" | "-"
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Lexer

struct LexedFile {
	File* file;

	array<Token> tokens;

	struct{
		struct{
			array<u32>   vars;
			array<u32>   funcs;
			array<u32>   structs;
		}glob;
		
		struct{
			array<u32>   vars;
			array<u32>   funcs;
			array<u32>   structs;
		}loc;
	}decl;
	

	struct{
		array<u32> imports;
		array<u32> internals;
		array<u32> runs;
	}preprocessor;
};

struct Lexer {
	LexedFile* lexfile;

	LexedFile* lex(str8 filepath);
}lexer;

map<str8, LexedFile> lexed_files;

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Preprocessor

struct PreprocessedFile{
	LexedFile* lexfile;

	struct{
		struct{
			array<u32> vars;
			array<u32> funcs;
			array<u32> structs;
		}exported;

		struct{
			array<u32> vars;
			array<u32> funcs;
			array<u32> structs;
		}internal;
	}decl;

	array<u32> runs;
	
};

struct Preprocessor {
	PreprocessedFile* prefile;

	PreprocessedFile* preprocess(LexedFile* lexfile);
	//str8 is the name of the file, it is the same as the lexer's map names

}preprocessor;

map<str8, PreprocessedFile> preprocessed_files;	

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Parser

struct ParsedFile{
	PreprocessedFile* prefile;

	struct{
		struct{
			array<Variable*> vars;
			array<Function*> funcs;
			array<Struct*> structs;
		}exported;

		struct{
			array<Variable*> vars;
			array<Function*> funcs;
			array<Struct*> structs;
		}internal;
	}decl;
};

struct Parser {
	ParsedFile* parfile;

	carray<Token> tokens;
	Token* curt;

	//stacks of known things 

	struct{
		array<u32> structs_pushed;
		array<Struct*> structs;
		array<u32> functions_pushed;
		array<Function*> functions;
		array<u32> variables_pushed;
		array<Variable*> variables;
	}stacks;

	struct{
		Arena* functions;
		Arena* variables;
		Arena* structs;
		Arena* scopes;
		Arena* expressions;

		FORCE_INLINE
		Function* make_function(){
			Function* ret = (Function*)functions->cursor;
			functions->cursor += sizeof(Function);
			functions->used += sizeof(Function);
			return ret;
		}

		FORCE_INLINE
		Struct* make_struct(){
			Struct* ret = (Struct*)structs->cursor;
			structs->cursor += sizeof(Struct);
			structs->used += sizeof(Struct);
			return ret;
		}

		FORCE_INLINE
		Variable* make_variable(){
			Variable* ret = (Variable*)variables->cursor;
			variables->cursor += sizeof(Variable);
			variables->used += sizeof(Variable);
			return ret;
		}

		FORCE_INLINE
		Expression* make_expression(){
			Expression* ret = (Expression*)expressions->cursor;
			expressions->cursor += sizeof(Expression);
			expressions->used += sizeof(Expression);
			return ret;
		}

		FORCE_INLINE
		void init(){
			functions   = memory_create_arena(Kilobytes(512));
			variables   = memory_create_arena(Kilobytes(512));
			structs     = memory_create_arena(Kilobytes(512));
			scopes      = memory_create_arena(Kilobytes(512));
			expressions = memory_create_arena(Kilobytes(512));
		}

	}arena;


	TNode* declare(Type type);
	TNode* define(TNode* node, ParseStage stage);
	ParsedFile* parse(PreprocessedFile* prefile);

}parser;

map<str8, ParsedFile> parsed_files;

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Compiler

struct Compiler{
	array<Lexer> lexers;
	array<Preprocessor> preprocessors;
	array<Parser> parsers;

	u32 max_threads = 3;

	void compile(str8 filepath);
}compiler;


#endif //SU_TYPES_H