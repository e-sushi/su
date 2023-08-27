namespace amu {
namespace code {

SourceCode*
from(Source* source) {
    SourceCode* out = pool::add(compiler::instance.storage.source_code);
    out->raw = source->buffer;
    out->source = source;
    out->kind = code::source;
    out->node.kind = node::code;
    out->name = source->name;
    return out;
}

Code*
from(Code* code, Token* start, Token* end) {
    Code* out;
    if(code->source) {
        SourceCode* sc = pool::add(compiler::instance.storage.source_code);
        sc->tokens.data = start;
        sc->tokens.count = end-start+1;
        out = (Code*)sc;
    } else {
        auto c = (VirtualCode*)code;
        VirtualCode* vc = pool::add(compiler::instance.storage.virtual_code);
        vc->tokens = array::copy(c->tokens, start-c->tokens.data, end-start+1);
    }
    out->source = code->source;
    out->raw.str = start->raw.str;
    out->raw.count = end->raw.str - start->raw.str;
    out->node.kind = node::code;

    out->name = dstring::init(code->name, ":subcode<", start, ",", end, ">");

    return out;
}

b32
is_virtual(Code* code) {
    return !code->source;
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

TokenIterator::TokenIterator(Code* code) {
    this->code = code;
    this->curt = view::readptr(code::get_tokens(code),  0);
    this->stop = view::readptr(code::get_tokens(code), -1);
}

FORCE_INLINE Token*
TokenIterator::current() {
    return this->curt;
}

FORCE_INLINE token::kind
TokenIterator::current_kind() {
    return this->curt->kind;
}

FORCE_INLINE Token* 
TokenIterator::increment() {
    if(this->curt == this->stop) return 0;
#if BUILD_SLOW
    this->curt++;
    if(this->curt->kind == token::directive_compiler_break) {
        this->curt++;
        DebugBreakpoint;
    }
    return this->curt;
#else
    return ++this->curt;
#endif
}

FORCE_INLINE Token* 
TokenIterator::next() { 
    return lookahead(1); 
}


FORCE_INLINE token::kind
TokenIterator::next_kind() { 
    Token* t = next(); 
    if(!t) return token::null; 
    return t->kind; 
}

FORCE_INLINE Token*
TokenIterator::prev() { 
    return lookback(1); 
}

FORCE_INLINE token::kind
TokenIterator::prev_kind() {
    Token* t = prev();
    if(!t) return token::null;
    return t->kind;
}


FORCE_INLINE Token*
TokenIterator::lookahead(u64 n) {
#if BUILD_SLOW
    Token* iter = this->curt;
    forI(n) {
        iter++;
        if(iter->kind == token::directive_compiler_break)
            iter++;
        if(iter > this->stop) return 0;
    }
    return iter;
#else
    if(this->curt + n > this->stop) return 0;
    return this->curt + n;
#endif
}

FORCE_INLINE Token*
TokenIterator::lookback(u64 n) {
#if BUILD_SLOW
    Token* iter = this->curt;
    forI(n) {
        iter--;
        if(iter->kind == token::directive_compiler_break)
            iter--;
        if(iter < code::get_tokens(this->code).data) return 0;
    }
    return iter;
#else
    if(this->curt - n < code::get_tokens(this->code).data) return 0;
    return this->curt - n;
#endif
}

void TokenIterator:: 
skip_to_matching_pair() {
    u64 nesting = 0;
    switch(this->curt->kind) {
        case token::open_brace: { 
            while(increment()) {
                if(current_kind() == token::open_brace) nesting++;
                else if(current_kind() == token::close_brace) {
                    if(nesting) nesting--; 
                    else return;
                }
            }
        } break;
        case token::open_paren: {
            while(increment()) {
                if(current_kind() == token::open_paren) nesting++;
                else if(current_kind() == token::close_paren) {
                    if(nesting) nesting--;
                    else return;
                }
            }
        } break;
        case token::open_square: {
            while(increment()) {
                if(current_kind() == token::open_square) nesting++;
                else if(current_kind() == token::close_square) {
                    if(nesting) nesting--;
                    else return;
                }
            }
        } break;
    }
}


template<typename... T> FORCE_INLINE void TokenIterator::
skip_until(T... args) {
    while(!is_any(args...)) increment();
}

FORCE_INLINE b32 TokenIterator::
is(u32 kind) {
    return current_kind() == kind;
}

template<typename... T> FORCE_INLINE b32 TokenIterator::
is_any(T... args) {
    return (is(args) || ...);
}

FORCE_INLINE b32 TokenIterator::
next_is(u32 kind) {
    return next_kind() == kind;
}

FORCE_INLINE b32 TokenIterator::
prev_is(u32 kind) {
    return prev_kind() == kind;
}

DString TokenIterator::
display_line() {
    u8* scan_left = curt->raw.str;

    while(scan_left != curt->code->raw.str && *scan_left != '\n')
        scan_left--;

    u8* scan_right = scan_left + 1;

    while(scan_right != curt->code->raw.str + curt->code->raw.count && *scan_right != '\n')
        scan_right++;

    String line = {scan_left, s32(scan_right - scan_left)};
    
    s32 depth = curt->raw.str - scan_left;

    DString out = dstring::init(line, "\n");

    forI(depth) {
        dstring::append(out, " ");
    }
    dstring::append(out, "^");

    return out;
}


namespace display {

} // namespace display

namespace format {

} // namespace format

namespace lines {

Lines 
get(Token* t, Options opt) {
    Lines out = {};
    out.lines = array::init<String>();
    out.opt = opt;
    out.token = t;
    
    u8* scan_left = t->raw.str;

    // scan to beginning of current line
    while(scan_left != t->code->raw.str && *scan_left != '\n')
        scan_left--;

    ScopedArray<s32> newlines = array::init<s32>();
    array::push(newlines, 0);

    // we need to store newlines as offsets into a string
    // so we don't collect indexes going backwards cause we'd 
    // have to readjust them anyways 
    u32 before = opt.before;
    forI(before) { 
        while(scan_left != t->code->raw.str && *scan_left != '\n')
            scan_left--;
        if(scan_left == t->code->raw.str) break;
    }

    u8* scan_right = scan_left;
    u32 n_lines = amu::util::Min(opt.before, before) + opt.after + 1;
    forI(n_lines) {
        while(scan_right != t->code->raw.str + t->code->raw.count && *scan_right != '\n')
            scan_right++;
        array::push(newlines, s32(scan_right++-scan_left) + 1);
        if(scan_right == t->code->raw.str) break;
    }

    // turn the entire thing into a DString
    out.str = dstring::init(String{
        scan_left, array::read(newlines, -1)
    });

    forI(newlines.count - 1) {
        array::push(out.lines, {
            out.str.str + array::read(newlines, i),
            array::read(newlines, i+1) - array::read(newlines, i) - 1 
        });
    }

    if(opt.remove_leading_whitespace) {
        u32 min_leading = MAX_U32;
        forI(out.lines.count) {
            String line = array::read(out.lines, i);
            if(!line.count) continue;
            s32 whitespace_len = string::eat_whitespace(line).count;
            if(whitespace_len == line.count) continue;
            min_leading = amu::util::Min(min_leading, whitespace_len);
            if(!min_leading) break;
        }

        if(min_leading) {
            DString nu = dstring::init();
            forI(out.lines.count) {
                String line = array::read(out.lines, i);
                if(!line.count) dstring::append(nu, "\n");
                else dstring::append(nu, String{line.str+min_leading, line.count-min_leading}, "\n");
            }
            DString save = out.str;
            out.str = nu;
            dstring::deinit(save);
        }

        out.lines = string::find_lines(out.str);
    }

    if(opt.line_numbers) {
        
    }

    return out;
}

} // namespace lines

} // namespace code

void
to_string(DString& current, Code* c) {
    if(code::is_virtual(c)) { // TODO(sushi) more info for virtual code whenever its actually used
        auto vc = (VirtualCode*)c;
        dstring::append(current, "VirtualCode<'", vc->name, "'>");
    } else {
        auto sc = (SourceCode*)c;
        dstring::append(current, "SourceCode<", code::strings[sc->kind], ">");
    }
}

DString
to_string(Code* c) {
    DString out = dstring::init();
    to_string(out, c);
    return out;
}

} // namespace amu