/*

    A singleton logging struct whose purpose is to recieve messages from various Senders
    and deliver them to the user in a thread safe and organized way.

*/
#ifndef AMU_MESSENGER_H
#define AMU_MESSENGER_H

#include "storage/Array.h"
#include "storage/DString.h"
#include "Source.h"
#include "Entity.h"
#include "Token.h"

namespace amu {

// standard terminal colors, numbers correspond to the escape code that activates them
namespace message{
enum : u32{
    color_none =     0,
    color_black =    30,
    color_red =      31,
    color_green =    32,
    color_yellow =   33,
    color_blue =     34,
    color_magenta =  35,
    color_cyan =     36,
    color_white =    37,
    color_extended = 38,
    color_def      =  39,
    color_bright_black =   90,
    color_bright_red =     91,
    color_bright_green =   92,
    color_bright_yellow =  93,
    color_bright_blue =    94,
    color_bright_magenta = 95,
    color_bright_cyan =    96,
    color_bright_white =   97,
};

namespace verbosity{
enum : u32 {
    always,
    filenames,
    stages,
    stageparts,
    detailed,
    debug,
};
}
} // namespace message

namespace messagepart {
enum kind{
    plain, token, identifier, path, source, place, structure, function, module, label, code, type
};
} // namespace messagepart

// a Message stores an SharedArray of Parts, which allows the Messenger to
// process a Message and format it according to current formatting 
// settings or where it is delivering the message to
struct Token;
struct Source;
struct MessagePart {
    messagepart::kind kind;
    union {
        String plain; // a plain String, path, or identifier
        Token* token; // a token, likely representing a source location
        Place* place;
        Structure* structure;
        Function* function;
        Module* module;
        Label* label; // a label whose name we will print
        Source* source; // a source file whose name we will likely print
        Type* type;
    };
    // if this is 0, default colors will be applied in processing
    // this is set to a number from message::color_
    u32 col;

    MessagePart() {}
    MessagePart(String s) : plain(s) {kind = messagepart::plain;}
    MessagePart(Token* t) : token(t) {kind = messagepart::token;}
    MessagePart(Place* p) : place(p) {kind = messagepart::place;}
    MessagePart(Structure* s) : structure(s) {kind = messagepart::structure;}
    MessagePart(Function* f) : function(f) {kind = messagepart::function;}
    MessagePart(Module* m) : module(m) {kind = messagepart::module;}
    MessagePart(Label* l) : label(l) {kind = messagepart::label;}
    MessagePart(Source* s) : source(s) {kind = messagepart::source;}
    MessagePart(Type* t) : type(t) {kind = messagepart::type;}
};

// indicates who sent the message
struct MessageSender {
    enum Type {
        Compiler, // the compiler when there is no specific source that 
        Code, // some Code 
        CodeLoc, // some Code and a token location
    };
    Type type;
    amu::Code* code;
    Token* token;

    MessageSender() : type(Compiler) {}
    MessageSender(Type type) : type(type) {}
    MessageSender(amu::Code* c) : type(Code), code(c) {}
    MessageSender(Token* t) : type(CodeLoc), code(t->code), token(t) {}
};

namespace message {
enum kind {
    normal,
    warning,
    error,
    note,
    debug,
};
}

// a representation of a single message to be delivered to some destination
// consists of MessageParts that the Messenger formats before delivering
struct Message {
    // the time this message was created
    u64 time;

    MessageSender sender;
    message::kind kind;

    u32 verbosity; // set with message::verbosity

    Array<MessagePart> parts;
};

// this struct defines the formatting of each different MessagePart
// its default values should always be the ones that result in the fastest processing of messages
// anything that slows it down should be set by the user
// if this winds up being too slow, then just get rid of custom formatting and hardcode 
// the default behavoir
struct MessageFormatting {
    // how to format message parts
    // default_color is the color applied to the entire part AND its prefix/suffix
    // prefix and suffix are strings applied before and after the part
    struct {
        u32 col = message::color_white;
        String prefix = "", suffix = "";
    } plain;

    struct {
        struct {
            u32 directory = message::color_white;
            u32 slash = message::color_white;
            u32 file = message::color_cyan;
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
        u32 col = message::color_white;
        b32 show_code_loc = false; // appends the Token's code location in the form (line,col)
        String prefix = "'", suffix = "'";
    } token;

    struct {
        u32 col = message::color_bright_white;
        String prefix = "'", suffix = "'";
    } identifier;

    struct {
        // dont show the original label when printing an alias
        b32 no_aka;
        // if aka is enabled, print the entire chain of aliases
        b32 full_aka;
        u32 col = message::color_green;
        String prefix = "'", suffix = "'";
    } label;

};

struct Destination {
    FILE* file;
    b32 allow_color;

    Destination(FILE* file, b32 allow_color = true) : file(file), allow_color(allow_color) {}
};

struct Messenger {
    Array<Message> messages; 
    Array<MessageFormatting> formatting_stack;
    Array<Destination> destinations;
}; // !Threading this will likely required SharedArray or a mutex to prevent messages from being dispatched at the same time



namespace messenger {

// global messenger instance, created in Messenger.cpp
extern Messenger instance;

// initialize the global messenger instance
void
init();

// give the Messenger a message
void
dispatch(Message message);

// dispatch a plain str8
void
dispatch(String message, Source* source = 0);

// deliver all queued messages to the stored destinations
// optionally clearing the message queue
void
deliver(b32 clear_messages = true);

// deliver all queued messages to the given destination
// optionally clearing messages
// TODO(sushi) when we dont clear messages, cache the formatted stuff and use it again if settings are compatible
void
deliver(Destination dest, b32 clear_messages = false);

void
deliver(Destination dest, Array<Message> messages);

void
push_formatting(MessageFormatting formatting);

void
pop_formatting();

// quick debug function
template<typename... T> void 
qdebug(MessageSender sender, T...args);

} // namespace messenger

namespace message {

// initializes a Message
Message
init();

// variadic way to initialize a Message
// Message::Parts may be passed
template<typename... T> Message
init(T... args);

// initialize a debug message
Message
make_debug(u32 verbosity);

// initialize a debug message with variadic arguments
template<typename... T> Message
make_debug(u32 verbosity, T... args);

// pushes a Message::Part into the Message
void
push(Message& m, MessagePart part);

// prefixes the message with some part
void
prefix(Message& m, MessagePart part);

// constructs a plain part
MessagePart
plain(String s, u32 c = message::color_none);

// constructs a sender
MessagePart
sender(String s, u32 c = message::color_none);

// constructs a path part
MessagePart
path(String s, u32 c = message::color_none);

// constructs an identifier part
// for referring to some identifier of an unknown type
MessagePart
identifier(String s);

// attaches a sender to the given message and returns it 
Message
attach_sender(MessageSender sender, Message m);

// the following helpers are unecessary to call unless you want to override the
// color used to format them, otherwise you can just pass the object directly
// TODO(sushi) finish these if we find a use for them 

// MessagePart
// variable(Entity::Variable* v, color c = Color_NONE);

// MessagePart
// structure(Entity::Structure* s, color c = Color_NONE);

// MessagePart
// function(Entity::Function* f, color c = Color_NONE);

// MessagePart
// module(Entity::Module* m, color c = Color_NONE);

// MessagePart
// label(Label* l, color c = Color_NONE)




} // namespace message
} // namespace amu

#endif // AMU_MESSENGER_H