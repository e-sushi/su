
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

    instance.storage.sources              = pool::init<Source>(32);
    instance.storage.source_code          = pool::init<SourceCode>(32);
    instance.storage.virtual_code         = pool::init<VirtualCode>(32);
    instance.storage.lexers               = pool::init<Lexer>(32);
    instance.storage.parsers              = pool::init<Parser>(32);
    instance.storage.semas           = pool::init<Sema>(32);
    instance.storage.generators           = pool::init<Generator>(32);
    instance.storage.modules              = pool::init<Module>(32);
    instance.storage.labels               = pool::init<Label>(32);
    instance.storage.structures           = pool::init<Structure>(32);
    instance.storage.functions            = pool::init<Function>(32);
    instance.storage.statements           = pool::init<Statement>(32);
    instance.storage.expressions          = pool::init<Expression>(32);
    instance.storage.call_expressions     = pool::init<CallExpression>(32);
    instance.storage.block_expressions    = pool::init<BlockExpression>(32);
    instance.storage.placeref_expressions = pool::init<PlaceRefExpression>(32);
    instance.storage.places               = pool::init<Place>(32);
    instance.storage.tuples               = pool::init<Tuple>(32);
    instance.storage.types                = pool::init<Type>(32);
    instance.storage.builtin_types        = pool::init<ScalarType>(32);
    instance.storage.structured_types     = pool::init<StructuredType>(32);
    instance.storage.pointer_types        = pool::init<PointerType>(32);
    instance.storage.array_types          = pool::init<ArrayType>(32);
    instance.storage.variant_types        = pool::init<VariantType>(32);
    instance.storage.function_types       = pool::init<FunctionType>(32);
    instance.storage.tuple_types          = pool::init<TupleType>(32);
    instance.storage.meta_types           = pool::init<MetaType>(32);

    instance.options.deliver_debug_immediately = true;

    messenger::init();  // TODO(sushi) compiler arguments to control this
    array::push(messenger::instance.destinations, Destination(stdout, (isatty(1)? true : false))); // TODO(sushi) isatty throws a warning on win32, make this portable 
    array::push(messenger::instance.destinations, Destination(fopen("temp/log", "w"), false));

    module = module::create();
}

global void
deinit() {} // TODO(sushi)

namespace internal {

b32 parse_arguments(Array<String> args) {
    for(s32 i = 1; i < args.count; i++) {
        String arg = array::read(args, i);
        u64 hash = string::hash(arg);
        switch(hash) {

            case string::static_hash("-q"): {
                instance.options.quiet = true;
            } break;

            case string::static_hash("--dump-tokens"): {
                while(i != args.count-1) {
                    arg = array::read(args, ++i);
                    if(string::equal(arg, "-human")) {
                        instance.options.dump_tokens.human = true;
                    }else if(string::equal(arg, "-exit")) {
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
                arg = array::read(args, ++i);
                if(string::equal(arg, "-source")) {
                    instance.options.dump_diagnostics.sources = array::init<String>();
                    arg = array::read(args, ++i);
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
                            array::push(instance.options.dump_diagnostics.sources, curt);
                            curt.str = arg.str + i + 1;
                            if(arg.str[i+1] == ',') curt.str++;
                            curt.count = 0;
                        }
                    }
                    arg = array::read(args, ++i);
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

b32 dump_diagnostics(String path, Array<String> sources) {
    FILE* out = fopen((char*)path.str, "w");
    if(sources.count) {
        NotImplemented; // TODO(sushi) selective diag dump
        // forI(sources.count){
        //     Source* s = lookup_source(array::read(sources, i));
        //     if(!s) return false;
            
        // }
    }

    struct DiagnosticEntry {
        u64 source_offset;
        Diagnostic diag;
    };
    
    auto source_table = array::init<Source*>();
    auto diagnostics = array::init<DiagnosticEntry>();;

    pool::Iterator<Source> iter = pool::iterator(instance.storage.sources);
    
    DString source_strings = dstring::init();

    Source* current = 0;
    while((current = pool::next(iter))) {
        if(!current->diagnostics.count) continue;
        array::push(source_table, current);
        u64 source_offset = sizeof(u64)+source_strings.count;
        forI(current->diagnostics.count) {
            array::push(diagnostics, 
                {source_offset, array::read(current->diagnostics, i)});
        }
        dstring::append(source_strings, '"', current->path, '"');
    }

    if(!diagnostics.count){
        int bleh = 0;
        fwrite(&bleh, sizeof(s64), 1, out);
        return true;
    } 

    fwrite(&source_strings.count, sizeof(s64), 1, out);
    fwrite(source_strings.str, source_strings.count, 1, out);
    fwrite(diagnostics.data, diagnostics.count*sizeof(DiagnosticEntry), 1, out);

    return true;
}

} // namespace internal

global void
begin(Array<String> args) {
    internal::parse_arguments(args);

    if(!instance.options.entry_path.str){
        diagnostic::compiler::
            no_path_given(MessageSender::Compiler);
        messenger::deliver();
        return;
    }

    instance.options.verbosity = message::verbosity::debug;

    Source* entry_source = source::load(instance.options.entry_path);
    if(!entry_source) {
        diagnostic::path::
            not_found(MessageSender::Compiler, instance.options.entry_path);
        messenger::deliver();
        return;
    }

    entry_source->code = code::from(entry_source);
    
    entry_source->code->lexer = lex::create();
    lex::execute(entry_source->code);
    
    if(instance.options.dump_tokens.path.str) {
        lex::output(entry_source->code, instance.options.dump_tokens.human, instance.options.dump_tokens.path);
        if(instance.options.dump_tokens.exit) {
            messenger::deliver();
            return;
        }
    }

    messenger::deliver();

    parser::parse(entry_source->code);
    
    messenger::deliver();

    sema::analyze(entry_source->code);

    messenger::deliver();

    if(instance.options.dump_diagnostics.path.str) {
        if(!internal::dump_diagnostics(instance.options.dump_diagnostics.path, instance.options.dump_diagnostics.sources)) return;
    }

}

} // namespace compiler
} // namespace amu