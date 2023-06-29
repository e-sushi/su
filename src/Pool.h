/*

    A thread safe chunked array in which pointers are guaranteed to never move.
    In order to facilitate this, everything allocated in a pool is prefixed with a 
    Pool::Header which enables Pools to remove items and replace them later 

    Chunks are amu::Arenas and are linked by amu::Nodes

    TODOs
        We can puts stats on this structure and guard it behind BUILD_DEBUG to make
        debugging a little easier

*/

#ifndef AMU_POOL_H
#define AMU_POOL_H

#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/array.h"
#include "Node.h"

namespace deshi {
#   include "core/threading.h"
}

namespace amu {

struct PoolHeader {
    Node node;
};

template<typename T>
struct Pool {
    deshi::mutex lock;

    // how many of 'T' we store per chunk
    u32 items_per_chunk; 
    // root of free blocks
    LNode free_blocks;
    // root of chunks
    LNode chunk_root;
};

namespace pool {

// initialize a pool that will allocate chunks to store 
// 'n_per_chunk' items of type 'T'
template<typename T> Pool<T>
init(spt n_per_chunk);

// deinitializes a pool
template<typename T> void
deinit(Pool<T>* pool);

// adds an item to the pool and returns a pointer to it
// allocates a new chunk if necessary
template<typename T> T*
add(Pool<T>* pool);

template<typename T> void
remove(Pool<T>* pool, T* ptr);

} // namespace pool
} // namespace amu

#endif // AMU_POOL_H