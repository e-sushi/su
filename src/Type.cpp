namespace amu {
namespace type {

Type*
create() {
    Type* out = pool::add(compiler::instance.storage.types);
    return out; 
}

namespace internal {

b32 
is_builtin(Type* t) {
    NotImplemented;
    return 0;//!t->structure->kind;
}

} // namespace internal


b32
can_coerce(Type* to, Type* from) { 
    if(from == to) return true;

    // all scalar types may coerce to each other
    // TODO(sushi) this should probably not allow float <-> int coercion in implicit cases, though
    if(to->kind == type::kind::scalar && from->kind == type::kind::scalar)
        return true;

    // pointers may coerce freely
    // TODO(sushi) stronger rules may be safer, though
    if(to->kind == type::kind::pointer && from->kind == type::kind::pointer)
        return true;

    // this may be too loose a rule
    // this allows arrays that have equal sizes but different types to coerce between each other
    // but I have no idea if this is a good idea or not 
    if(to->kind == type::kind::array && from->kind == type::kind::array) {
        ArrayType* to = to,* from = from;
        return to->size == from->size && can_coerce(to->type, from->type);
    }

    // allow implicit coercion of an array to its data pointer
    // this may not be a good idea either
    if(to->kind == type::kind::pointer && from->kind == type::kind::array) {
        auto pto = (PointerType*)to;
        auto pfrom = (ArrayType*)from;
        return pto->type == pfrom->type;
    }

    return false;
}   

} // namespace type



} // namespace amu

