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
    MessageFormatting& formatting = array::readref(instance.formatting_stack, -1);
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
            DString temp = dstring::init(part.token->raw);
            if(current_dest->allow_color) {
                wrap_color(temp, formatting.token.col);
            }
            dstring::prepend(temp, formatting.token.prefix);
            if(formatting.token.show_code_loc) {
                dstring::append(temp, "(", part.token->l0, ",", part.token->c0, ")");
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
    if(m.kind == message::debug && m.verbosity > compiler::instance.options.verbosity) return; 

    // TODO(sushi) we need to support a way to detect when there are conflicting file names 
    //             and in those cases show a relative path instead of just the name
    switch(m.sender.type) {
        case MessageSender::Compiler: {
            static constexpr String compiler_prefix = "\e[36mamu\e[0m: ";
            dstring::append(current, compiler_prefix);
        } break;
        case MessageSender::Source: {
            DString temp = dstring::init(m.sender.source->name);
            if(current_dest->allow_color)
                wrap_color(temp, message::color_cyan);
            dstring::append(current, temp, ": ");
            dstring::deinit(temp);
        } break;
        case MessageSender::SourceLoc: {
            DString temp = dstring::init(m.sender.source->name);
            if(current_dest->allow_color)
                wrap_color(temp, message::color_cyan);
            dstring::append(current, temp, ":", m.sender.token->l0, ":", m.sender.token->c0, ": ");
            dstring::deinit(temp);
            
        } break;
    }

    switch(m.kind) {
        case message::normal: { // dont need to do anything special
        } break;

        case message::debug: { 
            DString temp = dstring::init("debug");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_green);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case message::warning: {
            DString temp = dstring::init("warning");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_yellow);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case message::error: {
            DString temp = dstring::init("error");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_red);
            dstring::append(current, temp);
            dstring::append(current, ": ");
            dstring::deinit(temp);
        } break;

        case message::note: {
            DString temp = dstring::init("note");
            if(current_dest->allow_color)
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
    instance.messages = array::init<Message>();
    instance.formatting_stack = array::init<MessageFormatting>();

    push_formatting(MessageFormatting());
}

void
dispatch(Message message) {
    if(message.kind == message::debug && compiler::instance.options.deliver_debug_immediately) {
        Array<Message> temp = array::init<Message>(1); // this is weird, change it so that we can just deliver one message immediately
        array::push(temp, message);
        deliver(stdout, temp);
        array::deinit(temp);
    } else array::push(instance.messages, message);
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
    deliver(destination, instance.messages);
}


void
deliver(Destination destination, Array<Message> messages) {
    DString out = dstring::init();

    internal::current_dest = &destination;

    forI(messages.count) {
        internal::process_message(out, 
            array::readref(messages, i));
    }

    if(compiler::instance.options.quiet && destination.file == stdout) return;
    fwrite(out.str, 1, out.count, destination.file);
}

void
push_formatting(MessageFormatting formatting) {
    array::push(instance.formatting_stack, formatting);
}

void 
pop_formatting() {
    array::pop(instance.formatting_stack);
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
make_debug(u32 verbosity) {
    Message out = init();
    out.kind = message::debug;
    out.verbosity = verbosity;
    return out;
}

// initialize a debug message with variadic arguments
template<typename... T> Message
make_debug(u32 verbosity, T... args) {
    Message out = make_debug(verbosity);
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