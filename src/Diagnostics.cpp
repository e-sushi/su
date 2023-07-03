/*

    Definitions of any diagnostic message that amu may emit.

    Prefix an index name with '@' with find to go to it
    Index: 
        path     - messages relating to a path
        internal - internal messages

    TODOs
        figure out a nice way to store diagnostic codes so that we can collect them and output 
        a machine-readable diagnotics result for testing 

*/

#include "Messenger.h"

namespace amu {
namespace diagnostic {

enum Language {
    English,
    Japanese,
    Spanish,
    Chinese,
    Russian,
};

Language lang;

namespace path { // @path

global Message
not_found(String path) {
    Message out;
    switch(lang) {
        case English: {
            out = message::init(
                message::plain("the path "),
                message::path(path),
                message::plain(" could not be found.")
            );
        }break;
        case Japanese: {
            out = message::init(
                message::plain("パス"),
                message::path(path),
                message::plain("が見つかりません")
            );
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    out.type = Message::Error;
    return out;
}

namespace internal { // @internal



} // namespace internal

} // namespace path
} // namespace diagnostic
} // namespace amu