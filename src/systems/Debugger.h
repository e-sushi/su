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
		ncplane* com;
		ncplane* cli;
		ncplane* info;
	} planes;

	ncreader* input;

	VM* vm;
	BC* last_instr;

	static Debugger*
	create(Code* code);

    void
    start();
	
	// refreshes the source view
	// if 'instr' is provided then the src view
	// will center on the line containing it and
	// will highlight it
	void
	refresh_src(BC* instr = 0);

	void
	parse_command(String s);
};


} // namespace amu

#endif // AMU_DEBUGGER_H
