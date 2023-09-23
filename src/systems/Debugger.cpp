#include "notcurses.h"
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
	return out;
}

void Debugger::
start() {
    setlocale(LC_ALL, "en_US.UTF-8");
    notcurses_options opt{
        .flags = 0,
    };

    nc = notcurses_init(&opt, stdout);
    if(!nc) { 
        util::println("notcurses failed to initialize");
		return;
	}
		
	u32 width, height;
	notcurses_term_dim_yx(nc, &height, &width);

	planes.top = notcurses_top(nc);	

	// plane for showing source code
	// takes up half the screen
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
	planes.com = ncplane_create(planes.top, &comopt);
	ncplane_set_scrolling(planes.com, true);


	auto cliopt = ncplane_options {
		.y = (int)(height - 2),
		.x = 0,
		.cols = width,
		.rows = 1,
	};
	planes.cli = ncplane_create(planes.top, &cliopt);


	auto infoopt = ncplane_options {
		.y = 0,
		.x = (int)(width / 2),
		.cols = width / 2,
		.rows = height - 3,
	};
	planes.info = ncplane_create(planes.top, &infoopt);


	auto readeropt = ncreader_options {
		.flags = NCREADER_OPTION_CURSOR
	};

	input = ncreader_create(planes.cli, &readeropt);

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
			parse_command(line);
		}
	}

    notcurses_stop(nc);
}

void Debugger::
refresh_src(BC* instr) {
	auto height = ncplane_dim_y(planes.src);
	auto lines = code->raw.find_lines();
	ncplane_erase(planes.src);

	if(instr) {
		auto start_line_num = util::Max((s64)instr->node->start->l0 - height / 2, 0);
		forI(height) {
			auto s = lines.read(i + start_line_num);
			ncplane_putnstr(planes.src, s.count, (char*)s.str);
			dbghelper::next_line(planes.src);
			if(i == lines.count-1) break;
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
parse_command(String s) {
	auto word = s.eat_word();
	u64 hash = word.hash();
	switch(hash) {
		case string::static_hash("n"):{
			last_instr = vm->step();
			refresh_src(last_instr);
		} break;
		case string::static_hash("nl"): {
			auto last_line_num = vm->frame.ip->node->start->l0;
			while(1) {
				last_instr = vm->step();
				if(!last_instr || last_instr->node->start->l0 != last_line_num) {
					refresh_src(last_instr);
					break;
				}
			}
		} break;

	}
	ncreader_clear(input);
}


} // namespace amu
