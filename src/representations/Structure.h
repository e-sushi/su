/* 

    A Structure represents the way contiguous data of a Type is organized in memory
    A Structure is *not* a Type, certain Types use Structure to determine how its data
    is laid out in memory. When we have something like:
        Apple :: struct {}
    Apple is a StructuredType, which points to the Structure on the right.
    Currently this only applies to StructuredType, if this never gets used anywhere else
    just remove it and put all this information on StructuredType.

    The only way I can see this separation being useful is when we have 2 types with the same structure
    allowing us to reuse Structure for several different Types. This may be particularly useful if we ever
    decide to implement something like mixins, and it could possibly be useful for parameterized Types as well

*/

#ifndef AMU_STRUCTURE_H
#define AMU_STRUCTURE_H

#include "Label.h"
#include "Type.h"

namespace amu {

struct Expr;

const u64 MEMBER_OFFSET_UNKNOWN = -1;

// NOTE(sushi) Members are ordered by their appearance in the AST 
//             next() is the next member in the order they are defined
//             in the structure
struct Member : public Entity {
    Type* type = 0;
    // if this member is inherited into the structure
    b32 inherited = false; 
    // offset in bytes from the start of the structure
    u64 offset = MEMBER_OFFSET_UNKNOWN; 

    static Member*
    create();

    DString*
    display();

    DString*
    dump();

    Member() : Entity(entity::member) {}
};

struct Structure {
    u64 size; // size of this structure in bytes
    Map<String, Member*> members;


    // ~~~~~~ interface ~~~~~~~


    static Structure*
    create();

    void
    destroy();

    Member*
    find_member(String s);

    Member*
    add_member(String id);

    void
    add_member(String id, Member* m);
};

} // namespace amu 

#endif // AMU_STRUCTURE_H