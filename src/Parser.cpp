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

    out.source->module = compiler::create_module();
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
Array<TNode*> stack;

#define advance_curt() do {                            \
    curt++;                                            \
    if(curt->kind == token::directive_compiler_break){ \
        DebugBreakpoint;                               \
        curt++;                                        \
    }                                                  \
} while(0)

void
__stack_push(TNode* n, String caller) {
    array::push(stack, n);
    messenger::dispatch(message::attach_sender({parser->source, curt},
        message::make_debug(message::verbosity::debug,
            String("pushed: "), caller, String(" // "), 
                (n->start? n->start->raw : String("bad start")), String(" -> "), 
                (n->end? n->end->raw : String("bad end")))));
}

#define stack_push(n) __stack_push(n, __func__)

TNode*
__stack_pop(String caller) {
    TNode* ret = array::pop(stack);
    messenger::dispatch(message::attach_sender({parser->source, curt},
        message::make_debug(message::verbosity::debug,
            String("popped: "), caller, String(" // "), 
                (ret->start? ret->start->raw : String("bad start")), String(" -> "), 
                (ret->end? ret->end->raw : String("bad end")))));
    return ret;
}

#define stack_pop() __stack_pop(__func__)

FORCE_INLINE void
debug_announce_stage(String stage) {
    if(compiler::instance.options.verbosity < message::verbosity::debug) return;
    messenger::dispatch(message::attach_sender({parser->source, curt},
        message::make_debug(message::verbosity::debug, 
            String("parse level: "), stage)));
}

#define announce_stage debug_announce_stage(__func__)

#define check_error if(!array::read(stack, -1)) return;
#define push_error() do{                                                \
    messenger::dispatch(message::attach_sender({parser->source, curt}, \
        message::make_debug(message::verbosity::debug,                       \
            String(__func__), String(ErrorFormat(" pushed error")))));  \
    return (void)array::push(stack, (TNode*)0);                         \
}while(0)


#define set_start_end_from_children(n)                  \
do{                                                     \
((TNode*)n)->start = (((TNode*)n)->first_child)->start; \
((TNode*)n)->end = (((TNode*)n)->last_child)->end;      \
}while(0)                                    


void before_expr();
void label_after_colon();
void factor();
void after_typeref(); 
void label_after_id();
void label();
void logi_or();
void logi_and();
void bit_or();
void bit_xor();
void bit_and();
void equality();
void relational();
void bit_shift();
void additive();
void term();
void access();


// state8:
// factor: ID *
// reduce ID -> expr:id
void reduce_identifier_to_identifier_expression() { announce_stage;
    Expression* e = compiler::create_expression();
    e->kind = expression::identifier;
    TNode* n = (TNode*)e;
    n->start = n->end = curt;
    stack_push((TNode*)e);
}

/* there doesn't need to do anything special for reducing a literal, because they all become the same sort of expression
   which just stores the token
    literal: int | float | string | char

    TODO(sushi) this may only be used in one or two places so it can be removed if so 
*/
void reduce_literal_to_literal_expression() { announce_stage;
    Expression* e = compiler::create_expression();
    e->kind = expression::literal;
    TNode* n = (TNode*)e;
    n->start = n->end = curt;
    stack_push((TNode*)e);
}


/*
    typeref: "void" * 
           | "u8" *  | "u16" *  | "u32" *  | "u64" * 
           | "s8" *  | "s16" *  | "s32" *  | "s64" * 
           | "f32" * | "f64" *
*/
void reduce_builtin_type_to_typeref_expression() { announce_stage;
    Expression* e = compiler::create_expression();
    e->kind = expression::typeref;
    Type type = {};
    switch(curt->kind) {
        case token::void_:      type.structure = compiler::builtins.void_; break; 
        case token::unsigned8:  type.structure = compiler::builtins.unsigned8; break;
        case token::unsigned16: type.structure = compiler::builtins.unsigned16; break;
        case token::unsigned32: type.structure = compiler::builtins.unsigned32; break;
        case token::unsigned64: type.structure = compiler::builtins.unsigned64; break;
        case token::signed8:    type.structure = compiler::builtins.signed8; break;
        case token::signed16:   type.structure = compiler::builtins.signed16; break;
        case token::signed32:   type.structure = compiler::builtins.signed32; break;
        case token::signed64:   type.structure = compiler::builtins.signed64; break;
        case token::float32:    type.structure = compiler::builtins.float32; break;
        case token::float64:    type.structure = compiler::builtins.float64; break;
    }
    e->type = type;
    e->node.start = curt;
    e->node.end = curt;
    stack_push((TNode*)e);
}

/*
        tuple: '(' ... ')' *
    func_type: tuple * "->" factor { "," factor } 

*/
void tuple_after_close_paren() { announce_stage;
    if(curt->kind == token::function_arrow) {
        advance_curt();
        u32 count = 0;
        while(1) {
            factor();
            count++;
            if(curt->kind != token::comma) break;
            advance_curt();
        }

        if(!count) {
            diagnostic::parser::
                missing_function_return_type({parser->source, curt});
            push_error();
        }

        Expression* e = compiler::create_expression();
        e->kind = expression::typeref;
        e->type.structure = compiler::builtins.functype;

        if(count > 1) {
            Tuple* t = compiler::create_tuple();
            t->kind = tuple::multireturn;
            
            forI(count) {
                node::insert_first((TNode*)t, stack_pop());
            }

            node::insert_last((TNode*)e, (TNode*)t);

            set_start_end_from_children(t);
        } else {
            node::insert_first((TNode*)e, stack_pop());
        }

        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        // since we've made an entity, we need to push it to the stack first so that whatever
        // label is taking it can consume it 
        Function* f = compiler::create_function();
        stack_push((TNode*)f);
        stack_push((TNode*)e);

        after_typeref(); 
    }
}


/* 
    tuple: '(' * ( label | expr ) { ( label | expr ) "," } [ "," ] ')' 
    label: * ID ':' ...
     expr: ...
*/
void tuple_after_open_paren() { announce_stage; announce_stage;
    Token* save = curt-1;
    
    u32 count = 0;
    while(1) { // NOTE(sushi) label lists are not supported in tuples, so if a comma is encountered, it's assumed to be starting a new tuple item
        if(curt->kind == token::close_paren) break;
        switch(curt->kind) {
            case token::identifier: {
                // need to figure out if this is an expression or label 
                switch((curt+1)->kind) {
                    case token::colon:{
                        label(); check_error;
                    } break;
                    default: {
                        // otherwise this is assumed to be an expression handled by before_expr
                        before_expr(); check_error;
                    } break;
                }

            } break;
            default: {
                before_expr();
            }break;
        }
        count += 1;
        if(curt->kind == token::comma) advance_curt();
        else if(curt->kind != token::close_paren) { // there was some error in consuming an expr/label
            diagnostic::parser::
                tuple_expected_comma_or_close_paren({parser->source, curt});
            return;
        }
    }


    Tuple* tuple = compiler::create_tuple();
    tuple->kind = tuple::unknown;

    forI(count) {
        node::insert_first((TNode*)tuple, stack_pop());
    }

    if(count) set_start_end_from_children(tuple);
    else{
        tuple->node.start = save;
        tuple->node.end = curt;
    }   
    stack_push((TNode*)tuple);

    advance_curt();

    tuple_after_close_paren();
}

/*
    block: '{' { stmt } '}'
*/
void block() {
    u32 count = 0;
    Token* start = curt-1;
    while(1) {
        statement::kind skind = statement::unknown;
        if(curt->kind == token::close_brace) break;
        switch(curt->kind) {
            case token::identifier: {
                if((curt+1)->kind == token::colon) {
                    label(); check_error;
                    skind = statement::label;
                } else {
                    before_expr(); check_error;
                    skind = statement::expression;
                }
            } break;
            default: {
                before_expr(); check_error;
                skind = statement::expression;
            } break;
        }
        if(curt->kind != token::semicolon && curt->kind != token::close_brace) {
            diagnostic::parser::
                missing_semicolon({parser->source, curt});
        }

        Statement* s = compiler::create_statement();
        s->kind = skind;

        node::insert_first((TNode*)s, stack_pop());
        s->node.start = s->node.first_child->start;
        s->node.end = curt;

        stack_push((TNode*)s);

        advance_curt();
        count++;
    }

    Expression* e = compiler::create_expression();
    e->kind = expression::block;

    forI(count) {
        node::insert_first((TNode*)e, stack_pop());
    }
    e->node.start = start;
    e->node.end = curt;
    stack_push((TNode*)e);
}

/*
    comptime: * typeref ':' expr
            | typeref ':' * expr
            | * ':' expr
*/ 
void comptime_after_colon() { announce_stage;
    switch(curt->kind) {
        case token::identifier:      reduce_identifier_to_identifier_expression(); advance_curt(); break;
        case token::colon:           advance_curt(); comptime_after_colon(); check_error; break;
        case token::open_paren:      advance_curt(); tuple_after_open_paren(); check_error; break;
        default: {
            if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                advance_curt();
            }else if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                advance_curt();
            }
        }break;
    }

    TNode* last = array::read(stack, -1);

    switch(last->kind) {
        case node::expression: {
            Expression* expr = (Expression*)last;
            switch(expr->kind) {
                case expression::binary_plus: {
                    
                } break;
            }
        } break;    
    }
}

/*
    assignment: typeref * '=' expr
         ctime: typeref * ':' expr
        factor: typeref * [ block ]
*/ 
void after_typeref() { announce_stage;
    Token* save = curt;
    switch(curt->kind) {
        case token::colon: advance_curt(); comptime_after_colon(); break;
        case token::assignment: {
            advance_curt(); // assignment: typeref '=' * expr
            before_expr(); check_error;
            // now we reduce to binary assignment
            Expression* e = compiler::create_expression();
            e->kind = expression::binary_assignment;
            e->node.start = save;
            e->node.end = curt;
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);
        } break;
        case token::open_brace: {
            advance_curt();
            block(); check_error;
            node::insert_last(array::read(stack, -2), stack_pop());
            TNode* typeref = array::read(stack, -1);
            typeref->end = typeref->last_child->end;
        } break;
    }
}



/*
    loop: switch | "loop" expr
*/
void loop() { announce_stage;
    check_error;
    if(curt->kind == token::loop) {
        advance_curt();
        before_expr();

        Expression* loop = compiler::create_expression();
        loop->kind = expression::loop;
        node::insert_last((TNode*)loop, stack_pop());
        set_start_end_from_children(loop);
        stack_push((TNode*)loop);
    }
}

/*
    switch: conditional | "switch" '(' expr ')' '{' { expr "=>" expr } '}'
*/ 
void switch_() { announce_stage;
    check_error;
    if(curt->kind == token::switch_) {
        advance_curt();
        if(curt->kind != token::open_paren) {
            diagnostic::parser::
                switch_missing_open_paren({parser->source, curt});
            return;
        }

        advance_curt(); // "switch" "(" * expr ")" ...

        before_expr(); // dont care what this returns cause any expression is good (for now)

        if(curt->kind != token::close_paren) {
            diagnostic::parser::
                switch_missing_close_paren({parser->source, curt});
            push_error();
        }

        advance_curt(); // "switch" "(" expr ")" * "{" ...

        if(curt->kind != token::open_brace) {
            diagnostic::parser::
                switch_missing_open_brace({parser->source, curt});
            push_error();
        }

        advance_curt(); // "switch" "(" expr ")" "{" * { expr } "}"

        u32 count = 0;
        while(1) {
            if(curt->kind == token::close_brace) break;
            before_expr(); 
            
            if(curt->kind != token::match_arrow) {
                diagnostic::parser::
                    switch_missing_match_arrow_after_expr({parser->source, curt});
                push_error();
            }

            advance_curt();
            before_expr();

            if(curt->kind != token::comma && curt->kind != token::close_brace ) {
                diagnostic::parser::
                    switch_missing_comma_after_match_arm({parser->source, curt});
                push_error();
            }
            advance_curt();
            count++;

            Expression* e = compiler::create_expression();
            e->kind = expression::switch_case;
            
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);

            stack_push((TNode*)e);

        }

        if(!count) {
            diagnostic::parser::
                switch_empty_body({parser->source, curt});
        }

        Expression* e = compiler::create_expression();
        e->kind = expression::switch_expr;

        // reduce to switch expression
        forI(count+1){
            node::insert_first((TNode*)e, stack_pop());
        }

        set_start_end_from_children(e);
        stack_push((TNode*)e);
    }
}

/*
    conditional: logi-or | "if" '(' expr ')' expr [ "else" expr ]
*/
void conditional() { announce_stage;
    check_error;
    if(curt->kind == token::if_) {
        advance_curt();

        if(curt->kind != token::open_paren) {
            diagnostic::parser::
                if_missing_open_paren({parser->source, curt});
            push_error();
        }

        advance_curt();

        before_expr();

        if(curt->kind != token::close_paren) {
            diagnostic::parser::
                if_missing_close_paren({parser->source, curt});
            push_error();
        }

        advance_curt();

        before_expr();

        Expression* e = compiler::create_expression();
        e->kind = expression::conditional;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        if(curt->kind == token::else_) {
            advance_curt();
            before_expr();
            node::insert_last((TNode*)e, stack_pop());
        }
    }
}

// TODO(sushi) this long chain of binary op handlers can probably all be combined into one function

/*
    logi-or: logi-and { "||" logi-and }
*/
void logi_or() { announce_stage;
    check_error;
    if(curt->kind == token::logi_or) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        bit_and(); check_error;
        bit_xor(); check_error;
        bit_or(); check_error;
        logi_and(); check_error;
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_or;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        logi_or();
    }
}

/*
    logi-and: bit-or { "&&" bit-or }
*/
void logi_and() { announce_stage;
    check_error;
    if(curt->kind == token::logi_and) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        bit_and(); check_error;
        bit_xor(); check_error;
        bit_or(); check_error;
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_and;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        logi_and();
    }
}

/*
    bit-or: bit-xor { "|" bit-xor }
*/
void bit_or() { announce_stage;
    check_error;
    if(curt->kind == token::bit_or) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        bit_and(); check_error;
        bit_xor(); check_error;
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_or;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        bit_or();
    }
}

/*
    bit-xor: bit-and { "^" bit-and }
*/
void bit_xor() { announce_stage;
    check_error;
    if(curt->kind == token::bit_xor) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        bit_and(); check_error;
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_xor;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        bit_xor();
    }
}


/*
    bit-and: equality { "&" equality } 
*/
void bit_and() { announce_stage;
    check_error;
    if(curt->kind == token::bit_and) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_and;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);
        stack_push((TNode*)e);

        bit_and();
    }
}

/*
    equality: relational { ( "!=" | "==" ) relational }
*/
void equality() { announce_stage;
    check_error;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::equal:
        case token::not_equal: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            bit_shift(); check_error;
            relational(); check_error;
            Expression* e = compiler::create_expression();
            e->kind = kind == token::equal ? expression::binary_equal : expression::binary_not_equal;

            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);

            equality();
        } break;
    }
}

/*
    relational: bit-shift { ( ">" | "<" | "<=" | ">=" ) bit-shift }
*/
void relational() { announce_stage;
    check_error;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::less_than:
        case token::less_than_or_equal:
        case token::greater_than:
        case token::greater_than_or_equal: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            bit_shift(); check_error;
            Expression* e = compiler::create_expression();
            e->kind = kind == token::less_than ?
                      expression::binary_less_than :
                      kind == token::less_than_or_equal ? 
                      expression::binary_less_than_or_equal :
                      kind == token::greater_than ? 
                      expression::binary_greater_than :
                      expression::binary_greater_than_or_equal;

            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);

            relational();
        } break;
    }
}

/*
    bit-shift: additive { "<<" | ">>" additive }
*/
void bit_shift() { announce_stage;
    check_error;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::bit_shift_left: 
        case token::bit_shift_right: {
            advance_curt(); 
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            Expression* e = compiler::create_expression();
            e->kind = kind == token::bit_shift_left ? 
                      expression::binary_bit_shift_left :
                      expression::binary_bit_shift_right;
            
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);

            bit_shift();
        } break;
    }
}

/*
    additive: term * { ("+" | "-" ) term }
*/
void additive() { announce_stage;
    check_error;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::plus:
        case token::negation: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            Expression* e = compiler::create_expression();
            e->kind = kind == token::plus ? expression::binary_plus : expression::binary_minus;
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);

            additive();
        } break;
    }
}

/*
    term: access * { ( "*" | "/" | "%" ) access }
*/
void term() { announce_stage;
    check_error;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::modulo: 
        case token::division: 
        case token::multiplication: {
            advance_curt();
            factor(); check_error; 
            access(); check_error;
            Expression* e = compiler::create_expression();
            e->kind = 
                    kind == token::modulo ? expression::binary_modulo 
                    : kind == token::division ? expression::binary_division
                    : expression::binary_multiply;
            
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            set_start_end_from_children(e);
            stack_push((TNode*)e);

            term();
        } break;
    }
}

/*
    access: factor * { "." factor }
*/
void access() { announce_stage;
    check_error;
    if(curt->kind == token::dot) {
        advance_curt();
        factor(); check_error;
        // access: factor { "." factor * }
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_access;
        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());
        set_start_end_from_children(e);
        stack_push((TNode*)e);
        access();
    }
}

       

/*
    access: * factor { "." factor }
          | factor { "." * factor }
    factor: * (literal | id | tuple | type )
*/
void factor() { announce_stage;
    check_error;
    switch(curt->kind) {
        case token::identifier: reduce_identifier_to_identifier_expression(); advance_curt(); break;
        case token::open_paren: advance_curt(); tuple_after_open_paren(); break;
        case token::if_: conditional(); break;
        case token::switch_: switch_(); break;
        case token::open_brace: advance_curt(); block(); break;
        default: {
            if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                advance_curt();
            } else if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                advance_curt();
            } else {
                diagnostic::parser::
                    unexpected_token({parser->source, curt}, curt);
                push_error();
            }
        } break;
    }
}

void struct_decl() {
    Token* save = curt;
    advance_curt(); // TODO(sushi) struct parameters
    if(curt->kind != token::open_brace) {
        diagnostic::parser::
            missing_open_brace_for_struct({parser->source, curt});
        push_error();
    }

    advance_curt();
    
    u32 count = 0;
    while(1) {
        if(curt->kind == token::close_brace) break;
        if(curt->kind == token::identifier) {
            label(); check_error;
            if(curt->kind != token::semicolon) {
                diagnostic::parser::
                    missing_semicolon({parser->source, curt});
            }
            advance_curt();
            count++;
            TNode* last = array::read(stack, -1);
            if(last->last_child->kind == node::function) {
                diagnostic::parser::
                    struct_member_functions_not_allowed({parser->source, last->start});
                push_error();
            }
        } else {
            diagnostic::parser::
                struct_only_labels_allowed({parser->source, curt});
            push_error();
        }
    }

    Structure* s = compiler::create_structure();
    s->node.start = save;
    s->node.end = curt;

    forI(count) {
        node::insert_first((TNode*)s, stack_pop());
    }

    stack_push((TNode*)s);
    stack_push((TNode*)s);

}


/* general expr handler, since we will come across this alot
           expr: * ( loop | switch | assignment | ctime | conditional ) .
           loop: * "loop" expr
         switch: * "switch" '(' expr ')' '{' { expr } '}'
     assignment: * [ expr | typeref ] . '=' expr .
          ctime: * [ typeref ] ':' expr
    conditional: * ( logical-or | "if" '(' expr ')' expr [ "else" expr ] 
     logical-or: * logical-and { "||" logical-and }
    logical-and: * bit-or { "&&" bit-or }            
         bit-or: * bit-xor { "|" bit-xor }                                 
        bit-and: * equality { "&" equality }                              
       equality: * relational { ("!=" | "==" ) relational }              
      elational: * bit-shift { ( "<" | ">" | "<=" | ">=" ) bit-shift } 
           term: * factor { ( '*' | '/' | '%' ) factor }
         factor: * ( literal | ID | tuple | type )
          tuple: * '(' tupleargs ')'
        typeref: * ( func_type | factor decorators | "void" | "u8" | "u16" | "u32" | "u64" | "s8" | "s16" | "s32" | "s64" | "f32" | "f64" )
        literal: * ( string | float | int | char )
*/
void before_expr() { announce_stage;
    switch(curt->kind) {
        case token::assignment: {
            Token* save = curt;
            advance_curt();
            before_expr();

            Expression* e = compiler::create_expression();
            e->kind = expression::unary_assignment;

            node::insert_first((TNode*)e, stack_pop());
            e->node.start = save;
            e->node.end = e->node.last_child->end;

            stack_push((TNode*)e);
        } break;
        case token::colon: {
            Token* save = curt;
            advance_curt();
            before_expr(); check_error;

            Expression* e = compiler::create_expression();
            e->kind = expression::unary_comptime;

            node::insert_first((TNode*)e, stack_pop());
            e->node.start = save;
            e->node.end = e->node.last_child->end;
            stack_push((TNode*)e);
        } break;
        case token::structdecl: {
            struct_decl(); check_error;
        } break;
        default: factor(); check_error;
    }

    // loop and see if any operators are being used, if so call their entry point
    b32 search = true;
    while(search) {
        switch(curt->kind) {
            case token::dot: access(); check_error; break;
            case token::multiplication:
            case token::division: term(); check_error; break;
            case token::plus:
            case token::negation: additive(); check_error; break;
            case token::bit_shift_left:
            case token::bit_shift_right: bit_shift(); check_error; break;
            case token::less_than:
            case token::less_than_or_equal:
            case token::greater_than:
            case token::greater_than_or_equal: equality(); check_error; break;
            case token::bit_and: bit_and(); check_error; break;
            case token::bit_xor: bit_xor(); check_error; break;
            case token::bit_or: bit_or(); check_error; break;
            case token::logi_and: logi_and(); check_error; break;
            case token::logi_or: logi_or(); check_error; break;
            case token::directive_compiler_break: DebugBreakpoint; break;
            //case token::open_brace: advance_curt(); block(); check_error; break;
            default: search = false;
        }
    }

    // after_expr(); TODO(sushi)
}


/*
          label: ( ID | labelgroup ) ':' * expr ';'
           expr: * ( loop | switch | assignment | ctime | conditional ) .
           loop: * "loop" expr
         switch: * "switch" '(' expr ')' '{' { expr } '}'
     assignment: * [ expr | typeref ] . '=' expr .
          ctime: * [ typeref ] ':' expr
    conditional: * ( logical-or | "if" '(' expr ')' expr [ "else" expr ] 
            ...
           term: * factor { ( '*' | '/' | '%' ) factor }
         factor: * ( NUM | ID | tuple | type )
          tuple: * '(' tupleargs ')'
        typeref: * ( func_type | factor decorators | "void" | "u8" | "u16" | "u32" | "u64" | "s8" | "s16" | "s32" | "s64" | "f32" | "f64" )
*/ 
void label_after_colon() { announce_stage;
    check_error;
  
    switch(curt->kind) {
        case token::identifier:      reduce_identifier_to_identifier_expression(); advance_curt(); break;
        case token::colon:           before_expr(); break;
        case token::open_paren:      advance_curt(); tuple_after_open_paren(); break;
        case token::assignment:      before_expr(); break;
        default: {
            if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                advance_curt();
            }else if(curt->group == token::group_literal){
                reduce_literal_to_literal_expression();
                advance_curt();
            }
        } break;
    }

    check_error;

    TNode* last = array::read(stack, -1);

    switch(last->kind) {
        case node::expression: {
            Expression* expr = (Expression*)last;
            switch(expr->kind) {
                case expression::typeref: after_typeref(); check_error; break;
            }
        } break;
    }

    // switch(last->kind) {
    //     case node::expression: {
    //         Expression* expr = (Expression*)last;
    //         switch(expr->kind) {
    //             case expression::binary_assignment: reduce_assignment_to_expr(); break;
    //             case expression::binary_comptime:   reduce_comptime_to_expr(); break;
    //             case expression::binary_plus: 
    //             case expression::binary_minus:      reduce_additive_to_expr(); break;

    //         }
    //     } break;
    // }
    

}

// state3: module: label * EOF
void before_eof() { announce_stage;
    // switch(curt->kind) {
    //     case token::end_of_file: advance_curt(); after_eof(); break;
    // }
}

// state2: accept: module * end
void before_end() { announce_stage;
    // this is the end, just dont do anything
}

// // state1: 
// // label: ID * { "," ID } ':' expr ( ';' | ']' | '}' )
// void label_before_colon_or_list() { announce_stage;
//     switch(curt->kind) {
//         case token::colon: advance_curt(); label_after_colon(); break;
//         case token::comma: 
//     }
// }

/*
         label: labelgroup ':' expr ( ';' | ']' | '}' )
    labelgroup: ID ( "," * ID )+

    here we are gathering a 'labelgroup', which is internally represented as a Tuple
    with the identifiers as its children

*/
void label_group_after_comma() { announce_stage;
    check_error;

    while(1) {
        if(curt->kind != token::identifier) break; 
        Expression* expr = compiler::create_expression();
        expr->kind = expression::identifier;
        
        TNode* last = array::read(stack, -1);
        if(last->kind == node::tuple) {
            // a label group was already created, so just append to it
            node::insert_last(last, (TNode*)expr);
        } else {
            // this is the second label, so the last must be another identifier
            // make the label group tuple
            Tuple* group = compiler::create_tuple();
            group->kind = tuple::label_group;
            node::change_parent((TNode*)group, stack_pop());
            node::insert_last((TNode*)group, (TNode*)expr);
            set_start_end_from_children(group);
            stack_push((TNode*)group);
        }
        advance_curt();
        if(curt->kind == token::comma) advance_curt();
        else break;
    }

    // if we have come to a place where a comma was not followed by an identifier
    // we throw an error about it
    if((curt-1)->kind == token::comma) {
        diagnostic::parser::
            label_group_missing_id({parser->source, curt});
    }

    if(curt->kind == token::colon) {
        label_after_colon();
    } else {
        diagnostic::parser::
            label_missing_colon({parser->source, curt});
    }
}

/*
         label: ( ID * | idgroup ) ':' expr ( ';' | ']' | '}' )
    labelgroup: ID * ( "," ID )+
*/
void label_after_id() { announce_stage;
    check_error;

    switch(curt->kind) {
        case token::comma: advance_curt(); label_group_after_comma(); break;
        case token::colon: advance_curt(); label_after_colon(); break;
    }
}

void label() {
    Expression* expr = compiler::create_expression();
    expr->kind = expression::identifier;
    expr->node.start = curt;
    expr->node.end = curt;
    stack_push((TNode*)expr);

    advance_curt();
    label_after_id(); check_error;

    // reduce to a label 
    Label* label = compiler::create_label();
    node::insert_first((TNode*)label, stack_pop());

    TNode* cand = stack_pop();
    switch(cand->kind) {
        case node::place:
        case node::structure:
        case node::function:
        case node::module: {
            label->entity = cand;
            node::insert_first((TNode*)label, stack_pop());
        } break;
        default: node::insert_first((TNode*)label, cand);
    }

    set_start_end_from_children(label);
    stack_push((TNode*)label);

}

/* 
        module: * { label } EOF 
         label: * ( ID | labelgroup ) ':' expr ( ';' | ']' | '}' )  
    labelgroup: ID ( "," ID )+ 
*/
void start() { announce_stage;
    u32 count = 0;
    while(1) {
        if(curt->kind != token::identifier) break;
        label(); check_error;

        if(curt->kind != token::close_brace && curt->kind != token::semicolon) {
            diagnostic::parser::
                missing_semicolon({parser->source, curt});
            return;
        }

        count++;
        advance_curt();
    }

    Module* m = compiler::create_module();
    forI(count) {
        node::insert_first((TNode*)m, stack_pop());
    }

    set_start_end_from_children(m);
    stack_push((TNode*)m);
}
} // namespace internal

void
execute(Parser& parser) {
    Stopwatch parser_time = start_stopwatch();

    messenger::dispatch(message::attach_sender(parser.source,
        message::make_debug(message::verbosity::stages,
            String("beginning syntactic analysis"))));

    Lexer lexer = *parser.source->lexer;
    internal::parser = &parser;
    internal::curt = array::readptr(lexer.tokens, 0);
    internal::stack = array::init<TNode*>(32);
    internal::start();

    DString time_taken = util::format_time(peek_stopwatch(parser_time));
    messenger::dispatch(message::attach_sender(parser.source,
        message::make_debug(message::verbosity::stages, 
            String("syntactic analysis finished in "), String(time_taken))));
    if(compiler::instance.options.deliver_debug_immediately)
        dstring::deinit(time_taken);
}

} // namespace parser
} // namespace amu