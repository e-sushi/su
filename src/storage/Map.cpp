namespace amu {
namespace map {

template<typename K, typename V> Map<K,V>
init() {
    Map<K,V> out;
    out.keys = Array<typename Map<K,V>::Key>::create();
    out.values = Array<V>::create();
    return out;
}

template<typename K, typename V> void
deinit(Map<K,V>& m) {
    m.keys.destroy();
    m.values.destroy();
}

template<typename K, typename V> u32
add(Map<K,V>& m, const K& key) {
    u64 hash = util::hash(key);
    auto [idx, found] = find(m, hash);
    if(found) return idx;
    if(idx == -1) idx = 0;
    m.keys.insert(idx, {key, hash, 0});
    m.values.insert(idx, {});
    return idx;
}

template<typename K, typename V> u32
add(Map<K,V>& m, const K& key, const V& value) {
    u32 idx = add(m, key);
    array::readref(m.values, idx) = value;
    return idx;
}

template<typename K, typename V> void 
remove(Map<K,V>& m, const K& key) {
    auto [idx, found] = find(m, key);
    if(!found) return;
    array::remove(m.keys, idx);
    array::remove(m.values, idx);
}

template<typename K, typename V> b32 
has(Map<K,V>& m, const K& key) {
    return find(m, util::hash(key)).second;
}

template<typename K, typename V> FindResult
find(Map<K,V>& m, const K& key) {
    return find(m, util::hash(key));
}

template<typename K, typename V> FindResult
find(Map<K,V>& m, u64 hash) {
    spt index = -1;
    spt middle = -1;
    if(m.keys.count) {
        spt left = 0;
        spt right = m.keys.count - 1;
        while(left <= right) {
            middle = left+(right-left)/2;
            if(array::readref(m.keys, middle).hash == hash) {
                index = middle;
                break;
            }
            if(array::readref(m.keys, middle).hash < hash) {
                left = middle+1;
                middle = left+(right-left)/2;
            } else {
                right = middle-1;
            }
        }
    }
    return {middle, index != -1};
}

} // namespace map
} // namespace amu
