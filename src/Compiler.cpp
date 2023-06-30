
namespace amu {
namespace compiler {

// global compiler instance
Compiler instance;

void
init() {
    instance.deshi_mem_lock = mutex_init();
    instance.compiler_time = start_stopwatch();
}


}
}