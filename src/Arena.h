/*
    Thread safe memory region 

    This does no internal management of memory, it is solely for allocating some space of memory
    to read and write from in a thread safe way. It doesn't care what kind of memory is in it or how
    it is used.

    Only one thread my write to an Arena at any time, and while writing is occuring, no other thread
    may read from it.

    TODOs
        This needs to support any amount of threads reading at one time.
*/

#ifndef AMU_ARENA_H
#define AMU_ARENA_H

#include "kigu/common.h"

namespace deshi{
    #include "core/threading.h"
}

namespace amu {

struct Arena {
    void* data;
    u64  space;
    deshi::mutex write_lock;
    deshi::mutex read_lock;
};

namespace arena {

// initializes an Arena with some initial space in bytes
global Arena
init(upt space);

// deinitializes an Arena, freeing all of its memory
global void 
deinit(Arena* arena);

// reallocates an arena if necessary
// this may move memory
global void
reallocate(Arena* arena);

global void*
read(Arena* arena, void* addr);

global upt
write(Arena* arena, void* data, upt bytes);

} // namespace arena
} // namespace amu

#endif // AMU_ARENA_H