/*
	
   Miscellaneous helpers for units

*/

#ifndef AMU_UNITS_H
#define AMU_UNITS_H

#include "storage/String.h"

namespace amu {

static String 
si_prefix_symbol(u64 x) {
	if(x < 1000)                return "";
	if(x < 1000000)             return "k";
	if(x < 1000000000)          return "m";
	if(x < 1000000000000)       return "g";
	if(x < 1000000000000000)    return "t";
	if(x < 1000000000000000000) return "p";
	return "<<<si_prefix_symbol() number given is too large>>>";
}

static String
si_prefix(u64 x) {
	if(x < 1000)                return "";
	if(x < 1000000)             return "kilo";
	if(x < 1000000000)          return "mega";
	if(x < 1000000000000)       return "giga";
	if(x < 1000000000000000)    return "tera";
	if(x < 1000000000000000000) return "peta";
	return "<<<si_prefix() number given is too large>>>";
}

// divides the given number by the largest si unit 
// it contains
static u64 
si_divide(u64 x) {
	if(x < 1000)                return x;
	if(x < 1000000)             return x / 1000;
	if(x < 1000000000)          return x / 1000000;
	if(x < 1000000000000)       return x / 1000000000;
	if(x < 1000000000000000)    return x / 1000000000000;
	if(x < 1000000000000000000) return x / 1000000000000000;
	return x;
}

} // namespace amu

#endif
