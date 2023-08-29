namespace amu {

Structure*
Structure::create() {
    Structure* out = pool::add(compiler::instance.storage.structures);
    node::init(&out->node);
    out->node.kind = node::structure;
    out->table = label::table::init(&out->node);
    return out;
}


// namespace type::structure {
// Array<ExistingStructureType> set = amu::array::init<ExistingStructureType>();
// } // namespace structure

// Structured* Structured::
// create(Structure* s) {
//     auto [idx, found] = amu::array::util::
//         search<type::structure::ExistingStructureType, Structure*>(type::structure::set, s, [](type::structure::ExistingStructureType& s) { return s.structure; });
//     if(found) return amu::array::read(type::structure::set, idx).stype;
//     type::structure::ExistingStructureType* nu = amu::array::insert(structure::set, idx);
//     nu->stype = pool::add(compiler::instance.storage.structured_types);
//     nu->stype->kind = type::kind::structured;
//     nu->stype->node.kind = node::type;
//     nu->stype->structure = s;
//     nu->structure = s;
//     return nu->stype;
// }

Structured* Structured::
create(Structure* s) {
    auto out = pool::add(compiler::instance.storage.structured_types);
    out->kind = type::kind::structured;
    out->node.kind = node::type;
    out->structure = s;
    return out;
}

Label* Structured::
find_member(String id) {
    Structure* s = this->structure;
    auto [idx, found] = map::find(s->table.map, id);
    if(!found) return 0;
    return amu::array::read(s->table.map.values, idx);
}

void
to_string(DString& start, Structure* s) {
    dstring::append(start, "Structure");
}

} // namespace amu