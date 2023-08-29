namespace amu {

b32 Type::
can_cast_to(Type* to)  { 
    if(this == to) return true;

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
    if(to->kind == type::kind::array && this->kind == type::kind::array) {
        auto ato = (StaticArray*)to;
        auto athis = (StaticArray*)this;
        if(ato->type->can_cast_to(athis->type)) return true;
    }

    // allow implicit coercion of an array to its data pointer
    // this may not be a good idea either
    if(to->kind == type::kind::pointer && this->kind == type::kind::array) {
        auto pto = (Pointer*)to;
        auto pthis = (StaticArray*)this;
        return pto->type == pthis->type;
    }

    return false;
}   

Type* Type::
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
        case node::expression: return ((Expr*)n)->type;
        case node::tuple: ((Tuple*)n)->type;
        case node::place: return ((Place*)n)->type;
    }
    return 0;
}  

String Type::
name() {
    switch(this->kind) {
        case type::kind::void_:      return "void";
        case type::kind::scalar:     return ((ScalarType*)this)->name();
        case type::kind::structured: return ((Structured*)this)->name();
        case type::kind::function:   return ((FunctionType*)this)->name();
        case type::kind::array:      return ((StaticArray*)this)->name();
        case type::kind::pointer:    return ((Pointer*)this)->name();
    }

    Assert(0);
    return {};
}


String ScalarType::
name() {
    switch(this->kind) {
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
}

String Structured::
name() {
    return label::display(this->label);
}

String Pointer::
name() {
    return dstring::init(this->type->name(), "*");
}

String FunctionType::
name() {
    DString out = dstring::init("("); // !Leak
    for(TNode* n = this->parameters->first_child; n; n = n->next) {
        dstring::append(out, Type::resolve(n), (n->next? ", " : ""));
    }
    dstring::append(out, ") -> ");
    for(TNode* n = this->returns; n; n = n->next) {
        // weird logic, should probably adjust how the AST stores function info
        b32 should_comma = n->next && n->next->kind == node::expression && ((Expr*)n->next)->kind != expr::block;
        dstring::append(out, Type::resolve(n), (should_comma? ", " : ""));
        if(!should_comma) break;
    }
    return out;
}

String StaticArray::
name() {
    return dstring::init(this->type->name(), "[", this->count, "]"); 
}

StaticArray* StaticArray::
create(Type* type, u64 count) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<type::array::ExistantArray, u64>(type::array::set, hash, [](type::array::ExistantArray& a){return a.hash;});

    if(found) return amu::array::read(type::array::set, idx).atype;
    type::array::ExistantArray* nu = amu::array::insert(type::array::set, idx);
    nu->hash = hash;
    nu->atype = pool::add(compiler::instance.storage.array_types);
    nu->atype->count = count;
    nu->atype->type = type;
    nu->atype->kind = type::kind::array;
    nu->atype->node.kind = node::type;
    return nu->atype;
}

// FunctionType does not try to be unique for now
FunctionType* FunctionType::
create() {
    FunctionType* out = pool::add(compiler::instance.storage.function_types);
    out->kind = type::kind::function;
    out->node.kind = node::type;
    return out;
}


TupleType* TupleType::
create(Array<Type*>& types) {
    u64 hash = 1212515131534;
    forI(types.count) {
        hash <<= (u64)amu::array::read(types, i) * 167272723;
    }
    auto [idx, found] = amu::array::util::
        search<type::tuple::ExistantTupleType, u64>(type::tuple::set, hash, [](type::tuple::ExistantTupleType& t){return t.hash;});
    
    if(found) {
        amu::array::deinit(types);
        return amu::array::read(type::tuple::set, idx).ttype;
    } 
    type::tuple::ExistantTupleType* nu = amu::array::insert(type::tuple::set, idx);
    nu->hash = hash;
    nu->ttype = pool::add(compiler::instance.storage.tuple_types);
    nu->ttype->types = types;
    nu->ttype->kind = type::kind::tuple;
    nu->ttype->node.kind = node::type;
    return nu->ttype;
}

} // namespace amu

