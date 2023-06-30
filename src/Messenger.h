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
#include "Code.h"

namespace amu {

struct Message {
    // the time this message was created
    f32 time;
    // the Source this message originates from, if any
    Source* source; 

    // a Message stores an Array of Parts, which allows the Messenger to
    // process a Message and format it according to current formatting 
    // settings or where it is delivering the message to
    struct Part {
        Type type;
        enum Type : u32 {
            Plain, Entity, Label, Code,
        };
        union {
            DString plain;
            amu::Entity* entity;
            amu::Label* label;
            amu::Code code;
        };
        // if this is Color_None standard colors will be applied based on type
        // otherwise this overrides the color used
        color col;
    };

    Array<Part> parts;


};

struct Messenger {
    Array<Message> messages; 

    struct {

    } options;

    mutex delivering; // locked when the messenger is in the process of delivering messages
};

struct Destination {
    

};

namespace messenger {

// global messenger instance, created in Messenger.cpp
extern Messenger instance;

// give the Messenger a message
void
dispatch(Message message);

// dispatch a plain str8
void
dispatch(str8 message, Source* source = 0);

// deliver all available messages to the given destination
// optionally clearing messages
void
deliver(Destination dest, b32 clear_messages = false);

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
push(Message& m, Message::Part part);

} // namespace message
} // namespace amu

#endif // AMU_MESSENGER_H