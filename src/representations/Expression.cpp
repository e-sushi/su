namespace amu {

Expr*
Expr::create(expr::kind kind, Type* type) {
    Expr* out = pool::add(compiler::instance.storage.expressions);
    out->node.kind = node::expression;
    out->kind = kind;
    out->type = type;
    return out;
}

Block*
Block::create() {
    Block* out = pool::add(compiler::instance.storage.blocks);
    node::init(&out->node);
    out->node.kind = node::expression;
    out->kind = expr::block;
    out->table = label::table::init((TNode*)out);
    return out;
}

Call*
Call::create() {
    Call* out = pool::add(compiler::instance.storage.calls);
    node::init(&out->node);
    out->node.kind = node::expression;
    out->kind = expr::call;
    return out;
}

PlaceRef*
PlaceRef::create() {
    PlaceRef* out = pool::add(compiler::instance.storage.placerefs);
    node::init(&out->node);
    out->node.kind = node::expression;
    out->kind = expr::placeref;
    return out;
}

void
to_string(DString& start, Expr* e) {
    dstring::append(start, "Expr<");
    switch(e->kind) {
        case expr::typeref: {
            dstring::append(start, "typeref:", e->type);
        } break;
        case expr::identifier: {
            dstring::append(start, "id:'", e->node.start->raw, "'");
        } break;
        case expr::literal: {
            switch(e->node.start->kind) {
                case token::literal_character: dstring::append(start, "chr lit:'", e->node.start->raw, "'"); break;
                case token::literal_float:     dstring::append(start, "flt lit:", e->node.start->f64_val); break;
                case token::literal_integer:   dstring::append(start, "int lit:", e->node.start->s64_val); break;
                case token::literal_string:    dstring::append(start, "str lit:'", e->node.start->raw, "'"); break;
            }
        } break;
        case expr::placeref: {
            dstring::append(start, "placeref: ", ((PlaceRef*)e)->place);
        } break;
        default: {
            dstring::append(start, expr::strings[(u32)e->kind]);
        } break;
    }

    auto a = &start;

    dstring::append(start, ">");
}

} // namespace amu