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

void* allocate(upt size) {
    deshi::mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memalloc(size);
    deshi::mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void* reallocate(void* ptr, upt size) {
    deshi::mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memrealloc(ptr, size);
    deshi::mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void free(void* ptr) {
    deshi::mutex_lock(&compiler::instance.deshi_mem_lock);
    memzfree(ptr);
    deshi::mutex_lock(&compiler::instance.deshi_mem_lock);
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
}

}
} // namespace amu

#endif // AMU_UTIL_H