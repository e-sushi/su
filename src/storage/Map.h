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

} // namespace amu

#endif
