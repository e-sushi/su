/*

    A singleton logging struct whose purpose is to recieve messages from various Senders
    and deliver them to the user in a thread safe and organized way.

    Messages consist of MessageParts which can be several different things such as plain text, an Entity
    a Type, etc. These parts are formatted before being joined to the actual message.

    The Messenger has a collection of destinations that it outputs to when 'deliver' is called.

*/
#ifndef AMU_MESSENGER_H
#define AMU_MESSENGER_H

#include "storage/Array.h" 
#include "storage/DString.h"
#include "systems/Threading.h"
#include "utils/Time.h"
#include "utils/Colorize.h"
#include "util.h"

// macros for outputting information about the compiler's state
// these should NEVER be used to report things about what the compiler is working
// with because that information must be localized

#define HELPER(x, y, ...)                                           \
	do {                                                            \
		if(compiler.options.verbosity >= x) {                       \
			messenger.dispatch(Message::create(y, x, __VA_ARGS__)); \
		}                                                           \
	} while(0)

#define FATAL(sender, ...) HELPER(Message::Kind::Fatal, sender, __VA_ARGS__)
#define ERROR(sender, ...) HELPER(Message::Kind::Error, sender, __VA_ARGS__)
#define WARNING(sender, ...) HELPER(Message::Kind::Warning, sender, __VA_ARGS__)
#define NOTICE(sender, ...) HELPER(Message::Kind::Notice, sender, __VA_ARGS__)
#define INFO(sender, ...) HELPER(Message::Kind::Info, sender, __VA_ARGS__)
#define DEBUG(sender, ...) HELPER(Message::Kind::Debug, sender, __VA_ARGS__)

// NOTE(sushi) this is set to deliver the message immediately
#ifdef AMU_ENABLE_TRACE
#define TRACE(sender, ...)                                                                 \
	do {                                                                                   \
		if(compiler.options.verbosity >= Message::Kind::Trace) {                           \
			messenger.deliver(Message::create(sender, Message::Kind::Trace, __VA_ARGS__)); \
		}                                                                                  \
	} while(0)
#else
#define TRACE(...)
#endif

namespace amu {

struct Code;
struct Var;
struct Function;
struct Module;
struct Token;
struct Source;
struct Structure;
struct Label;
struct Type;

template<typename T, int N>
int arrsize(T(&)[N]) { return N; }

struct MessagePart {
	enum class Kind {
		Plain,
		Token,
		Identifier,
		Path,
		Source,
		Var,
		Structure,
		Function,
		Module,
		Label,
		Code,
		Type,
	};

    Kind kind;
	Color color;

    union {
        MaybeDString plain; 	
		Token* token; // a token, likely representing a source location
        Var* place;
        Structure* structure;
        Function* function;
        Module* module;
        Label* label; // a label whose name we will print
        Source* source; // a source file whose name we will likely print
        Type* type;
    };

    MessagePart() {}
	
    MessagePart(MaybeDString s) : plain(s),     kind(Kind::Plain) {}
    MessagePart(Token* t)       : token(t),     kind(Kind::Token) {}
    MessagePart(Var* p)         : place(p),     kind(Kind::Var) {}
    MessagePart(Structure* s)   : structure(s), kind(Kind::Structure) {}
    MessagePart(Function* f)    : function(f),  kind(Kind::Function) {}
    MessagePart(Module* m)      : module(m),    kind(Kind::Module) {}
    MessagePart(Label* l)       : label(l),     kind(Kind::Label) {}
    MessagePart(Source* s)      : source(s),    kind(Kind::Source) {}
    MessagePart(Type* t)        : type(t),      kind(Kind::Type) {}

	MessagePart(const char* s) : kind(Kind::Plain) {plain = String::from(s);}

	// generic to turn arbitrary things into plain Strings
	template<typename T>
	MessagePart(T x);

	~MessagePart() {
		if(util::any_match(kind, Kind::Plain, Kind::Path, Kind::Identifier)) 
			plain.~MaybeDString();
	}

	// TODO(sushi) get str len at compile time here somehow

	//template<int N>
	//MessagePart(const char s[N]) : plain(s, N), kind(Kind::Plain), color(Color::White) {} 

	MessagePart& colored(Color color) { this->color = color; return *this; }

	void
	append_to(DString& dstr, b32 allow_color);
};

template<typename T>
MessagePart::MessagePart(T x) {
	kind = Kind::Plain;
	plain = MaybeDString(DString());
	to_string(plain.dstr, x);
}

// indicates who sent a message
struct MessageSender {
    enum class Type {
        Compiler,
        Code,  
		Token, 
    };

    Type type;
    Code* code;
    Token* token;

    MessageSender();
    MessageSender(Type type);
    MessageSender(Code* c);
    MessageSender(Token* t);
};

// a representation of a single message to be delivered to some destination
// consists of MessageParts that the Messenger formats before delivering
struct Message {
	enum Kind {
		Fatal,   // the compiler cannot continue
		Error,   // a specific operation cannot continue
		Warning, // odd situation that may cause problems
		// the following are used primarily for reporting stuff about 
		// the compiler's state
		Notice,  // normal but significant conditions
		Info,    // generally useful information
		Debug,   // used for general diagnosing of compiler problems
		Trace,   // used for tracing code 
	};

	Time::Point time;
    Kind kind;

    MessageSender sender;

    Array<MessagePart> parts;

	static Message
	create(MessageSender sender, Kind kind);

	template<typename... T> static Message
	create(MessageSender sender, Kind kind, T... args);

	DString
	build(b32 allow_color);
};

struct MessageBuilder {
	Message message;
	
	MessageBuilder(Message message) : message(message) {}
	MessageBuilder(MessageSender sender, Message::Kind kind);

	static MessageBuilder
	start(MessageSender sender, Message::Kind kind);

	static MessageBuilder
	from(Message message);

	MessageBuilder& plain(String s);
	MessageBuilder& path(String s);
	MessageBuilder& identifier(String s);
	MessageBuilder& append(MessagePart part);
	MessageBuilder& prepend(MessagePart part);
	
	// dispatches the message to the messenger
	void dispatch();
	
	// delivers the message immediately
	void deliver();
};

// this struct defines the formatting of each different MessagePart
struct MessageFormatting {
	using enum Color;
    // how to format message parts
    // default_color is the color applied to the entire part AND its prefix/suffix
    // prefix and suffix are strings applied before and after the part
    struct {
		Color col = White;
        String prefix = "", suffix = "";
    } plain;

    struct {
        struct {
            Color directory = White;
            Color slash = White;
            Color file = Cyan;
        } col;
		enum class PathStyle {
			Filename, // only show the filename, eg. "main.amu"
			RelativePath, // show the path relative to the working directory of the compiler
			AbsolutePath, // show the entire path
		};
		PathStyle path_style = PathStyle::Filename;
        String prefix = "'", suffix = "'";
    } path;

    struct {
        Color col = White;
        b32 show_code_loc = false; // appends the Token's code location in the form (line,col)
        String prefix = "'", suffix = "'";
    } token;

    struct {
        Color col = White;
        String prefix = "'", suffix = "'";
    } identifier;

    // struct {
    //     // dont show the original label when printing an alias
    //     b32 no_aka;
    //     // if aka is enabled, print the entire chain of aliases
    //     b32 full_aka;
    //     u32 col = message::color_green;
    //     String prefix = "'", suffix = "'";
    // } label;

};

// A place to send messages to. 
struct Destination {
    FILE* file;
    b32 allow_color;

    Destination(FILE* file, b32 allow_color = true) : file(file), allow_color(allow_color) {}
};

struct Messenger {
    Array<Message> messages;
    Array<Destination> destinations;

	Array<MessageSender> sender_stack;
	MessageSender current_sender;

	MessageFormatting formatting;
	
	Mutex outmtx;

	void init();
	void deinit();

	void push_sender(MessageSender sender);
	void pop_sender();

	struct ScopedSender {
		ScopedSender(MessageSender sender);
		~ScopedSender();
	};

	void dispatch(Message message);
	void dispatch(String message, Source* source = 0);

	void deliver(b32 clear_messages = true);
	void deliver(Destination dest, b32 clear_messages = false);
	void deliver(Destination dest, Array<Message> messages);
	void deliver(Destination dest, Message message);
	// deliver a single message to all destinations immediately
	void deliver(Message message);

	template<typename... T> void qdebug(MessageSender sender, T... args);
}; 

extern Messenger messenger;

template<typename... T> Message Message::
create(MessageSender sender, Message::Kind kind, T... args) {
	Message out = create(sender, kind);
	(out.parts.push(args), ...);
	return out;
}

} // namespace amu

#endif // AMU_MESSENGER_H
