#include <mutex>
namespace amu {
namespace messenger { // ---------------------------------------------------- messenger

Messenger instance;

void 
init() {
    instance.messages = Array<Message>::create();
    instance.formatting_stack = Array<MessageFormatting>::create();
    instance.destinations = Array<Destination>::create();

    push_formatting(MessageFormatting());
}

namespace internal {

Destination* current_dest;

// wraps a DString in ANSI terminal color
// TODO(sushi) need to setup using the 8 original colors so that terminal themes work properly
void
wrap_color(DString* current, u32 color) {
    DString* temp = DString::create("\e[", color, "m");
    current->prepend(temp);
    current->append("\e[0m");
    temp->deref();
}

void
process_part(DString* current, const MessagePart& part) {
    MessageFormatting& formatting = instance.formatting_stack.readref(-1);
    switch(part.kind) {
        case messagepart::plain: {
            DString* temp = DString::create(part.plain);
            if(current_dest->allow_color && formatting.plain.col) {
                wrap_color(temp, (part.col? part.col : formatting.plain.col));
            }
            temp->prepend(formatting.plain.prefix);
            temp->append(formatting.plain.suffix);
            current->append(temp);
            temp->deref();
        } break;
        case messagepart::path: {
            DString* temp = DString::create(part.plain);
            // TODO(sushi) implement fancy path formatting
            if(current_dest->allow_color) {
                wrap_color(temp, (part.col? part.col : formatting.path.col.directory));
            }
            temp->prepend(formatting.path.prefix);
            temp->append(formatting.path.suffix);
            current->append(temp);
            temp->deref();

        } break;
        case messagepart::token: {
            DString* temp = DString::create(part.token->raw);
            if(current_dest->allow_color) {
                wrap_color(temp, formatting.token.col);
            }
            temp->prepend(formatting.token.prefix);
            if(formatting.token.show_code_loc) {
                temp->append("(", part.token->l0, ",", part.token->c0, ")");
            }
            temp->append(formatting.token.suffix);
            current->append(temp);
            temp->deref();
        } break;
        case messagepart::identifier: {
            DString* temp = DString::create(part.plain);
            if(current_dest->allow_color) {
                wrap_color(temp, formatting.identifier.col);
            }
            temp->prepend(formatting.identifier.prefix);
            temp->append(formatting.identifier.suffix);
            current->append(temp);
            temp->deref();
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
            label::display(part.label, formatting.label, current_dest->allow_color);
        } break;
        case messagepart::code: {
            NotImplemented;
        } break;
        case messagepart::type: {
            Type* step = part.type;
            current->append(step->display());
        } break;
    }
}

// processes a given Message into a str8
// TODO(sushi) decide if the color of message type prefixes will be customizable and if not
//             store a static string of them wrapped in their colors
void
process_message(DString* current, Message& m) {
    // dont do anything if current verbosity is less than the message's
    if(m.kind == message::debug && m.verbosity > compiler::instance.options.verbosity) return; 

    // TODO(sushi) we need to support a way to detect when there are conflicting file names 
    //             and in those cases show a relative path instead of just the name
    switch(m.sender.type) {
        case MessageSender::Compiler: {
            DString* compiler_prefix = DString::create("amu");
            if(current_dest->allow_color)
                wrap_color(compiler_prefix, message::color_cyan);
                compiler_prefix->append(String(": "));
            current->append(compiler_prefix);
        } break;
        case MessageSender::Code: {
            DString* temp = DString::create(m.sender.code->display());
            if(current_dest->allow_color)
                wrap_color(temp, message::color_cyan);
            current->append(temp, ": ");
            temp->deref();
        } break;
        case MessageSender::CodeLoc: {
            DString* temp = DString::create(m.sender.code->display());
            if(current_dest->allow_color)
                wrap_color(temp, message::color_cyan);
            current->append(temp, ":", m.sender.token->l0, ":", m.sender.token->c0, ": ");
            temp->deref();
            
        } break;
    }

    switch(m.kind) {
        case message::normal: { // dont need to do anything special
        } break;

        case message::debug: { 
            DString* temp = DString::create("debug");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_green);
            current->append(temp);
            current->append(": ");
            temp->deref();
        } break;

        case message::warning: {
            DString* temp = DString::create("warning");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_yellow);
            current->append(temp);
            current->append(": ");
            temp->deref();
        } break;

        case message::error: {
            DString* temp = DString::create("error");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_red);
            current->append(temp);
            current->append(": ");
            temp->deref();
        } break;

        case message::note: {
            DString* temp = DString::create("note");
            if(current_dest->allow_color)
                wrap_color(temp, message::color_magenta);
            current->append(temp);
            current->append(": ");
            temp->deref();
        } break;
    }

    forI(m.parts.count) {
        process_part(current, m.parts.readref(i));
    }

    current->append("\n");
}

} // namespace internal



void
dispatch(Message message) {
    if(compiler::instance.options.deliver_all_immediately ||
			message.kind == message::debug && compiler::instance.options.deliver_debug_immediately) {
        deliver(stdout, message);
	} else instance.messages.push(message);
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
deliver(b32 clear_messages) {
    forI(instance.destinations.count) {
        deliver(instance.destinations.read(i), instance.messages);
    }
    if(clear_messages) instance.messages.clear();
}

void 
deliver(Destination destination, b32 clear_messages) {
    deliver(destination, instance.messages);
    if(clear_messages) instance.messages.clear();
}


void
deliver(Destination destination, Message message) {
	auto l = std::scoped_lock(instance.outmtx);
	auto out = DString::create();

	internal::current_dest = &destination;

	internal::process_message(out, message);
	if(compiler::instance.options.quiet && destination.file == stdout) return;
	fwrite(out->str, 1, out->count, destination.file);
}

void
deliver(Destination destination, Array<Message> messages) {
	auto l = std::scoped_lock(instance.outmtx);
    DString* out = DString::create();

    internal::current_dest = &destination;

    forI(messages.count) {
        internal::process_message(out, 
            messages.readref(i));
    }

    if(compiler::instance.options.quiet && destination.file == stdout) return;
    fwrite(out->str, 1, out->count, destination.file);
}

void
push_formatting(MessageFormatting formatting) {
    instance.formatting_stack.push(formatting);
}

void 
pop_formatting() {
    instance.formatting_stack.pop();
}

template<typename... T> void 
qdebug(MessageSender sender, T...args) {
    messenger::dispatch(
        message::attach_sender(sender, 
            message::make_debug(message::verbosity::debug, args...)));
}

} // namespace messenger

namespace message { // ---------------------------------------------------- message

Message
init() {
    Message out = {};
    out.time = time(0);
    out.parts = Array<MessagePart>::create();
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
    m.parts.push(part);
}

void
prefix(Message& m, MessagePart part) {
    m.parts.insert(0, part);
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
