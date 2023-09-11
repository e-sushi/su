namespace amu {

Member* Member::
create() {
    auto out = pool::add(compiler::instance.storage.members);
    return out;
}

DString* Member::
display() {
    return label->display();
}

DString* Member::
dump() {
    return DString::create("Member<", ScopedDeref(label->display()).x, ">");
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

DString* Structure::
display_members_from_address(u8* start) {
    auto out = DString::create();
    for(Member* m = first_member; m; m = m->next<Member>()) {
        out->append(ScopedDeref(m->label->display()).x, ": ", *(f32*)(start + m->offset), "\n");
    }

    return out;
}

} // namespace amu