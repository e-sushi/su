/*
    Representation of a label in amu
    
    A label is a handle to any entity in amu

*/

#ifndef AMU_LABEL_H
#define AMU_LABEL_H

#include "basic/Node.h"
#include "storage/String.h"

namespace amu {

struct Token;
struct LabelTable;

struct Label : public ASTNode {
	Code* code;
    Entity* entity;

    // if this label is an alias of another label, this is the original
    Label* aliased; 

    b32 is_virtual;
	
	// points to the LabelTable this Label belongs to
	LabelTable* table;


    // ~~~~~~ interface ~~~~~~~


    static Label*
    create();

    Label*
    base();

    DString*
    display();

    DString*
    dump();

    // returns the type of the Label's expression
    Type*
    resolve_type();

    Label() : ASTNode(ast::label), is_virtual(false) {}
    Label(b32 is_virt) : ASTNode(ast::label), is_virtual(is_virt) {}
};

template<> b32 inline Base::
is<Label>() { return is<ASTNode>() && as<ASTNode>()->kind == ast::label; }

struct LabelTable {
    LabelTable* last;
    Map<String, Label*> map;
	

	// ~~~~ interface ~~~~
	
	
	static LabelTable*
	create();

	void
	add(String s, Label* l);

	Label*
	search(u64 hashed_id);
};

// a Label created internally 
struct VirtualLabel : public Label {
    DString* id;
	Token virtual_token;


    // ~~~~ interface ~~~~~


    // NOTE(sushi) this takes ownership of 'display', so it does not increment its ref count
    static VirtualLabel*
    create(DString* display);

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    VirtualLabel() : Label(true) {} 
};

global void
to_string(DString* start, Label* l);

DString*
to_string(Label* l) {
    DString* out = DString::create();
    to_string(out, l);
    return out;
}

} //namespace amu

#endif // AMU_LABEL_H
