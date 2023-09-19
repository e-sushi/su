/*

    Representation of a range in amu, which is generally formed with the syntax
        <expr> .. <expr>

    A range forms a generator that iterates between two values of some type.

*/

#ifndef AMU_RANGE_H
#define AMU_RANGE_H

namespace amu {

struct Range : public Expr {
    DString*
    display();

    DString*
    dump();
};


} // namespace amu 

#endif // AMU_RANGE_H