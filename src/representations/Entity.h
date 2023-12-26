/*
   
   An Entity is anything that a label can point to.

*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "basic/Node.h"
#include "storage/Pool.h"
#include "representations/AST.h"

namespace amu{

struct Structure;
struct Trait;
struct FunctionType;
struct OverloadedFunction;
struct Type;
struct TAC;
struct Label;
struct Code;

struct Entity : public ASTNode {
	enum class Kind {
		Expr,
		Type,
		Func,
		Var,
		Module,
		Member,
	};

    Kind kind;

    Label* label; // the most recent label used to represent this entity, null if it is anonymous
    Code* code;   // the Code object this Entity belongs to 
	ASTNode* def; // the syntax used to define this entity. label cannot be used to retrieve this because it points to the most recent label used for this Entity
 
				  
	// ~~~~ interface ~~~~

	IS_TEMPLATE_DECLS;

	Entity(Kind kind) : ASTNode(ASTNode::Kind::Entity) {}

};

} // namespace amu

#endif // AMU_ENTITY_H
