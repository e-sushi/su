/*

    A singleton logging struct whose purpose is to recieve messages from various Senders
    and deliver them to the user in a thread safe and organized way.

*/
#ifndef AMU_MESSENGER_H
#define AMU_MESSENGER_H

#include "kigu/color.h"
#include "storage/Array.h"
#include "storage/DString.h"
#include "Source.h"
#include "Entity.h"
#include "Code.h"

namespace amu {

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

// a Message stores an SharedArray of Parts, which allows the Messenger to
// process a Message and format it according to current formatting 
// settings or where it is delivering the message to
struct MessagePart {
    enum Type : u32 {
        Plain, Token, Identifier, Path, Source, Variable, Structure, Function, Module, Label, Code, COUNT
    };
    Type type;
    union {
        String plain; // a plain String, path, or identifier
        amu::Token token; // a token, likely representing a source location
        Entity::Variable variable;
        Entity::Structure structure;
        Entity::Function function;
        Entity::Module module;
        amu::Label* label; // a label whose name we will print
        amu::Code code; // code region to print
        amu::Source* source; // a source file whose name we will likely print
    };
    // if this is 0, default colors will be applied in processing
    // this is set to a number from message::color_
    u32 col;

    MessagePart() {}
    MessagePart(String s) : plain(s) {type = Plain;}
    MessagePart(amu::Token t) : token(t) {type = Token;}
    MessagePart(Entity::Variable v) : variable(v) {type = Variable;}
    MessagePart(Entity::Structure s) : structure(s) {type = Structure;}
    MessagePart(Entity::Function f) : function(f) {type = Function;}
    MessagePart(Entity::Module m) : module(m) {type = Module;}
    MessagePart(amu::Label* l) : label(l) {type = Label;}
    MessagePart(amu::Code c) : code(c) {type = Code;} 
    MessagePart(amu::Source* s) : source(s) {type = Source;}
};

// indicates who sent the message
struct MessageSender {
    enum Type {
        Compiler, // the compiler when there is no specific source that 
        Source, // some source file, no location
        SourceLoc, // some source file with location
    };
    Type type;
    amu::Source* source;
    u32 line, column; // if the sender is Source, give us the line and column 

    MessageSender() : type(Compiler) {}
    MessageSender(Type type) : type(type) {}
    MessageSender(amu::Source* s) : type(Source), source(s) {}
    MessageSender(amu::Source* s, Token& t) : type(SourceLoc), source(s) {line = t.l0; column = t.c0;}
};

// a representation of a single message to be delivered to some destination
// consists of MessageParts that the Messenger formats before delivering
struct Message {
    // the time this message was created
    f32 time;

    MessageSender sender;

    enum Type {
        Normal,
        Warning,
        Error,
        Note,
        Debug,
    };
    Type type;

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

};

struct Messenger {
    SharedArray<Message> messages; 
    SharedArray<MessageFormatting> formatting_stack;

    mutex delivering; // locked when the messenger is in the process of delivering messages
};

struct Destination {
    Type type;
    enum {
        DESHFILE,
        STLFILE,
    };

    union {
        File* desh_file;
        FILE* stl_file;
    };

    b32 allow_color;

    Destination(File* file, b32 allow_color = false) : desh_file(file), type(DESHFILE) {}
    // allow color is default true here, because it is most likely that this is chosen
    // when passing stdout or stderr as a destination
    Destination(FILE* file, b32 allow_color = true) : stl_file(file), type(STLFILE) {}
};

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


// deliver all available messages to the given destination
// optionally clearing messages
// TODO(sushi) when we dont clear messages, cache the formatted stuff and 
//             use it again if settings are compatible
void
deliver(Destination dest, b32 clear_messages = false);

void
push_formatting(MessageFormatting formatting);

void
pop_formatting();

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
debug(u32 verbosity);

// initialize a debug message with variadic arguments
template<typename... T> Message
debug(u32 verbosity, T... args);

// pushes a Message::Part into the Message
void
push(Message& m, MessagePart part);

// prefixes the message with some part
void
prefix(Message& m, MessagePart part);

// prefixes a Message with warning formatting
void
warning(Message& m);

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