/*

    Definitions of any diagnostic message that amu may emit.

    Prefix an index name with '@' with find to go to it
    Index: 
        path     - messages relating to a path
        internal - internal messages

    TODOs
        figure out a nice way to store diagnostic codes so that we can collect them and output 
        a machine-readable diagnotics result for testing 

        maybe convert this to not be functional

        verify any Japanese that I write, mostly doing it to learn some vocabulary and it is 
        going to be very far from correct.

*/

#include "Messenger.h"
#ifndef AMU_DIAGNOSTIC_IMPL
#define AMU_DIAGNOSTIC_IMPL
#include "data/diagnostic-impl.generated"
#endif

// namespace amu {


// namespace diagnostic {

// enum Language {
//     English,
//     Japanese,
//     Spanish,
//     Chinese,
//     Russian,
// };

// Language lang;

// namespace path { // @path

// global Message
// not_found(String path) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(
//                 message::plain("the path "),
//                 message::path(path),
//                 message::plain(" could not be found.")
//             );
//         }break;
//         case Japanese: {
//             out = message::init(
//                 message::plain("パス"),
//                 message::path(path),
//                 message::plain("が見つかりません")
//             );
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// } // namespace path

// namespace compiler {

// global Message
// expected_a_path_for_arg(String arg) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a path for arg '"), arg, String("'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// unknown_option(String option) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unknown option '"), option, String("'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// no_path_given() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("no input file"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// } // namespace compiler

// namespace lexer { // @lexer

// global Message
// unexpected_eof_single_quotes() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unexpected EOF encountered while parsing single quotes"));
//         } break;
//         case Japanese: {
//             out = message::init(String("一重引用符は解析すながら予期せぬファイルの終わり見つかり"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// unexpected_eof_double_quotes() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unexpected EOF encountered while parsing double quotes"));
//         } break;
//         case Japanese: {
//             out = message::init(String("二重引用符は解析すながら予期せぬファイルの終わり見つかり"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// multiline_comment_missing_end() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("multiline comment missing '*/'"));
//         } break;
//         case Japanese: {
//             out = message::init(String("複数行コメントの\"*/\"がぬけています"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// unknown_directive(String inv) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unknown directive "), message::identifier(inv));
//         } break;
//         case Japanese: {
//             out = message::init(String("不明の命令"), message::identifier(inv));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// invalid_token() { // this should probably never happen
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("invalid token"));
//         } break;
//         case Japanese: {
//             out = message::init(String("無効のトークン"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// } // namespace lexer

// namespace parser {

// global Message
// expected_type() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a type"));
//         } break;
//         case Japanese: {
//             out = message::init(String("タイプが期待した"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_label_or_import() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a label or import"));
//         } break;
//         case Japanese: {
//             out = message::init(String("名札か輸入が期待した"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_identifier() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected an identifier"));
//         } break;
//         case Japanese: {
//             out = message::init(String("識別名が期待した"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_import_directive() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected an import directive"));
//         } break;
//         case Japanese: {
//             out = message::init(String("輸入命令が期待した"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_colon_for_label() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a ':' after label(s)"));
//         } break;
//         case Japanese: {
//             out = message::init(String("ラベルの後にコロンが必要"));
//         } break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_comma_or_closing_paren_tuple() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a ',' or ')' for tuple"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// unexpected_token(Token* t = 0) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unexpected token "));
//             if(t) message::push(out, *t);
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_open_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a '('"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_close_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a ')'"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_open_brace() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a '{'"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// expected_close_brace() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a '}'"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// empty_switch() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("empty switch"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Warning;
//     return out;
// }


// // labelgroup: ID ( "," ID )+
// //                      ^ is missing
// global Message
// label_group_missing_id() { 
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("trailing comma not allowed in multi-label declaration"));
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break; 
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// label_missing_colon() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing a ':' after label identifier(s)"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// switch_missing_open_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing a '(' after 'switch'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// switch_missing_close_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing a ')' after switch's expression"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// switch_missing_open_brace() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing a '{' to start switch expression"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }


// global Message
// switch_empty_body() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("empty switch body"));
//         } break;
//     }
//     out.type = Message::Warning;
//     return out;
// }

// global Message
// switch_missing_match_arrow_after_expr() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a '=>' after expression in switch expression"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// switch_missing_comma_after_match_arm() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a ',' after switch arm that does not end with a '}'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// tuple_expected_comma_or_close_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("expected a ',' or ')' for tuple"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// if_missing_open_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing '(' after 'if'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// if_missing_close_paren() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing ')' after if expression's condition"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// missing_semicolon() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing ';'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// missing_function_return() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing return type(s) for function"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// missing_open_brace_for_struct() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("missing '{' after 'struct'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// struct_only_labels_allowed() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("only labels are allowed in structures"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// struct_member_functions_not_allowed() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("member functions are not allowed"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// extraneous_close_brace() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("extraneous closing blace '}'"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// global Message
// unexpected_token_in_module() {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(String("unexpected token in module, only labels and directives are allowed"));
//         } break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// } // namespace parser

// namespace internal { // @internal

// global Message
// valid_path_but_internal_err(String path, String err) {
//     Message out;
//     switch(lang) {
//         case English: {
//             out = message::init(
//                     String("valid path was given, "),
//                     message::path(path),
//                     String(", but couldn't be opened to due to an internal error: "),
//                     err);
//         } break;
//         case Japanese: NotImplemented; break;
//         case Spanish: NotImplemented; break;
//         case Chinese: NotImplemented; break;
//         case Russian: NotImplemented; break;
//     }
//     out.type = Message::Error;
//     return out;
// }

// } // namespace internal

// } // namespace diagnostic
// } // namespace amu