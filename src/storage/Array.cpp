namespace amu {

namespace array::internal {

template<typename T> void 
grow_if_needed(Array<T>& arr) {
    if(arr.count >= arr.space) {
        if(!arr.space) arr.space = 4;
        arr.space = arr.space * 2;
        arr.data = (T*)memory::reallocate(arr.data, arr.space * sizeof(T));
    }
}

} // namespace array::internal

template<typename T> Array<T> Array<T>::
create(u32 initial_space) {
    Array<T> out;
    out.data = (T*)memory::allocate(sizeof(T)*initial_space);
    out.count = 0;
    out.space = initial_space;
    return out;
}

template<typename T> void Array<T>::
destroy() {
    memory::free(this->data);
    *this = {};
}

template<typename T> T*  Array<T>::
push() {
    array::internal::grow_if_needed(*this);
    T* out = new (this->data + this->count) T();
    this->count += 1;

    return out;
}

template<typename T> void Array<T>::
push(const T& val) {
    array::internal::grow_if_needed(*this);
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

    array::internal::grow_if_needed(*this);

    memory::move(this->data + idx + 1, this->data + idx, sizeof(T) * (this->count - idx));
    memory::zero(this->data + idx, sizeof(T));
    this->count += 1;
    return this->data + idx;
}

template<typename T> void Array<T>::
insert(spt idx, const T& val) {
    Assert(idx <= this->count);

    array::internal::grow_if_needed(*this);

    if(!this->count) {
        push(val);
    } else {
        memory::move(this->data + idx + 1, this->data + idx, sizeof(T) * (this->count - idx));
        memory::zero(this->data + idx, sizeof(T));
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
        memory::move(this->data+idx, this->data+idx+1, sizeof(T)*(this->count - idx));
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
            this->data = memory::reallocate(this->data, count*sizeof(T));
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
        this->data = memory::reallocate(this->data, count*sizeof(T));
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
    memory::copy(out.data, this->data, sizeof(T)*this->count);
    out.count = this->count;
    return out;
}

template<typename T> Array<T> Array<T>::
copy(u64 start, u64 count) {
    Assert(start < this->count && start + count < this->count);
    Array<T> out = create(count);
    memory::copy(out.data, this->data+start, sizeof(T)*count);
    out.count = count;
    return out;
}

template<typename T> View<T> Array<T>::
view() {
    View<T> out = {};
    out.data = this->data;
    out.count = this->count;
    return out;
}


namespace array::util {


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
} // namespace amu