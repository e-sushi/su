/*

    Definitions of any diagnostic message that amu may emit.

    Prefix an index name with '@' with find to go to it
    Index: 
        path     - messages relating to a path
        internal - internal messages

    TODOs
        figure out a nice way to store diagnostic codes so that we can collect them and output 
        a machine-readable diagnotics result for testing 

        verify any Japanese that I write, mostly doing it to learn some vocabulary and it is 
        going to be very far from correct.

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

} // namespace path

namespace lexer { // @lexer

global Message
unexpected_eof_single_quotes() {
    Message out;
    switch(lang) {
        case English: {
            out = message::init(String("unexpected EOF encountered while parsing single quotes"));
        } break;
        case Japanese: {
            out = message::init(String("一重引用符は解析すながら予期せぬファイルの終わり見つかり"));
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    out.type = Message::Error;
    return out;
}

global Message
unexpected_eof_double_quotes() {
    Message out;
    switch(lang) {
        case English: {
            out = message::init(String("unexpected EOF encountered while parsing double quotes"));
        } break;
        case Japanese: {
            out = message::init(String("二重引用符は解析すながら予期せぬファイルの終わり見つかり"));
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    out.type = Message::Error;
    return out;
}

global Message
multiline_comment_missing_end() {
    Message out;
    switch(lang) {
        case English: {
            out = message::init(String("multiline comment missing '*/'"));
        } break;
        case Japanese: {
            out = message::init(String("複数行コメントの\"*/\"がぬけています"));
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    out.type = Message::Error;
    return out;
}

global Message
unknown_directive(String inv) {
    Message out;
    switch(lang) {
        case English: {
            out = message::init(String("unknown directive "), message::identifier(inv));
        } break;
        case Japanese: {
            out = message::init(String("不明の命令"), message::identifier(inv));
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    return out;
}

global Message
invalid_token(String inv) { // this should probably never happen
    Message out;
    switch(lang) {
        case English: {
            out = message::init(String("invalid token "), message::identifier(inv));
        } break;
        case Japanese: {
            out = message::init(String("無効のトークン"), message::identifier(inv));
        } break;
        case Spanish: NotImplemented; break;
        case Chinese: NotImplemented; break;
        case Russian: NotImplemented; break;
    }
    return out;
}

} // namespace lexer

namespace internal { // @internal



} // namespace internal

} // namespace diagnostic
} // namespace amu