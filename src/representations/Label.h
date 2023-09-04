/*
    Representation of a label in amu
    
    A label is a handle to any entity in amu

*/

#ifndef AMU_LABEL_H
#define AMU_LABEL_H

#include "basic/Node.h"
#include "storage/String.h"

namespace amu{

struct Token;
struct Label : public ASTNode {
    Entity* entity;

    // if this label is an alias of another label, this is the original
    Label* aliased; 

    b32 is_virtual;


    // ~~~~~~ interface ~~~~~~~


    static Label*
    create();

    Label*
    base();

    String
    name();

    DString
    debug_str();

    // returns the type of the Label's expression
    Type*
    resolve_type();

    Label() : ASTNode(ast::label), is_virtual(false) {}
    Label(b32 is_virt) : ASTNode(ast::label), is_virtual(is_virt) {}
};

template<> b32 inline ASTNode::
is<Label>() { return kind == ast::label; }

struct LabelTable {
    LabelTable* last;
    Map<String, Label*> map;
    ASTNode* owner; // temp debug so I can figure out who these tables belong to 
};

// a Label created internally 
struct VirtualLabel : public Label {
    DString id;


    // ~~~~~~ interface ~~~~~~~


    static VirtualLabel*
    create(DString name);

    void
    destroy();

    String
    name();

    DString
    debug_str();

    VirtualLabel() : Label(true) {} 
};

namespace label {


// TODO(sushi) this same idea can be used for all the other things in amu,
//             i just dont want to spend time on setting all of that up right now 
//             plus I think there could be a better way to do it 
struct Formatting {
    // dont show the original label when printing an alias
    b32 no_aka;
    // if aka is enabled, print the entire chain of aliases
    b32 full_aka;
    u32 col;
    String prefix, suffix;
};

// returns a formatted string representing the given Label
void
display(DString& current, Label* l, Formatting format = Formatting(), b32 allow_color = true);

DString
display(Label* l, Formatting format = Formatting(), b32 allow_color = true) {
    DString out = dstring::init();
    display(out, l, format);
    return out;
}

namespace table {

LabelTable
init(ASTNode* creator);

FORCE_INLINE void
add(LabelTable* table, String id, Label* l);

Label*
search(LabelTable* table, u64 hashed_id);

} // namespace table

} // namespace label

global void
to_string(DString& start, Label* l);

DString
to_string(Label* l) {
    DString out = dstring::init();
    to_string(out, l);
    return out;
}

} //namespace amu

#endif // AMU_LABEL_H