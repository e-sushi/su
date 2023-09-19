
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

    */

   instance.global_symbols = map::init<u8*, Var*>();

    instance.storage.sources             = pool::init<Source>(8);
    instance.storage.source_code         = pool::init<SourceCode>(8);
    instance.storage.virtual_code        = pool::init<VirtualCode>(8);
    instance.storage.lexers              = pool::init<Lexer>(8);
    instance.storage.parsers             = pool::init<Parser>(8);
    instance.storage.semas               = pool::init<Sema>(8);
    instance.storage.tac_gens            = pool::init<GenTAC>(8);
    instance.storage.air_gens            = pool::init<GenAIR>(8);
    instance.storage.vm                  = pool::init<VM>(8);
    instance.storage.modules             = pool::init<Module>(8);
    instance.storage.labels              = pool::init<Label>(8);
    instance.storage.virtual_labels      = pool::init<VirtualLabel>(8);
    instance.storage.structures          = pool::init<Structure>(8);
    instance.storage.members             = pool::init<Member>(8);
    instance.storage.functions           = pool::init<Function>(8);
    instance.storage.statements          = pool::init<Stmt>(8);
    instance.storage.expressions         = pool::init<Expr>(8);
    instance.storage.comp_times          = pool::init<CompileTime>(8);
    instance.storage.scalar_literals     = pool::init<ScalarLiteral>(8);
    instance.storage.string_literals     = pool::init<StringLiteral>(8);
    instance.storage.array_literals      = pool::init<ArrayLiteral>(8);
    instance.storage.tuple_literals      = pool::init<TupleLiteral>(8);
    instance.storage.calls               = pool::init<Call>(8);
    instance.storage.blocks              = pool::init<Block>(8);
    instance.storage.varrefs             = pool::init<VarRef>(8);
    instance.storage.fors                = pool::init<For>(8);
    instance.storage.vars                = pool::init<Var>(8);
    instance.storage.tuples              = pool::init<Tuple>(8);
    instance.storage.scalars             = pool::init<Scalar>(8);
    instance.storage.structured_types    = pool::init<Structured>(8);
    instance.storage.pointer_types       = pool::init<Pointer>(8);
    instance.storage.static_array_types  = pool::init<StaticArray>(8);
    instance.storage.view_array_types    = pool::init<ViewArray>(8);
    instance.storage.dynamic_array_types = pool::init<DynamicArray>(8);
    instance.storage.range_types         = pool::init<Range>(8);
    instance.storage.variant_types       = pool::init<Variant>(8);
    instance.storage.function_types      = pool::init<FunctionType>(8);
    instance.storage.tuple_types         = pool::init<TupleType>(8);
    instance.storage.meta_types          = pool::init<MetaType>(8);

    instance.options.deliver_debug_immediately = true;

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

global void
begin(Array<String> args) {
    internal::parse_arguments(args);

    // if we happen to exit early, we still want whatever is queued in the messenger
    // to be delivered
    defer {messenger::deliver();};

    if(!instance.options.entry_path.str){
        diagnostic::compiler::
            no_path_given(MessageSender::Compiler);
        return;
    }

    instance.options.verbosity = message::verbosity::debug;

    Source* entry_source = source::load(instance.options.entry_path);
    if(!entry_source) {
        diagnostic::path::
            not_found(MessageSender::Compiler, instance.options.entry_path);
        return;
    }

    entry_source->code = code::from(entry_source);
    
    auto lexer = Lexer::create(entry_source->code);
    lexer->start();
    
    if(instance.options.dump_tokens.path.str) {
        lexer->output(instance.options.dump_tokens.human, instance.options.dump_tokens.path);
        if(instance.options.dump_tokens.exit) return;
    }

    messenger::deliver();

    if(!Parser::create(entry_source->code)->parse()) return;
    
    messenger::deliver();

    if(!sema::analyze(entry_source->code)) return;

    messenger::deliver();
    
    GenTAC::create(entry_source->code)->generate();
    GenAIR::create(entry_source->code)->generate();

    VM::create(entry_source->code->last_child<Code>())
        ->run();

    if(instance.options.dump_diagnostics.path.str) {
        if(!internal::dump_diagnostics(instance.options.dump_diagnostics.path, instance.options.dump_diagnostics.sources)) return;
    }

}

b32
funnel(Code* code, code::level level) {
    while(1) {
        if(code->level >= level) return true;
        switch(code->level) {
            case code::none: {
                if(!code->lexer) Lexer::create(code);
                code->lexer->start();
                code->level = code::lex;
            } break;
            case code::lex: {
                if(!code->parser) Parser::create(code);
                if(!code->parser->parse()) return false;
                code->level = code::parse;
            } break;
            case code::parse: {
                if(!code->sema) code->sema = sema::create();
                if(!sema::analyze(code)) return false;
                code->level = code::sema;
            } break;
            case code::sema: {
                if(!code->tac_gen) GenTAC::create(code);
                code->tac_gen->generate();
                code->level = code::tac;
            } break;
            case code::tac: {
                if(!code->air_gen) GenAIR::create(code);
                code->air_gen->generate();
                code->level = code::air;
            } break;
            case code::air: {
                if(!code->machine) VM::create(code);
                code->machine->run();
                code->level = code::machine;
            } break;
        }
    }
}

} // namespace compiler
} // namespace amu