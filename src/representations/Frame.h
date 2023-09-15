/* 

    A Frame is anything that needs to keep track of an instruction and frame pointer
    and a list of local variables so we can display them in debug. This is currently used for
    functions and evaluating isolated expressions.

    Frames always end with a ret instruction.
 
*/

#ifndef AMU_FRAME_H
#define AMU_FRAME_H

namespace amu {

struct BC;

struct Frame {
    BC* ip;
    u8* fp;
    DString* identifier;
    u64 return_size;
    Array<Var*> locals;
}; 

} // namespace amu

#endif // AMU_FRAME_H