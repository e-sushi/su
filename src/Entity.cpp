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

global b32
structure::is_builtin(Structure* s) {
    forI(sizeof(compiler::builtins) / sizeof(Structure)) // oh my 
        if(s == *(&compiler::builtins.void_ + i)) return true;
    return false;
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

void
to_string(DString& start, Place* p) {
    dstring::append(start, "Place<'", p->label->node.start->raw, "' type:", p->type, ">");
}

void
to_string(DString& start, Function* f) {
    dstring::append(start, "Function<TODO>");
}

void
to_string(DString& start, Module* m) {
    dstring::append(start, "Module<TODO>");
}

void
to_string(DString& start, Structure* s) {
    dstring::append(start, "Structure<'");
    if(s == compiler::builtins.void_) {
        dstring::append(start, "void");
    }else if(s == compiler::builtins.unsigned8) {
        dstring::append(start, "u8");
    }else if(s == compiler::builtins.unsigned16) {
        dstring::append(start, "u16");
    }else if(s == compiler::builtins.unsigned32) {
        dstring::append(start, "u32");
    }else if(s == compiler::builtins.unsigned64) {
        dstring::append(start, "u64");
    }else if(s == compiler::builtins.signed8) {
        dstring::append(start, "s8");
    }else if(s == compiler::builtins.signed16) {
       dstring::append(start, "s16");
    }else if(s == compiler::builtins.signed32) {
        dstring::append(start, "s32");
    }else if(s == compiler::builtins.signed64) {
        dstring::append(start, "s64");
    }else if(s == compiler::builtins.float32) {
        dstring::append(start, "f32");
    }else if(s == compiler::builtins.float64) {
        dstring::append(start, "f64");
    }else if(s == compiler::builtins.array) {
        dstring::append(start, "array");
    }else if(s == compiler::builtins.darray) {
        dstring::append(start, "darray");
    }else if(s == compiler::builtins.functype) {
        dstring::append(start, "func");
    } else {
        dstring::append(start, s->label->node.start->raw);
    }

    dstring::append(start, "'>");

}

} // namespace amu
