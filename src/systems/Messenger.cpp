#include "Messenger.h"
#include "representations/Token.h"
#include "stdio.h"

namespace amu {

Messenger messenger;

void Messenger::
init() {
    messages = Array<Message>::create();
    destinations = Array<Destination>::create();
	formatting = MessageFormatting();
	sender_stack = Array<MessageSender>::create(16);
	current_sender = MessageSender::Type::Compiler;
}

namespace internal {

Destination* current_dest;

void
process_part(DString* current, const MessagePart& part) {
	FixMe;
   // MessageFormatting& formatting = instance.formatting;
   // switch(part.kind) {
   //     case messagepart::plain: {
   //         DString* temp = DString::create(part.plain);
   //         if(current_dest->allow_color && formatting.plain.col) {
   //             wrap_color(temp, (part.col? part.col : formatting.plain.col));
   //         }
   //         temp->prepend(formatting.plain.prefix);
   //         temp->append(formatting.plain.suffix);
   //         current->append(temp);
   //         temp->deref();
   //     } break;
   //     case messagepart::path: {
   //         DString* temp = DString::create(part.plain);
   //         // TODO(sushi) implement fancy path formatting
   //         if(current_dest->allow_color) {
   //             wrap_color(temp, (part.col? part.col : formatting.path.col.directory));
   //         }
   //         temp->prepend(formatting.path.prefix);
   //         temp->append(formatting.path.suffix);
   //         current->append(temp);
   //         temp->deref();

   //     } break;
   //     case messagepart::token: {
   //         DString* temp = DString::create(part.token->raw);
   //         if(current_dest->allow_color) {
   //             wrap_color(temp, formatting.token.col);
   //         }
   //         temp->prepend(formatting.token.prefix);
   //         if(formatting.token.show_code_loc) {
   //             temp->append("(", part.token->l0, ",", part.token->c0, ")");
   //         }
   //         temp->append(formatting.token.suffix);
   //         current->append(temp);
   //         temp->deref();
   //     } break;
   //     case messagepart::identifier: {
     //       DString* temp = DString::create(part.plain);
     //       if(current_dest->allow_color) {
     //           wrap_color(temp, formatting.identifier.col);
     //       }
     //       temp->prepend(formatting.identifier.prefix);
     //       temp->append(formatting.identifier.suffix);
     //       current->append(temp);
     //       temp->deref();
     //   }break;
     //   case messagepart::place: {
     //       NotImplemented;
     //   } break;
     //   case messagepart::structure: {
     //       NotImplemented;
     //   } break;
     //   case messagepart::function: {
     //       NotImplemented;
     //   } break;
     //   case messagepart::module: {
     //       NotImplemented;
     //   } break;
     //   case messagepart::label: {
	 //   	current->append(part.label->display());
     //   } break;
     //   case messagepart::code: {
     //       NotImplemented;
     //   } break;
     //   case messagepart::type: {
     //       Type* step = part.type;
     //       current->append(step->display());
     //   } break;
    //}
}

// processes a given Message into a str8
// TODO(sushi) decide if the color of message type prefixes will be customizable and if not
//             store a static string of them wrapped in their colors
void
process_message(DString* current, Message& m) {
	FixMe;
  //  // dont do anything if current verbosity is less than the message's
  //  if(m.kind == message::debug && m.verbosity > compiler::instance.options.verbosity) return; 

  //  // TODO(sushi) we need to support a way to detect when there are conflicting file names 
  //  //             and in those cases show a relative path instead of just the name
  //  switch(m.sender.type) {
  //      case MessageSender::Compiler: {
  //          DString* compiler_prefix = DString::create("amu");
  //          if(current_dest->allow_color)
  //              wrap_color(compiler_prefix, message::color_cyan);
  //          compiler_prefix->append(String(": "));
  //          current->append(compiler_prefix);
  //      } break;
  //      case MessageSender::Code: {
  //          DString* temp = DString::create(m.sender.code->display());
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_cyan);
  //          current->append(temp, ": ");
  //          temp->deref();
  //      } break;
  //      case MessageSender::CodeLoc: {
  //          DString* temp = DString::create(m.sender.code->display());
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_cyan);
  //          current->append(temp, ":", m.sender.token->l0, ":", m.sender.token->c0, ": ");
  //          temp->deref();
  //          
  //      } break;
  //  }

  //  switch(m.kind) {
  //      case message::normal: { // dont need to do anything special
  //      } break;

  //      case message::debug: { 
  //          DString* temp = DString::create("debug");
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_green);
  //          current->append(temp);
  //          current->append(": ");
  //          temp->deref();
  //      } break;

  //      case message::warning: {
  //          DString* temp = DString::create("warning");
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_yellow);
  //          current->append(temp);
  //          current->append(": ");
  //          temp->deref();
  //      } break;

  //      case message::error: {
  //          DString* temp = DString::create("error");
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_red);
  //          current->append(temp);
  //          current->append(": ");
  //          temp->deref();
  //      } break;

  //      case message::note: {
  //          DString* temp = DString::create("note");
  //          if(current_dest->allow_color)
  //              wrap_color(temp, message::color_magenta);
  //          current->append(temp);
  //          current->append(": ");
  //          temp->deref();
  //      } break;
  //  }

  //  forI(m.parts.count) {
  //      process_part(current, m.parts.readref(i));
  //  }

  //  current->append("\n");
}

} // namespace internal

//
//
//void
//dispatch(Message message) {
//    if(compiler::instance.options.deliver_all_immediately ||
//			message.kind == message::debug && compiler::instance.options.deliver_debug_immediately) {
//        deliver(stdout, message);
//	} else instance.messages.push(message);
//}
//
//// dispatch a plain str8
//void
//dispatch(String message, Source* source) {
//    Message m = message::init(source);
//    MessagePart part; 
//    part.kind = messagepart::plain;
//    //part.plain = message;
//    message::push(m, part);
//    dispatch(message);
//}
//
//void
//deliver(b32 clear_messages) {
//    forI(instance.destinations.count) {
//        deliver(instance.destinations.read(i), instance.messages);
//    }
//    if(clear_messages) instance.messages.clear();
//}
//
//void 
//deliver(Destination destination, b32 clear_messages) {
//    deliver(destination, instance.messages);
//    if(clear_messages) instance.messages.clear();
//}
//
//
//void
//deliver(Destination destination, Message message) {
//	auto l = std::scoped_lock(instance.outmtx);
//	auto out = DString::create();
//
//	internal::current_dest = &destination;
//
//	internal::process_message(out, message);
//	if(compiler::instance.options.quiet && destination.file == stdout) return;
//	fwrite(out->str, 1, out->count, destination.file);
//}
//
//void
//deliver(Destination destination, Array<Message> messages) {
//	auto l = std::scoped_lock(instance.outmtx);
//    DString* out = DString::create();
//
//    internal::current_dest = &destination;
//
//    forI(messages.count) {
//        internal::process_message(out, 
//            messages.readref(i));
//    }
//
//    if(compiler::instance.options.quiet && destination.file == stdout) return;
//    fwrite(out->str, 1, out->count, destination.file);
//}
//
//template<typename... T> void 
//qdebug(MessageSender sender, T...args) {
//    messenger::dispatch(
//        message::attach_sender(sender, 
//            message::make_debug(message::verbosity::debug, args...)));
//}
//
//namespace message { // ---------------------------------------------------- message
//
//Message
//init() {
//    Message out = {};
//    out.time = time(0);
//    out.parts = Array<MessagePart>::create();
//    return out;
//}
//
//Message
//init(String s) {
//    Message out = init();
//    push(out, s);
//    return out;
//}
//
//// TODO(sushi) there has just GOT to be a way for me to pass string literals to this function and them still
////             be turned into Strings at compile time
//template<typename... T> Message
//init(T... args) {
//    Message out = init();
//    MessagePart arr[sizeof...(T)] = {args...};
//    forI(sizeof...(T)) {
//        push(out, arr[i]);
//    }
//    return out;
//}
//
//// initialize a debug message
//Message
//make_debug(u32 verbosity) {
//    Message out = init();
//    out.kind = message::debug;
//    out.verbosity = verbosity;
//    return out;
//}
//
//// initialize a debug message with variadic arguments
//template<typename... T> Message
//make_debug(u32 verbosity, T... args) {
//    Message out = make_debug(verbosity);
//    MessagePart arr[sizeof...(T)] = {args...};
//    forI(sizeof...(T)) {
//        push(out, arr[i]);
//    }
//    return out;
//}
//
//void
//push(Message& m, MessagePart part) {
//    m.parts.push(part);
//}
//
//void
//prefix(Message& m, MessagePart part) {
//    m.parts.insert(0, part);
//}
//
//template<typename... T>
//void
//prefix(Message& m, T... args) {
//    MessagePart arr[sizeof...(T)] = {args...};
//    forI_reverse(sizeof...(T)) {
//        prefix(m, arr[i]);
//    }
//}
//
//MessagePart
//plain(String s, u32 c) {
//    MessagePart out;
//    out.kind = messagepart::plain;
//    out.col = c;
//    out.plain = s;
//    return out;
//}
//
//MessagePart
//path(String s, u32 c) {
//    MessagePart out;
//    out.kind = messagepart::path;
//    out.col = c;
//    out.plain = s;
//    return out;
//}
//
//MessagePart
//identifier(String s) {
//    MessagePart out;
//    out.kind = messagepart::identifier;
//    out.plain = s;
//    return out;
//}
//
//Message
//attach_sender(MessageSender sender, Message m) {
//    m.sender = sender;
//    return m;
//}
//
//} // namespace message
//
//

void MessagePart::
append_to(DString& dstr, b32 allow_color) {
	using enum Kind;
	
	auto do_color = [&](DString& s, Color formatting_color, Color message_color) {
		if(!allow_color) return;
		colorize(s, (message_color == Color::Null? formatting_color : message_color));
	};

	switch(kind) {
		case Plain: {
			auto temp = DString(messenger.formatting.plain.prefix, plain.get_string(), messenger.formatting.plain.suffix);
			do_color(temp, messenger.formatting.plain.col, color);
			dstr.append(temp);
		} break;
		case Identifier: {
			auto temp = DString(messenger.formatting.identifier.prefix, plain.get_string(), messenger.formatting.identifier.suffix);
			do_color(temp, messenger.formatting.identifier.col, color);
			dstr.append(temp);
		} break;
		default: {
			NotImplemented;
		} break;
	}
}


void Messenger::
dispatch(Message message) {
	outmtx.lock();
	defer { outmtx.unlock(); };
	
	messages.push(message);
}

void Messenger::
deliver(Message message) {
	outmtx.lock();
	defer { outmtx.unlock(); };

	forX(destidx, destinations.count) {
		b32 allow_color = destinations[destidx].allow_color;
		FILE* file = destinations[destidx].file;
		DString out;
		forI(message.parts.count) {
			message.parts[i].append_to(out, allow_color);
		}
		fwrite(out.ptr, out.count(), 1, file);
	}
}

void Messenger::
deliver(b32 clear_messages) {
	forI(messages.count) {
		deliver(messages[i]);
	}
	if(clear_messages)
		messages.clear();
}

void Messenger::
deliver(Destination dest, Message message) {
	DString built = message.build(dest.allow_color);
	String s = built.get_string();
	fwrite(s.str, s.count, 1, dest.file);
}

void Messenger::
push_sender(MessageSender m) {
	sender_stack.push(current_sender);
	current_sender = m;
}

void Messenger::
pop_sender() {
	current_sender = sender_stack.pop();
}

Messenger::ScopedSender::
ScopedSender(MessageSender sender) {
	messenger.push_sender(sender);
}

Messenger::ScopedSender::
~ScopedSender() {
	messenger.pop_sender();
}


MessageSender::MessageSender() {
	type = Type::Compiler;
}

MessageSender::MessageSender(Type type) {
	type = type;
}

MessageSender::MessageSender(Code* c) {
	type = Type::Code;
	code = c;
}

MessageSender::MessageSender(Token* t) {
	type = Type::Token;
	token = t;
}

Message Message::
create(MessageSender sender, Message::Kind kind) {
	Message out = {};
	out.time = Time::Point::now();
	out.kind = kind;
	out.sender = sender;
	out.parts = Array<MessagePart>::create();
	return out;
}

DString Message::
build(b32 allow_color) {
	DString out;
	forI(parts.count) {
		parts[i].append_to(out, allow_color);
	}
	return out;
}

MessageBuilder::MessageBuilder(MessageSender sender, Message::Kind kind) {
	message = Message::create(sender, kind);
}

MessageBuilder MessageBuilder::
start(MessageSender sender, Message::Kind kind) {
	return MessageBuilder(sender, kind);
}

MessageBuilder MessageBuilder::
from(Message message) {
	return MessageBuilder(message);
}

MessageBuilder& MessageBuilder::
plain(String s) {
	message.parts.push(s);
	return *this;
}

MessageBuilder& MessageBuilder::
path(String s) {
	auto p = MessagePart(s);
	p.kind = MessagePart::Kind::Path;
	message.parts.push(p);
	return *this;
}

MessageBuilder& MessageBuilder::
append(MessagePart part) {
	message.parts.push(part);
	return *this;
}



} // namespace amu
