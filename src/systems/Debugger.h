/*

    VM debugger system, or an attempt at one

*/

#ifndef AMU_DEBUGGER_H
#define AMU_DEBUGGER_H

namespace amu {

struct DbgCmd {
	String name;
	String desc;
	
	void (*action)(String args) = 0;

	// TODO(sushi) commands with args
};

// idea to come back to later whenever the debugger layout 
// should become more flexible
struct DebuggerWindow {
	TNode node;
	// whether this window displays its children horizontally
	b32 horizontal;
	
	b32 flex;
	f32 ratio;

	u32 width,height;
	
	ncplane* plane;
};

struct Debugger {
	Code* code;

	notcurses* nc;

	struct {
		ncplane* top;
		ncplane* src;
		ncplane* command_line;
		ncplane* console;
		ncplane* info;
	} planes;


	ncreader* input;

	VM* vm;
	BC* last_instr;

	b32 finished;
	
	// the last command entered into the command line
	// pressing Enter with an empty command line performs
	// the last command again
	String last_command;
	
	// collection of commands usable in the debugger
	Map<String, DbgCmd> cmds;

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
	refresh_info();

	void
	parse_command(String s);

	void	
	refresh_windows();
};




} // namespace amu

#endif // AMU_DEBUGGER_H
