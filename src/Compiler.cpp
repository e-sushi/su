
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;

void
init() {
    instance.deshi_mem_lock = mutex_init();
    instance.compiler_time = start_stopwatch();

    instance.log_file = file_init(str8l("temp/log"), FileAccess_WriteCreate);

    instance.storage.sources = pool::init<Source>(32);
    instance.storage.lexers = pool::init<Lexer>(32);
    instance.storage.parsers = pool::init<Parser>(32);
    instance.storage.labels = pool::init<Label>(32);
    instance.storage.entities = pool::init<Entity>(32);
    instance.storage.statements = pool::init<Statement>(32);
    instance.storage.tuples = pool::init<Tuple>(32);

    instance.options.deliver_debug_immediately = true;

    messenger::init();
}

global void
deinit() {
    mutex_deinit(&instance.deshi_mem_lock);
}

global void
begin(Array<String> args) {
    String cwd = array::read(args, 0);
    String path = array::read(args, 1);

    instance.options.verbosity = message::verbosity::debug;

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
    messenger::deliver(stdout);
    messenger::deliver(instance.log_file);
    lex::output(*lexer, "temp/lexout");

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
    out->node.type = node::type::label;
    return out;
}

global Entity*
create_entity() {
    Entity* out = pool::add(instance.storage.entities);
    out->node.type = node::type::entity;
    return out;
}

global Statement*
create_statement() {
    Statement* out = pool::add(instance.storage.statements);
    out->node.type = node::type::statement;
    return out;
}

global Tuple*
create_tuple() {
    Tuple* out = pool::add(instance.storage.tuples);
    out->node.type = node::type::tuple;
    return out;
}

global Expression*
create_expression() {
    Expression* out = pool::add(instance.storage.expressions);
    out->node.type = node::type::expression;
    return out;
}

} // namespace compiler
} // namespace amu