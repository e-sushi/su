/*
    An Entity represents any thing that a label may be attached to and the way a label 
    may be used is dependent upon its entity.
*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "basic/Node.h"
#include "storage/Pool.h"

namespace amu{

struct Structure;
struct Trait;
struct FunctionType;
struct OverloadedFunction;
struct Type;
struct TAC;
struct Label;

namespace entity {
// @genstrings(data/entity_strings.generated)
enum kind {
    expr,
    type,
    func,
    var,
    module,
    member,
};
#include "data/entity_strings.generated"
} // namespace entity

struct Entity : public ASTNode {
    entity::kind kind;
    Label* label; // the most recent label used to represent this entity, null if it is anonymous
    Code* code; // the Code object this Entity belongs to 


    Entity(entity::kind k) : kind(k), ASTNode(ast::entity) {}
};

template<> inline b32 ASTNode::
is<Entity>() { return kind == ast::entity; }

template<> inline b32 ASTNode::
is(entity::kind k) { return is<Entity>() && as<Entity>()->kind == k; }

template<> inline b32 ASTNode::
next_is<Entity>() { return next() && next()->is<Entity>(); }

// a TemplateParameter denotes a position in an AST where we need to place a template argument 
// when some parameter is filled in for an Entity
struct TemplateParameter {
    TNode node; // representation of the parameter
    Array<TNode*> points; // places in the Entity's AST where this parameter is replaced
};

struct TemplatedEntity : public Entity {
    Array<TemplateParameter> parameters;
};

struct Trait : public Entity {

};

namespace trait {

}

void
to_string(DString& start, Type* t);

namespace entity {

String
get_name(Entity* e);

} // namespace entity

} // namespace amu

#endif // AMU_ENTITY_H