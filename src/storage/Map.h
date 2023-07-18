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

template<typename K, typename V>
struct Map {
    struct Key {
        K key;
        u64 hash;
        u32 collisions;
    };

    Array<Key> keys;
    Array<V> values;
};


struct FindResult {
    spt index;
    b32 found;
};

namespace map {

template<typename K, typename V> Map<K,V>
init();

template<typename K, typename V> void
deinit(Map<K,V>& m);

template<typename K, typename V> u32
add(Map<K,V>& m, const K& key);

template<typename K, typename V> u32
add(Map<K,V>& m, const K& key, const V& value);

template<typename K, typename V> void 
remove(Map<K,V>& m, const K& key);

template<typename K, typename V> b32 
has(Map<K,V>& m, const K& key);

// returns the index reached in a binary search of the map
// and a boolean indicating if it actually found the key or not
template<typename K, typename V> FindResult
find(Map<K,V>& m, const K& key);

template<typename K, typename V> FindResult
find(Map<K,V>& m, u64 hash);

} // namespace map
} // namespace amu

#endif