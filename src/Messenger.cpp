namespace amu {
namespace messenger { // ---------------------------------------------------- messenger

namespace internal {

MessageFormatting* current_formatting;

void
process_part(DString& current, MessagePart& part) {
    switch(part.type) {
        case MessagePart::Plain: {
            dstring::append(current, part.plain);
        } break;
        case MessagePart::Variable: {
            NotImplemented;
        } break;
        case MessagePart::Structure: {
            NotImplemented;
        } break;
        case MessagePart::Function: {
            NotImplemented;
        } break;
        case MessagePart::Module: {
            NotImplemented;
        } break;
        case MessagePart::Label: {
            NotImplemented;
        } break;
        case MessagePart::Code: {
            NotImplemented;
        } break;
    }
}

// processes a given Message into a str8
void
process_message(DString& current, Message& m) {
    forI(m.parts.count) {
        process_part(current, array::readref(m.parts, i));
    }
    dstring::append(current, "\n");
}

} // namespace internal

Messenger instance;

void 
init() {
    instance.messages = array::init<Message>();
    instance.delivering = mutex_init();

    MessageFormatting def = {
        .allow_color = true,
        .warning = {
            .default_color = Color_Yellow,
            .prefix = "warning:"
        }
        .error = {
            
        }
    }

}

void
dispatch(Message message) {
    array::push(instance.messages, message);
}

// dispatch a plain str8
void
dispatch(String message, Source* source) {
    Message m = message::init(source);
    MessagePart part; 
    part.type = MessagePart::Plain;
    //part.plain = message;
    message::push(m, part);
    dispatch(message);
}

void 
deliver(Destination destination, b32 clear_messages) {
    array::lock(instance.messages);
    mutex_lock(&instance.delivering);

    DString out = dstring::init();

    forI(instance.messages.count) {
        internal::process_message(out, 
            array::readref(instance.messages, i));
    }

    switch(destination.type) {
        case Destination::STLFILE: {
            size_t bytes_written = fwrite(out.s.str, 1, out.s.count, destination.stl_file);
            printf("%zu\n", bytes_written);
        } break;
        case Destination::DESHFILE: {
            file_append(destination.desh_file, out.s.str, out.s.count);
        } break;
    }
}

void
push_formatting(MessageFormatting formatting) {
    array::push(instance.formatting_stack);
}

void 
pop_formatting() {
    array::pop(instance.formatting_stack);
}

} // namespace messenger

namespace message { // ---------------------------------------------------- message

Message
init(Source* source) {
    Message out;
    out.time = time(0);
    out.source = source;
    out.parts = array::init<MessagePart>();
    return out;
}

template<typename... T> Message
init(Source* source, T... args) {
    Message out = init(source);
    MessagePart arr[sizeof...(T)] = {args...};
    forI(sizeof...(T)) {
        push(out, arr[i]);
    }
    return out;
}

void
push(Message& m, MessagePart part) {
    array::push(m.parts, part);
}

void
prefix(Message& m, MessagePart part) {
    array::insert(m.parts, 0, part);
}

template<typename... T>
void
prefix(Message& m, T... args) {
    MessagePart arr[sizeof...(T)] = {args...};
    forI_reverse(sizeof...(T)) {
        prefix(m, arr[i]);
    }
}

MessagePart
plain(String s, color c) {
    MessagePart out;
    out.type = MessagePart::Plain;
    out.col = c;
    out.plain = s;
    return out;
}

MessagePart
path(String s, color c) {
    MessagePart out;
    out.type = MessagePart::Path;
    out.col = c;
    out.plain = s;
    return out;
}

} // namespace message
} // namespace amu