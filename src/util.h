/*
    Various helpful utilities for use throughout amu
*/

#ifndef AMU_UTIL_H
#define AMU_UTIL_H

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "Compiler.h"

namespace deshi {
#   include "core/threading.h"
}

namespace amu {
namespace util {

// it's probably better to just use stdlib's allocation at this point, but I will keep it around for now
// incase I ever fix up deshi's interface to be a little nicer
void* 
allocate(upt size) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memalloc(size);
    mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void* 
reallocate(void* ptr, upt size) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memrealloc(ptr, size);
    mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void 
free(void* ptr) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    memzfree(ptr);
    mutex_lock(&compiler::instance.deshi_mem_lock);
}

// debug print functions
void 
print(str8 s) {
    printf("%s", s.str);
}

void 
println(str8 s) {
    printf("%s\n", s.str);
}

//this is temp allocated, so just clear temp mem or free the str yourself
dstr8 format_time(f64 ms){
	FixMe;
    //if(floor(Minutes(ms))){
	//	//hope it never gets this far :)
	//	f64 fmin = floor(Minutes(ms));
	//	f64 fsec = floor(Seconds(ms)) - fmin * 60;
	//	f64 fms  = ms - fmin*60*1000 - fsec*1000;
	//	return to_str8_amu(fmin, "m ", fsec, "s ", fms, " ms");
	//}else if(floor(Seconds(ms))){
	//	f64 fsec = floor(Seconds(ms));
	//	f64 fms  = ms - fsec*SecondsToMS(1);
	//	return to_str8_amu(fsec, "s ", fms, "ms");
	//}else{
	//	return to_str8_amu(ms, " ms");
	//}
    return {};
}

}
} // namespace amu

#endif // AMU_UTIL_H