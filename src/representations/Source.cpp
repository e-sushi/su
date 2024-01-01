#include "Source.h"
#include "systems/Compiler.h"
#include "storage/String.h"
#include "storage/DString.h"
#include <filesystem>

namespace amu {

Source* Source::
load(String path) {
    std::filesystem::path p = (char*)path.str;
    if(!std::filesystem::exists(p)) return 0;

    Source* out = new (compiler.bump.allocate(sizeof(Source))) Source;
    std::filesystem::path ab = std::filesystem::absolute(p);
    out->path = String::from(ab.c_str()).replace('\\', '/');

	String view = out->path.get_string();

		
	u8* scan = view.str + view.count;

	b32 found_dot = 0;
	s32 namelen = 0;
	s32 extlen = 0;

	while(1) {
		if(scan == view.str) break; 

		if(*scan == '/') break;
		namelen++;

		if(!found_dot) {
			if(*scan == '.') {
				found_dot = true; 
			} else {
				extlen++;
			}
		}
	}

	out->name = {view.str + view.count - namelen, namelen};
	out->front = {out->name.str + namelen - extlen, extlen};
	out->file = fopen((char*)out->path.get_string().str, "r");

	if(!out->file) {
		FATAL(MessageSender(), "Failed to open file '", out->path, "'");
		return 0;
	}
	
	upt file_size = std::filesystem::file_size(p);

	u8* buffer = (u8*)memory.allocate(file_size + 1);
	fread(buffer, file_size, 1, out->file);
	out->buffer.str = buffer;
	out->buffer.count = file_size;

	fclose(out->file);

	return out;
}

void Source::
unload() {
	FixMe;
}

// TODO(sushi) we can store a map String -> Source* and do this more efficiently
Source* Source::
lookup(String display) {
	FixMe;
	return 0;
}

} // namespace amu
