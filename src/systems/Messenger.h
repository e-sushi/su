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
#include "representations/Source.h"
#include "representations/Entity.h"
#include "representations/Token.h"

namespace amu {

struct Code;
struct Var;
struct Function;
struct Module;
struct Token;
struct Source;

// standard terminal colors
// FormattingColor indicates to use the color defined by a
// message part's formatting. 
enum class Color {
	FormattingColor = 0, 
	Black = 30,
	Red = 31,
	Green = 32,
	Yellow = 33,
	Blue = 34,
	Magenta = 35,
	Cyan = 36,
	White = 37,
	Extended = 38,
	Default = 49,
	BrightBlack = 90,
	BrightRed = 91,
	BrightGreen = 92,
	BrightYellow = 93,
	BrightBlue = 94,
	BrightMagenta = 95,
	BrightCyan = 96,
	BrightWhite = 97,
};


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
        String plain; // a plain String, path, or identifier
        Token* token; // a token, likely representing a source location
        Var* place;
        Structure* structure;
        Function* function;
        Module* module;
        Label* label; // a label whose name we will print
        Source* source; // a source file whose name we will likely print
        Type* type;
    };
    // if this is 0, default colors will be applied in processing
    // this is set to a number from message::color_

    MessagePart() {}

	
    MessagePart(String s)     : plain(s),     kind(Kind::Plain) {}
    MessagePart(Token* t)     : token(t),     kind(Kind::Token) {}
    MessagePart(Var* p)       : place(p),     kind(Kind::Var) {}
    MessagePart(Structure* s) : structure(s), kind(Kind::Structure) {}
    MessagePart(Function* f)  : function(f),  kind(Kind::Function) {}
    MessagePart(Module* m)    : module(m),    kind(Kind::Module) {}
    MessagePart(Label* l)     : label(l),     kind(Kind::Label) {}
    MessagePart(Source* s)    : source(s),    kind(Kind::Source) {}
    MessagePart(Type* t)      : type(t),      kind(Kind::Type) {}

	// IDK WHY but for some reason I cannot get a variadic templated function to take in 
	// JUST a string literal and have it know that it is a MessagePart at compile time
	// for example: messenger::qdebug(MessageSender::Compiler, "hello!") will not work
	// and I have no idea how to get it to
	consteval MessagePart(const char* s) : plain(s), kind(Kind::Plain), color(Color::White) {} 

	MessagePart& colored(Color color) { this->color = color; }
};

// indicates who sent a message
struct MessageSender {
    enum Type {
        Compiler,
        Code,  
		CodeLoc, 
    };
    Type type;
    amu::Code* code;
    Token* token;

    MessageSender() : type(Compiler) {}
    MessageSender(Type type) : type(type) {}
    MessageSender(amu::Code* c) : type(Code), code(c) {}
    MessageSender(Token* t) : type(CodeLoc), code(t->code), token(t) {}
};

// a representation of a single message to be delivered to some destination
// consists of MessageParts that the Messenger formats before delivering
struct Message {
	enum Kind {
		Fatal,
		Error,
		Warning,
		Notice,
		Info,
		Debug,
		Trace,
	};

    u64 time;
    Kind kind;

    MessageSender sender;

    Array<MessagePart> parts;

	static Message
	create(Kind kind);

	template<typename... T> static Message
	create(Kind kind, T... args);
};

struct MessageBuilder {
	Message message;

	static MessageBuilder
	start(MessageSender sender, Message::Kind kind);

	static MessageBuilder
	from(Message message);

	MessageBuilder& plain(String s);
	MessageBuilder& path(String s);
	MessageBuilder& identifier(String s);
	MessageBuilder& append(MessagePart part);
	MessageBuilder& prepend(MessagePart part);
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
        // when a path is very long, shorten it by replacing the interior with ...
        // for example
        // some/stupidly/long/path/to/the/naughty/file
        // some/stupidly/.../naughty/file
        b32 shorten_long_paths = false;
        u32 long_path_min_len = 60;
        b32 always_absolute = false;
        b32 always_forward_slash = true;
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

	MessageFormatting formatting;
	
	// locked anytime data is being output 
	std::mutex outmtx;	

	void dispatch(Message message);
	void dispatch(String message, Source* source = 0);

	void deliver(b32 clear_messages = true);
	void deliver(Destination dest, b32 clear_messages = false);
	void deliver(Destination dest, Array<Message> messages);
	void deliver(Destination dest, Message message);

	template<typename... T> void qdebug(MessageSender sender, T... args);
}; 

extern Messenger messenger;

} // namespace amu

#endif // AMU_MESSENGER_H
