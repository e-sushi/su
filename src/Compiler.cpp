
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

    Source* initsource = 0;
    {
        auto load = load_source(path);
        if(!load) {
            load.error.sender = MessageSender::Compiler;  
            messenger::dispatch(load.error);
            messenger::deliver(stdout); // deliver immediately because we can't start anyways 
            messenger::deliver(instance.log_file);
            return;
        }
        initsource = load.result;
    }

    LexicalAnalyzer lexer = lex::init(initsource);
    lex::analyze(lexer);
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
        return message::init(
            message::plain("valid path, but unable to open file due to internal error: "),
            message::plain(string::init((char*)result.message.str))
        );
    }

    // load the source's contents into memory
    u8* buffer = (u8*)memory::allocate(out->file->bytes + 1);
    file_read(out->file, buffer, out->file->bytes);
    out->buffer.s.str = buffer;
    out->buffer.s.count = out->file->bytes;
    out->buffer.s.space = out->file->bytes + 1;

    return out;
}

// attempts to locate a given source name
// returns 0 if it doesn't exist
global Source*
lookup_source(String name);


} // namespace compiler
} // namespace amu