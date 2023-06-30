namespace amu {
namespace array{

namespace internal {

// make sure that you write lock the arr before giving it to this function
template<typename T> void 
grow_if_needed(Array<T>& arr) {
    if(arr.count >= arr.space) {
        arr.data = (T*)util::reallocate(arr.data, sizeof(T) * arr.space * 2);
    }
}

} // namespace internal

template<typename T> Array<T>
init(u32 initial_space) {
    Array<T> out;
    out.lock = shared_mutex_init();
    out.data = (T*)util::allocate(sizeof(T)*initial_space);
    out.count = 0;
    out.space = initial_space;
    return out;
}

template<typename T> Array<T>
deinit(Array<T>& arr) {
    shared_mutex_lock(&arr.lock);
    util::free(arr.data);
    shared_mutex_unlock(&arr.lock);
    shared_mutex_deinit(&arr.lock);
    arr = {};
}

template<typename T> T* 
push(Array<T>& arr) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    internal::grow_if_needed(arr);

    T* out = arr.data + arr.count;
    arr.count += 1;
    return out;
}

template<typename T> void
push(Array<T>& arr, T& val) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    T* place = push(arr);
    *place = val;
}

template<typename T> T
pop(Array<T>& arr, u32 count) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    
    forI(count-1) {
        arr.count -= 1;
    }

    T out = *(arr.data + arr.count - 1);
    return out;
}

template<typename T> void
remove(Array<T>& arr, u32 idx, b32 unordered) {
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
clear(Array<T>& arr) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    forI(arr.count) {
        arr.data[i] = {};
    }
}

template<typename T> void
resize(Array<T>& arr, u32 count) {
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    if(count > arr.count) {
        if(count > arr.space) {
            arr.data = util::reallocate(arr.data, count*sizeof(T));
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
    shared_mutex_lock(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};

    if(count > arr.space) {
        arr.data = util::reallocate(arr.data, count*sizeof(T));
        arr.space = count;
    }
}

template<typename T> T
read(Array<T>& arr, spt idx) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    Assert(idx < arr.count);

    return *(arr.data + idx);
}

template<typename T> T*
readptr(Array<T>& arr, spt idx) {
    shared_mutex_lock_shared(&arr.lock);
    defer{shared_mutex_unlock(&arr.lock);};
    Assert(idx < arr.count);

    return arr.data + idx;
}

template<typename T> void
lock(Array<T>& arr) {
    shared_mutex_lock(&arr.lock);
}

template<typename T> void
unlock(Array<T>& arr) {
    shared_mutex_unlock(&arr.lock);
}

} // namespace array
} // namespace amu