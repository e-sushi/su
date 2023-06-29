/*
    Various helpful utilities for use throughout amu
*/

#ifndef AMU_UTIL_H
#define AMU_UTIL_H

#include "kigu/common.h"
#include "kigu/unicode.h"

namespace amu {


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


} // namespace amu

#endif // AMU_UTIL_H