/*
    Contiguous thread safe array.

    In order to keep this array thread safe, reading must also be done through the interface.
    Well, it doesn't HAVE to be (because we do not use private variables) but you need to 
    to guarantee thread safety.

    This uses shared_mutex, which allows an arbitrary amount of reading threads, but only allows
    one thread to access any data when writing is occuring.
*/

#include "kigu/common.h"

namespace amu {

template<typename T>
struct Array {
    T* data;
    spt count;
    spt space;

    shared_mutex lock;
};

namespace array {

// initializes an array with an initial space
template<typename T> Array<T>
init(u32 initial_space = 4);

// deinitializes an array
template<typename T> Array<T>
deinit(Array<T>& arr);

// pushes an item to the end of the array and 
// return a pointer to it, growing if needed
template<typename T> T* 
push(Array<T>& arr);

// pushes an item to the end of the array and
// sets its value to 'val', growing if needed
template<typename T> void
push(Array<T>& arr, T& val);

// pops 'count' items from the end of the array
// and returns the last item popped's value
template<typename T> T
pop(Array<T>& arr, u32 count = 1);

// removes the item at idx
// if the array is 'unordered', the item at
// the end of the array is moved to the given 'idx'
// but if it is ordered, then all items to the left of 
// 'idx' are moved left
template<typename T> void
remove(Array<T>& arr, u32 idx, b32 unordered = false);

// sets all items in an array to 0
// does not affect space
template<typename T> void
clear(Array<T>& arr);

// sets the count of the array and zero inits any new items
template<typename T> void
resize(Array<T>& arr, u32 count);

// allocates a new amount of space for items
// but does not affect the count of items
template<typename T> void
reserve(Array<T>& arr, u32 count);

// sets the item at 'idx' to 'val'
template<typename T> void
set(Array<T>& arr, spt idx, T val);

// makes a copy of the item at 'idx' and returns it 
// supports negative indexing
template<typename T> T
read(Array<T>& arr, spt idx);

// returns a pointer to the item at 'idx' in the array
// supports negative indexing
// the item can move with the entire array, so this can become invalid
// at any time!
template<typename T> T*
readptr(Array<T>& arr, spt idx);

// lock the entire array such that only the calling thread
// may manipulate it until unlock is called
template<typename T> void
lock(Array<T>& arr);

// release a previous lock on the array
template<typename T> void
unlock(Array<T>& arr);

}

}