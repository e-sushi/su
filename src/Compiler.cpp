
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

    auto load = load_source(path);
    if(!load) {
        message::prefix(load.error, 
            message::plain("compiler", Color_DarkYellow),
            message::plain(": "));
        messenger::dispatch(load.error);
        messenger::deliver(stdout);
        messenger::deliver(instance.log_file);
    }
}

global Result<Source*, Message>
load_source(String path) {
    if(!file_exists(path.s)) {
        return message::init(0,
            message::plain("unable to locate path '"),
            message::path(path, Color_Cyan),
            message::plain("'")
        );
    }

    Source* out = pool::add(instance.storage.sources);
    FileResult result = {};
    out->file = file_init_result(path.s, FileAccess_Read, &result);

    if(!out->file) {
        pool::remove(instance.storage.sources, out);
        return message::init(0, 
            message::plain("valid path, but unable to open file due to internal error: "),
            message::plain(string::init((char*)result.message.str))
        );
    }

    return out;
}

// attempts to locate a given source name
// returns 0 if it doesn't exist
global Source*
lookup_source(String name);


} // namespace compiler
} // namespace amu