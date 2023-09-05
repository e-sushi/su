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
name() { // TODO(sushi) switch on expr kind
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
        case expr::literal: {
            out->append(ScopedDeref(literal.dump()).x);
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
// void Literal::
// cast_to(literal::kind k) {
//     #define inner(from)                             \
//         switch(k) {                                 \
//             case literal::u64_: from = _u64; break; \
//             case literal::u32_: from = _u32; break; \
//             case literal::u16_: from = _u16; break; \
//             case literal::u8_ : from = _u8 ; break; \
//             case literal::s64_: from = _s64; break; \
//             case literal::s32_: from = _s32; break; \
//             case literal::s16_: from = _s16; break; \
//             case literal::s8_ : from = _s8 ; break; \
//             case literal::f64_: from = _f64; break; \
//             case literal::f32_: from = _f32; break; \
//             default: {  \
//                 /* there shouldn't be a case where a tuple/array literal is casted to anything else  */ \
//                 Assert(0); \
//             } break; \
//         }
//      switch(kind) {
//         case literal::u64_: inner(_u64); break;
//         case literal::u32_: inner(_u32); break;
//         case literal::u16_: inner(_u16); break;
//         case literal::u8_ : inner(_u8 ); break;
//         case literal::s64_: inner(_s64); break;
//         case literal::s32_: inner(_s32); break;
//         case literal::s16_: inner(_s16); break;
//         case literal::s8_ : inner(_s8 ); break;
//         case literal::f64_: inner(_f64); break;
//         case literal::f32_: inner(_f32); break;
//         default: {
//             Assert(0);
//             // there shouldn't be a case where a tuple/array literal is casted to a scalar
//         } break;
//     }
// }

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
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* Block::
dump() {
    return DString::create("Block<", (type? ScopedDeref(type->name()).x->fin : "unknown type"), ">");
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
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* Call::
dump() {
    return DString::create("Call<", 
                (callee? ScopedDeref(callee->name()).x->fin : "null callee"), ", ", 
                (arguments? ScopedDeref(arguments->name()).x->fin : "null args"), ">");
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
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString* VarRef::
dump() {
    return DString::create("VarRef<", (var? ScopedDeref(var->name()).x->fin : "null var"), ">");
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