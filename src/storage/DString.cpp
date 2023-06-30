namespace amu{
namespace dstring {

DString
init(String s) {
    DString out;
    dstr8_init(&out.s, s.s, amu_allocator);
    return out;
}

void
deinit(DString& s) {
    dstr8_deinit(&s.s);
}

// appends 'b' to 'a'
void
append(DString& a, String b) {
    dstr8_append(&a.s, b.s);
}

// appends 'b' to 'a'
void
append(DString& a, DString& b) {
    dstr8_append(&a.s, b.s.fin);
}

// concatenates two Strings into a new String
DString
concat(DString& a, DString& b) {
    DString out = init(a);
    append(out, b);
    return out;
}

// concatenates a String to a String and returns a new String
DString
concat(DString& a, String b) {
    DString out = init();
    append(out, b);
    return out;
}

// concatenates a String to a String and returns a new String
DString
concat(String a, DString& b) {
    DString out = init(a);
    append(out, b);
    return out;
}

} // namespace string
} // namespace amu