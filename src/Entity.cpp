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
    out->table = label::table::init(&out->node);
    return out;
}

Function*
function::create() {
    Function* out = pool::add(compiler::instance.storage.functions);
    node::init(&out->node);
    out->node.kind = node::function;
    out->table = label::table::init((TNode*)out);
    return out;
}

namespace module {

Module*
create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    node::init(&out->node);
    out->node.kind = node::module;
    out->table = label::table::init((TNode*)out);
    return out;
}

} // namespace module


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
name(Type* type) {
    switch(type->kind) {
        case type::kind::void_: {
            return "void";
        } break;
        case type::kind::scalar: {
            auto stype = (ScalarType*)type;
            switch(stype->kind) {
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
            return label::display(stype->label);
        } break;

        case type::kind::function: {
            auto ftype = (FunctionType*)type;
            DString out = dstring::init("("); // !Leak
            for(TNode* n = ftype->parameters->first_child; n; n = n->next) {
                dstring::append(out, type::resolve(n), (n->next? ", " : ""));
            }
            dstring::append(out, ") -> ");
            for(TNode* n = ftype->returns; n; n = n->next) {
                // weird logic, should probably adjust how the AST stores function info
                b32 should_comma = n->next && n->next->kind == node::expression && ((Expression*)n->next)->kind != expression::block;
                dstring::append(out, type::resolve(n), (should_comma? ", " : ""));
                if(!should_comma) break;
            }
            return out;
        } break;

        case type::kind::array: {
            auto atype = (ArrayType*)type;
            // !Leak
            return dstring::init(name(atype->type), "[", atype->size, "]"); 
        } break;

        case type::kind::pointer: {
            auto ptype = (PointerType*)type;
            // !Leak
            return dstring::init(name(ptype->type), "*");
        } break;
    }

    return "<<type::name couldn't find a name>>";
}

String
debug_name(Type* type){
    switch(type->kind) {
        case type::kind::void_: {
            return "void";
        } break;
        case type::kind::scalar: {
            auto stype = (ScalarType*)type;
            switch(stype->kind) {
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
            // !Leak
            DString out = dstring::init("StructuredType<");
            if(stype->label) {
                dstring::append(out, "'", stype->label->node.start->raw, "' ");
                dstring::append(out, node::util::print_tree(&stype->structure->node));
            } else {
                dstring::append(out, "unknown>");
            }
            return out;
        } break;

        case type::kind::function: {
            auto ftype = (FunctionType*)type;
            DString out = dstring::init("func<args: "); // !Leak
            dstring::append(out, node::util::print_tree<[](DString& current, TNode* n) {
                to_string(current, n, true);
            }>(ftype->parameters, false));
            dstring::append(out, ", ret: ", ftype->return_type, ">");
            return out;
        } break;

        case type::kind::array: {
            auto atype = (ArrayType*)type;
            // !Leak
            return dstring::init(debug_name(atype->type), "[", atype->size, "]"); 
        } break;

        case type::kind::pointer: {
            auto ptype = (PointerType*)type;
            // !Leak
            return dstring::init(debug_name(ptype->type), "*");
        } break;
    }

    return "<<type::debug_name couldn't find a name>>";
}

namespace structured {
Array<ExistingStructureType> set = amu::array::init<ExistingStructureType>();

StructuredType*
create(Structure* s) {
    auto [idx, found] = amu::array::util::
        search<ExistingStructureType, Structure*>(set, s, [](ExistingStructureType& s) { return s.structure; });
    if(found) return amu::array::read(set, idx).stype;
    ExistingStructureType* nu = amu::array::insert(set, idx);
    nu->stype = pool::add(compiler::instance.storage.structured_types);
    nu->stype->kind = type::kind::structured;
    nu->stype->node.kind = node::type;
    nu->stype->structure = s;
    nu->structure = s;
    return nu->stype;
}

Label*
find_member(StructuredType* st, String id) {
    Structure* s = st->structure;
    auto [idx, found] = map::find(s->table.map, id);
    if(!found) return 0;
    return amu::array::read(s->table.map.values, idx);
}
} // namespace structured

namespace pointer {
Array<ExistantPointer> set = amu::array::init<ExistantPointer>();

PointerType*
create(Type* type) {
    auto [idx,found] = amu::array::util::
        search<ExistantPointer, Type*>(set, type, [](ExistantPointer& p){ return p.type; });
    if(found) return amu::array::read(set, idx).ptype;
    ExistantPointer* nu = amu::array::insert(set, idx);
    nu->ptype = pool::add(compiler::instance.storage.pointer_types);
    nu->ptype->kind = type::kind::pointer;
    nu->ptype->node.kind = node::type;
    nu->ptype->type = type;
    nu->type = type;
    return nu->ptype;
}
} // namespace pointer

namespace array {
Array<ExistantArray> set = amu::array::init<ExistantArray>();

ArrayType*
create(Type* type, u64 size) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<ExistantArray, u64>(set, hash, [](ExistantArray& a){return a.hash;});

    if(found) return amu::array::read(set, idx).atype;
    ExistantArray* nu = amu::array::insert(set, idx);
    nu->hash = hash;
    nu->atype = pool::add(compiler::instance.storage.array_types);
    nu->atype->size = size;
    nu->atype->type = type;
    nu->atype->kind = type::kind::array;
    nu->atype->node.kind = node::type;
    return nu->atype;
}

} // namespace array

namespace function {

// FunctionType does not try to be unique for now
FunctionType*
create() {
    FunctionType* out = pool::add(compiler::instance.storage.function_types);
    out->kind = type::kind::function;
    out->node.kind = node::type;
    return out;
}

} // namespace function

namespace tuple {
Array<ExistantTupleType> set;

TupleType*
create(Array<Type*>& types) {
    u64 hash = 1212515131534;
    forI(types.count) {
        hash <<= (u64)amu::array::read(types, i) * 167272723;
    }
    auto [idx, found] = amu::array::util::
        search<ExistantTupleType, u64>(set, hash, [](ExistantTupleType& t){return t.hash;});
    
    if(found) {
        amu::array::deinit(types);
        return amu::array::read(set, idx).ttype;
    } 
    ExistantTupleType* nu = amu::array::insert(set, idx);
    nu->hash = hash;
    nu->ttype = pool::add(compiler::instance.storage.tuple_types);
    nu->ttype->types = types;
    nu->ttype->kind = type::kind::tuple;
    nu->ttype->node.kind = node::type;
    return nu->ttype;
}
} // namespace tuple

Type*
resolve(TNode* n) {
    if(!n) return 0;
    switch(n->kind) {
        case node::label: {
            return resolve((TNode*)((Label*)n)->entity);
        } break;
        case node::function: {
            return ((Function*)((Label*)n)->entity)->type;
        } break;
        case node::module: return 0;
        case node::statement: {
            auto s = (Statement*)n;
            switch(s->kind) {
                case statement::defer_:
                case statement::unknown: return 0;
                case statement::label: 
                case statement::block_final:
                case statement::expression: return resolve(s->node.first_child);
            }
        } break;
        case node::expression: return ((Expression*)n)->type;
        case node::tuple: ((Tuple*)n)->type;
        case node::place: return ((Place*)n)->type;
    }
    return 0;
}  

u64
size(Type* t) {
    switch(t->kind) {
        case type::kind::array: {
            return sizeof(void*) + sizeof(u64);
        } break;
        case type::kind::pointer: {
            return sizeof(void*);
        } break;
        case type::kind::scalar: {
            auto stype = (ScalarType*)t;
            switch(stype->kind) {
                case type::scalar::kind::signed8: return 1;
                case type::scalar::kind::unsigned8: return 1;
                case type::scalar::kind::signed16: 
                case type::scalar::kind::unsigned16: return 2;
                case type::scalar::kind::float32:
                case type::scalar::kind::signed32:
                case type::scalar::kind::unsigned32: return 4;
                case type::scalar::kind::float64:
                case type::scalar::kind::signed64:
                case type::scalar::kind::unsigned64: return 8;
            } 
        } break;
        case type::kind::structured: {
            auto stype = (StructuredType*)t;
            return stype->structure->size;
        } break;
    }
    return 0;
}

} // namespace type

void
to_string(DString& start, Type* t) {
    if(!t) return dstring::append(start, "Type<null>");
    dstring::append(start, 
        node::util::print_tree<[](DString& current, TNode* n) {
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
