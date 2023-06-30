namespace amu {
namespace messenger { // ---------------------------------------------------- messenger

namespace internal {
    
DString
process_part(Message::Part& part) {
    dstr8 out; dstr8_init(&out, str8l(""), deshi_allocator);
    switch(part.type) {
        case Message::Part::Plain: {
            return part.plain;
        } break;
        case Message::Part::Entity: {
            NotImplemented;
        } break;
        case Message::Part::Label: {
            NotImplemented;
        } break;
        case Message::Part::Code: {
            NotImplemented;
        } break;
    }
    return {};
}

// processes a given Message into a str8
dstr8 
process_message(Message& m) {
    forI(m.parts.count) {
        process_part(array::readref(m.parts, i));
    }
    return {};
}

} // namespace internal

Messenger instance;

void
dispatch(Message message) {
    array::push(instance.messages, message);
}

// dispatch a plain str8
void
dispatch(str8 message, Source* source) {
    Message m = message::init(source);
    Message::Part part; 
    part.type = Message::Part::Plain;
    //part.plain = message;
    message::push(m, part);
    dispatch(message);
}

void 
deliver() {
    array::lock(instance.messages);
    mutex_lock(&instance.delivering);

    //dstr8 processed;

    //forI(instance.messages.count) {
    //    array::push(processed,
    //        internal::process_message(instance.messages.data[i]));
    //}
}

} // namespace messenger

namespace message { // ---------------------------------------------------- message

Message
init(Source* source) {
    Message out;
    out.time = time(0);
    out.source = source;
    out.parts = array::init<Message::Part>();
    return out;
}

template<typename... T> Message
init(Source* source, T... args) {
    Message out = init(source);
    Message::Part arr[sizeof...(T)] = {args...};
    forI(sizeof...(T)) {
        push(out, arr[i]);
    }
    return out;
}

void
push(Message& m, Message::Part part) {
    array::push(m.parts, part);
}

} // namespace message
} // namespace amu