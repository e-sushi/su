namespace amu {

Member* Member::
create() {
    auto out = pool::add(compiler::instance.storage.members);
    return out;
}

String Member::
name() {
    return label->name();
}

DString Member::
debug_str() {
    return dstring::init("Member<", label->name(), ">");
}

Structure*
Structure::create() {
    Structure* out = pool::add(compiler::instance.storage.structures);
    out->members = map::init<String, Member*>();
    return out;
}

Member* Structure::
find_member(String s) {
    auto [idx, found] = map::find(members, s);
    if(!found) return 0;
    return amu::array::read(members.values, idx);
}

Member* Structure::
add_member(String id) {
    auto out = pool::add(compiler::instance.storage.members); 
    map::add(members, id, out);
    return out;
}

void Structure::
add_member(String id, Member* m) {
    map::add(members, id, m);
}

} // namespace amu