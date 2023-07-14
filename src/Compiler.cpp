
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;

void
init() {
    instance.deshi_mem_lock = mutex_init();
    instance.compiler_time = start_stopwatch();

    instance.log_file = file_init(str8l("temp/log"), FileAccess_WriteCreate);

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

    compiler::builtins.void_ = compiler::create_structure();
    compiler::builtins.void_->size = 0;
    compiler::builtins.unsigned8 = compiler::create_structure();
    compiler::builtins.unsigned8->size = 1;
    compiler::builtins.unsigned16 = compiler::create_structure();
    compiler::builtins.unsigned16->size = 2;
    compiler::builtins.unsigned32 = compiler::create_structure();
    compiler::builtins.unsigned32->size = 4;
    compiler::builtins.unsigned64 = compiler::create_structure();
    compiler::builtins.unsigned64->size = 8;
    compiler::builtins.signed8 = compiler::create_structure();
    compiler::builtins.signed8->size = 1;
    compiler::builtins.signed16 = compiler::create_structure();
    compiler::builtins.signed16->size = 2;
    compiler::builtins.signed32 = compiler::create_structure();
    compiler::builtins.signed32->size = 4;
    compiler::builtins.signed64 = compiler::create_structure();
    compiler::builtins.signed64->size = 8;
    compiler::builtins.float32 = compiler::create_structure();
    compiler::builtins.float32->size = 4;
    compiler::builtins.float64 = compiler::create_structure();
    compiler::builtins.float64->size = 8;
    compiler::builtins.array = compiler::create_structure();
    compiler::builtins.array->size = sizeof(s64) + sizeof(void*);
    map::add(compiler::builtins.array->members, String("data"), compiler::builtins.void_);
    map::add(compiler::builtins.array->members, String("count"), compiler::builtins.signed64);

    compiler::builtins.functype = compiler::create_structure();
    compiler::builtins.functype->size = compiler::builtins.unsigned64->size;

    messenger::init();
}

global void
deinit() {
    mutex_deinit(&instance.deshi_mem_lock);
}

global void
begin(Array<String> args) {
    String path;

    for(s32 i = 1; i < args.count; i++) {
        String arg = array::read(args, i);
        u64 hash = string::hash(arg);
        switch(hash) {

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
                    messenger::dispatch(message::attach_sender(MessageSender::Compiler,
                        diagnostic::compiler::expected_a_path_for_arg("--dump-tokens")));
                    messenger::deliver(stdout); messenger::deliver(instance.log_file);
                    return;
                }
                instance.options.dump_tokens.path = arg;
            } break;

            default: {
                if(arg.str[0] == '-') {
                    messenger::dispatch(message::attach_sender(MessageSender::Compiler,
                        diagnostic::compiler::unknown_option(arg)));
                    messenger::deliver(stdout); messenger::deliver(instance.log_file);
                    return;
                }
                // otherwise this is (hopefully) a path
                path = arg;

            } break;
        }
    }

    if(!path.str){
        messenger::dispatch(message::attach_sender(MessageSender::Compiler,
            diagnostic::compiler::no_path_given()));
        messenger::deliver(stdout); messenger::deliver(instance.log_file);
        return;
    }

    instance.options.verbosity = message::verbosity::always;

    Source* initsource = 0;
    {
        auto load = load_source(path);
        if(!load) {
            messenger::dispatch(
                message::attach_sender(MessageSender::Compiler, load.error));
            messenger::deliver(stdout); // deliver immediately because we can't start
            messenger::deliver(instance.log_file);
            return;
        }
        initsource = load.result;
    }

    Lexer* lexer = pool::add(instance.storage.lexers, 
            lex::init(initsource));
    initsource->lexer = lexer;
    lex::execute(*lexer);
    
    if(instance.options.dump_tokens.path.str) {
        lex::output(*lexer, instance.options.dump_tokens.human, instance.options.dump_tokens.path);
        if(instance.options.dump_tokens.exit) {
            messenger::deliver(stdout); messenger::deliver(instance.log_file);
            return;
        }
    }

    messenger::deliver(stdout);
    messenger::deliver(instance.log_file);

    Parser* parser = pool::add(instance.storage.parsers,
            parser::init(initsource));

    parser::execute(*parser);
    messenger::deliver(stdout);
    messenger::deliver(instance.log_file);

}

global Result<Source*, Message>
load_source(String path) {
    if(!file_exists(path.s)) {
        return diagnostic::path::not_found(path);
    }

    Source* out = pool::add(instance.storage.sources);
    FileResult result = {};
    out->file = file_init_result(path.s, FileAccess_Read, &result);

    if(!out->file) {
        pool::remove(instance.storage.sources, out);
        return diagnostic::internal::valid_path_but_internal_err(path, String(result.message));
    }

    // load the source's contents into memory
    u8* buffer = (u8*)memory::allocate(out->file->bytes + 1);
    file_read(out->file, buffer, out->file->bytes);
    out->buffer.s.str = buffer;
    out->buffer.s.count = out->file->bytes;
    out->buffer.s.space = out->file->bytes + 1;

    return out;
}

global Source*
lookup_source(String name);

global Label*
create_label() {
    Label* out = pool::add(instance.storage.labels);
    out->node.kind = node::label;
    return out;
}

global Place*
create_place(){
    Place* out = pool::add(instance.storage.places);
    out->node.kind = node::place;
    return out;
}

global Structure*
create_structure(){
    Structure* out = pool::add(instance.storage.structures);
    out->members = map::init<String, Structure*>();
    out->node.kind = node::structure;
    return out;
}

global Function*
create_function(){
    Function* out = pool::add(instance.storage.functions);
    out->node.kind = node::function;
    return out;
}

global Module*
create_module(){
    Module* out = pool::add(instance.storage.modules);
    node::init(&out->node);
    out->node.kind = node::module;
    return out;
}

global Statement*
create_statement() {
    Statement* out = pool::add(instance.storage.statements);
    node::init(&out->node);
    out->node.kind = node::statement;
    return out;
}

global Tuple*
create_tuple() {
    Tuple* out = pool::add(instance.storage.tuples);
    node::init(&out->node);
    out->node.kind = node::tuple;
    return out;
}

global Expression*
create_expression() {
    Expression* out = pool::add(instance.storage.expressions);
    node::init(&out->node);
    out->node.kind = node::expression;
    return out;
}

} // namespace compiler
} // namespace amu