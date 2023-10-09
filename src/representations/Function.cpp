#include "systems/Messenger.h"
namespace amu {

Function* Function::
create(FunctionType* type) {
    Function* out = pool::add(compiler::instance.storage.functions);
    out->type = type;
    return out;
}


DString* Function::
display() {
    return label->display();
}

DString* Function::
dump() {
    return DString::create(
        "Function<", 
            ScopedDeref(label->display()).x, 
            ": ", 
            ScopedDeref(type->display()).x, 
        ">");
}

void
to_string(DString* start, Function* f) {
    start->append("Function<TODO>");
}

} // namespace amu 
