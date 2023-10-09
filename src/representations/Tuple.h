/*

    Representation of any contiguous grouping of things in amu.
    For example, 
    
    label groups
        a,b,c := func();
        ~~~~~Tuple

    function parameters
        func :: (a:u32, b:u32) -> u32;
                 ~~~~~~~~~~~~Tuple

    function call arguments
        func(1, 2);
             ~~~~Tuple

    function multi return
        func :: () -> u32, s32, u8[..];
                      ~~~~~~~~~~~~~~~~Tuple

    amu's builtin Tuple type
        thing : (u32, b32, u8[..]);
                 ~~~~~~~~~~~~~~~~Tuple

    The elements of a Tuple are its children in the AST. 

	Tuples are used extensively and are probably the most flexible part of amu. Pretty much any 
	contiguous grouping of things that is not already grouped by something else is represented 
	by a Tuple. A lot of stuff is generically parsed as a tuple and is semantically analyzed later.

	Tuple elements can take labels in any order but whether or not the Tuple is valid depends on the 
	context in which it is used. For example:
		func :: (u32, a: s32, 1) -> void {...}
	is a specialization of 'func' where the first and third parameters match u32 and 1 exactly, and 
	the second matches any s32. But here:
		thing: vec2 = (y: 1, 2);
	The tuple is invalid, because we don't allow unlabeled elements after labeled elements when using 
	a tuple as a struct initializer, because it becomes confusing when trying to fill out the struct.
	The same thing applies to function calls. 

	TODO

	A big problem with Tuples at the moment is that they need to store a LabelTable. 


*/

#ifndef AMU_TUPLE_H
#define AMU_TUPLE_H

#include "representations/AST.h"
#include "Label.h"

namespace amu {

struct TupleType;

namespace tuple{
// @genstrings(data/tuple_strings.generated)
enum kind : u32 {
    unknown,
    label_group,
    parameters,
    arguments,
    multireturn,
    builtin,
};

#include "data/tuple_strings.generated"

} // namespace tuple

struct Tuple : public ASTNode {
    tuple::kind kind;
    // if this tuple applies names to its elements, they are stored here
    LabelTable* table;
    // when this is a valued Tuple, this points to the underlying type
    TupleType* type;


	// ~~~~~~ interface ~~~~~~~


    static Tuple*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    Tuple() : ASTNode(ast::tuple) {}
};

template<> inline b32 Base::
is<Tuple>() { return is<ASTNode>() && as<ASTNode>()->kind == ast::tuple; }

void
to_string(DString* start, Tuple* t);

} // namespace amu

#endif // AMU_TUPLE_H
