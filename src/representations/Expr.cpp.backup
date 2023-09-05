namespace amu {

Expr* Expr::
create(expr::kind kind, Type* type) {
    Expr* out = pool::add(compiler::instance.storage.expressions);
    // out->node.kind = node::expression;
    out->kind = kind;
    out->type = type;
    return out;
}

DString Expr::
name() { // TODO(sushi) switch on expr kind
    return dstring::init("Expression");
}

DString Expr::
dump() {
    DString out = dstring::init("Expr<");

    switch(this->kind) {
        case expr::typeref: {
            dstring::append(out, "typeref:", this->type);
        } break;
        case expr::identifier: {
            dstring::append(out, "id:'", this->start->raw, "'");
        } break;
        case expr::literal: {
            dstring::append(out, literal.dump());
        } break;
        case expr::cast: {
            dstring::append(out, "cast to ", this->type);
        } break;
        default: {
            dstring::append(out, expr::strings[(u32)this->kind]);
        } break;
    }

    dstring::append(out, ">");

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

DString Block::
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString Block::
dump() {
    return dstring::init("Block<", (type? type->name() : dstring::init("unknown type")), ">");
}

Call*
Call::create() {
    Call* out = pool::add(compiler::instance.storage.calls);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::call;
    return out;
}

DString Call::
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString Call::
dump() {
    return dstring::init("Call<", 
                (callee? callee->name().fin : "null callee"), ", ", 
                (arguments? arguments->name().fin : "null args"), ">");
}

VarRef* VarRef::
create() {
    VarRef* out = pool::add(compiler::instance.storage.varrefs);
    // node::init(&out->node);
    // out->node.kind = node::expression;
    out->kind = expr::varref;
    return out;
}

DString VarRef::
name() { // !Leak TODO(sushi) get this to print something nicer
    return dump();
}

DString VarRef::
dump() {
    return dstring::init("VarRef<", (var? var->name() : dstring::init("null var")), ">");
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