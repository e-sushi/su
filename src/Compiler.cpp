
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;

void
init() {
    instance.deshi_mem_lock = mutex_init();
    instance.compiler_time = start_stopwatch();
}

global void
deinit() {
    mutex_deinit(&instance.deshi_mem_lock);
}

global Source*
load_source(str8 path) {
    FileResult result = {};
    if(!file_exists_result(path, &result)) {

    }

    return 0;
}

// attempts to locate a given source name
// returns 0 if it doesn't exist
global Source*
lookup_source(str8 name);


} // namespace compiler
} // namespace amu