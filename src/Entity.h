/*
    An Entity represents any thing that a label may be attached to and the way a label 
    may be used is dependent upon its entity.
*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "basic/Node.h"
#include "storage/Pool.h"
#include "Label.h"
#include "Type.h"
#include <source_location>

namespace amu{

struct Structure;
struct Trait;
struct FunctionType;

struct Entity {
    TNode node;
    Label* label; // the most recent label used to represent this entity
};

// a TemplateParameter denotes a position in an AST where we need to place a template argument 
// when some parameter is filled in for an Entity
struct TemplateParameter {
    TNode node; // representation of the parameter
    Array<TNode*> points; // places in the Entity's AST where this parameter is replaced
};

struct TemplatedEntity : public Entity {
    Array<TemplateParameter> parameters;
};

// representation of something that has a place in memory, name borrowed from rust
// aka an lvalue
struct Place : public Entity {
    // type information for this place in memory
    Type* type;
};

namespace place {

global Place*
create();

global void
destroy(Place* p);

} // namespace place

struct Member {
    Structure* s;
    b32 inherited;
};

// a Structure represents the way contiguous data of a Type is organized in memory
// a Structure is *not* a Type, certain Types use Structure to determine how its data
// is laid out in memory 
// when we have something like 
//   Apple :: struct {}
// Apple is a StructuredType, which points to the Structure on the right

struct Structure : public Entity {
    u64 size; // size of this structure in bytes
    Map<String, Member> fields;
};

namespace structure {

Structure*
create();

void
destroy(Structure* s);

b32
is_builtin(Structure* s);

b32
can_coerce(Structure* from, Structure* to);

} // namespace structure

struct Function : public Entity {
    FunctionType* type;
    LabelTable table;
};

namespace function {

global Function*
create();

global void
destroy(Function& f);

} // namespace function

struct Module : public Entity {
    Array<spt> labels; 

    LabelTable table;    
};

namespace module {

global Module*
create();

global void
destroy(Module& m);

} // namespace module

struct Trait : public Entity {

};

namespace trait {

}


namespace type {
enum class kind {
    scalar,
    structured,
    pointer,
    array,
    function,
    tuple,
    meta,
};
} // namespace type

struct Type : public Entity {
    type::kind kind;
    // set of traits applied to this Type
    Array<Trait*> traits;
};

namespace type::scalar {
enum class kind {
    void_,
    unsigned8,
    unsigned16,
    unsigned32,
    unsigned64,
    signed8,
    signed16,
    signed32,
    signed64,
    float32,
    float64,
};
} // namespace type::builtin

// a numerical type
struct ScalarType : public Type {
    type::scalar::kind kind;
    ScalarType(type::scalar::kind kind) {
        Type::kind = type::kind::scalar;
        this->kind = kind;
    }
};

namespace type::scalar {
global ScalarType void_      = {type::scalar::kind::void_};
global ScalarType unsigned8  = {type::scalar::kind::unsigned8};
global ScalarType unsigned16 = {type::scalar::kind::unsigned16};
global ScalarType unsigned32 = {type::scalar::kind::unsigned32};
global ScalarType unsigned64 = {type::scalar::kind::unsigned64};
global ScalarType signed8    = {type::scalar::kind::signed8};
global ScalarType signed16   = {type::scalar::kind::signed16};
global ScalarType signed32   = {type::scalar::kind::signed32};
global ScalarType signed64   = {type::scalar::kind::signed64};
global ScalarType float32    = {type::scalar::kind::float32};
global ScalarType float64    = {type::scalar::kind::float64};
} // namespace type::scalar

// a Type which is defined by a user-defined structure
struct StructuredType : public Type {
    Structure* structure;
};

namespace type::structured {
struct ExistingStructureType {
    Structure* structure;
    StructuredType* stype;
};

extern Array<ExistingStructureType> set;

StructuredType*
create(Structure* structure);
} // namespace type::structured

struct PointerType : public Type {
    Type* type;
};

namespace type::pointer {
struct ExistantPointer {
    Type* type;
    PointerType* ptype;
};
extern Array<ExistantPointer> set;

PointerType*
create(Type* type);
} // namespace type::pointer

struct ArrayType : public Type {
    Type* type;
    u64 size;
};

namespace type::array {
struct ExistantArray {
    u64 hash;
    ArrayType* atype;
};
extern Array<ExistantArray> set;

ArrayType*
create(Type* type, u64 size);
} // namespace type::array

// a Type which may take on the form of some collection of Types
// this is a tagged union
struct VariantType : public Type {
    Array<Type*> variants;
};

namespace type::variant {
VariantType*
create();
} // namespace type::variant

struct FunctionType : public Type {
    // pointers to the nodes that define these things
    TNode* parameters;
    TNode* returns;
    Type* return_type;
};

namespace type::function {
FunctionType*
create();
} // namespace type::function

// a tuple acting as a type, eg. one that only contains references to types and not values
// while this represents the types of the tuple, the names given to each element and other syntactic
// information is handled by the Entity representation of the tuple.
struct TupleType : public Type {
    Array<Type*> types;
};

namespace type::tuple {
struct ExistantTupleType {
    u64 hash;
    TupleType* ttype;
};
extern Array<ExistantTupleType> set;

// creates a Tuple type from an array of Type*
// this takes ownership of the types array
// if the TupleType already exists, the array is deinitialized
TupleType*
create(Array<Type*>& types);
} // namespace type::tuple

namespace type::meta {
enum class kind {
    place,
    structure,
    module,
    function,
    trait,
};
} // namespace type::meta

// a Type which represents information about the language
// planned to be mapped 1:1 with the types we use in the compiler
struct MetaType : public Type {
    type::meta::kind kind;
    Structure* s;
};

namespace type {

extern Map<Type*, Type*> type_map;

Type*
create();

void
destroy(Type* t);

Type*
base(Type* t);

b32
can_coerce(Type* to, Type* from);

String
name(Type* type);

u64
hash(Type* type);

b32
has_trait(String name);

b32
has_trait(Trait* trait);

b32
is_scalar(Type* type) { return type->kind == type::kind::scalar; }

} // namespace type

void
to_string(DString& start, Type* t);

namespace entity {

// retrieves the label that was originally used when declaring this entity
global Label*
declared_label(Function* f);

global Label*
declared_label(Structure* f);

global Label*
declared_label(Module* f);

global Label*
declared_label(Place* f);

String
get_name(Entity* e);

} // namespace entity

void
to_string(DString& start, Place* p);

DString
to_string(Place* p);

void
to_string(DString& start, Function* p);

void
to_string(DString& start, Module* p);

void
to_string(DString& start, Structure* p);

} // namespace amu

#endif // AMU_ENTITY_H