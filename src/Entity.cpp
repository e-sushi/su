namespace amu {

Place*
place::create() {
    Place* out = pool::add(compiler::instance.storage.places);
    node::init(&out->node);
    out->node.kind = node::place;
    return out;
}

Structure*
structure::create() {
    Structure* out = pool::add(compiler::instance.storage.structures);
    node::init(&out->node);
    out->node.kind = node::structure;
    return out;
}

Function*
function::create() {
    Function* out = pool::add(compiler::instance.storage.functions);
    node::init(&out->node);
    out->node.kind = node::function;
    return out;
}

Module*
module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    node::init(&out->node);
    out->node.kind = node::module;
    out->labels = array::init<spt>();
    out->table.map = map::init<String, Label*>();
    return out;
}


namespace type {

Type*
create();

void
destroy(Type* t);

Type*
base(Type* t);

b32
can_coerce(Type* to, Type* from);

String
name(Type* type){
    switch(type->kind) {
        case type::kind::scalar: {
            ScalarType* stype = (ScalarType*)type;
            switch(stype->kind) {
                case type::scalar::kind::void_:      return "void";
                case type::scalar::kind::unsigned8:  return "u8";
                case type::scalar::kind::unsigned16: return "u16";
                case type::scalar::kind::unsigned32: return "u32";
                case type::scalar::kind::unsigned64: return "u64";
                case type::scalar::kind::signed8:    return "s8";
                case type::scalar::kind::signed16:   return "s16";
                case type::scalar::kind::signed32:   return "s32";
                case type::scalar::kind::signed64:   return "s64";
                case type::scalar::kind::float32:    return "f32";
                case type::scalar::kind::float64:    return "f64";
            }
        } break;

        case type::kind::structured: {
            auto stype = (StructuredType*)type;
            return stype->label->node.start->raw;
        } break;

        case type::kind::function: {
            return "func";
        } break;    
    }

    return "type::name couldn't find a name";
}

namespace structured {
Array<ExistingStructureType> set;

StructuredType*
create(Structure* s) {
    auto [idx, found] = amu::array::util::
        search<ExistingStructureType, Structure*>(set, s, [](ExistingStructureType& s) { return s.structure; });
    if(found) return amu::array::read(set, idx).stype;
    ExistingStructureType* nu = amu::array::insert(set, idx);
    nu->stype = pool::add(compiler::instance.storage.structured_types);
    nu->stype->kind = type::kind::structured;
    nu->stype->node.kind = node::type;
    nu->structure = s;
    return nu->stype;
}
} // namespace structured

namespace pointer {
Array<ExistantPointer> set;

PointerType*
create(Type* type) {
    auto [idx,found] = amu::array::util::
        search<ExistantPointer, Type*>(set, type, [](ExistantPointer& p){ return p.type; });
    if(found) return amu::array::read(set, idx).ptype;
    ExistantPointer* nu = amu::array::insert(set, idx);
    nu->ptype = pool::add(compiler::instance.storage.pointer_types);
    nu->ptype->kind = type::kind::pointer;
    nu->ptype->node.kind = node::type;
    nu->type = type;
    return nu->ptype;
}
}

ArrayType*
array::create(Type* type, u64 size) {
    ArrayType* out = pool::add(compiler::instance.storage.array_types);
    out->type = type;
    out->size = size;
    out->kind = type::kind::array;
    return out;
}

namespace function {

// FunctionType does not try to be unique for now
FunctionType*
create() {
    FunctionType* out = pool::add(compiler::instance.storage.function_types);
    out->kind = type::kind::function;
    return out;
}

} // namespace function
} // namespace type

void
to_string(DString& start, Type* t) {
    if(!t) return dstring::append(start, "Type<null>");
    dstring::append(start, 
        node::util::print_tree<[](DString& current, TNode* n) {
            //if(n->kind != node::type) return to_string(current, n, true);
            dstring::append(current, type::name((Type*)n));
        }>((TNode*)t, false));
}

void
to_string(DString& start, Place* p) {
    dstring::append(start, "Place<'", p->label, "' type:", p->type, ">");
}

DString
to_string(Place* p) {
    DString out = dstring::init();
    to_string(out, p);
    return out;
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
    dstring::append(start, "Structure");
}

} // namespace amu
