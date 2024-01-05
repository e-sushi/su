
#include "processors/Lexer.h"
#include "systems/Messenger.h"
#include "storage/Array.h"
#include "storage/String.h"
#include "representations/Code.h"
#include "Compiler.h"

namespace amu {

// global compiler instance
Compiler compiler;

void Compiler::
init() {
	bump.init();
	DString::init_allocator();
	messenger.init();
	options.verbosity = Message::Kind::Trace;
	messenger.destinations.push(Destination(stdout));

}

void Compiler::
deinit() {
	bump.deinit();
	DString::deinit_allocator();
} 

b32 Compiler::
parse_arguments(Array<String> args) {
	TRACE(MessageSender(), "parsing arguments");
    for(s32 i = 1; i < args.count; i++) {
        String arg = args.read(i);
		TRACE(MessageSender(), "argument: ", arg);
        u64 hash = arg.hash();
        switch(hash) {

			case util::static_string_hash("-q"): {
                options.quiet = true;
            } break;

            case util::static_string_hash("--dump-tokens"): {
                while(i != args.count-1) {
                    arg = args.read(++i);
                    if(arg.equal("-human")) {
                        options.dump_tokens.human = true;
                    }else if(arg.equal("-exit")) {
                        options.dump_tokens.exit = true;
                    } else break;
                }
                if(arg.str[0] == '-') {
					Diag::expected_a_path_for_arg(diags, MessageSender(), "--dump-tokens");
                    return false;
                }
                options.dump_tokens.path = arg;
            } break;

            case util::static_string_hash("--dump-diagnostics"): {
                arg = args.read(++i);
                if(arg.equal("-source")) {
                    compiler.options.dump_diagnostics.sources = Array<String>::create();
                    arg = args.read(++i);
                    if(arg.str[0] == '-') {
						Diag::expected_path_or_paths_for_arg_option(diags, MessageSender(), "--dump-diagnostics -source");
                        return false;
                    }
                    String curt = arg;
                    curt.count = 0;
                    forI(arg.count) {
                        curt.count++;
                        if(i == arg.count-1 || arg.str[i] == ' ' || arg.str[i+1] == ',') {
                            options.dump_diagnostics.sources.push(curt);
                            curt.str = arg.str + i + 1;
                            if(arg.str[i+1] == ',') curt.str++;
                            curt.count = 0;
                        }
                    }
                    arg = args.read(++i);
                }
                if(arg.str[0] == '-') {
					Diag::expected_a_path_for_arg(diags, MessageSender(), "--dump-diagnostics");
                    return false;
                }
                options.dump_diagnostics.path = arg;
            } break;

            default: {
                if(arg.str[0] == '-') {
					Diag::unknown_option(diags, MessageSender(), arg);
                    return false;
                }
                // otherwise this is (hopefully) a path
                options.entry_path = arg;
            } break;
        }
    }

    return true;
} 

b32 Compiler::
dump_diagnostics(String path, Array<String> sources) {
	NotImplemented;
	return false;
   // FILE* out = fopen((char*)path.str, "w");
   // if(sources.count) {
   //     NotImplemented; // TODO(sushi) selective diag dump
   //     // forI(sources.count){
   //     //     Source* s = lookup_source(sources.read(i));
   //     //     if(!s) return false;
   //         
   //     // }
   // }

   // struct DiagnosticEntry {
   //     u64 source_offset;
   //     Diagnostic diag;
   // };
   // 
   // auto source_table = Array<Source*>::create();
   // auto diagnostics = Array<DiagnosticEntry>::create();;

   // pool::Iterator<Source> iter = pool::iterator(instance.storage.sources);
   // 
   // DString* source_strings = DString::create();

   // Source* current = 0;
   // while((current = pool::next(iter))) {
   //     if(!current->diagnostics.count) continue;
   //     source_table.push(current);
   //     u64 source_offset = sizeof(u64)+source_strings->count;
   //     forI(current->diagnostics.count) {
   //         diagnostics.push(
   //             {source_offset, current->diagnostics.read(i)});
   //     }
   //     source_strings->append('"', current->path, '"');
   // }

   // if(!diagnostics.count){
   //     int bleh = 0;
   //     fwrite(&bleh, sizeof(s64), 1, out);
   //     return true;
   // } 

   // fwrite(&source_strings->count, sizeof(s64), 1, out);
   // fwrite(source_strings->str, source_strings->count, 1, out);
   // fwrite(diagnostics.data, diagnostics.count*sizeof(DiagnosticEntry), 1, out);

   // return true;
}

b32 Compiler::
begin(Array<String> args) {
	TRACE(MessageSender(), "compiler begin");
	parse_arguments(args);

    // if we happen to exit early, we still want whatever is queued in the messenger
    // to be delivered
    defer {messenger.deliver();};

    if(!options.entry_path.str){
		Diag::no_path_given(MessageSender()).emit();
        return false;
    }

    Source* entry_source = Source::load(options.entry_path);
    if(!entry_source) {
		Diag::path_not_found(MessageSender(), options.entry_path).emit();
        return false;
    }
	
    entry_source->code = Code::from(entry_source);

	// likely the filename isn't able to be used as an identifier
	if(!entry_source->code) return false;

	if(!entry_source->code->process_to(Code::Stage::Lex)) return false;
    messenger.deliver();

	return true;
    
	if(options.dump_tokens.path.str) {
        entry_source->code->lexer->
			output(options.dump_tokens.human, options.dump_tokens.path);
        if(options.dump_tokens.exit) return true;
    }

//	if(!entry_source->code->process_to(Code::Stage::Parse)) return false;
//    messenger.deliver();
//
//	if(!entry_source->code->process_to(Code::Stage::Sema)) return false;
//    messenger.deliver();
//
//	if(!entry_source->code->process_to(Code::Stage::TAC)) return false;
//	messenger.deliver();
//
//	if(!entry_source->code->process_to(Code::Stage::AIR)) return false;
//	messenger.deliver();

	// Debugger::create(entry_source->code->last_child<Code>())->start();

    //VM::create(entry_source->code->last_child<Code>())
    //    ->run();

    if(options.dump_diagnostics.path.str) {
        if(!dump_diagnostics(options.dump_diagnostics.path, options.dump_diagnostics.sources)) return false;
    }

	return true;
}

} // namespace amu
