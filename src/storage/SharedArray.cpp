namespace amu {
namespace shared_array{

namespace internal {

// make sure that you write lock the arr before giving it to this function
template<typename T> void 
grow_if_needed(SharedArray<T>& arr) {
    if(arr.count >= arr.space) {
        arr.data = (T*)memory::reallocate(arr.data, sizeof(T) * arr.space * 2);
    }
}

} // namespace internal

template<typename T> SharedArray<T>
init(u32 initial_space) {
    SharedArray<T> out;
    out.lock = shared_mutex_init();
    out.data = (T*)memory::allocate(sizeof(T)*initial_space);
    out.count = 0;
    out.space = initial_space;
    return out;
}

template<typename T> SharedArray<T>
deinit(SharedArray<T>& arr) {
    shared_mutex_lock(&arr.lock);
    memory::free(arr.data);
    shared_mutex_unlock(&arr.lock);
    shared_mutex_deinit(&arr.lock);
    arr = {};
}

template<typename T> T* 
push(SharedArray<T>& arr) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    internal::grow_if_needed(arr);
    T* out = arr.data + arr.count;
    arr.count += 1;

    return out;
}

template<typename T> void
push(SharedArray<T>& arr, const T& val) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    internal::grow_if_needed(arr);
    *(arr.data + arr.count) = val;
    arr.count += 1;
}


template<typename T> T
pop(SharedArray<T>& arr, u32 count) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    forI(count-1) {
        arr.count -= 1;
    }

    T out = *(arr.data + arr.count - 1);
    return out;
}

template<typename T> T*
insert(SharedArray<T>& arr, spt idx) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    Assert(idx < arr.count);

    internal::grow_if_needed(arr);

    MoveMemory(arr.data + idx + 1, arr.data + idx + 2, sizeof(T) * (arr.count - idx));
    arr.count += 1;
    return arr.data + idx;
}

template<typename T> void
insert(SharedArray<T>& arr, spt idx, T& val) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    Assert(idx < arr.count);

    internal::grow_if_needed(arr);

    MoveMemory(arr.data + idx + 1, arr.data + idx, sizeof(T) * (arr.count - idx));
    arr.count += 1;
    *(arr.data + idx) = val;
}

template<typename T> void
remove(SharedArray<T>& arr, u32 idx, b32 unordered) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    Assert(idx < arr.count);

    arr.count -= 1;
    if(unordered) {
        arr.data[idx] = arr.data[arr.count];
    } else {
        MoveMemory(arr.data+idx, arr.data+idx+1, sizeof(T)*(arr.count - idx));
    }
}

template<typename T> void
clear(SharedArray<T>& arr) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    forI(arr.count) {
        arr.data[i] = {};
    }
}

template<typename T> void
resize(SharedArray<T>& arr, u32 count) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

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
reserve(SharedArray<T>& arr, u32 count) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    if(count > arr.space) {
        arr.data = memory::reallocate(arr.data, count*sizeof(T));
        arr.space = count;
    }
}

template<typename T> T
read(SharedArray<T>& arr, spt idx) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return *(arr.data + arr.count + idx);
    } else {
        Assert(idx < arr.count);
        return *(arr.data + idx);
    }
}

template<typename T> T*
readptr(SharedArray<T>& arr, spt idx) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return arr.data + arr.count + idx;
    } else {
        Assert(idx < arr.count);
        return arr.data + idx;
    }
}

template<typename T> T&
readref(SharedArray<T>& arr, spt idx) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    if(idx < 0) {
        Assert(arr.count + idx >= 0);
        return arr.data[arr.count + idx];
    } else {
        Assert(idx < arr.count);
        return arr.data[idx];
    }
}

template<typename T> Array<T>
lock(SharedArray<T>& arr) {
    shared_mutex_lock(&arr.lock);
    Array<T> out;
    out.data = arr.data;
    out.count = arr.count;
    out.space = arr.space;
    return out;
}

template<typename T> void
unlock(SharedArray<T>& arr, Array<T>& sync) {
    arr.data = sync.data;
    arr.count = sync.count;
    arr.space = sync.space;
    shared_mutex_unlock(&arr.lock);
}

template<typename T> SharedArray<T>
copy(SharedArray<T>& arr) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    SharedArray<T> out = init(arr.count);
    CopyMemory(out.data, arr.data, sizeof(T)*arr.count);

    return out;
}

} // namespace shared_array
} // namespace amu