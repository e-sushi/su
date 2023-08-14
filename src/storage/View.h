#ifndef AMU_VIEW_H
#define AMU_VIEW_H

namespace amu {

template<typename T>
struct View {
    T* data;
    u64 count;
};

} // namespace amu


#endif // AMU_VIEW_H