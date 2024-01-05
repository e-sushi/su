/*
    Contiguous array.

    These arrays do NOT implicitly copy, to make a copy of an Array you must call array::copy

*/

#ifndef AMU_ARRAY_H
#define AMU_ARRAY_H

// #include "View.h"
#include "Common.h"
#include "basic/Memory.h"

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
    //View<T>
    //qview();

    T operator[](s64 i) const {
        return read(i);
    }

	T& operator[](s64 i) {
		return readref(i);
	}
};

template<typename T> void 
grow_if_needed(Array<T>& arr) {
    if(arr.count >= arr.space) {
        if(!arr.space) arr.space = 4;
        arr.space = arr.space * 2;
        arr.data = (T*)memory.reallocate(arr.data, arr.space * sizeof(T));
    }
}

template<typename T> Array<T> Array<T>::
create(u32 initial_space) {
    Array<T> out;
    out.data = (T*)memory.allocate(sizeof(T) * initial_space);
    out.count = 0;
    out.space = initial_space;
    return out;
}

template<typename T> void Array<T>::
destroy() {
    memory.free(this->data);
    *this = {};
}

template<typename T> T*  Array<T>::
push() {
    grow_if_needed(*this);
    auto out = new (this->data + this->count) T;
    this->count += 1;

    return out;
}

template<typename T> void Array<T>::
push(const T& val) {
    grow_if_needed(*this);
    *(this->data + this->count) = val;
    this->count += 1;
}


template<typename T> T Array<T>::
pop(u32 count) {
    Assert(this->count);
    forI(count-1) {
        this->count -= 1;
    }

    T out = *(this->data + this->count-- - 1);
    return out;
}

template<typename T> T* Array<T>::
insert(spt idx) {
    if(!this->count && !idx) return push(*this);
    
    Assert(idx < this->count);

    grow_if_needed(*this);

    memory.move(this->data + idx + 1, this->data + idx, sizeof(T) * (this->count - idx));
    memory.zero(this->data + idx);
    this->count += 1;
    return this->data + idx;
}

template<typename T> void Array<T>::
insert(spt idx, const T& val) {
    Assert(idx <= this->count);

    grow_if_needed(*this);

    if(!this->count) {
        push(val);
    } else {
        memory.move(this->data + idx + 1, this->data + idx, sizeof(T) * (this->count - idx));
        memory.zero(this->data + idx, sizeof(T));
        this->count += 1;
        *(this->data + idx) = val;
    }
}

template<typename T> void Array<T>::
remove(u32 idx, b32 unordered) {
    Assert(idx < this->count);

    this->count -= 1;
    if(unordered) {
        this->data[idx] = this->data[this->count];
    } else {
        memory.move(this->data+idx, this->data+idx+1, sizeof(T)*(this->count - idx));
    }
}

template<typename T> void Array<T>::
clear() {
    forI(this->count) {
        this->data[i] = {};
    }
    this->count = 0;
}

template<typename T> void Array<T>::
resize(u32 count) {
    if(count > this->count) {
        if(count > this->space) {
            this->data = (T*)memory.reallocate(this->data, count*sizeof(T));
        }

        forI(count - this->count) {
            *(this->data + this->count + i) = {};
        }

        this->count = count;
    } else if (count < this->count) {
        this->count = count;
    }
}

template<typename T> void Array<T>::
reserve(u32 count) {
    if(count > this->space) {
        this->data = memory.reallocate(this->data, count*sizeof(T));
        this->space = count;
    }
}

template<typename T> T Array<T>::
read(spt idx) {
    if(idx < 0) {
        Assert(this->count + idx >= 0);
        return *(this->data + this->count + idx);
    } else {
        Assert(idx < this->count);
        return *(this->data + idx);
    }
}

template<typename T> T* Array<T>::
readptr(spt idx) {
    if(idx < 0) {
        Assert(this->count + idx >= 0);
        return this->data + this->count + idx;
    } else {
        Assert(idx < this->count);
        return this->data + idx;
    }
}

template<typename T> T& Array<T>::
readref(spt idx) {
    if(idx < 0) {
        Assert(this->count + idx >= 0);
        return this->data[this->count + idx];
    } else {
        Assert(idx < this->count);
        return this->data[idx];
    }
}

template<typename T> Array<T> Array<T>::
copy() {
    Array<T> out = init<T>(this->count);
    memory.copy(out.data, this->data, sizeof(T)*this->count);
    out.count = this->count;
    return out;
}

template<typename T> Array<T> Array<T>::
copy(u64 start, u64 count) {
    Assert(start < this->count && start + count < this->count);
    Array<T> out = create(count);
    memory.copy(out.data, this->data+start, sizeof(T)*count);
    out.count = count;
    return out;
}

namespace array {
namespace util {

struct SearchResult {
    s64 index;
    b32 found;
};

// if the given array is sorted by the value retrieved by 'get', then
// this performs a binary search for 'element'
template<typename T, typename I> SearchResult
search(Array<T>& arr, I element, I (*get)(T)) {
    s64 index = -1;
    s64 middle = -1;
    if(arr.count) {
        s64 left = 0;
        s64 right = arr.count - 1;
        while(left <= right) {
            middle = left+(right-left)/2;
            I elem = get(arr[middle]);
            if(elem == element) {
                index = middle;
                break;
            }
            if(elem < element) {
                left = middle+1;
            } else {
                right = middle - 1;
            }
        }
    }

    return {(index==-1? 0 : index), index!=-1};
}

} // namespace util
} // namespace array

// template<typename T>
// using ScopedArray = util::scoped<Array<T>, [](Array<T>* a) { a->destroy(); }>;

} // namespace amu

#endif // AMU_ARRAY_H
