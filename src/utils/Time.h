/*

	Utilities for dealing with time

*/

#ifndef AMU_TIME_H
#define AMU_TIME_H

#include "Common.h"

namespace amu {

struct String;
struct DString;

namespace Time {

struct Span;
struct Date;

struct Point {
	s64 s,n;

	static Point now();

	Date utc_date();
	Date local_date();

	friend Span operator-(Point lhs, Point rhs);
};

struct Date {
	s32 nanosecond;
	s32 microsecond;
	s32 millisecond;
	s32 second;
	s32 minute;
	s32 hour;
	s32 month_day;
	s32 month;
	s32 year_day;
	s32 year;
	s32 week_day;

	String week_day_string(b32 abbreviated = false);
	String month_string(b32 abbreviated = false);

	DString pretty();
};

// Representation of a span of time.
struct Span {
	s64 nanoseconds; 
	
	// NOTE(sushi) these are implement as needed
	static Span from_nanoseconds(s64 n);
	static Span from_microseconds(s64 n);
	static Span from_milliseconds(s64 n);
	static Span from_seconds(s64 n);
	static Span from_minutes(s64 n);
	static Span from_hours(s64 n);
	static Span from_days(s64 n);

	f64 to_nanoseconds();
	f64 to_microseconds();
	f64 to_milliseconds();
	f64 to_seconds();
	f64 to_minutes();
	f64 to_hours();
	f64 to_days();
	f64 to_weeks();
	f64 to_months();
	f64 to_years();

	DString pretty(u32 n_units = 2);
};

}

} // namespace amu

#endif
