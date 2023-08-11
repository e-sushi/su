namespace amu {

global Place*
place::create() {
    Place* out = pool::add(compiler::instance.storage.places);
    node::init(&out->node);
    out->node.kind = node::place;
    return out;
}

global Structure*
structure::create() {
    Structure* out = pool::add(compiler::instance.storage.structures);
    out->members = map::init<String, Structure*>();
    node::init(&out->node);
    out->node.kind = node::structure;
    return out;
}

global Function*
function::create() {
    Function* out = pool::add(compiler::instance.storage.functions);
    node::init(&out->node);
    out->node.kind = node::function;
    return out;
}

global Module*
module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    node::init(&out->node);
    out->node.kind = node::module;
    out->labels = array::init<spt>();
    out->table.map = map::init<String, Label*>();
    return out;
}

namespace entity {

} // namespace entity
} // namespace amu