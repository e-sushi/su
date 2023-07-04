namespace amu {
namespace parser{

Parser
init(Source* source) {\
    Parser out;
    out.source = source;
    out.labels.exported = shared_array::init<Label*>();
    out.labels.imported = shared_array::init<Label*>();
    out.labels.internal = shared_array::init<Label*>();
    return out;
}

void
deinit(Parser& parser) {
    shared_array::deinit(parser.labels.exported);
    shared_array::deinit(parser.labels.imported);
    shared_array::deinit(parser.labels.internal);
    parser.source = 0;
}

namespace internal {

Parser* parser;
Token* curt;

TNode* label(TNode* parent) {
    Assert(curt->type == Token::Identifier);

    Label* label = compiler::create_label();
    label->token = curt;
    node::insert_last(parent, &label->node);

    curt++;

    switch(curt->type) {
        case Token::Comma: {
            // we are making multiple labels
            while(1) {
                curt++;
                if(curt->type != Token::Identifier) {
                    messenger::dispatch(message::attach_sender({parser->source, *curt},
                        diagnostic::parser::expected_identifier()));
                }

                Label* label = compiler::create_label();
                label->token = curt;
            }
        } break;
    }
}

TNode* import(TNode* parent) {

}

TNode* file(TNode* parent) {
    switch(curt->type) {
        case Token::Pound: import(parent); break;
        case Token::Identifier: label(parent); break;

        default: {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::expected_label_or_import()));
            return 0;
        } break;
    }
}

} // namespace internal

void
execute(Parser& parser) {
    Stopwatch parser_time = start_stopwatch();

    messenger::dispatch(message::attach_sender(parser.source,
        message::debug(message::verbosity::stages,
            String("beginning syntactic analysis"))));

    Lexer& lexer = *parser.source->lexer;

    messenger::dispatch(message::attach_sender(parser.source,
        message::debug(message::verbosity::stageparts,
            String("performing initial scan"))));

    internal::parser = &parser;
    internal::curt = array::readptr(lexer.tokens, 0);
    internal::file(&parser.source->module->node);
}

} // namespace parser
} // namespace amu