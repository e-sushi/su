namespace amu {
namespace messenger { // ---------------------------------------------------- messenger

namespace internal {

Destination* current_dest;

// wraps a DString in ANSI terminal color
// TODO(sushi) need to setup using the 8 original colors so that terminal themes work properly
void
wrap_color(DString& current, u32 color) {
    DString temp = dstring::init("\e[", color, "m");
    dstring::prepend(current, temp);
    dstring::append(current, "\e[0m");
    dstring::deinit(temp);
}

void
process_part(DString& current, MessagePart& part) {
    MessageFormatting& formatting = shared_array::readref(instance.formatting_stack, -1);
    switch(part.kind) {
        case messagepart::plain: {
            DString temp = dstring::init(part.plain);
            if(current_dest->allow_color && formatting.plain.col) {
                wrap_color(temp, (part.col? part.col : formatting.plain.col));
            }
            dstring::prepend(temp, formatting.plain.prefix);
            dstring::append(temp, formatting.plain.suffix);
            dstring::append(current, temp);
            dstring::deinit(temp);
        } break;
        case messagepart::path: {
            DString temp = dstring::init(part.plain);
            // TODO(sushi) implement fancy path formatting
            if(current_dest->allow_color) {
                wrap_color(temp, (part.col? part.col : formatting.path.col.directory));
            }
            dstring::prepend(temp, formatting.path.prefix);
            dstring::append(temp, formatting.path.suffix);
            dstring::append(current, temp);
            dstring::deinit(temp);

        } break;
        case messagepart::token: {
            DString temp = dstring::init(part.token.raw);
            if(current_dest->allow_color) {
                wrap_color(temp, formatting.token.col);
            }
            dstring::prepend(temp, formatting.token.prefix);
            if(formatting.token.show_code_loc) {
                dstring::append(temp, "(", part.token.l0, ",", part.token.c0, ")");
            }
            dstring::append(temp, formatting.token.suffix);
            dstring::append(current, temp);
            dstring::deinit(temp);
        } break;
        case messagepart::identifier: {
            DString temp = dstring::init(part.plain);
            if(current_dest->allow_color) {
                wrap_color(temp, formatting.identifier.col);
            }
            dstring::prepend(temp, formatting.identifier.prefix);
            dstring::append(temp, formatting.identifier.suffix);
            dstring::append(current, temp);
            dstring::deinit(temp);
        }break;
        case messagepart::place: {
            NotImplemented;
        } break;
        case messagepart::structure: {
            NotImplemented;
        } break;
        case messagepart::function: {
            NotImplemented;
        } break;
        case messagepart::module: {
            NotImplemented;
        } break;
        case messagepart::label: {
            NotImplemented;
        } break;
        case messagepart::code: {
            NotImplemented;
        } break;
    }
}

// processes a given Message into a str8
// TODO(sushi) decide if the color of message type prefixes will be customizable and if not
//             store a static string of them wrapped in their colors
void
process_message(DString& current, Message& m) {
    // dont do anything if current verbosity is less than the message's
    if(m.type == Message::Debug && m.verbosity > compiler::instance.options.verbosity) return; 

    switch(m.sender.type) {
        case MessageSender::Compiler: {
            static constexpr String compiler_prefix = "\e[36mcompiler\e[0m: ";
            dstring::append(current, compiler_prefix);
        } break;
        case MessageSender::Source: {
            DString temp = dstring::init(String(m.sender.source->file->name));
            wrap_color(temp, message::color_cyan);
            dstring::append(current, temp, ": ");
            dstring::deinit(temp);
        } break;
        case MessageSender::SourceLoc: {
            DString temp = dstring::init(String(m.sender.source->file->name));
            wrap_color(temp, message::color_cyan);
            dstring::append(current, temp, ":", m.sender.line, ":", m.sender.column, ": ");
            dstring::deinit(temp);
            
        } break;
    }

    switch(m.type) {
        case Message::Normal: { // dont need to do anything special
        } break;

        case Message::Debug: { 
            DString temp = dstring::init("debug");
            wrap_color(temp, message::color_green);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case Message::Warning: {
            DString temp = dstring::init("warning");
            wrap_color(temp, message::color_yellow);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case Message::Error: {
            DString temp = dstring::init("error");
            wrap_color(temp, message::color_red);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case Message::Note: {
            DString temp = dstring::init("note");
            wrap_color(temp, message::color_magenta);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;
    }

    forI(m.parts.count) {
        process_part(current, array::readref(m.parts, i));
    }

    dstring::append(current, "\n");
}

} // namespace internal

Messenger instance;

void 
init() {
    instance.messages = shared_array::init<Message>();
    instance.formatting_stack = shared_array::init<MessageFormatting>();
    instance.delivering = mutex_init();

    push_formatting(MessageFormatting());
}

void
dispatch(Message message) {
    if(message.type == Message::Debug && compiler::instance.options.deliver_debug_immediately) {
        Array<Message> temp = array::init<Message>(1); // this is weird, change it so that we can just deliver one message immediately
        array::push(temp, message);
        deliver(stdout, temp);
        array::deinit(temp);
    } else shared_array::push(instance.messages, message);
}

// dispatch a plain str8
void
dispatch(String message, Source* source) {
    Message m = message::init(source);
    MessagePart part; 
    part.kind = messagepart::plain;
    //part.plain = message;
    message::push(m, part);
    dispatch(message);
}

void 
deliver(Destination destination, b32 clear_messages) {
    Array<Message> temp = shared_array::lock(instance.messages);
    defer{ shared_array::unlock(instance.messages, temp); };
    deliver(destination, temp);
}


void
deliver(Destination destination, Array<Message> messages) {
    mutex_lock(&instance.delivering);
    defer{ mutex_unlock(&instance.delivering); };

    DString out = dstring::init();

    internal::current_dest = &destination;

    forI(messages.count) {
        internal::process_message(out, 
            array::readref(messages, i));
    }

    switch(destination.kind) {
        case Destination::STLFILE: {
            size_t bytes_written = fwrite(out.s.str, 1, out.s.count, destination.stl_file);
        } break;
        case Destination::DESHFILE: {
            file_append(destination.desh_file, out.s.str, out.s.count);
        } break;
    }
}

void
push_formatting(MessageFormatting formatting) {
    shared_array::push(instance.formatting_stack, formatting);
}

void 
pop_formatting() {
    shared_array::pop(instance.formatting_stack);
}

} // namespace messenger

namespace message { // ---------------------------------------------------- message

Message
init() {
    Message out;
    out.time = time(0);
    out.parts = array::init<MessagePart>();
    return out;
}

Message
init(String s) {
    Message out = init();
    push(out, s);
    return out;
}

// TODO(sushi) there has just GOT to be a way for me to pass string literals to this function and them still
//             be turned into Strings at compile time
template<typename... T> Message
init(T... args) {
    Message out = init();
    MessagePart arr[sizeof...(T)] = {args...};
    forI(sizeof...(T)) {
        push(out, arr[i]);
    }
    return out;
}

// initialize a debug message
Message
debug(u32 verbosity) {
    Message out = init();
    out.type = Message::Debug;
    out.verbosity = verbosity;
    return out;
}

// initialize a debug message with variadic arguments
template<typename... T> Message
debug(u32 verbosity, T... args) {
    Message out = debug(verbosity);
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
plain(String s, u32 c) {
    MessagePart out;
    out.kind = messagepart::plain;
    out.col = c;
    out.plain = s;
    return out;
}

MessagePart
path(String s, u32 c) {
    MessagePart out;
    out.kind = messagepart::path;
    out.col = c;
    out.plain = s;
    return out;
}

MessagePart
identifier(String s) {
    MessagePart out;
    out.kind = messagepart::identifier;
    out.plain = s;
    return out;
}

Message
attach_sender(MessageSender sender, Message m) {
    m.sender = sender;
    return m;
}

} // namespace message
} // namespace amu