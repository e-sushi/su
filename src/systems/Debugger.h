/*

    VM debugger system, or an attempt at one

*/

#ifndef AMU_DEBUGGER_H
#define AMU_DEBUGGER_H

namespace amu {

struct Debugger {
	Code* code;

	notcurses* nc;

	struct {
		ncplane* top;
		ncplane* src;
		ncplane* cli;
	} planes;

	ncreader* input;

	static Debugger*
	create(Code* code);

    void
    start();
	
	// refreshes the source view
	void
	refresh_src();
};


} // namespace amu

#endif // AMU_DEBUGGER_H
