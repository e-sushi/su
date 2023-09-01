namespace amu {

Expr*
Expr::create(expr::kind kind, Type* type) {
    Expr* out = pool::add(compiler::instance.storage.expressions);
    // out->node.kind = node::expression;
    out->kind = kind;
    out->type = type;
    return out;
}

String 
Expr::name() {
    return "Expression";
}

DString
Expr::debug_str() {
    DString out = dstring::init("Expr<");

    switch(this->kind) {
        case expr::typeref: {
            dstring::append(out, "typeref:", this->type);
        } break;
        case expr::identifier: {
            dstring::append(out, "id:'", this->start->raw, "'");
        } break;
        case expr::literal: {
            switch(this->start->kind) {
                case token::literal_character: dstring::append(out, "chr lit:'", this->start->raw, "'"); break;
                case token::literal_float:     dstring::append(out, "flt lit:", this->start->f64_val); break;
                case token::literal_integer:   dstring::append(out, "int lit:", this->start->s64_val); break;
                case token::literal_string:    dstring::append(out, "str lit:'", this->start->raw, "'"); break;
            }
        } break;
        default: {
            dstring::append(out, expr::strings[(u32)this->kind]);
        } break;
    }

    dstring::append(out, ">");

    return out;
}

Type*
Expr::resolve_type() {
    return type;
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

String Block::
name() { // !Leak TODO(sushi) get this to print something nicer
    return debug_str();
}

DString Block::
debug_str() {
    return dstring::init("Block<", (type? type->name() : "unknown type"), ">");
}

Call*
Call::create() {
    Call* out = pool::add(compiler::instance.storage.calls);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::call;
    return out;
}

String Call::
name() { // !Leak TODO(sushi) get this to print something nicer
    return debug_str();
}

DString Call::
debug_str() {
    return dstring::init("Call<", 
                (callee? callee->name() : "null callee"), ", ", 
                (arguments? arguments->name() : "null args"), ">");
}

VarRef* VarRef::
create() {
    VarRef* out = pool::add(compiler::instance.storage.varrefs);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::varref;
    return out;
}

String VarRef::
name() { // !Leak TODO(sushi) get this to print something nicer
    return debug_str();
}

DString VarRef::
debug_str() {
    return dstring::init("VarRef<", (var? var->name() : "null var"), ">");
}

void
to_string(DString& start, Expr* e) {
    dstring::append(start, "Expr<");
    // switch(e->kind) {
    //     case expr::typeref: {
    //         dstring::append(start, "typeref:", e->type);
    //     } break;
    //     case expr::identifier: {
    //         dstring::append(start, "id:'", e->node.start->raw, "'");
    //     } break;
    //     case expr::literal: {
    //         switch(e->node.start->kind) {
    //             case token::literal_character: dstring::append(start, "chr lit:'", e->node.start->raw, "'"); break;
    //             case token::literal_float:     dstring::append(start, "flt lit:", e->node.start->f64_val); break;
    //             case token::literal_integer:   dstring::append(start, "int lit:", e->node.start->s64_val); break;
    //             case token::literal_string:    dstring::append(start, "str lit:'", e->node.start->raw, "'"); break;
    //         }
    //     } break;
    //     case expr::varref: {
    //         dstring::append(start, "varref: ", ((VarRef*)e)->place);
    //     } break;
    //     default: {
    //         dstring::append(start, expr::strings[(u32)e->kind]);
    //     } break;
    // }

    auto a = &start;

    dstring::append(start, ">");
}

} // namespace amu