
#include "processors/Lexer.h"
#include "systems/Messenger.h"
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;
Module* module;

void
init() {
    /*
        Initialize most components of the singleton Compiler instance.
    */
    instance.compiler_time = util::stopwatch::start();

    instance.log_file = fopen("temp/log", "w");

    /*

        This is where all persistent memory lives. Anything that needs to exist and not move
        is stored in a pool stored on the Compiler instance. The majority of things put
        here will never be deleted. 

		TODO(sushi) see Compiler.h on why this needs to be redone 

    */

    instance.global_symbols = Map<u8*, Var*>::create();

    instance.storage.sources             = Pool<Source>::create(8);
    instance.storage.source_code         = Pool<SourceCode>::create(8);
    instance.storage.virtual_code        = Pool<VirtualCode>::create(8);
    instance.storage.lexers              = Pool<Lexer>::create(8);
	instance.storage.lexical_scopes      = Pool<LexicalScope>::create(8);
    instance.storage.parsers             = Pool<Parser>::create(8);
    instance.storage.semas               = Pool<Sema>::create(8);
    instance.storage.tac_gens            = Pool<GenTAC>::create(8);
    instance.storage.air_gens            = Pool<GenAIR>::create(8);
    instance.storage.vm                  = Pool<VM>::create(8);
    instance.storage.modules             = Pool<Module>::create(8);
    instance.storage.labels              = Pool<Label>::create(8);
	instance.storage.label_tables        = Pool<LabelTable>::create(8);
    instance.storage.virtual_labels      = Pool<VirtualLabel>::create(8);
    instance.storage.structures          = Pool<Structure>::create(8);
    instance.storage.members             = Pool<Member>::create(8);
    instance.storage.functions           = Pool<Function>::create(8);
    instance.storage.statements          = Pool<Stmt>::create(8);
    instance.storage.expressions         = Pool<Expr>::create(8);
    instance.storage.comp_times          = Pool<CompileTime>::create(8);
    instance.storage.scalar_literals     = Pool<ScalarLiteral>::create(8);
    instance.storage.string_literals     = Pool<StringLiteral>::create(8);
    instance.storage.array_literals      = Pool<ArrayLiteral>::create(8);
    instance.storage.tuple_literals      = Pool<TupleLiteral>::create(8);
    instance.storage.calls               = Pool<Call>::create(8);
    instance.storage.blocks              = Pool<Block>::create(8);
    instance.storage.fors                = Pool<For>::create(8);
    instance.storage.vars                = Pool<Var>::create(8);
    instance.storage.tuples              = Pool<Tuple>::create(8);
    instance.storage.scalars             = Pool<Scalar>::create(8);
    instance.storage.structured_types    = Pool<Structured>::create(8);
    instance.storage.pointer_types       = Pool<Pointer>::create(8);
    instance.storage.static_array_types  = Pool<StaticArray>::create(8);
    instance.storage.view_array_types    = Pool<ViewArray>::create(8);
    instance.storage.dynamic_array_types = Pool<DynamicArray>::create(8);
    instance.storage.range_types         = Pool<Range>::create(8);
    instance.storage.variant_types       = Pool<Variant>::create(8);
    instance.storage.function_types      = Pool<FunctionType>::create(8);
    instance.storage.tuple_types         = Pool<TupleType>::create(8);
	instance.storage.module_types        = Pool<ModuleType>::create(8);
    instance.storage.meta_types          = Pool<MetaType>::create(8);
	instance.storage.debuggers           = Pool<Debugger>::create(8);

    instance.options.deliver_debug_immediately = true;
	//instance.options.deliver_all_immediately = true;
	//instance.options.break_on_error = true;

    messenger::init();  // TODO(sushi) compiler arguments to control this
    messenger::instance.destinations.push(Destination(stdout, (isatty(1)? true : false))); // TODO(sushi) isatty throws a warning on win32, make this portable 
    messenger::instance.destinations.push(Destination(fopen("temp/log", "w"), false));

    module = Module::create();
}

global void
deinit() {} // TODO(sushi)

namespace internal {

b32 
parse_arguments(Array<String> args) {
    for(s32 i = 1; i < args.count; i++) {
        String arg = args.read(i);
        u64 hash = arg.hash();
        switch(hash) {

            case string::static_hash("-q"): {
                instance.options.quiet = true;
            } break;

            case string::static_hash("--dump-tokens"): {
                while(i != args.count-1) {
                    arg = args.read(++i);
                    if(arg.equal("-human")) {
                        instance.options.dump_tokens.human = true;
                    }else if(arg.equal("-exit")) {
                        instance.options.dump_tokens.exit = true;
                    } else break;
                }
                if(arg.str[0] == '-') {
                    diagnostic::compiler::
                        expected_a_path_for_arg(MessageSender::Compiler, "--dump-tokens");
                    messenger::deliver();
                    return false;
                }
                instance.options.dump_tokens.path = arg;
            } break;

            case string::static_hash("--dump-diagnostics"): {
                arg = args.read(++i);
                if(arg.equal("-source")) {
                    instance.options.dump_diagnostics.sources = Array<String>::create();
                    arg = args.read(++i);
                    if(arg.str[0] == '-') {
                        diagnostic::compiler::
                            expected_path_or_paths_for_arg_option(MessageSender::Compiler, "--dump-diagnostics -source");
                        messenger::deliver();
                        return false;
                    }
                    String curt = arg;
                    curt.count = 0;
                    forI(arg.count) {
                        curt.count++;
                        if(i == arg.count-1 || arg.str[i] == ' ' || arg.str[i+1] == ',') {
                            instance.options.dump_diagnostics.sources.push(curt);
                            curt.str = arg.str + i + 1;
                            if(arg.str[i+1] == ',') curt.str++;
                            curt.count = 0;
                        }
                    }
                    arg = args.read(++i);
                }
                if(arg.str[0] == '-') {
                    diagnostic::compiler::
                        expected_a_path_for_arg(MessageSender::Compiler, "--dump-diagnostics");
                    messenger::deliver();
                    return false;
                }
                instance.options.dump_diagnostics.path = arg;
            } break;

            default: {
                if(arg.str[0] == '-') {
                    diagnostic::compiler::
                        unknown_option(MessageSender::Compiler, arg);
                    messenger::deliver();
                    return false;
                }
                // otherwise this is (hopefully) a path
                instance.options.entry_path = arg;
            } break;
        }
    }

    return true;
} 

b32 
dump_diagnostics(String path, Array<String> sources) {
    FILE* out = fopen((char*)path.str, "w");
    if(sources.count) {
        NotImplemented; // TODO(sushi) selective diag dump
        // forI(sources.count){
        //     Source* s = lookup_source(sources.read(i));
        //     if(!s) return false;
            
        // }
    }

    struct DiagnosticEntry {
        u64 source_offset;
        Diagnostic diag;
    };
    
    auto source_table = Array<Source*>::create();
    auto diagnostics = Array<DiagnosticEntry>::create();;

    pool::Iterator<Source> iter = pool::iterator(instance.storage.sources);
    
    DString* source_strings = DString::create();

    Source* current = 0;
    while((current = pool::next(iter))) {
        if(!current->diagnostics.count) continue;
        source_table.push(current);
        u64 source_offset = sizeof(u64)+source_strings->count;
        forI(current->diagnostics.count) {
            diagnostics.push(
                {source_offset, current->diagnostics.read(i)});
        }
        source_strings->append('"', current->path, '"');
    }

    if(!diagnostics.count){
        int bleh = 0;
        fwrite(&bleh, sizeof(s64), 1, out);
        return true;
    } 

    fwrite(&source_strings->count, sizeof(s64), 1, out);
    fwrite(source_strings->str, source_strings->count, 1, out);
    fwrite(diagnostics.data, diagnostics.count*sizeof(DiagnosticEntry), 1, out);

    return true;
}

} // namespace internal

global b32
begin(Array<String> args) {
    internal::parse_arguments(args);

    // if we happen to exit early, we still want whatever is queued in the messenger
    // to be delivered
    defer {messenger::deliver();};

    if(!instance.options.entry_path.str){
        diagnostic::compiler::
            no_path_given(MessageSender::Compiler);
        return false;
    }

    instance.options.verbosity = message::verbosity::stages;

    Source* entry_source = source::load(instance.options.entry_path);
    if(!entry_source) {
        diagnostic::path::
            not_found(MessageSender::Compiler, instance.options.entry_path);
        return false;
    }
	
    entry_source->code = Code::from(entry_source);

	// likely the filename isn't able to be used as an identifier
	if(!entry_source->code) return false;

	if(!entry_source->code->process_to(code::lex)) return false;
    messenger::deliver();
    
	if(instance.options.dump_tokens.path.str) {
        entry_source->code->lexer->
			output(instance.options.dump_tokens.human, instance.options.dump_tokens.path);
        if(instance.options.dump_tokens.exit) return true;
    }

	if(!entry_source->code->process_to(code::parse)) return false;
    messenger::deliver();

	if(!entry_source->code->process_to(code::sema)) return false;
    messenger::deliver();

	if(!entry_source->code->process_to(code::tac)) return false;
	messenger::deliver();

	if(!entry_source->code->process_to(code::air)) return false;
	messenger::deliver();

	// Debugger::create(entry_source->code->last_child<Code>())->start();

    //VM::create(entry_source->code->last_child<Code>())
    //    ->run();

    if(instance.options.dump_diagnostics.path.str) {
        if(!internal::dump_diagnostics(instance.options.dump_diagnostics.path, instance.options.dump_diagnostics.sources)) return false;
    }

	return true;
}

} // namespace compiler
} // namespace amu
