namespace amu {

b32 Type::
can_cast_to(Type* to)  { 
    if(this == to) return true;

    // the Whatever type is a wildcard that anything can cast to
    // but you cannot use something of this type, and checks for that
    // are handled elsewhere
    if(this->is<Whatever>() || to->is<Whatever>()) 
        return true;
    

    // all scalar types may coerce to each other
    // TODO(sushi) this should probably not allow float <-> int coercion in implicit cases, though
    if(to->kind == type::kind::scalar && this->kind == type::kind::scalar)
        return true;

    // pointers may coerce freely
    // TODO(sushi) stronger rules may be safer, though
    if(to->kind == type::kind::pointer && this->kind == type::kind::pointer)
        return true;

    // arrays can coerce between each other as long as a conversion exists
    // between their underlying types
    if(to->kind == type::kind::static_array && this->kind == type::kind::static_array) {
        auto ato = (StaticArray*)to;
        auto athis = (StaticArray*)this;
        if(ato->type->can_cast_to(athis->type)) return true;
    }

    // allow implicit coercion of an array to its data pointer
    // this may not be a good idea either
    if(to->kind == type::kind::pointer && this->kind == type::kind::static_array) {
        auto pto = (Pointer*)to;
        auto pthis = (StaticArray*)this;
        return pto->type == pthis->type;
    }

    return false;
}

String Scalar::
name() {
    switch(this->kind) {
        case scalar::unsigned8:  return "u8";
        case scalar::unsigned16: return "u16";
        case scalar::unsigned32: return "u32";
        case scalar::unsigned64: return "u64";
        case scalar::signed8:    return "s8";
        case scalar::signed16:   return "s16";
        case scalar::signed32:   return "s32";
        case scalar::signed64:   return "s64";
        case scalar::float32:    return "f32";
        case scalar::float64:    return "f64";
    }
}

u64 Scalar::
size() {
    switch(this->kind) {
        case scalar::signed8:
        case scalar::unsigned8:  return 1;
        case scalar::signed16:
        case scalar::unsigned16: return 2;
        case scalar::float32:
        case scalar::signed32:
        case scalar::unsigned32: return 4;
        case scalar::float64:
        case scalar::signed64:
        case scalar::unsigned64: return 8;
    }
}

DString Scalar::
debug_str() {
    DString out = dstring::init("ScalarType<");
    switch(this->kind) {
        case scalar::unsigned8:  dstring::append(out, "u8");    break;
        case scalar::unsigned16: dstring::append(out, "u16");   break;
        case scalar::unsigned32: dstring::append(out, "u32");   break;
        case scalar::unsigned64: dstring::append(out, "u64");   break;
        case scalar::signed8:    dstring::append(out, "s8");    break;
        case scalar::signed16:   dstring::append(out, "s16");   break;
        case scalar::signed32:   dstring::append(out, "s32");   break;
        case scalar::signed64:   dstring::append(out, "s64");   break;
        case scalar::float32:    dstring::append(out, "f32");   break;
        case scalar::float64:    dstring::append(out, "f64");   break;
    }
    dstring::append(out, ">");
    return out;
}

Array<Pointer*> Pointer::set = array::init<Pointer*>();

String Pointer::
name() { // !Leak
    return dstring::init(type->name(), "*");
}

DString Pointer::
debug_str() {
    return dstring::init(type->name());
}

u64 Pointer::
size() {
    return sizeof(void*);
}

Array<StaticArray*> StaticArray::set = array::init<StaticArray*>();


// NOTE(sushi) I am VERY sorry to whoever reads or needs to fix the following functions 
//             I am not interested in trying to setup a concrete implementation of storing 
//             and accessing unique types yet, so the following code is stupidly scuffed
StaticArray* StaticArray::
create(Type* type, u64 count) {
    u64 hash = (u64(type) << count) * 1234;
    auto [idx, found] = amu::array::util:: // this suuuuuuuucks
        search<StaticArray*, u64>(StaticArray::set, hash, 
            [](StaticArray* a){ return (u64(a->type) << a->count) * 1234; });

    if(found) return amu::array::read(StaticArray::set, idx);
    StaticArray* nu = pool::add(compiler::instance.storage.static_array_types);
    nu->type = type;
    nu->count = count;
    amu::array::insert(StaticArray::set, idx, nu);
    return nu;
}

String StaticArray::
name() {
    return dstring::init(this->type->name(), "[", this->count, "]"); 
}

DString StaticArray::
debug_str() {
    return dstring::init(name());
}

u64 StaticArray::
size() {
    return sizeof(void*) + sizeof(u64);
}

Array<ViewArray*> ViewArray::set = array::init<ViewArray*>();

ViewArray* ViewArray::
create(Type* type) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<ViewArray*, u64>(ViewArray::set, hash, [](ViewArray* a){return u64(a->type);});

    if(found) return amu::array::read(ViewArray::set, idx);

    ViewArray* nu = pool::add(compiler::instance.storage.view_array_types);
    nu->type = type;
    amu::array::insert(ViewArray::set, idx, nu);
    return nu;
}

String ViewArray::
name() {
    return dstring::init(this->type->name(), "[]");
}

DString ViewArray::
debug_str() {
    return dstring::init(name());
}

u64 ViewArray::
size() {
    return sizeof(void*) + sizeof(u64);
}

Array<DynamicArray*> DynamicArray::set = array::init<DynamicArray*>();

DynamicArray* DynamicArray::
create(Type* type) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<DynamicArray*, u64>(DynamicArray::set, hash, [](DynamicArray* a){return u64(a->type);});

    if(found) return amu::array::read(DynamicArray::set, idx);

    DynamicArray* nu = pool::add(compiler::instance.storage.dynamic_array_types);
    nu->type = type;
    amu::array::insert(DynamicArray::set, idx, nu);
    return nu;
}

String DynamicArray::
name() {
    return dstring::init(this->type->name(), "[..]");
}

DString DynamicArray::
debug_str() {
    return dstring::init(name());
}

u64 DynamicArray::
size() { // TODO(sushi) size of Allocators when they are implemented
    return sizeof(void*) + sizeof(u64) + sizeof(u64);
}

// FunctionType does not try to be unique for now
FunctionType* FunctionType::
create() {
    FunctionType* out = pool::add(compiler::instance.storage.function_types);
    out->kind = type::kind::function;
    out->ASTNode::kind = ast::entity;
    return out;
}

String FunctionType::
name() {
    DString out = dstring::init("("); // !Leak
    for(ASTNode* n = this->parameters->first_child(); n; n = n->next()) {
        dstring::append(out, n->resolve_type(), (n->next()? ", " : ""));
    }
    dstring::append(out, ") -> ");
    for(ASTNode* n = this->returns; n; n = n->next()) {
        b32 block_next = n->next_is<Block>();
        dstring::append(out, n->resolve_type(), (block_next? "" : ", "));
        if(block_next) break;
    }
    return out;
}

DString FunctionType::
debug_str() { // !Leak: double leak
    return dstring::init(name());
}

u64 FunctionType::
size() {
    return sizeof(void*); // treated as pointers for now 
}

Array<TupleType*> TupleType::set = array::init<TupleType*>();

TupleType* TupleType::
create(Array<Type*>& types) {
    // extremely bad, awful, no good
    u64 hash = 1212515131534;
    forI(types.count) {
        hash <<= (u64)amu::array::read(types, i) * 167272723;
    }
    auto [idx, found] = amu::array::util::
        search<TupleType*, u64>(TupleType::set, hash, [](TupleType* t){
            u64 hash = 1212515131534;
            forI(t->types.count) {
                hash <<= (u64)amu::array::read(t->types, i) * 167272723;
            }
            return hash;
        });
    
    if(found) {
        amu::array::deinit(types);
        return amu::array::read(TupleType::set, idx);
    } 
    TupleType* nu = pool::add(compiler::instance.storage.tuple_types);
    nu->types = types;
    return nu;
}

String TupleType::
name() { // TODO(sushi) this sucks
    return String{start->raw.str, end->raw.str - start->raw.str};
}

DString TupleType::
debug_str() {
    return dstring::init("TupleType<TODO>");
}

u64 TupleType::
size() {
    u64 count = 0;
    forI(types.count) {
        count += array::read(types, i)->size();
    }
    return count;
}

// namespace type::structure {
// Array<ExistingStructureType> set = amu::array::init<ExistingStructureType>();
// } // namespace structure

// Structured* Structured::
// create(Structure* s) {
//     auto [idx, found] = amu::array::util::
//         search<type::structure::ExistingStructureType, Structure*>(type::structure::set, s, [](type::structure::ExistingStructureType& s) { return s.structure; });
//     if(found) return amu::array::read(type::structure::set, idx).stype;
//     type::structure::ExistingStructureType* nu = amu::array::insert(structure::set, idx);
//     nu->stype = pool::add(compiler::instance.storage.structured_types);
//     nu->stype->kind = type::kind::structured;
//     nu->stype->node.kind = node::type;
//     nu->stype->structure = s;
//     nu->structure = s;
//     return nu->stype;
// }

Structured* Structured::
create(Structure* s) {
    auto out = pool::add(compiler::instance.storage.structured_types);
    out->kind = type::kind::structured;
    // out->node.kind = node::type;
    out->structure = s;
    return out;
}

Label* Structured::
find_member(String id) {
    Structure* s = this->structure;
    auto [idx, found] = map::find(s->table.map, id);
    if(!found) return 0;
    return amu::array::read(s->table.map.values, idx);
}

String Structured::
name() {
    return label->name();
}

DString Structured::
debug_str() {
    return dstring::init("Structured<TODO>");
}

u64 Structured::
size() {
    return structure->size;
}

} // namespace amu

