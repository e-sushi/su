namespace amu {
namespace parser{

Parser
init(Source* source) {
    Parser out;
    out.source = source;
    out.labels.exported = shared_array::init<Label*>();
    out.labels.imported = shared_array::init<Label*>();
    out.labels.internal = shared_array::init<Label*>();
    out.label.stack = array::init<Label*>();
    out.label.table = map::init<String, Label*>();

    out.source->module = compiler::create_entity();
    out.source->module->type = entity::module;
    node::init(&out.source->module->node);
    out.source->module->node.type = node::type::entity;

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

FORCE_INLINE void
debug_announce_stage(String stage) {
    if(compiler::instance.options.verbosity < message::verbosity::debug) return;
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug, 
            String("parse level: "), stage)));
}

// parses a subtype, which is of the form
// id { "." id }
TNode* subtype(TNode* parent) {
    Assert(curt->type == Token::Identifier);
    debug_announce_stage("subtype");


    return 0;
}

TNode* label(TNode* parent) {
    Assert(curt->type == Token::Identifier);
    debug_announce_stage("label");


    Statement* stmt = compiler::create_statement();
    stmt->type = statement::label;
    node::insert_last(parent, &stmt->node);

    Label* label = compiler::create_label();
    label->token = curt;
    map::add(parser->label.table, curt->raw, label);
    node::insert_last(&stmt->node, &label->node);

    curt++;
    switch(curt->type) {
        case Token::Comma: {
            // we are making multiple labels
            Tuple* t = compiler::create_tuple();
            t->type = tuple::label_group;
            node::change_parent(&t->node, &label->node);
            node::insert_last(&stmt->node, &t->node);

            while(1) {
                curt++;
                if(curt->type != Token::Identifier) {
                    messenger::dispatch(message::attach_sender({parser->source, *curt},
                        diagnostic::parser::expected_identifier()));
                }

                Label* label = compiler::create_label();
                label->token = curt;
                node::insert_last(&t->node, &label->node);

                curt++;
                if(curt->type != Token::Comma) break;
            }
        } break;
    }

    return 0;
}

// parses an import
// "#" "import" ( id | string )
TNode* import(TNode* parent) {
    Assert(curt->type == Token::Pound);
    debug_announce_stage("import");

    curt++;
    if(curt->type != Token::Directive_Import) {
        messenger::dispatch(message::attach_sender({parser->source, *curt},
            diagnostic::parser::expected_import_directive()));
        return 0;
    }
    curt++;
    switch(curt->type) {
        case Token::Identifier: {

        } break;
        case Token::String: {

        } break;
    }

    return 0;
}

TNode* file(TNode* parent) {
    debug_announce_stage("file");

    switch(curt->type) {
        case Token::Pound: import(parent); break;
        case Token::Identifier: label(parent); break;

        default: {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::expected_label_or_import()));
            return 0;
        } break;
    }

    return 0;
}

} // namespace internal

void
execute(Parser& parser) {
    Stopwatch parser_time = start_stopwatch();

    messenger::dispatch(message::attach_sender(parser.source,
        message::debug(message::verbosity::stages,
            String("beginning syntactic analysis"))));

    Lexer& lexer = *parser.source->lexer;
    internal::parser = &parser;
    internal::curt = array::readptr(lexer.tokens, 0);
    internal::file(&parser.source->module->node);
}

} // namespace parser
} // namespace amu