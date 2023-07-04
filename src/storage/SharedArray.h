/*
    Contiguous thread safe array.

    In order to keep this array thread safe, reading must also be done through the interface.
    Well, it doesn't HAVE to be (because we do not use private variables) but you need to 
    to guarantee thread safety.

    This uses shared_mutex, which allows an arbitrary amount of reading threads, but only allows
    one thread to access any data when writing is occuring.

    These arrays do NOT implicitly copy, to make a copy of a SharedArray you must call shared_array::copy

    !!!!!!! IMPORTANT !!!!!!!!
    if you plan to add functionality to this structure, you CANNOT allow recursive locks to happen
    since shared_mutex does not support it.
*/

#ifndef AMU_SHARED_ARRAY_H
#define AMU_SHARED_ARRAY_H

#include "kigu/common.h"

namespace amu {

template<typename T>
struct SharedArray {
    T* data;
    spt count;
    spt space;

    shared_mutex lock;
    b32 writer_lock;
};

namespace shared_array {

// initializes an array with an initial space
template<typename T> SharedArray<T>
init(u32 initial_space = 4);

// deinitializes an array
template<typename T> void
deinit(SharedArray<T>& arr);

// pushes an item to the end of the array and 
// return a pointer to it, growing if needed
template<typename T> T* 
push(SharedArray<T>& arr);

// pushes an item to the end of the array and
// sets its value to 'val', growing if needed
template<typename T> void
push(SharedArray<T>& arr, const T& val);

// pops 'count' items from the end of the array
// and returns the last item popped's value
template<typename T> T
pop(SharedArray<T>& arr, u32 count = 1);

// inserts a new item at 'idx' and returns a pointer to it
template<typename T> T*
insert(SharedArray<T>& arr, spt idx);

// inserts a new item at 'idx' and sets its value to 'val'
template<typename T> void
insert(SharedArray<T>& arr, spt idx, T& val);

// removes the item at idx
// if the array is 'unordered', the item at
// the end of the array is moved to the given 'idx'
// but if it is ordered, then all items to the left of 
// 'idx' are moved left
template<typename T> void
remove(SharedArray<T>& arr, u32 idx, b32 unordered = false);

// sets all items in an array to 0
// does not affect space
template<typename T> void
clear(SharedArray<T>& arr);

// sets the count of the array and zero inits any new items
template<typename T> void
resize(SharedArray<T>& arr, u32 count);

// allocates a new amount of space for items
// but does not affect the count of items
template<typename T> void
reserve(SharedArray<T>& arr, u32 count);

// sets the item at 'idx' to 'val'
// the thread safe alternative to trying to set directly
template<typename T> void
set(SharedArray<T>& arr, spt idx, T val);

// makes a copy of the item at 'idx' and returns it 
// supports negative indexing
template<typename T> T
read(SharedArray<T>& arr, spt idx);

// returns a pointer to the item at 'idx' in the array
// supports negative indexing
// the item can move with the entire array, so this can become invalid
// at any time!
template<typename T> T*
readptr(SharedArray<T>& arr, spt idx);

// returns a reference to the item at 'idx' in the array
// supports negative indexing
// the item can move with the entire array, so this can become invalid
// at any time!
template<typename T> T&
readref(SharedArray<T>& arr, spt idx);

// lock the entire array and return a normal Array representation of it
// so that you may operate on it normally. 'unlock' must be called 
// on the same SharedArray and the Array passed so that the SharedArray may
// be synced to any changes made to the Array
template<typename T> Array<T>
lock(SharedArray<T>& arr);

// release a previous lock on the array, passing in the Array that was
// given earlier by 'lock' so that it may be synchronized
// NOTE(sushi) this is very experimental, so remove later and just use SharedArray normally
//             if it doesn't work well or is too confusing
template<typename T> void
unlock(SharedArray<T>& arr, Array<T>& sync);

// makes a copy of the given SharedArray's contents 
// and returns a new SharedArray with those contents
template<typename T> SharedArray<T>
copy(SharedArray<T>& arr);

} // namespace array
} // namespace amu

#endif // AMU_SHARED_ARRAY_H