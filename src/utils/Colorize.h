#ifndef AMU_COLORIZE_H
#define AMU_COLORIZE_H

#include "storage/DString.h"

namespace amu {

enum class Color {
	Null = 0, 
	Black = 30,
	Red = 31,
	Green = 32,
	Yellow = 33,
	Blue = 34,
	Magenta = 35,
	Cyan = 36,
	White = 37,
	Extended = 38,
	Default = 49,
	BrightBlack = 90,
	BrightRed = 91,
	BrightGreen = 92,
	BrightYellow = 93,
	BrightBlue = 94,
	BrightMagenta = 95,
	BrightCyan = 96,
	BrightWhite = 97,
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
		colorcase(White, 37);
		colorcase(Extended, 38);
		colorcase(Default, 49);
		colorcase(BrightBlack, 90);
		colorcase(BrightRed, 91);
		colorcase(BrightGreen, 92);
		colorcase(BrightYellow, 93);
		colorcase(BrightBlue, 94);
		colorcase(BrightMagenta, 95);
		colorcase(BrightCyan, 96);
		colorcase(BrightWhite, 97);
		default: {
			s.append("INTERNAL ERROR: unknown color: ", (u32)color);
			return;
		} break;
	}
	s.append("\e[49m");
}

} // namespace amu

#endif
