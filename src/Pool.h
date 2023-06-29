/*

    A thread safe chunked array in which pointers are guaranteed to never move.
    In order to facilitate this, everything allocated in a pool is prefixed with a 
    Pool::Header which enables Pools to remove items and replace them later 

    Chunks are amu::Arenas and are linked by amu::Nodes

*/

#ifndef AMU_POOL_H
#define AMU_POOL_H

#include "kigu/common.h"
#include "Node.h"

namespace amu {

template<typename T>
struct Pool {
    struct Header {
        Node node;
    };

    // how many of 'T' we store per block
    u32 chunks_per_block; 
    // start of 
    Header root_block;
    Header free_blocks;


};

namespace pool {

// initialize a pool that will allocate chunks to store 
// 'n_per_chunk' items of type 'T'
template<typename T> Pool<T>
init(spt n_per_chunk);

// deinitializes a pool
template<typename T> void
deinit(Pool<T>* pool);

} // namespace pool
} // namespace amu

#endif // AMU_POOL_H