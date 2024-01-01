#include "Diagnostics.h"
#include "Messenger.h"

namespace amu {

Lang language = Lang::English;

Diag Diag::
create(MessageSender sender, Kind kind, Message::Kind severity, s32 n_args) {
	Diag out;
	out.kind = kind;
	out.severity = severity;
	out.sender = sender;
	if(n_args) {
		out.args = Array<Arg>::create();
		out.args.resize(n_args);
	}
	return out;
}

void Diag::
destroy() {
	args.destroy();
}


Diag Diag::
no_path_given(MessageSender m) {
	auto out = Diag::create(m, Kind::NoPathGiven, Message::Kind::Fatal, 0);
	out.emit_callback = [](Diag* diag) -> Message {
		switch(language) {
			default:
			case Lang::English:
				return MessageBuilder::
					 start(diag->sender, diag->severity)
					.append("no input files were given")
					.message;
		}
	};
	return out;
}
void Diag::no_path_given(Array<Diag>& to, MessageSender m) { to.push(no_path_given(m)); }


Diag Diag::
path_not_found(MessageSender m, String path) {
	auto out = Diag::create(m, Kind::PathNotFound, Message::Kind::Fatal, 1);
	out.args[0].string = path;
	out.emit_callback = [](Diag* diag) -> Message {
		switch(language) {
			default:
			case Lang::English:
				return MessageBuilder::
					 start(diag->sender, diag->severity)
					.append("the given path ")
					.path(diag->args[0].string)
					.append(" could not be found")
					.message;
		}
	};
	return out;
}
void Diag::path_not_found(Array<Diag>& to, MessageSender m, String path) { to.push(path_not_found(m, path)); }

Diag Diag::
expected_a_path_for_arg(MessageSender m, String arg) {
	auto out = Diag::create(m, Kind::ExpectedAPathForArg, Message::Kind::Fatal, 1);
	out.args[0].string = arg;
	out.emit_callback = [](Diag* diag) -> Message {
		return MessageBuilder::
			 start(diag->sender, diag->severity)
			.append("expected a path for arg '")
			.append(diag->args[0].string)
			.append("'")
			.message;
	};
	return out;
}
void Diag::expected_a_path_for_arg(Array<Diag>& to, MessageSender m, String arg) { to.push(expected_a_path_for_arg(m, arg)); }

Diag Diag::
expected_path_or_paths_for_arg_option(MessageSender m, String arg) {
	auto out = Diag::create(m, Kind::ExpectedPathOrPathsForArgOption, Message::Kind::Fatal, 1);
	out.args[0].string = arg;
	out.emit_callback = [](Diag* diag) -> Message {
		switch(language) {
			default:
			case Lang::English: 
				return MessageBuilder::
					 start(diag->sender, diag->severity)
					.append("expected a path or paths for arg option '")
					.append(diag->args[0].string)
					.append("'")
					.message;
		}
	};
	return out;
}
void Diag::expected_path_or_paths_for_arg_option(Array<Diag>& to, MessageSender sender, String arg) { to.push(expected_path_or_paths_for_arg_option(sender, arg)); }

Diag Diag::
unknown_option(MessageSender m, String arg) {
	auto out = Diag::create(m, Kind::UnknownOption, Message::Kind::Fatal, 1);
	out.args[0].string = arg;
	out.emit_callback = [](Diag* diag) -> Message {
		switch(language) {
			default:
			case Lang::English:
				return MessageBuilder::
					 start(diag->sender, diag->severity)
					.append("unknown cli option '")
					.append(diag->args[0].string)
					.append("'")
					.message;
		}
	};
	return out;
}
void Diag::unknown_option(Array<Diag>& to, MessageSender m, String arg) { to.push(unknown_option(m, arg)); }

Diag Diag::
invalid_token(MessageSender m, Token* tok) {
	auto out = Diag::create(m, Kind::InvalidToken, Message::Kind::Error, 1);
	out.args[0].token = tok;
	out.emit_callback = [](Diag* diag) -> Message {
		return MessageBuilder::
			 start(diag->sender, diag->severity)
			.append("invalid token: ")
			.append(diag->args[0].token)
			.message;
	};
	return out;
}
void Diag::invalid_token(Array<Diag>& to, MessageSender m, Token* tok) { to.push(invalid_token(m, tok)); }

Diag Diag::
unknown_sid(MessageSender m, String s) {
	auto out = Diag::create(m, Kind::UnknownSid, Message::Kind::Error, 1);
	out.args[0].string = s;
	out.emit_callback = [](Diag* diag) -> Message {
		return MessageBuilder::
			 start(diag->sender, diag->severity)
			.append("unknown special id: ")
			.append(diag->args[0].string)
			.message;
	};
	return out;
}
void Diag::unknown_sid(Array<Diag>& to, MessageSender m, String s) { to.push(unknown_sid(m, s)); } 

Diag Diag::
unknown_directive(MessageSender m, String s) {
	auto out = Diag::create(m, Kind::UnknownDirective, Message::Kind::Error, 1);
	out.args[0].string = s;
	out.emit_callback = [](Diag* diag) -> Message {
		return MessageBuilder::
			 start(diag->sender, diag->severity)
			.append("unknown directive: ")
			.append(diag->args[0].string)
			.message;
	};
	return out;
}
void Diag::unknown_directive(Array<Diag>& to, MessageSender m, String s) { to.push(unknown_directive(m, s)); }

} // namespace amu
