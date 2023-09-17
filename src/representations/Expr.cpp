namespace amu {

Expr* Expr::
create(expr::kind kind, Type* type) {
    Expr* out = pool::add(compiler::instance.storage.expressions);
    // out->node.kind = node::expression;
    out->kind = kind;
    out->type = type;
    return out;
}

DString* Expr::
display() { // TODO(sushi) switch on expr kind
    return DString::create("Expression");
}

DString* Expr::
dump() {
    DString* out = DString::create("Expr<");

    switch(this->kind) {
        case expr::typeref: {
            out->append("typeref:", this->type);
        } break;
        case expr::identifier: {
            out->append("id:'", this->start->raw, "'");
        } break;
        case expr::cast: {
            out->append("cast to ", this->type);
        } break;
        default: {
            out->append(expr::strings[(u32)this->kind]);
        } break;
    }

    out->append(">");

    return out;
}

Type* Expr::
resolve_type() {
    return type;
}

CompileTime* CompileTime::
create(Type* type) {
    auto out = pool::add(compiler::instance.storage.comp_times);
    out->frame.locals = Array<Var*>::create();
    out->type = type;
    return out;
}

DString* CompileTime::
display() {
    return DString::create("CompileTime");
}

DString* CompileTime::
dump() {
    auto out = DString::create("CompileTime<");
    if(first_child()) {
        out->append(ScopedDeref(first_child()->dump()).x);
    } else {
        out->append("null");
    }
    out->append(">");
    return out;
}

ScalarLiteral* ScalarLiteral::
create() {
    auto out = pool::add(compiler::instance.storage.scalar_literals);
    return out;
}

DString* ScalarLiteral::
display() { 
    return value.display();
}

DString* ScalarLiteral::
dump() {
    return DString::create("ScalarLiteral<", ScopedDeref(type->display()).x, " ", ScopedDeref(value.display()).x, ">");
}

void ScalarLiteral::
cast_to(scalar::kind k) {
    value.cast_to(k);
    switch(k) {
        case scalar::unsigned64: type = &scalar::_u64; break;
        case scalar::unsigned32: type = &scalar::_u32; break;
        case scalar::unsigned16: type = &scalar::_u16; break;
        case scalar::unsigned8: type = &scalar::_u8; break;
        case scalar::signed64: type = &scalar::_s64; break;
        case scalar::signed32: type = &scalar::_s32; break;
        case scalar::signed16: type = &scalar::_s16; break;
        case scalar::signed8: type = &scalar::_s8; break;
        case scalar::float64: type = &scalar::_f64; break;
        case scalar::float32: type = &scalar::_f32; break;
    }
}

void ScalarLiteral::
cast_to(Type* t) {
    Assert(t->is<Scalar>());
    cast_to(t->as<Scalar>()->kind);
}

b32 ScalarLiteral::
is_signed() {
    return value.is_signed();
}

b32 ScalarLiteral::
is_float() {
    return value.is_float();
}

b32 ScalarLiteral::
is_negative() {
    return value.is_negative();
}

StringLiteral* StringLiteral::
create() {
    return pool::add(compiler::instance.storage.string_literals);
}

DString* StringLiteral::
display() {
    return DString::create("\"", raw, "\"");
}

DString* StringLiteral::
dump() {
    return DString::create("StringLiteral<", ScopedDeref(display()).x, ">");
}

ArrayLiteral* ArrayLiteral::
create() {
    auto out = pool::add(compiler::instance.storage.array_literals);
    return out;
}

DString* ArrayLiteral::
display() {
    auto out = DString::create();
    out->append("[");
    for(Expr* e = first_child<Expr>(); e; e = e->next<Expr>()) {
        out->append(ScopedDeref(e->display()).x, ",");
    }
    out->append("]");
    return out;
}

DString* ArrayLiteral::
dump() {
    return DString::create("ArrayLiteral<", ScopedDeref(display()).x, ">");
}

void ArrayLiteral::
cast_to(Type* t) {
    for(Expr* e = first_child<Expr>(); e; e = e->next<Expr>()) {
        if(type->as<StaticArray>()->type->is<Scalar>()) {
            if(e->is<ScalarLiteral>()) {
                e->as<ScalarLiteral>()->cast_to(t);
            } else {
                auto cast = Expr::create(expr::cast);
                cast->type = t;
                cast->start = e->start;
                cast->end = e->end;
                node::insert_above(e, cast);
            }
        }
    }
}

Block* Block::
create() {
    Block* out = pool::add(compiler::instance.storage.blocks);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::block;
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

DString* Block::
display() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* Block::
dump() {
    return DString::create("Block<", (type? ScopedDeref(type->display()).x->fin : "unknown type"), ">");
}

Call*
Call::create() {
    Call* out = pool::add(compiler::instance.storage.calls);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::call;
    return out;
}

DString* Call::
display() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* Call::
dump() {
    return DString::create("Call<", 
                (callee? ScopedDeref(callee->display()).x->fin : "null callee"), ", ", 
                (arguments? ScopedDeref(arguments->display()).x->fin : "null args"), ">");
}

VarRef* VarRef::
create() {
    VarRef* out = pool::add(compiler::instance.storage.varrefs);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::varref;
    return out;
}

DString* VarRef::
display() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* VarRef::
dump() {
    return DString::create("VarRef<", (var? ScopedDeref(var->display()).x->fin : "null var"), ">");
}

void
to_string(DString* start, Expr* e) {
    start->append("Expr<");
    // switch(e->kind) {
    //     case expr::typeref: {
    //         start->append("typeref:", e->type);
    //     } break;
    //     case expr::identifier: {
    //         start->append("id:'", e->node.start->raw, "'");
    //     } break;
    //     case expr::literal: {
    //         switch(e->node.start->kind) {
    //             case token::literal_character: start->append("chr lit:'", e->node.start->raw, "'"); break;
    //             case token::literal_float:     start->append("flt lit:", e->node.start->f64_val); break;
    //             case token::literal_integer:   start->append("int lit:", e->node.start->s64_val); break;
    //             case token::literal_string:    start->append("str lit:'", e->node.start->raw, "'"); break;
    //         }
    //     } break;
    //     case expr::varref: {
    //         start->append("varref: ", ((VarRef*)e)->place);
    //     } break;
    //     default: {
    //         start->append(expr::strings[(u32)e->kind]);
    //     } break;
    // }

    auto a = &start;

    start->append(">");
}

} // namespace amu