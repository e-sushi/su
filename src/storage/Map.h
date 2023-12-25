/*

    Map struct for storing keys and values

    TODOs
        if we start encountering collisions, we'll need to implement the collision manager stuff
        but for now I'm going to skip over it 

*/

#ifndef AMU_MAP_H
#define AMU_MAP_H

#include "Array.h"
namespace amu {

struct FindResult {
    spt index;
    b32 found;
};

template<typename K, typename V>
struct Map {
    struct Key {
        K key;
        u64 hash;
        u32 collisions;
    };

    Array<Key> keys;
    Array<V> values;


	// ~~~~ interface ~~~~
	

	static Map<K,V>
	create();

	void
	destroy();

	u32
	add(const K& key);

	u32
	add(const K& key, const V& value);

	void
	remove(const K& key);

	b32
	has(const K& key);

	FindResult
	find(const K& key);

	FindResult
	find(u64 hash);
};

template<typename K, typename V> Map<K,V> Map<K, V>::
create() {
    Map<K,V> out;
    out.keys = Array<typename Map<K,V>::Key>::create();
    out.values = Array<V>::create();
    return out;
}

template<typename K, typename V> void Map<K, V>::
destroy() {
    this->keys.destroy();
    this->values.destroy();
}

template<typename K, typename V> u32 Map<K,V>::
add(const K& key) {
    u64 hash = util::hash(key);
    auto [idx, found] = find(hash);
    if(found) return idx;
    if(idx == -1) idx = 0;
    this->keys.insert(idx, {key, hash, 0});
    this->values.insert(idx, {});
    return idx;
}

template<typename K, typename V> u32 Map<K,V>::
add(const K& key, const V& value) {
    u32 idx = add(key);
    this->values.readref(idx) = value;
    return idx;
}

template<typename K, typename V> void Map<K,V>::
remove(const K& key) {
    auto [idx, found] = find(key);
    if(!found) return;
    this->keys.remove(idx);
    this->values.remove(idx);
}

template<typename K, typename V> b32 Map<K,V>::
has(const K& key) {
    return find(util::hash(key)).second;
}

template<typename K, typename V> FindResult Map<K,V>::
find(const K& key) {
    return find(util::hash(key));
}

template<typename K, typename V> FindResult Map<K,V>::
find(u64 hash) {
    spt index = -1;
    spt middle = -1;
    if(this->keys.count) {
        spt left = 0;
        spt right = this->keys.count - 1;
        while(left <= right) {
            middle = left+(right-left)/2;
            if(this->keys.readref(middle).hash == hash) {
                index = middle;
                break;
            }
            if(this->keys.readref(middle).hash < hash) {
                left = middle+1;
                middle = left+(right-left)/2;
            } else {
                right = middle-1;
            }
        }
    }
    return {middle, index != -1};
}


} // namespace amu

#endif
