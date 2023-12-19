#ifndef AMU_DIAGNOSTICS_H
#define AMU_DIAGNOSTICS_H

#define try(x) if(!x) return false;

#include "Messenger.h"

namespace amu {

struct Token;
struct Type;

namespace diagnostic {
#include "data/diagnostics-data.generated"
extern language lang;
}

struct Diag {
	enum class Kind {
		InvalidToken,
		UnknownSid,
		UnknownDirective,
	};

	Kind kind;
	Message::Kind severity;

	MessageSender sender;

	union Arg {
		String string;
		Token* token;
		Type* type;
	};

	Array<Arg> args;

	Message (*emit_callback)(Diag*);

	void
	emit() {
		messenger.dispatch(emit_callback(this));
	}

	static Diag 
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

	void
	destroy() {
		args.destroy();

	}

	static Diag
	invalid_token(MessageSender m, Token* tok) {
		auto out = Diag::create(m, Kind::InvalidToken, Message::Kind::Error, 1);
		out.args[0].token = tok;
		out.emit_callback = [](Diag* diag) {
			return MessageBuilder::
				 start(diag->sender, diag->severity)
				.append("invalid token: ")
				.append(diag->args[0].token)
				.message;
		};
		return out;
	}

	static Diag
	unknown_sid(MessageSender m, String s) {
		auto out = Diag::create(m, Kind::UnknownSid, Message::Kind::Error, 1);
		out.args[0].string = s;
		out.emit_callback = [](Diag* diag) {
			return MessageBuilder::
				 start(diag->sender, diag->severity)
				.append("unknown special id: ")
				.append(diag->args[0].string)
				.message;
		};
		return out;
	}

	static Diag
	unknown_directive(MessageSender m, String s) {
		auto out = Diag::create(m, Kind::UnknownDirective, Message::Kind::Error, 1);
		out.args[0].string = s;
		out.emit_callback = [](Diag* diag) {
			return MessageBuilder::
				 start(diag->sender, diag->severity)
				.append("unknown directive: ")
				.append(diag->args[0].string)
				.message;
		};
		return out;
	}

};

} // namespace amu
  
#include "data/diagnostic-header.generated"

#endif // AMU_DIAGNOSTICS_H
