/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

#include "storage/Bump.h"
#include "Messenger.h"
#include "Diagnostics.h"

namespace amu {

struct Compiler {
    // anything that is specified on the command line
    // later on all of these things should be changable from within the language
    struct {
        String entry_path;

		Message::Kind verbosity;

        b32 deliver_debug_immediately;
		b32 deliver_all_immediately;

		b32 break_on_error;

        b32 quiet;

        struct{
            String path;
            b32 exit;
            b32 human;
        }dump_tokens;

        struct{
            String path;
            Array<String> sources;
        }dump_diagnostics; 
    } options;

	// Central bump allocator. Primarily for Code and Source objects.
	Bump bump;

	// diagnostics emitted by the compiler itself
	Array<Diag> diags;

	void
	push_diag(Diag d);

	static Compiler
	create();

	void 
	init();

	void
	deinit();

	void
	destroy();

	b32
	begin(Array<String> args);

	b32
	parse_arguments(Array<String> args);

	b32
	dump_diagnostics(String path, Array<String> sources);
};

extern Compiler compiler;

} // namespace amu

#endif // AMU_COMPILER_H
