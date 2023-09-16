/*
    Contiguous array.

    These arrays do NOT implicitly copy, to make a copy of an Array you must call array::copy

*/

#ifndef AMU_ARRAY_H
#define AMU_ARRAY_H

#include "View.h"

namespace amu {

template<typename T>
struct Array {
    T* data;
    spt count;
    spt space;

    static Array<T>
    create(u32 initial_space = 4);

    void
    destroy();

    // pushes an item to the end of the array and 
    // returns a pointer to it, growing if needed
    T*
    push();

    // pushes an item to the end of the array and
    // sets its value to 'val', growing if needed
    void
    push(const T& val);


    // pops 'count' items from the end of the array
    // and returns the last item popped's value
    T
    pop(u32 count = 1);

    // inserts a new item at 'idx' and returns a pointer to it
    T*
    insert(spt idx);

    // inserts a new item at 'idx' and sets its value to 'val'
    void
    insert(spt idx, const T& val);

    // removes the item at idx
    // if the array is 'unordered', the item at
    // the end of the array is moved to the given 'idx'
    // but if it is ordered, then all items to the left of 
    // 'idx' are moved left
    void
    remove(u32 idx, b32 unordered = false);

    // sets all items in an array to 0
    // does not affect space
    void
    clear();

    // sets the count of the array and zero inits any new items
    void
    resize(u32 count);

    // allocates a new amount of space for items
    // but does not affect the count of items
    void
    reserve(u32 count);

    // sets the item at 'idx' to 'val'
    // the thread safe alternative to trying to set directly
    void
    set(spt idx, T val);

    // makes a copy of the item at 'idx' and returns it 
    // supports negative indexing
    T
    read(spt idx);

    // returns a pointer to the item at 'idx' in the array
    // supports negative indexing
    // the item can move with the entire array, so this can become invalid
    // at any time!
    T*
    readptr(spt idx);

    // returns a reference to the item at 'idx' in the array
    // supports negative indexing
    // the item can move with the entire array, so this can become invalid
    // at any time!
    T&
    readref(spt idx);

    // makes a copy of the given Array's contents 
    // and returns a new Array with those contents
    Array<T>
    copy();

    // makes a copy of a slice of the given Array's contents 
    // and returns a new Array with those contents
    Array<T>
    copy(u64 start, u64 count);

    // returns a View over the given Array
    View<T>
    view();

    T operator[](s64 i) {
        return read(i);
    }
};

namespace array {
namespace util {

struct SearchResult {
    s64 index;
    b32 found;
};

// if the given array is sorted by the value retrieved by 'get', then
// this performs a binary search for 'element'
template<typename T, typename I> SearchResult
search(Array<T>& arr, I element, I (*get)(T&));

} // namespace util
} // namespace array

template<typename T>
using ScopedArray = util::scoped<Array<T>, [](Array<T>* a) { a->destroy(); }>;

} // namespace amu

#endif // AMU_ARRAY_H