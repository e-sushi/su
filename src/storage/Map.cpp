namespace amu {
namespace map {

template<typename K, typename V> Map<K,V>
init() {
    Map<K,V> out;
    out.keys = array::init<typename Map<K,V>::Key>();
    out.values = array::init<V>();
    return out;
}

template<typename K, typename V> void
deinit(Map<K,V>& m) {
    array::deinit(m.keys);
    array::deinit(m.values);
}

template<typename K, typename V> u32
add(Map<K,V>& m, const K& key) {
    u64 hash = util::hash(key);
    auto [idx, found] = find(m, hash);
    if(found) return idx;
    if(idx == -1) idx = 0;
    array::insert(m.keys, idx, {key, hash, 0});
    array::insert(m.values, idx, {});
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
    array::remove(m.valuesm, idx);
}

template<typename K, typename V> b32 
has(Map<K,V>& m, const K& key) {
    return find(m, util::hash(key)).second;
}

template<typename K, typename V> pair<spt,b32>
find(Map<K,V>& m, const K& key) {
    return find(m, util::hash(key));
}

template<typename K, typename V> pair<spt,b32>
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
