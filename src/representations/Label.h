/*
    Representation of a label in amu
    
    A label is a handle to any entity in amu

*/

#ifndef AMU_LABEL_H
#define AMU_LABEL_H

#include "AST.h"
#include "storage/Map.h"

namespace amu {

struct Token;
struct LabelTable;
struct Code;
struct Entity;
struct Allocator;

// @amuobject(Label, ASTNode)
struct Label : public ASTNode {
	Code* code;
    Entity* entity;

    // if this label is an alias of another label, this is the original
    Label* aliased; 

	// points to the LabelTable this Label belongs to
	LabelTable* table;


    // ~~~~~~ interface ~~~~~~~


    static Label* create(Allocator* allocator);

    Label* base();

    DString display();

    DString dump();

    // returns the type of the Label's expression
    Type* resolve_type();

    Label() : ASTNode(ASTNode::Kind::Label) {}
    Label(b32 is_virt) : ASTNode(ASTNode::Kind::Label) {}
};

struct LabelTable {
    LabelTable* last;
    Map<String, Label*> map;
	

	// ~~~~ interface ~~~~
	
	
	static LabelTable* create(Allocator* allocator);

	void add(String s, Label* l);

	Label* search(u64 hashed_id);
};

void to_string(DString& start, Label& l);

} //namespace amu

#endif // AMU_LABEL_H
