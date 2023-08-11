
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;

void
init() {
    instance.compiler_time = util::stopwatch::start();

    instance.log_file = fopen("temp/log", "w");

    instance.storage.sources     = pool::init<Source>(32);
    instance.storage.lexers      = pool::init<Lexer>(32);
    instance.storage.parsers     = pool::init<Parser>(32);
    instance.storage.modules     = pool::init<Module>(32);
    instance.storage.labels      = pool::init<Label>(32);
    instance.storage.structures  = pool::init<Structure>(32);
    instance.storage.functions   = pool::init<Function>(32);
    instance.storage.statements  = pool::init<Statement>(32);
    instance.storage.expressions = pool::init<Expression>(32);
    instance.storage.places      = pool::init<Place>(32);
    instance.storage.tuples      = pool::init<Tuple>(32);
    instance.storage.types       = pool::init<Type>(32);

    instance.options.deliver_debug_immediately = true;

    compiler::builtins.void_ = structure::create();
    compiler::builtins.void_->size = 0;
    compiler::builtins.unsigned8 = structure::create();
    compiler::builtins.unsigned8->size = 1;
    compiler::builtins.unsigned16 = structure::create();
    compiler::builtins.unsigned16->size = 2;
    compiler::builtins.unsigned32 = structure::create();
    compiler::builtins.unsigned32->size = 4;
    compiler::builtins.unsigned64 = structure::create();
    compiler::builtins.unsigned64->size = 8;
    compiler::builtins.signed8 = structure::create();
    compiler::builtins.signed8->size = 1;
    compiler::builtins.signed16 = structure::create();
    compiler::builtins.signed16->size = 2;
    compiler::builtins.signed32 = structure::create();
    compiler::builtins.signed32->size = 4;
    compiler::builtins.signed64 = structure::create();
    compiler::builtins.signed64->size = 8;
    compiler::builtins.float32 = structure::create();
    compiler::builtins.float32->size = 4;
    compiler::builtins.float64 = structure::create();
    compiler::builtins.float64->size = 8;

    compiler::builtins.array = structure::create();
    compiler::builtins.array->size = 2*sizeof(s64);
    map::add(compiler::builtins.array->members, String("data"), compiler::builtins.signed64);
    map::add(compiler::builtins.array->members, String("count"), compiler::builtins.signed64);

    compiler::builtins.darray = structure::create();
    compiler::builtins.darray->size = 3*sizeof(s64);
    map::add(compiler::builtins.darray->members, String("data"), compiler::builtins.signed64);
    map::add(compiler::builtins.darray->members, String("count"), compiler::builtins.signed64);
    map::add(compiler::builtins.darray->members, String("space"), compiler::builtins.signed64);

    compiler::builtins.functype = structure::create();
    compiler::builtins.functype->size = compiler::builtins.unsigned64->size;
    
    messenger::init();  // TODO(sushi) compiler arguments to control this
    array::push(messenger::instance.destinations, Destination(stdout, (isatty(1)? true : false))); // TODO(sushi) isatty throws a warning on win32, make this portable 
    array::push(messenger::instance.destinations, Destination(fopen("temp/log", "w"), false));
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

    instance.options.verbosity = message::verbosity::always;

    Source* entry_source = load_source(instance.options.entry_path);
    if(!entry_source) {
        diagnostic::path::
            not_found(MessageSender::Compiler, instance.options.entry_path);
        messenger::deliver();
        return;
    }

    Lexer* lexer = pool::add(instance.storage.lexers, lex::init(entry_source));
    entry_source->lexer = lexer;
    lex::execute(*lexer);
    
    if(instance.options.dump_tokens.path.str) {
        lex::output(*lexer, instance.options.dump_tokens.human, instance.options.dump_tokens.path);
        if(instance.options.dump_tokens.exit) {
            messenger::deliver();
            return;
        }
    }

    messenger::deliver();

    Parser* parser = pool::add(instance.storage.parsers, parser::init(entry_source));
    entry_source->parser = parser;
    parser::execute(*parser);
    
    messenger::deliver();

    if(instance.options.dump_diagnostics.path.str) {
        if(!internal::dump_diagnostics(instance.options.dump_diagnostics.path, instance.options.dump_diagnostics.sources)) return;
    }
}

global Source*
load_source(String path) {
    // to get absolute path and different parts of the path
    std::filesystem::path p = (char*)path.str;
    if(!std::filesystem::exists(p)) return 0;


    Source* out = pool::add(instance.storage.sources);

    std::filesystem::path ab = std::filesystem::absolute(p);
    out->path = dstring::init(ab.c_str());

    u8* scan = out->path.str + out->path.count;
    while(*scan != '/' && *scan != '\\') {
        if(*scan == '.' && !out->ext.str) {
            out->ext = {scan+1, out->path.count-(scan-out->path.str)};
        }
        scan--;
    }
    out->name = {scan+1, out->path.count-(scan-out->path.str)};
    out->front = {out->name.str, (out->ext.str? out->ext.str-out->name.str-1 : out->name.count)};

    out->file = fopen((char*)out->path.str, "r");

    if(!out->file) return 0;

    // load the source's contents into memory
    upt file_size = std::filesystem::file_size(p);
    u8* buffer = (u8*)memory::allocate(file_size + 1);
    fread(buffer, file_size, 1, out->file);
    out->buffer.str = buffer;
    out->buffer.count = file_size;
    out->buffer.space = file_size + 1;

    out->diagnostics = array::init<Diagnostic>();
    return out;
}

// TODO(sushi) we can store a map String -> Source* and do this more efficiently
global Source*
lookup_source(String name) {
    auto iter = pool::iterator(instance.storage.sources);
    Source* current = pool::next(iter);
    std::filesystem::path path = (char*)name.str;
    while(current) {
        if(std::filesystem::equivalent(path, std::filesystem::path((char*)current->path.str))) return current;
        pool::next(iter);        
    }
    return 0;
}

} // namespace compiler
} // namespace amu