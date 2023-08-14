namespace amu::source {

global Source*
load(String path) {
    // to get absolute path and different parts of the path
    std::filesystem::path p = (char*)path.str;
    if(!std::filesystem::exists(p)) return 0;

    Source* out = pool::add(compiler::instance.storage.sources);

    std::filesystem::path ab = std::filesystem::absolute(p);
    out->path = String(dstring::init(ab.c_str()));

    u8* scan = out->path.str + out->path.count;
    while(*scan != '/' && *scan != '\\') {
        if(*scan == '.' && !out->ext.str) {
            out->ext = {scan+1, out->path.count-(scan-out->path.str)};
        }
        scan--;
    }
    out->name = {scan+1, out->path.count-(scan-out->path.str)-1};
    out->front = {out->name.str, (out->ext.str? out->ext.str-out->name.str-1 : out->name.count)};

    out->file = fopen((char*)out->path.str, "r");

    if(!out->file) return 0;

    // load the source's contents into memory
    upt file_size = std::filesystem::file_size(p);
    u8* buffer = (u8*)memory::allocate(file_size + 1);
    fread(buffer, file_size, 1, out->file);
    out->buffer.str = buffer;
    out->buffer.count = file_size;

    // don't keep the file locked
    fclose(out->file);

    out->diagnostics = array::init<Diagnostic>();
    return out;
}

global void
unload(Source* source) {
    memory::free(source->path.str);
    source->path.str = 0;
    source->path.count = 0;

    array::deinit(source->diagnostics);

    // module::destroy(*source->module);
    code::destroy(source->code);
}

// TODO(sushi) we can store a map String -> Source* and do this more efficiently
global Source*
lookup(String name) {
    auto iter = pool::iterator(compiler::instance.storage.sources);
    Source* current = pool::next(iter);
    std::filesystem::path path = (char*)name.str;
    while(current) {
        if(std::filesystem::equivalent(path, std::filesystem::path((char*)current->path.str))) return current;
        pool::next(iter);        
    }
    return 0;
}

} // namespace amu::source