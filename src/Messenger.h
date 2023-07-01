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

// a Message stores an Array of Parts, which allows the Messenger to
// process a Message and format it according to current formatting 
// settings or where it is delivering the message to
struct MessagePart {
    enum Type : u32 {
        Plain, Sender, Path, Source, Variable, Structure, Function, Module, Label, Code,
    };
    Type type;
    union {
        String plain; // a plain String or path
        Entity::Variable variable;
        Entity::Structure structure;
        Entity::Function function;
        Entity::Module module;
        amu::Label* label; // a label whose name we will print
        amu::Code code; // code region to print
        amu::Source* source; // a source file whose name we will likely print
    };
    // if this is Color_None standard colors will be applied based on type
    // otherwise this overrides the color used
    color col;

    MessagePart() {}
    MessagePart(Entity::Variable v) : variable(v) {type = Variable;}
    MessagePart(Entity::Structure s) : structure(s) {type = Structure;}
    MessagePart(Entity::Function f) : function(f) {type = Function;}
    MessagePart(Entity::Module m) : module(m) {type = Module;}
    MessagePart(amu::Label* l) : label(l) {type = Label;}
    MessagePart(amu::Code c) : code(c) {type = Code;} 
    MessagePart(amu::Source* s) : source(s) {type = Source;}
};

// a representation of a single message to be delivered to some destination
// consists of MessageParts that the Messenger formats before delivering
struct Message {
    // the time this message was created
    f32 time;
    // the Source this message originates from, if any
    Source* source; 

    enum Type {
        Normal,
        Warning,
        Error,
        Note,
    };
    Type type;

    Array<MessagePart> parts;
};

struct MessageFormatting {
    b32 allow_color;
    
    // how to format message parts
    // default_color is the color applied to the entire part AND its prefix/suffix
    // prefix and suffix are strings applied before and after the part
    struct {
        color default_color = Color_White;
        String prefix = "", suffix = "";
    }
    plain, path, sender, 
    source, variable, structure, function, module, label, code;

};

struct Messenger {
    Array<Message> messages; 

    struct {

    } options;

    Array<MessageFormatting> formatting_stack;

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
init(Source* source = 0);

// variadic way to initialize a Message
// Message::Parts may be passed
template<typename... T> Message
init(Source* source, T... args);

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
plain(String s, color c = Color_NONE);

// constructs a sender
MessagePart
sender(String s, color c = Color_NONE);

// constructs a path part
MessagePart
path(String s, color c = Color_NONE);

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