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
    if(to->is<Scalar>() && this->is<Scalar>())
        return true;

    // pointers may coerce freely
    // TODO(sushi) stronger rules may be safer, though
    // TODO(sushi) this is NOT safe
    if(to->is<Pointer>() && this->is<Pointer>())
        return true;

    // arrays can coerce between each other as long as a conversion exists
    // between their underlying types
    if(to->is<StaticArray>() && this->is<StaticArray>()) {
        auto ato = (StaticArray*)to;
        auto athis = (StaticArray*)this;
        if(ato->type->can_cast_to(athis->type)) return true;
    }

    // allow implicit coercion of an array to its data pointer
    // this may not be a good idea either
    if(to->is<Pointer>() && this->is<StaticArray>()) {
        auto pto = to->as<Pointer>();
        auto pthis = this->as<StaticArray>();
        return pto->type == pthis->type;
    }

    return false;
}

DString* Scalar::
name() {
    switch(this->kind) {
        case scalar::unsigned8:  return DString::create("u8");
        case scalar::unsigned16: return DString::create("u16");
        case scalar::unsigned32: return DString::create("u32");
        case scalar::unsigned64: return DString::create("u64");
        case scalar::signed8:    return DString::create("s8");
        case scalar::signed16:   return DString::create("s16");
        case scalar::signed32:   return DString::create("s32");
        case scalar::signed64:   return DString::create("s64");
        case scalar::float32:    return DString::create("f32");
        case scalar::float64:    return DString::create("f64");
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

DString* Scalar::
dump() {
    DString* out = DString::create("ScalarType<");
    switch(this->kind) {
        case scalar::unsigned8:  out->append("u8");    break;
        case scalar::unsigned16: out->append("u16");   break;
        case scalar::unsigned32: out->append("u32");   break;
        case scalar::unsigned64: out->append("u64");   break;
        case scalar::signed8:    out->append("s8");    break;
        case scalar::signed16:   out->append("s16");   break;
        case scalar::signed32:   out->append("s32");   break;
        case scalar::signed64:   out->append("s64");   break;
        case scalar::float32:    out->append("f32");   break;
        case scalar::float64:    out->append("f64");   break;
    }
    out->append(">");
    return out;
}

Array<Pointer*> Pointer::set = array::init<Pointer*>();

Pointer* Pointer::
create(Type* type) {
    auto [idx,found] = amu::array::util::
        search<Pointer*, Type*>(Pointer::set, type, [](Pointer* p){ return p->type; });
    if(found) return amu::array::read(Pointer::set, idx);
    Pointer* nu = pool::add(compiler::instance.storage.pointer_types);
    nu->type = type;
    amu::array::insert(Pointer::set, idx, nu);
    return nu;
}

DString* Pointer::
name() { // !Leak
    return DString::create(type->name(), "*");
}

DString* Pointer::
dump() {
    return DString::create(type->name(), "*");
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

    auto s = Structure::create();
    
    auto data = s->add_member("data");
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count_ = s->add_member("count");
    count_->type = &scalar::scalars[scalar::unsigned64];
    count_->inherited = false;
    count_->offset = data->type->size();

    nu->structure = s;

    return nu;
}

DString* StaticArray::
name() {
    return DString::create(ScopedDStringRef(this->type->name()).x, "[", this->count, "]"); 
}

DString* StaticArray::
dump() {
    return DString::create(name());
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

    auto s = Structure::create();

    auto data = s->add_member("data"); 
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count = s->add_member("count");
    count->type = &scalar::scalars[scalar::unsigned64];
    count->inherited = false;
    count->offset = data->type->size();

    nu->structure = s;

    return nu;
}

DString* ViewArray::
name() {
    return DString::create(ScopedDStringRef(this->type->name()).x, "[]");
}

DString* ViewArray::
dump() {
    return DString::create(name());
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

    auto s = Structure::create();

    auto data = s->add_member("data");
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count = s->add_member("count");
    count->type = &scalar::scalars[scalar::unsigned64];
    count->inherited = false;
    count->offset = data->type->size();
    
    auto space = s->add_member("space");
    space->type = &scalar::scalars[scalar::unsigned64];
    space->inherited = false;
    space->offset = data->type->size() + count->type->size();

    nu->structure = s;

    return nu;
}

DString* DynamicArray::
name() {
    return DString::create(this->type->name(), "[..]");
}

DString* DynamicArray::
dump() {
    return DString::create(name());
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

DString* FunctionType::
name() {
    DString* out = DString::create("("); // !Leak
    for(ASTNode* n = this->parameters->first_child(); n; n = n->next()) {
        out->append(n->resolve_type(), (n->next()? ", " : ""));
    }
    out->append(") -> ");
    for(ASTNode* n = this->returns; n; n = n->next()) {
        b32 block_next = n->next_is<Block>();
        out->append(n->resolve_type(), (block_next? "" : ", "));
        if(block_next) break;
    }
    return out;
}

DString* FunctionType::
dump() { // !Leak: double leak
    return name();
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

DString* TupleType::
name() { // TODO(sushi) this sucks
    return DString::create(String{start->raw.str, end->raw.str - start->raw.str});
}

DString* TupleType::
dump() {
    return DString::create("TupleType<TODO>");
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
    out->structure = s;
    return out;
}

Member* Structured::
find_member(String id) {
    return structure->find_member(id);
}

DString* Structured::
name() {
    return label->name()->ref();
}

DString* Structured::
dump() {
    return DString::create("Structured<TODO>");
}

u64 Structured::
size() {
    return structure->size;
}



} // namespace amu

