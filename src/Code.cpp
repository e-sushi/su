namespace amu {
namespace code {

Code*
create() {
    return pool::add(compiler::instance.storage.code);
}

Code*
from(Source* source) {
    Code* out = code::create();
    out->raw = source->buffer;
    out->source = source;
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
    return code->tokens;
}

Array<Token>&
get_token_array(Code* code) {
    if(code::is_virtual(code)) {
        return ((VirtualCode*)code)->vtokens;
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

namespace format {

} // namespace format

} // namespace code
} // namespace amu