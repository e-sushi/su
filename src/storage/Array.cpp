namespace amu {
namespace array{

namespace internal {

template<typename T> void 
grow_if_needed(Array<T>& arr) {
    if(arr.count >= arr.space) {
        arr.space = arr.space * 2;
        arr.data = (T*)memory::reallocate(arr.data, arr.space * sizeof(T));
    }
}

} // namespace internal

template<typename T> Array<T>
init(u32 initial_space) {
    Array<T> out;
    out.data = (T*)memory::allocate(sizeof(T)*initial_space);
    out.count = 0;
    out.space = initial_space;
    return out;
}

template<typename T> void
deinit(Array<T>& arr) {
    memory::free(arr.data);
    arr = {};
}

template<typename T> T* 
push(Array<T>& arr) {
    internal::grow_if_needed(arr);
    T* out = arr.data + arr.count;
    arr.count += 1;

    return out;
}

template<typename T> void
push(Array<T>& arr, const T& val) {
    internal::grow_if_needed(arr);
    *(arr.data + arr.count) = val;
    arr.count += 1;
}


template<typename T> T
pop(Array<T>& arr, u32 count) {
    forI(count-1) {
        arr.count -= 1;
    }

    T out = *(arr.data + arr.count-- - 1);
    return out;
}

template<typename T> T*
insert(Array<T>& arr, spt idx) {
    if(!arr.count && !idx) return push(arr);
    
    Assert(idx < arr.count);

    internal::grow_if_needed(arr);

    memory::move(arr.data + idx + 1, arr.data + idx + 2, sizeof(T) * (arr.count - idx));
    arr.count += 1;
    return arr.data + idx;
}

template<typename T> void
insert(Array<T>& arr, spt idx, const T& val) {
    Assert(idx <= arr.count);

    internal::grow_if_needed(arr);

    if(!arr.count) {
        array::push(arr, val);
    } else {
        memory::move(arr.data + idx + 1, arr.data + idx, sizeof(T) * (arr.count - idx));
        arr.count += 1;
        *(arr.data + idx) = val;
    }
}

template<typename T> void
remove(Array<T>& arr, u32 idx, b32 unordered) {
    Assert(idx < arr.count);

    arr.count -= 1;
    if(unordered) {
        arr.data[idx] = arr.data[arr.count];
    } else {
        memory::move(arr.data+idx, arr.data+idx+1, sizeof(T)*(arr.count - idx));
    }
}

template<typename T> void
clear(Array<T>& arr) {
    forI(arr.count) {
        arr.data[i] = {};
    }
    arr.count = 0;
}

template<typename T> void
resize(Array<T>& arr, u32 count) {
    if(count > arr.count) {
        if(count > arr.space) {
            arr.data = memory::reallocate(arr.data, count*sizeof(T));
        }

        forI(count - arr.count) {
            *(arr.data + arr.count + i) = {};
        }

        arr.count = count;
    } else if (count < arr.count) {
        arr.count = count;
    }
}

template<typename T> void
reserve(Array<T>& arr, u32 count) {
    if(count > arr.space) {
        arr.data = memory::reallocate(arr.data, count*sizeof(T));
        arr.space = count;
    }
}

template<typename T> T
read(Array<T>& arr, spt idx) {
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return *(arr.data + arr.count + idx);
    } else {
        Assert(idx < arr.count);
        return *(arr.data + idx);
    }
}

template<typename T> T*
readptr(Array<T>& arr, spt idx) {
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return arr.data + arr.count + idx;
    } else {
        Assert(idx < arr.count);
        return arr.data + idx;
    }
}

template<typename T> T&
readref(Array<T>& arr, spt idx) {
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return arr.data[arr.count + idx];
    } else {
        Assert(idx < arr.count);
        return arr.data[idx];
    }
}

template<typename T> Array<T>
copy(Array<T>& arr) {
    Array<T> out = init(arr.count);
    CopyMemory(out.data, arr.data, sizeof(T)*arr.count);

    return out;
}


namespace util {


template<typename T, typename I> SearchResult
search(Array<T>& arr, I element, I (*get)(T&)) {
    s64 index = -1;
    s64 middle = -1;
    if(arr.count) {
        s64 left = 0;
        s64 right = arr.count - 1;
        while(left <= right) {
            middle = left+(right-left)/2;
            I elem = get(array::readref(arr,middle));
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
} // namespace amu