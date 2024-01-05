#ifndef AMU_COLORIZE_H
#define AMU_COLORIZE_H

#include "storage/DString.h"

namespace amu {

enum class Color {
	Null = 0, 
	Black,
	Red,
	Green,
	Yellow,
	Blue,
	Magenta,
	Cyan,
	White,
	BrightBlack,
	BrightRed,
	BrightGreen,
	BrightYellow,
	BrightBlue,
	BrightMagenta,
	BrightCyan,
	BrightWhite,
};


static void 
colorize(DString& s, Color color) {
	using enum Color;
	switch(color) {
		case Null: Assert(0); return;
#define colorcase(a, b) case a: s.prepend("\e[" STRINGIZE(b) "m"); break;
		colorcase(Black, 30);
		colorcase(Red, 31);
		colorcase(Green, 32);
		colorcase(Yellow, 33);
		colorcase(Blue, 34);
		colorcase(Magenta, 35);
		colorcase(Cyan, 36);
		colorcase(White, 0);
		colorcase(BrightBlack, 90);
		colorcase(BrightRed, 91);
		colorcase(BrightGreen, 92);
		colorcase(BrightYellow, 93);
		colorcase(BrightBlue, 94);
		colorcase(BrightMagenta, 95);
		colorcase(BrightCyan, 96);
		colorcase(BrightWhite, 97);
		default: {
			s.append("\e[31mINTERNAL ERROR: unknown color: ", (u32)color);
		} break;
	}
	s.append("\e[0m");
}

} // namespace amu

#endif
