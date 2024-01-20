#include "Time.h"
#include <ctime>
#include "storage/String.h"
#include "storage/DString.h"
#include "math.h"

// TODO(sushi) localization

namespace amu {
namespace Time {

Point Point::
now() {
	timespec ts;
	timespec_get(&ts, TIME_UTC);

	Point out = {};
	out.s = ts.tv_sec;
	out.n = ts.tv_nsec;

	return out;
}

Date Point::
utc_date() {
	Date out = {};
	out.nanosecond = n;
	out.microsecond = n / 1000;
	out.millisecond = n / 1000000000;
	out.second = s;
	
	tm t;
#if AMU_WIN32
	gmtime_s(&t, (time_t*)&s);
#else
	gmtime_r((time_t*)&s, &t);
#endif
	
	out.minute = t.tm_min;
	out.hour = t.tm_hour;
	out.month_day = t.tm_mday;
	out.month = t.tm_mon;
	out.year_day = t.tm_yday;
	out.year = 1900 + t.tm_year;
	out.week_day = t.tm_wday;

	return out;
}

Span 
operator-(Point lhs, Point rhs) {
	return {(lhs.s * billion(1) + lhs.n) - (rhs.s * billion(1) + rhs.n)};
}

String Date::
week_day_string(b32 abbreviated) {
	if(abbreviated) {
		switch(week_day) {
			case 0: return "Mon";
			case 1: return "Tue";
			case 2: return "Wed";
			case 3: return "Thu";
			case 4: return "Fri";
			case 5: return "Sat";
			case 6: return "Sun";
		}
		return {};
	} else {
		switch(week_day) {
			case 0: return "Monday";
			case 1: return "Tuesday";
			case 2: return "Wednesday";
			case 3: return "Thursday";
			case 4: return "Friday";
			case 5: return "Saturday";
			case 6: return "Sunday";
		}
		return {};
	}
	return {};
}

String Date::
month_string(b32 abbreviated) {
	if(abbreviated) {
		switch(month) {
			case 0:  return "Jan";
			case 1:  return "Feb";
			case 2:  return "Mar";
			case 3:  return "Apr";
			case 4:  return "May";
			case 5:  return "Jun";
			case 6:  return "Jul";
			case 7:  return "Aug";
			case 8:  return "Sep";
			case 9:  return "Oct";
			case 10: return "Nov";
			case 11: return "Dec";
		}
		return {};
	} else {
		switch(month) {
			case 0:  return "January";
			case 1:  return "February";
			case 2:  return "March";
			case 3:  return "April";
			case 4:  return "May";
			case 5:  return "June";
			case 6:  return "July";
			case 7:  return "August";
			case 8:  return "September";
			case 9:  return "October";
			case 10: return "November";
			case 11: return "December";
		}
		return {};
	}
	return {};
}

f64 Span::to_nanoseconds()  { return (f64)nanoseconds; }
f64 Span::to_microseconds() { return (f64)nanoseconds / 1e3; }
f64 Span::to_milliseconds() { return (f64)nanoseconds / 1e6; }
f64 Span::to_seconds()      { return (f64)nanoseconds / 1e9; }
f64 Span::to_minutes()      { return (f64)nanoseconds / 6e10; }
f64 Span::to_hours()        { return (f64)nanoseconds / 3.6e12; }
f64 Span::to_days()         { return (f64)nanoseconds / 8.64e13; }
f64 Span::to_weeks()        { return (f64)nanoseconds / 6.048e14; }
f64 Span::to_months()       { return (f64)nanoseconds / 2.628e15; }
f64 Span::to_years()        { return (f64)nanoseconds / 3.154e16; }

DString Span::
pretty(u32 n_units) {
	DString out;
	s64 x = 0;
	s64 save = nanoseconds;
#define unit(f, u)                       \
	x = floor(f());                      \
	if(x) {                              \
		out.append(x, STRINGIZE(u) " "); \
		n_units--;                       \
		if(!n_units) {                   \
			nanoseconds = save;          \
			return out;                  \
		}                                \
		nanoseconds -= x;                \
	}
	
	unit(to_years,        years);
	unit(to_months,       months);
	unit(to_weeks,        weeks);
	unit(to_days,         days);
	unit(to_hours,        h);
	unit(to_minutes,      m);
	unit(to_seconds,      s);
	unit(to_milliseconds, ms);
	unit(to_microseconds, μs);
	unit(to_nanoseconds,  ns);

	nanoseconds = save;
	return out;
#undef unit
}


} // namespace Time
} // namespace amu
