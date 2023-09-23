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
	auto srcopt = ncplane_options {
		.y = 0,
		.x = 0,
		.cols = height - 3,
		.rows = width / 2,
	};
	planes.src = ncplane_create(planes.top, &srcopt);
	
	refresh_src();

	auto cliopt = ncplane_options {
		.y = (int)(height - 2),
		.x = 0,
		.cols = width,
		.rows = 1,
	};
	planes.cli = ncplane_create(planes.top, &cliopt);


	auto readeropt = ncreader_options {
		.flags = NCREADER_OPTION_CURSOR
	};

	input = ncreader_create(planes.cli, &readeropt);

	ncinput in;

//	while(1) {
//		ncpile_render(planes.src);
//		notcurses_render(nc);
//		ncreader_offer_input(input, &in);
//
//	}

    notcurses_stop(nc);
}

void Debugger::
refresh_src() {
	auto height = ncplane_dim_y(planes.src);
	auto lines = code->raw.find_lines();
	forI(lines.count) {
		auto line = lines.read(i);
		ncplane_putnstr(planes.src, line.count, (char*)line.str);
		dbghelper::next_line(planes.src);
		if(i == height - 1) return;
	}
}


} // namespace amu
