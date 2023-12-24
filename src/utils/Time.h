/*

	Utilities for dealing with time

*/

#ifndef AMU_TIME_H
#define AMU_TIME_H

#include "time.h"

namespace amu {
namespace time {

struct Span;

struct Nanoseconds;
struct Microseconds;
struct Milliseconds;
struct Seconds;
struct Minutes;
struct Hours;
struct Days;

enum class Unit {
	Nanosecond,
	Microsecond,
	Millisecond,
	Second,
	Minute,
	Hour,
	Day,
};

// Representation of a single point in time
// in nanoseconds since the program started.
struct Point {
	s64 n;

	static Point
	now();

	friend Span operator-(Point lhs, Point rhs);
};

// Representation of a span of time.
struct Span {
	Point start, end;

	s64 nanoseconds();
	s64 microseconds();
	s64 milliseconds();
	s64 seconds();
	s64 minutes();
	s64 hours();
};

} // namespace time

namespace IO {



} // namespace IO

} // namespace amu

#endif
