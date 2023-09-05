#ifndef AMU_VIEW_H
#define AMU_VIEW_H

namespace amu {

template<typename T>
struct View {
    T* data;
    u64 count;
};

namespace view {

template<typename T> T
read(const View<T>& v, s64 idx) {
    if(idx < 0) {
        Assert(v.count + idx >= 0);
        return *(v.data + v.count + idx);
    } else {
        Assert(idx < v.count);
        return *(v.data + idx);
    }
}

template<typename T> T&
readref(const View<T>& v, s64 idx) {
    if(idx < 0) {
        Assert(v.count + idx >= 0);
        return v.data[v.count + idx];
    } else {
        Assert(idx < v.count);
        return v.data[idx];
    }
}

template<typename T> T*
readptr(const View<T>& v, s64 idx) {
    if(idx < 0) {
        Assert(v.count + idx >= 0);
        return v.data + v.count + idx;
    } else {
        Assert(idx < v.count);
        return v.data + idx;
    }
}

} // namespace view

} // namespace amu


#endif // AMU_VIEW_H