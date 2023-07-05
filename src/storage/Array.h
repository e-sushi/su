/*
    Contiguous array.

    These arrays do NOT implicitly copy, to make a copy of an Array you must call array::copy

*/

#ifndef AMU_ARRAY_H
#define AMU_ARRAY_H

#include "kigu/common.h"

namespace amu {

template<typename T>
struct Array {
    T* data;
    spt count;
    spt space;
};

namespace array {

// initializes an array with an initial space
template<typename T> Array<T>
init(u32 initial_space = 4);

// deinitializes an array
template<typename T> void
deinit(Array<T>& arr);

// pushes an item to the end of the array and 
// return a pointer to it, growing if needed
template<typename T> T* 
push(Array<T>& arr);

// pushes an item to the end of the array and
// sets its value to 'val', growing if needed
template<typename T> void
push(Array<T>& arr, const T& val);

// pops 'count' items from the end of the array
// and returns the last item popped's value
template<typename T> T
pop(Array<T>& arr, u32 count = 1);

// inserts a new item at 'idx' and returns a pointer to it
template<typename T> T*
insert(Array<T>& arr, spt idx);

// inserts a new item at 'idx' and sets its value to 'val'
template<typename T> void
insert(Array<T>& arr, spt idx, const T& val);

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
// the thread safe alternative to trying to set directly
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

// returns a reference to the item at 'idx' in the array
// supports negative indexing
// the item can move with the entire array, so this can become invalid
// at any time!
template<typename T> T&
readref(Array<T>& arr, spt idx);

// makes a copy of the given Array's contents 
// and returns a new Array with those contents
template<typename T> Array<T>
copy(Array<T>& arr);

} // namespace array
} // namespace amu

#endif // AMU_ARRAY_H