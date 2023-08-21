namespace amu {
namespace code {

SourceCode*
from(Source* source) {
    SourceCode* out = pool::add(compiler::instance.storage.source_code);
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

namespace format {

} // namespace format

} // namespace code
} // namespace amu