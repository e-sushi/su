#include "notcurses.h"
#include "representations/AIR.h"
namespace amu {

namespace dbghelper {

void
next_line(ncplane* p) {
	ncplane_cursor_move_rel(p, 1, 0);
	ncplane_cursor_move_yx(p, -1, 0);
}

} // namespace internal


Debugger* Debugger::
create(Code* code) {
	auto out = pool::add(compiler::instance.storage.debuggers);
	out->code = code;

    setlocale(LC_ALL, "en_US.UTF-8");
	
	auto ncopt = notcurses_options {};
	out->nc = notcurses_init(&ncopt, stdout);
	if(!out->nc) {
		util::println("notcurses failed to initialize");
		return 0;
	}
	return out;
}

void Debugger::
start() {
	planes.top = notcurses_top(nc);	

	u32 width, height;

	ncplane_dim_yx(planes.top, &height, &width);

	// plane for showing source code
	auto srcopt = ncplane_options {
		.y = 0,
		.x = 0,
		.cols = width / 2,
		.rows = height / 2,
	};
	u32 bgchan = 0;
	u32 fgchan = 0;
	ncchannel_set_rgb8(&bgchan, 0x20, 0x20, 0x20);
	ncchannel_set_rgb8(&fgchan, 0xff, 0xff, 0xff);
	planes.src = ncplane_create(planes.top, &srcopt);
	if(!ncplane_set_base(planes.src, " ", 0,
			ncchannels_combine(fgchan, bgchan))){
		util::println("failed to set base of src");
		return;
	}

	refresh_src();

	// plane for command output
	// takes up half the screen minus the room
	// we leave for the actual command line
	auto comopt = ncplane_options {
		.y = (int)(height / 2),
		.x = 0,
		.cols = width,
		.rows = height / 2 - 3,
	};
	planes.console = ncplane_create(planes.top, &comopt);
	ncplane_set_scrolling(planes.console, true);

	auto cliopt = ncplane_options {
		.y = (int)(height - 2),
		.x = 0,
		.cols = width,
		.rows = 1,
	};
	planes.command_line = ncplane_create(planes.top, &cliopt);

	auto infoopt = ncplane_options {
		.y = 0,
		.x = (int)(width / 2),
		.cols = width / 2,
		.rows = height - 3,
	};
	planes.info = ncplane_create(planes.top, &infoopt);
	ncplane_set_scrolling(planes.info, true);

	auto readeropt = ncreader_options {
		.flags = NCREADER_OPTION_CURSOR
	};
	input = ncreader_create(planes.command_line, &readeropt);

	ncinput in;

	vm = VM::create(code);

	while(1) {
		ncpile_render(planes.src);
		notcurses_render(nc);
		notcurses_get_blocking(nc, &in);
		ncreader_offer_input(input, &in);

		String line;
		line.str = (u8*)ncreader_contents(input);
		line.count = strlen((char*)line.str);
		if(in.id == NCKEY_ENTER) {
			if(!line.count) {
				parse_command(last_command);
			} else {
				parse_command(line);
				last_command = line;
			}
			refresh_info();
		}
	}

    notcurses_stop(nc);
}

void Debugger::
refresh_src(BC* instr) {
	auto height = ncplane_dim_y(planes.src);
	auto lines = code->source->buffer.find_lines();
	ncplane_erase(planes.src);

	if(instr) {
		auto start_line_num = util::Max((s64)instr->node->start->l0 - height / 2, 0);
		forI(height) {
			auto line_num = i + start_line_num;
			auto s = lines.read(line_num);
			ncplane_putnstr(planes.src, s.count, (char*)s.str);
			dbghelper::next_line(planes.src);
			if(line_num == lines.count-1) break;
		}
		
		u32 bg = 0;
		u32 fg = 0;
		ncchannel_set_rgb8(&bg, 0x88, 0x22, 0x22);
		ncchannel_set_rgb8(&fg, 0xff, 0xff, 0xff);
		auto channel = ncchannels_combine(fg, bg);
		
		ncplane_stain(planes.src, instr->node->start->l0 - 1 - start_line_num, 0, 1, ncplane_dim_x(planes.src), channel, channel, channel, channel);
	} else {
		forI(lines.count) {
			auto line = lines.read(i);
			ncplane_putnstr(planes.src, line.count, (char*)line.str);
			dbghelper::next_line(planes.src);
			if(i == height) return;
		}
	}
}

void Debugger::
refresh_info() {
	auto out = DString::create(
		"sp: ", vm->sp - vm->frame.fp, "\n"
	);

	forI(vm->frame.locals.count) {
		auto loc = vm->frame.locals.read(i);
		auto val = loc->type->print_from_address(vm->frame.fp + loc->stack_offset);
		out->append(ScopedDeref(loc->display()).x, "(", loc->stack_offset, "): ", ScopedDeref(val).x, "\n");
	}

	ncplane_erase(planes.info);
	ncplane_putnstr(planes.info, out->count, (char*)out->str);

	out->deref();
}

void Debugger::
parse_command(String s) {
	auto word = s.eat_word();
	switch(word.hash()) {
		case string::static_hash("n"):{
			last_instr = vm->step();
			refresh_src(last_instr);
			auto out = DString::create(
				"next instruction: \n\t",
				to_string(*last_instr) , "\n"
			);
			ncplane_putnstr(planes.console, out->count, (char*)out->str);
		} break;
		case string::static_hash("nl"): {
			auto last_line_num = vm->frame.ip->node->start->l0;
			u64 count = 0;
			while(1) {
				last_instr = vm->step();
				count++;
				if(!last_instr || last_instr->node->start->l0 != last_line_num) {
					refresh_src(last_instr);
					auto out = DString::create("stepped to next line (", count, " instructions)\n");
					ncplane_putnstr(planes.console, out->count, (char*)out->str);
					out->deref();
					break;
				}
			}
		} break;
		case string::static_hash("c"): {
			last_instr = vm->run();
			if(!last_instr) {
				auto out = String("program finished");
				ncplane_putnstr(planes.console, out.count, (char*)out.str);
				finished = true;
			} else {
				auto out = String("hit vm breakpoint\n");
				ncplane_putnstr(planes.console, out.count, (char*)out.str);
				refresh_src(last_instr);
			}
		} break;
		case string::static_hash("cw"): {
			while(1) {
				last_instr = vm->step();
				if(!last_instr) {
					auto out = String("program finished");
					ncplane_putnstr(planes.console, out.count, (char*)out.str);
					finished = true;
					return;
				}
				if(last_instr->instr == air::vm_break) {
					auto out = String("hit vm breakpoint");
					ncplane_putnstr(planes.console, out.count, (char*)out.str);
					refresh_src(last_instr);
					break;
				}
				refresh_src(last_instr);
				ncpile_render(planes.src);
				notcurses_render(nc);

			}
		} break;
		case string::static_hash("sl"): {
			auto last_line_num = vm->frame.ip->node->start->l0;
			u64 count = 0;
			while(1) {
				last_instr = vm->step();
				count++;
				if(last_instr->instr == air::call) {
					// skip until the next return
					u64 layers = 1;
					while(layers) {
						last_instr = vm->step();
						// there should never be a call to a function that doesn't end with a ret
						// so if we somehow run out of instructions something went wrong in
						// generation
						if(!last_instr) Assert(0);
						if(last_instr->instr == air::call) {
							layers++;
						} else if(last_instr->instr == air::ret) {
							layers--;
						}
					}
					last_instr = vm->step();
				}
				if(last_instr->node->start->l0 != last_line_num) {
					refresh_src(last_instr);
					auto out = DString::create("skipped line (", count, " instructions )\n");
					ncplane_putnstr(planes.console, out->count, (char*)out->str);
					out->deref();
					break;
				}
			}
		} break;
		case string::static_hash("r"): {
			vm->destroy();
			vm = VM::create(code);
			refresh_src(vm->frame.ip);
		} break;
	}
	ncreader_clear(input);
}

void Debugger::
refresh_windows() {
	
}


} // namespace amu
