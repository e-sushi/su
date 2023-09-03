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

struct Structure {
    u64 size; // size of this structure in bytes
    LabelTable table;


    // ~~~~~~ interface ~~~~~~~


    static Structure*
    create();

    void
    destroy();
};

void
to_string(DString& start, Structure* s);

DString
to_string(Structure* s) {
    DString out = dstring::init();
    to_string(out, s);
    return out;
}

} // namespace amu 

#endif // AMU_STRUCTURE_H