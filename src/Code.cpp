namespace amu {
namespace code {

SourceCode*
from(Source* source) {
    SourceCode* out = pool::add(compiler::instance.storage.source_code);
    out->raw = source->buffer;
    out->source = source;
    return out;
}

Code*
from(Code* code, Token* start, Token* end) {
    Code* out;
    if(code->source) {
        SourceCode* sc = pool::add(compiler::instance.storage.source_code);
        sc->tokens.data = start;
        sc->tokens.count = end-start;
        out = (Code*)sc;
    } else {
        auto c = (VirtualCode*)code;
        VirtualCode* vc = pool::add(compiler::instance.storage.virtual_code);
        vc->tokens = array::copy(c->tokens, start-c->tokens.data, end-start);
    }
    out->source = code->source;
    out->raw.str = start->raw.str;
    out->raw.count = end->raw.str - start->raw.str;

    return out;
}

b32
is_virtual(Code* code) {
    return !code->source;
}

String
name(Code* code) {
    if(code::is_virtual(code)) {
        return ((VirtualCode*)code)->name;
    } else {
        return code->source->name;
    }
}

View<Token>
get_tokens(Code* code) {
    if(is_virtual(code)) {
        return array::view(((VirtualCode*)code)->tokens);
    } else {
        return ((SourceCode*)code)->tokens;
    }
}


Array<Token>&
get_token_array(Code* code) {
    if(is_virtual(code)) {
        return ((VirtualCode*)code)->tokens;
    } else {
        return code->source->tokens;
    }
}

void
add_diagnostic(Code* code, Diagnostic d) {
    if(code::is_virtual(code)) {
        array::push(((VirtualCode*)code)->diagnostics, d);
    } else {
        array::push(code->source->diagnostics, d);
    }
}

TokenIterator
token_iterator(Code* code) {
    TokenIterator out;
    out.code = code;
    out.curt = code::get_tokens(code).data;
    return out;
}

Token*
current(TokenIterator& iter) {
    return iter.curt;
}

Token*
next(TokenIterator& iter) {
    if(iter.curt == iter.stop) return 0;
#if BUILD_SLOW
    iter.curt++;
    if(iter.curt->kind == token::directive_compiler_break) {
        iter.curt++;
        DebugBreakpoint;
    }
    return iter.curt;
#else
    return ++iter.curt;
#endif
}

Token*
lookahead(TokenIterator& iter, u64 n) {
    if(iter.curt + n > iter.stop) return 0;
    return iter.curt + n;
}

Token*
lookback(TokenIterator& iter, u64 n) {
    if(iter.curt - n < code::get_tokens(iter.code).data) return 0;
    return iter.curt - n;
}

void
skip_to_matching_pair(TokenIterator& iter) {
    u64 nesting = 0;
    switch(iter.curt->kind) {
        case token::open_brace: { 
            while(next(iter)) {
                if(current(iter)->kind == token::open_brace) nesting++;
                else if(current(iter)->kind == token::close_brace) {
                    if(nesting) nesting--; 
                    else return;
                }
            }
        } break;
        case token::open_paren: {
            while(next(iter)) {
                if(current(iter)->kind == token::open_paren) nesting++;
                else if(current(iter)->kind == token::close_paren) {
                    if(nesting) nesting--;
                    else return;
                }
            }
        } break;
        case token::open_square: {
            while(next(iter)) {
                if(current(iter)->kind == token::open_square) nesting++;
                else if(current(iter)->kind == token::close_square) {
                    if(nesting) nesting--;
                    else return;
                }
            }
        } break;
    }
}


template<typename... T> b32
is_any(TokenIterator& iter, T... args) {
    return ((current(iter)->kind == args) || ...);
}

template<typename... T> void
skip_until(TokenIterator& iter, T... args) {
    while(!is_any(iter, args...)) next(iter);
}

namespace format {

} // namespace format

} // namespace code
} // namespace amu