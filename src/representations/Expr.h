/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#ifndef AMu_EXPRESSION_H
#define AMu_EXPRESSION_H

#include "basic/Node.h"
#include "Type.h"
#include "Label.h"
#include "Entity.h"
#include "Frame.h"

namespace amu {

struct Token;
struct Entity;
struct Type;
struct Function;
struct Tuple;
struct Allocator;

struct Expr : public Entity {
	enum class Kind {
		Null,

		Identifier,
		LiteralScalar,
		LiteralString,
		LiteralArray,
		LiteralTuple,
		LiteralStruct,
		Function,
		Type,
		VarRef,
		Module,
		Call,

		Block,
		Loop,
		For,
		Switch,
		SwitchCase,
		Conditional,
		Return,
		Using,
		Break,
		Subscript,

		BitwiseComplement,
		Not,
		Negate,
		Reference,
		Dereference,
		AssignmentUnary,
		CompileTimeUnary,

		Plus,
		Minus,
		Multiply,
		Division,
		And,
		BitwiseAnd,
		Or,
		BitwiseOr,
		LessThan,
		GreaterThan,
		LessThanOrEqual,
		GreaterThanOrEqual,
		Equal,
		NotEqual,
		Modulo,
		XOR,
		ShiftLeft,
		ShiftRight,
		Access,
		AssignmentBinary,
		CompileTimeBinary,
		Range,

		Cast,
		Reinterpret,

		VMBreak,

		IntrinsicRandInt,
	};

	Kind kind;

    Type* type; // the semantic type of this expression

    // when true, this expression represents a location in memory 
    // and operations performed on it should directly affect it 
    b32 lvalue;
	
	// set true on expressions in Sema that can be considered 
	// computable at compile time
	b32 compile_time;

	union {
		Var* varref; 
		Module* moduleref; 
    	Member* member; 
		Function* function;
	};


    // ~~~~~~ interface ~~~~~~~

	static Expr* create(Allocator* allocator);
    static Expr* create(Allocator* allocator, Kind kind, Type* type = 0);

    void destroy();

    DString display();
    DString dump();

    Type* resolve_type();

    Expr() : kind(Kind::Null), Entity(Entity::Kind::Expr) {}
    Expr(Kind k) : kind(k), Entity(Entity::Kind::Expr) {def = this;}

	IS_TEMPLATE_DECLS;
};

IS_TEMPLATE_DEF(Entity, Expr, Entity::Kind::Expr);

// representation of a single expression meant to be evaluated at compile time
// this is primarily so that we can keep track of frame information that an 
// Expr may need, which is just local variables for now 
// the original expression is the first child, the evaluated AST is the last
struct CompileTime : public Expr {
    // frame information for this compile time expression
    Frame frame;


    // ~~~~~~ interface ~~~~~~~


    static CompileTime*
    create(Type* type = 0);

    void
    destroy();

    DString
    display();

    DString
    dump();

    CompileTime() : Expr(Expr::Kind::CompileTimeUnary) {}
};

IS_TEMPLATE_DEF(Expr, CompileTime, Expr::Kind::CompileTimeUnary);

struct ScalarLiteral : public Expr {
	using Kind = ScalarValue::Kind;

    ScalarValue value;


    // ~~~~ interface ~~~~


    static ScalarLiteral*
    create();

    void
    destroy();

    DString
    display();

    DString
    dump();

    // NOTE(sushi) it is IMPORTANT!!! that you call this and NOT the function
    //             on 'value' because otherwise the type of this expression will
    //             not change! 
    //             this KINDA SUCKS but whatever
    void
    cast_to(Kind k);

    void
    cast_to(Type* t);

    // NOTE(sushi) this returns if the scalar is of signed INTEGER type
    b32 
    is_signed();

    b32
    is_float();

    b32
    is_negative();

    ScalarLiteral() : Expr(Expr::Kind::LiteralScalar) {}
};

IS_TEMPLATE_DEF(Expr, ScalarLiteral, Expr::Kind::LiteralScalar);

struct Block : public Expr {
    LabelTable* table;


    // ~~~~~~ interface ~~~~~~~


    static Block*
    create();

    void
    destroy();

    DString
    display();

    DString
    dump();

    Block() : Expr(Expr::Kind::Block) {}
};

IS_TEMPLATE_DEF(Expr, Block, Expr::Kind::Block);

// for loops hold a table
struct For : public Expr {
    LabelTable* table;


    // ~~~~~~ interface ~~~~~~~


    static For*
    create();

    void
    destroy();

    DString
    display();

    DString
    dump();

    For() : Expr(Expr::Kind::For) {} 
};

IS_TEMPLATE_DEF(Expr, For, Expr::Kind::For);

struct IntegerRange : public Expr {
    s64 left;
    s64 right;
};

} // namespace amu

#endif // AMu_EXPRESSION_H
