/*

    A thread safe chunked array in which pointers are guaranteed to never move.

    A pool consists of linked chunks which store some fixed amount of items. These
    chunks are prefixed by a header (just an LNode), and every item in the chunk
    is also prefixed by a header (LNode). 

    This only cares about being thread safe on writing, if you want to iterate the 
    data use amu::pool::iterator(), which gives an iterator that locks the pool
    when it is iterating it. But keep in mind that it is only locked when calling
    next or prev, the pool can still change between those calls.

    Beyond this, you can manually lock the entire structure if you need to.

    TODOs
        We can puts stats on this structure and guard it behind BUILD_DEBUG to make
        debugging a little easier

*/

#ifndef AMU_POOL_H
#define AMU_POOL_H

#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/array.h"
#include "basic/Node.h"

namespace amu {

template<typename T>
struct Pool {
    mutex lock;

    // how many of 'T' we store per chunk
    u32 items_per_chunk; 
    // root of free blocks
    LNode free_blocks;
    // root of chunks
    LNode chunk_root;
    // root of allocated items
    // this isn't guaranteed to be in any order
    // the order likely depends on the sequence of additions and removals of items
    LNode items; 
};

namespace pool {

// initialize a pool that will allocate chunks to store 
// 'n_per_chunk' items of type 'T'
template<typename T> Pool<T>
init(spt n_per_chunk);

// deinitializes a pool
template<typename T> void
deinit(Pool<T>& pool);

// adds an item to the pool and returns a pointer to it
// allocates a new chunk if necessary
template<typename T> T*
add(Pool<T>& pool);

template<typename T> T*
add(Pool<T>& pool, const T& val);

template<typename T> void
remove(Pool<T>& pool, T* ptr);

template<typename T>
struct Iterator {
    Pool<T>* pool;
    LNode* current;
};

// returns an Iterator representing the given Pool
template<typename T> Iterator<T>
iterator(Pool<T>& pool);

// returns the current item of the Iterator
// and then increments it
// if we are at the end of the iteration, 0 is returned
template<typename T>  T*
next(Iterator<T>* iter);

// returns the current item of the Iterator
// and the decrements it
// if we are at the beginning of the iteration, 0 is returned
template<typename T> T*
prev(Iterator<T>* iter);

} // namespace pool
} // namespace amu

#endif // AMU_POOL_H