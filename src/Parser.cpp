/*

    NOTES
    -----

    When an Entity is created, eg. a Function, Place, etc. It is pushed onto the stack 
    *before* the actual node that represents it is placed


*/

namespace amu {
namespace parser{

Parser
init(Code* code) {
    Parser out;
    out.code = code;
    out.labels.exported = array::init<Label*>();
    out.labels.imported = array::init<Label*>();
    out.labels.internal = array::init<Label*>();
    out.module_stack = array::init<Module*>();
    out.current_module = 0;
    out.table_stack = array::init<LabelTable*>();
    out.current_table = 0;
    return out;
}

void
deinit(Parser& parser) {
    array::deinit(parser.labels.exported);
    array::deinit(parser.labels.imported);
    array::deinit(parser.labels.internal);
    array::deinit(parser.module_stack);
    array::deinit(parser.table_stack);
    parser.code = 0;
}

namespace internal {

Parser* parser;
Token* curt;
Array<TNode*> stack;

FORCE_INLINE void 
advance_curt() {
    curt++;
    if(curt->kind == token::directive_compiler_break) {
        DebugBreakpoint; curt++;
    }
}

// we will have already hit the break token, so dont break again
#define backtrack() do {                                \
    curt--;                                             \
    if(curt->kind == token::directive_compiler_break) { \
        curt--;                                         \
    }                                                   \
} while(0)

FORCE_INLINE Token* 
lookahead(u32 n) {
    Token* out = curt + n;
    while(out->kind == token::directive_compiler_break) out++;
    return out;
}

void
__stack_push(TNode* n, String caller) {
    array::push(stack, n);
    // messenger::dispatch(message::attach_sender(curt,
    //     message::make_debug(message::verbosity::debug,
    //         String("pushed: "), caller, String(" // "), 
    //             (n->start? n->start->raw : String("bad start")), String(" -> "), 
    //             (n->end? n->end->raw : String("bad end")))));
}

#define stack_push(n) __stack_push(n, __func__)

TNode*
__stack_pop(String caller) {
    TNode* ret = array::pop(stack);
    // messenger::dispatch(message::attach_sender(curt,
    //     message::make_debug(message::verbosity::debug,
    //         String("popped: "), caller, String(" // "), 
    //             (ret->start? ret->start->raw : String("bad start")), String(" -> "), 
    //             (ret->end? ret->end->raw : String("bad end")))));
    return ret;
}

#define stack_pop() __stack_pop(__func__)

FORCE_INLINE void
push_table(LabelTable* table) {
    array::push(parser->table_stack, parser->current_table);
    table->last = parser->current_table;
    parser->current_table = table;
}

FORCE_INLINE void
pop_table() {
    parser->current_table = array::pop(parser->table_stack);
}

FORCE_INLINE void 
push_module(Module* m) {
    array::push(parser->module_stack, parser->current_module);
    parser->current_module = m;
    push_table(&m->table);
}

FORCE_INLINE void 
pop_module() {
    parser->current_module = array::pop(parser->module_stack);
    pop_table();
}



FORCE_INLINE void
debug_announce_stage(String stage) {
    if(compiler::instance.options.verbosity < message::verbosity::debug) return;
    messenger::dispatch(message::attach_sender(curt,
        message::make_debug(message::verbosity::debug, 
            String("parse level: "), stage)));
}

#define announce_stage //debug_announce_stage(__func__)

#define check_error if(!array::read(stack, -1)) return;
#define push_error() do{                                                \
    messenger::dispatch(message::attach_sender(curt, \
        message::make_debug(message::verbosity::debug,                       \
            String(__func__), String(ErrorFormat(" pushed error")))));  \
    return (void)array::push(stack, (TNode*)0);                         \
}while(0)


#define set_start_end_from_children(n)                  \
do{                                                     \
((TNode*)n)->start = (((TNode*)n)->first_child)->start; \
((TNode*)n)->end = (((TNode*)n)->last_child)->end;      \
}while(0)                                    


Label* 
search_for_label(LabelTable* table, u64 hash) {
    while(1) {
        auto [idx, found] = map::find(table->map, hash);
        if(found) return array::read(table->map.values, idx);
        if(!table->last) return 0;
        table = table->last;
    }
}


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
void block();
void start();


// parses identifiers separated by commas and groups them under a tuple 
// this must be called after the first identifier and comma have been parsed
// and the first identfier must have been pushed onto the stack
void identifier_group() { announce_stage;
    while(1) {
        if(curt->kind != token::identifier) break; 
        Expression* expr = expression::create();
        expr->kind = expression::identifier;
        expr->node.start = expr->node.end = curt;
        
        TNode* last = array::read(stack, -1);
        if(last->kind == node::tuple) {
            // a label group was already created, so just append to it
            node::insert_last(last, (TNode*)expr);
            last->end = curt;
        } else {
            // this is the second label, so the last must be another identifier
            // make the label group tuple
            Tuple* group = tuple::create();
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

}

// state8:
// factor: ID *
// reduce ID -> expr:id
void reduce_identifier_to_identifier_expression() { announce_stage;
    Expression* e = expression::create();
    e->kind = expression::identifier;
    TNode* n = (TNode*)e;
    n->start = n->end = curt;
    stack_push(n);
}

/* there doesn't need to do anything special for reducing a literal, because they all become the same sort of expression
   which just stores the token
    literal: int | float | string | char

    TODO(sushi) this may only be used in one or two places so it can be removed if so 
*/
void reduce_literal_to_literal_expression() { announce_stage;
    Expression* e = expression::create();
    e->kind = expression::literal;
    TNode* n = (TNode*)e;
    n->start = n->end = curt;
    switch(curt->kind) {
        case token::literal_character: {
            e->type = &type::scalar::unsigned8;
        } break;
        case token::literal_float: {
            e->type = &type::scalar::float64;
        } break;
        case token::literal_integer: {
            e->type = &type::scalar::signed64;
        } break;
        case token::literal_string: {
            e->type = type::array::create(&type::scalar::unsigned8, curt->raw.count);
        } break;
    }
    stack_push(n);
}


/*
    typeref: "void" * 
           | "u8" *  | "u16" *  | "u32" *  | "u64" * 
           | "s8" *  | "s16" *  | "s32" *  | "s64" * 
           | "f32" * | "f64" *
*/
void reduce_builtin_type_to_typeref_expression() { announce_stage;
    Expression* e = expression::create();
    e->kind = expression::typeref;
    switch(curt->kind) {
        case token::void_:      e->type = &type::scalar::void_; break; 
        case token::unsigned8:  e->type = &type::scalar::unsigned8; break;
        case token::unsigned16: e->type = &type::scalar::unsigned16; break;
        case token::unsigned32: e->type = &type::scalar::unsigned32; break;
        case token::unsigned64: e->type = &type::scalar::unsigned64; break;
        case token::signed8:    e->type = &type::scalar::signed8; break;
        case token::signed16:   e->type = &type::scalar::signed16; break;
        case token::signed32:   e->type = &type::scalar::signed32; break;
        case token::signed64:   e->type = &type::scalar::signed64; break;
        case token::float32:    e->type = &type::scalar::float32; break;
        case token::float64:    e->type = &type::scalar::float64; break;
    }
    e->node.start = curt;
    e->node.end = curt;
    stack_push((TNode*)e);
}

/* 
    tuple: '(' * ( label | expr ) { ( label | expr ) "," } [ "," ] ')' 
    label: * ID ':' ...
     expr: ...
*/
void tuple_after_open_paren() { announce_stage; announce_stage;
    Token* save = curt-1;
    
    Tuple* tuple = tuple::create();
    tuple->kind = tuple::unknown;

    u32 count = 0;
    b32 found_label = 0;
    while(1) { // NOTE(sushi) label lists are not supported in tuples, so if a comma is encountered, it's assumed to be starting a new tuple item
        if(curt->kind == token::close_paren) break;
        switch(curt->kind) {
            case token::identifier: {
                // need to figure out if this is an expression or label 
                switch((curt+1)->kind) {
                    case token::colon:{
                        if(!found_label) {
                            tuple->table.map = map::init<String, Label*>();
                            push_table(&tuple->table);
                        }
                        found_label = true;
                        label(); check_error;
                    } break;
                    default: {
                        if(found_label) {
                            diagnostic::parser::tuple_positional_arg_but_found_label(curt);
                            push_error();
                        }
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
                tuple_expected_comma_or_close_paren(curt);
            return;
        }
    }

    

    forI(count){
        node::insert_first((TNode*)tuple, stack_pop());
    }

    if(count) set_start_end_from_children(tuple);
    else{
        tuple->node.start = save;
        tuple->node.end = curt;
    }   
    stack_push((TNode*)tuple);

    // check for function type definition
    if(lookahead(1)->kind == token::function_arrow) {
        advance_curt(); advance_curt();
        u32 count = 0;
        while(1) {
            factor();
            count++;
            if(curt->kind != token::comma) break;
            advance_curt();
        }

        if(!count) {
            diagnostic::parser::
                missing_function_return_type(curt);
            push_error();
        }

        Expression* e = expression::create();
        e->kind = expression::typeref;
        e->type = type::function::create();
        auto ft = (FunctionType*)e->type;

        if(count > 1) {
            Tuple* t = tuple::create();
            t->kind = tuple::multireturn;
            
            // need to collect types to form a TupleType
            auto types = array::init<Type*>(count);

            forI(count) {
                auto n = (Type*)stack_pop();
                array::push(types, n);
                node::insert_first((TNode*)t, (TNode*)n);
            }

            ft->return_type = type::tuple::create(types);
            ft->returns = (TNode*)t;
            node::insert_last((TNode*)e, (TNode*)t);

            set_start_end_from_children(t);
        } else {
            auto t = (Type*)stack_pop();
            ft->return_type = t;
            ft->returns = (TNode*)t;
            node::insert_first((TNode*)e, ft->returns);
        }

        ft->parameters = stack_pop();
        node::insert_first((TNode*)e, ft->parameters);

        set_start_end_from_children(e);
        
        // this is a function entity declaration
        if(curt->kind == token::open_brace) {
            advance_curt();
            block(); check_error;
            node::insert_last((TNode*)e, stack_pop());
            e->node.end = e->node.last_child->end;
            Function* f = function::create();
            f->node.start = e->node.start;
            f->node.end = e->node.end;
            f->type = ft;
            stack_push((TNode*)f);
        }

        stack_push((TNode*)e);
    }

    if(found_label) pop_table();
}

/*
    block: '{' { stmt } '}'
*/
void block() {
    BlockExpression* e = block_expression::create();
    e->node.start = curt-1;
    push_table(&e->table);

    u32 count = 0;
    while(1) {
        // this is set to false when a nested block ends, so that we skip checking for a semicolon
        b32 need_semicolon = true; 
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
            case token::directive_print_meta_type: {
                // special directive to emit the meta type of the given identifier
                advance_curt();
                if(curt->kind != token::identifier) {
                    diagnostic::parser::expected_identifier(curt);
                    push_error();
                }

                Label* l = search_for_label(parser->current_table, curt->hash);
                if(!l) {
                    diagnostic::parser::unknown_identifier(curt);
                    push_error();
                }

                switch(l->entity->node.kind) {
                    case node::place: {
                        messenger::dispatch(message::attach_sender(curt, 
                            message::make_debug(message::verbosity::debug, message::plain("place"))));
                    } break;
                }
                advance_curt();
                if(curt->kind != token::semicolon) {
                    diagnostic::parser::missing_semicolon(curt);
                    push_error();
                }
                advance_curt();
                continue; // this isn't an actual statement, so prevent making one
            } break;
            case token::directive_print_type: {
                // special directive to emit the type of the given identifier
                advance_curt();
                if(curt->kind != token::identifier) {
                    diagnostic::parser::expected_identifier(curt);
                    push_error();
                }

                Label* l = search_for_label(parser->current_table, curt->hash);
                if(!l) {
                    diagnostic::parser::unknown_identifier(curt);
                    push_error();
                }


                String out;
                switch(l->entity->node.kind) {
                    case node::place: {
                        out = type::name(((Place*)l->entity)->type);                        
                    } break;
                    case node::function: {
                        out = type::name(((Function*)l->entity)->type);
                    } break;
                }
                messenger::dispatch(message::attach_sender(curt,
                            message::make_debug(message::verbosity::debug, out)));
                advance_curt();
                if(curt->kind != token::semicolon) {
                    diagnostic::parser::missing_semicolon(curt);
                    push_error();
                }
                advance_curt();
                continue; // this isn't an actual statement, so prevent making one
            } break;
            default: {
                before_expr(); check_error;
                skind = statement::expression;
                if(curt->kind == token::close_brace) {
                    // this must be the end of a block expression, so we dont need a semicolon
                    need_semicolon = false;
                }
            } break;
        }
        if(need_semicolon && curt->kind != token::semicolon) {
            if(curt->kind != token::close_brace) {
                diagnostic::parser::
                    missing_semicolon(curt);
                return;
            }
            // if there's a close brace, this must be the last expression of a block
            // so we dont't do anything, allowing the expression to be attached as the last
            // node of the block below
            count++;
            break;
        } else { // this is just a normal statement
            Statement* s = statement::create();
            s->kind = skind;

            node::insert_first((TNode*)s, stack_pop());
            s->node.start = s->node.first_child->start;
            s->node.end = curt;

            stack_push((TNode*)s);

            advance_curt();
            count++;
        }
    }

    

    forI(count) {
        node::insert_first((TNode*)e, stack_pop());
    }
    e->node.end = curt;
    stack_push((TNode*)e);
    pop_table();
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
        factor: typeref * [ block | '*' ]
*/ 
void after_typeref() { announce_stage;
    Token* save = curt;
    switch(curt->kind) {
        case token::colon: advance_curt(); comptime_after_colon(); break;
        case token::equal: {
            advance_curt(); // assignment: typeref '=' * expr
            before_expr(); check_error;
            // now we reduce to binary assignment
            Expression* e = expression::create();
            e->kind = expression::binary_assignment;
            e->node.start = save;
            e->node.end = curt;
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());
            // always take the type of the left side because it controls the type of the expression
            e->type = ((Expression*)e->node.first_child)->type;

            set_start_end_from_children(e);

            // this is a place in memory, so we need to create a Place entity for it 
            Place* p = place::create();
            p->node.start = e->node.start;
            p->node.end = e->node.end;
            p->type = e->type;

            stack_push((TNode*)p);
            stack_push((TNode*)e);

        } break;
        case token::open_brace: {
            advance_curt();
            block(); check_error;
            node::insert_last(array::read(stack, -2), stack_pop());
            TNode* typeref = array::read(stack, -1);
            typeref->end = typeref->last_child->end;
        } break;
        case token::asterisk: {
            Expression* last = (Expression*)array::read(stack, -1);
            last->type = type::pointer::create(last->type);
            advance_curt();
            after_typeref(); check_error;
        } break;
    }
}

/*
    loop: "loop" expr
*/
void loop() { announce_stage;
    check_error;
    if(curt->kind == token::loop) {
        advance_curt();
        before_expr();

        Expression* loop = expression::create();
        loop->kind = expression::loop;
        node::insert_last((TNode*)loop, stack_pop());
        set_start_end_from_children(loop);
        stack_push((TNode*)loop);
    }
}

/*
    for: "for" ( "(" label ";" expr ";"" expr ")" | "(" id { "," id } "in" expr ")" ) expr
*/
void for_() {
    Token* save = curt;

    advance_curt();
    if(curt->kind != token::open_paren) {
        diagnostic::parser::
            for_missing_open_paren(curt);
        push_error();
    }

    advance_curt();
    if(curt->kind != token::identifier) {
        diagnostic::parser::for_expected_some_identfier(curt);
        push_error();
    }

    b32 c_style = false;
    
    switch((curt+1)->kind) {
        case token::in:
        case token::comma: { // this is a list of identifiers that must be followed by 'in'
            Expression* expr = expression::create();
            expr->kind = expression::identifier;
            expr->node.start = curt;
            expr->node.end = curt;
            stack_push((TNode*)expr);
            advance_curt();

            if(curt->kind == token::comma){
                advance_curt();
                identifier_group(); check_error;
                // TODO(sushi) maybe look into properly supporting this later
                if(curt->kind == token::colon) { 
                    diagnostic::parser::for_label_group_not_allowed(curt);
                    push_error();
                }
            }

            if(curt->kind != token::in) {
                diagnostic::parser::for_expected_in(curt);
                push_error();
            }

            advance_curt();
            before_expr(); check_error;
        } break;
        case token::colon: { 
            c_style = true;
            label();
            if(curt->kind != token::semicolon) {
                diagnostic::parser::missing_semicolon(curt);
                push_error();
            }

            advance_curt();
            before_expr(); check_error;

            if(curt->kind != token::semicolon) {
                diagnostic::parser::missing_semicolon(curt);
                push_error();
            }

            advance_curt();
            before_expr(); check_error;

        } break;
        
    }   

    if(curt->kind != token::close_paren) {
        diagnostic::parser::for_missing_close_paren(curt);
        push_error();
    }
    
    advance_curt();

    // we should be before some expression now 
    before_expr(); check_error;

    Expression* e = expression::create();
    e->kind = expression::for_;

    node::insert_first((TNode*)e, stack_pop());
    node::insert_first((TNode*)e, stack_pop());
    if(c_style) {
        node::insert_first((TNode*)e, stack_pop());
    }
    node::insert_first((TNode*)e, stack_pop());

    e->node.start = save;
    e->node.end = e->node.last_child->end;

    stack_push((TNode*)e);
}

/*
    switch: conditional | "switch" '(' expr ')' '{' { expr "=>" expr } '}'
*/ 
void switch_() { announce_stage;
    advance_curt();
    if(curt->kind != token::open_paren) {
        diagnostic::parser::
            switch_missing_open_paren(curt);
        return;
    }

    advance_curt(); // "switch" "(" * expr ")" ...

    before_expr(); // dont care what this returns cause any expression is good (for now)

    if(curt->kind != token::close_paren) {
        diagnostic::parser::
            switch_missing_close_paren(curt);
        push_error();
    }

    advance_curt(); // "switch" "(" expr ")" * "{" ...

    if(curt->kind != token::open_brace) {
        diagnostic::parser::
            switch_missing_open_brace(curt);
        push_error();
    }

    advance_curt(); // "switch" "(" expr ")" "{" * { expr } "}"

    u32 count = 0;
    while(1) {
        if(curt->kind == token::close_brace) break;
        before_expr(); 
        
        if(curt->kind != token::match_arrow) {
            diagnostic::parser::
                switch_missing_match_arrow_after_expr(curt);
            push_error();
        }

        advance_curt();
        before_expr();

        if(curt->kind != token::comma && curt->kind != token::close_brace ) {
            diagnostic::parser::
                switch_missing_comma_after_match_arm(curt);
            push_error();
        }
        advance_curt();
        count++;

        Expression* e = expression::create();
        e->kind = expression::switch_case;
        
        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        set_start_end_from_children(e);

        stack_push((TNode*)e);

    }

    if(!count) {
        diagnostic::parser::
            switch_empty_body(curt);
    }

    Expression* e = expression::create();
    e->kind = expression::switch_expr;

    // reduce to switch expression
    forI(count+1){
        node::insert_first((TNode*)e, stack_pop());
    }

    set_start_end_from_children(e);
    stack_push((TNode*)e);
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
                if_missing_open_paren(curt);
            push_error();
        }

        advance_curt();

        before_expr();

        if(curt->kind != token::close_paren) {
            diagnostic::parser::
                if_missing_close_paren(curt);
            push_error();
        }

        advance_curt();

        before_expr();

        Expression* e = expression::create();
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
        Expression* e = expression::create();
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
    if(curt->kind == token::double_ampersand) {
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
        Expression* e = expression::create();
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
    if(curt->kind == token::vertical_line) {
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
        Expression* e = expression::create();
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
    if(curt->kind == token::caret) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        bit_and(); check_error;
        Expression* e = expression::create();
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
    if(curt->kind == token::ampersand) {
        advance_curt();
        factor(); check_error;
        access(); check_error;
        term(); check_error;
        additive(); check_error;
        bit_shift(); check_error;
        relational(); check_error;
        equality(); check_error;
        Expression* e = expression::create();
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
        case token::double_equal:
        case token::explanation_mark_equal: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            bit_shift(); check_error;
            relational(); check_error;
            Expression* e = expression::create();
            e->kind = kind == token::double_equal ? expression::binary_equal : expression::binary_not_equal;

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
        case token::less_than_equal:
        case token::greater_than:
        case token::greater_than_equal: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            bit_shift(); check_error;
            Expression* e = expression::create();
            e->kind = kind == token::less_than ?
                      expression::binary_less_than :
                      kind == token::less_than_equal ? 
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
        case token::double_less_than: 
        case token::double_greater_than: {
            advance_curt(); 
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            additive(); check_error;
            Expression* e = expression::create();
            e->kind = kind == token::double_less_than ? 
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
        case token::minus: {
            advance_curt();
            factor(); check_error;
            access(); check_error;
            term(); check_error;
            Expression* e = expression::create();
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
        case token::percent: 
        case token::solidus: 
        case token::asterisk: {
            advance_curt();
            factor(); check_error; 
            access(); check_error;
            Expression* e = expression::create();
            e->kind = 
                    kind == token::percent ? expression::binary_modulo 
                    : kind == token::solidus ? expression::binary_division
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
        Expression* e = expression::create();
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
        case token::identifier: {
            reduce_identifier_to_identifier_expression();
            Label* label = search_for_label(parser->current_table, curt->hash); 
            if(!label) {
                diagnostic::parser::unknown_identifier(curt);
                push_error();
            }
            
            // in order to determine how we will treat this identfier syntactically, we figure out what sort of 
            // entity the label points to
            switch(label->entity->node.kind) {
                case node::function: {
                    if(lookahead(1)->kind == token::open_paren) {
                        // must be a function call
                        CallExpression* e = call_expression::create();
                        e->kind = expression::call;
                        e->node.start = curt;
                        e->callee = (Function*)label->entity;
                        advance_curt(); advance_curt();
                        tuple_after_open_paren(); check_error;          
                        e->node.end = curt;
                        node::insert_last((TNode*)e, stack_pop()); // append argument tuple
                        node::insert_last((TNode*)e, stack_pop()); // append identfier found above
                        stack_push((TNode*)e);

                    } else {
                        NotImplemented;
                        // // probably just a reference to a function entity
                        // Expression* last = (Expression*)array::read(stack, -1);
                        // last->kind = expression::entity_func;
                        // last->entity = label->entity;
                        // advance_curt();
                    }
                } break;
                case node::structure: {
                    Expression* last = (Expression*)array::read(stack, -1);
                    last->kind = expression::typeref;
                    last->type = (Type*)label->entity;
                    advance_curt();
                    after_typeref(); check_error;
                } break;
                case node::place: {
                    Expression* last = (Expression*)array::read(stack, -1);
                    last->kind = expression::identifier;
                    last->type = ((Place*)label->entity)->type;
                } break;
            }
            advance_curt(); 
        } break;
        case token::open_paren: advance_curt(); tuple_after_open_paren(); break;
        case token::if_:        conditional(); break;
        case token::switch_:    switch_(); break;
        case token::for_:       for_(); break;
        case token::open_brace: advance_curt(); block(); break;
        case token::ampersand: {
            Expression* ref = expression::create();
            ref->kind = expression::unary_reference;
            ref->node.start = curt;

            advance_curt();
            factor(); check_error;
            
            auto last = (Expression*)stack_pop();
            ref->type = type::pointer::create(last->type);
            ref->node.end = last->node.end;
            node::insert_last((TNode*)ref, (TNode*)last);
            stack_push((TNode*)ref);
        } break;
        default: {
            if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                advance_curt();
            } else if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                advance_curt();
            } else {
                diagnostic::parser::
                    unexpected_token(curt, curt);
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
            missing_open_brace_for_struct(curt);
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
                    missing_semicolon(curt);
            }
            advance_curt();
            count++;
            TNode* last = array::read(stack, -1);
            if(last->last_child->kind == node::function) {
                diagnostic::parser::
                    struct_member_functions_not_allowed(last->start);
                push_error();
            }
        } else {
            diagnostic::parser::
                struct_only_labels_allowed(curt);
            push_error();
        }
    }

    Structure* s = structure::create();
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
        case token::equal: {
            Token* save = curt;
            advance_curt();
            before_expr(); check_error;

            Expression* e = expression::create();
            e->kind = expression::unary_assignment;

            Expression* last = (Expression*)stack_pop();

            node::insert_first((TNode*)e, (TNode*)last);
            e->node.start = save;
            e->node.end = e->node.last_child->end;
            e->type = last->type;

            Place* p = place::create();
            p->type = last->type;

            stack_push((TNode*)p);
            stack_push((TNode*)e);
        } break;
        case token::colon: {
            Token* save = curt;
            advance_curt();
            before_expr(); check_error;

            Expression* e = expression::create();
            e->kind = expression::unary_comptime;

            node::insert_first((TNode*)e, stack_pop());
            e->node.start = save;
            e->node.end = e->node.last_child->end;
            stack_push((TNode*)e);
        } break;
        case token::structdecl: {
            struct_decl(); check_error;
        } break;
        case token::moduledecl: {
            push_module(curt->module);
            advance_curt();
            advance_curt();
            start();
            pop_module();
        } break;
        case token::return_: {
            Token* save = curt;
            advance_curt();
            before_expr();

            Expression* e = expression::create();
            e->kind = expression::return_;

            node::insert_first((TNode*)e, stack_pop());
            e->node.start = save;
            e->node.end = e->node.last_child->end;

            stack_push((TNode*)e);
        }break;
        case token::using_: {
            // this should just be an alias
            Expression* e = expression::create();
            e->kind = expression::using_;
            e->node.start = curt;

            // TODO(sushi) this needs to be changed to take in an expression that may result in an identifier
            advance_curt();
            if(curt->kind != token::identifier) {
                diagnostic::parser::expected_identifier(curt);
                push_error();
            }

            e->node.end = curt;

            Expression* i = expression::create();
            i->kind = expression::identifier;
            i->node.start = i->node.end = curt;

            node::insert_last((TNode*)e, (TNode*)i);

            Label* label = search_for_label(parser->current_table, curt->hash);
            if(!label) {
                diagnostic::parser::unknown_identifier(curt);
                push_error();
            }

            stack_push((TNode*)label->entity);
            stack_push((TNode*)e);

            advance_curt();
        } break;
        default: factor(); check_error;
    }

    // loop and see if any operators are being used, if so call their entry point
    b32 search = true;
    while(search) {
        switch(curt->kind) {
            case token::dot: access(); check_error; break;
            case token::asterisk:
            case token::solidus: term(); check_error; break;
            case token::plus:
            case token::minus: additive(); check_error; break;
            case token::double_less_than:
            case token::double_greater_than: bit_shift(); check_error; break;
            case token::double_equal: 
            case token::explanation_mark_equal: equality(); check_error; break;
            case token::less_than:
            case token::less_than_equal:
            case token::greater_than:
            case token::greater_than_equal: relational(); check_error; break;
            case token::ampersand: bit_and(); check_error; break;
            case token::caret: bit_xor(); check_error; break;
            case token::vertical_line: bit_or(); check_error; break;
            case token::double_ampersand: logi_and(); check_error; break;
            case token::logi_or: logi_or(); check_error; break;
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
    switch(curt->kind) {
        case token::identifier: {
            reduce_identifier_to_identifier_expression(); 
            Label* label = search_for_label(parser->current_table, curt->hash);
            if(!label) {
                diagnostic::parser::unknown_identifier(curt);
                push_error();
            }

            switch(label->entity->node.kind) {
                case node::type: {
                    Expression* last = (Expression*)array::read(stack, -1);
                    last->kind = expression::typeref;
                    last->type = (Type*)label->entity;
                    advance_curt();
                    after_typeref(); check_error;

                    // auto last = (Expression*)array::read(stack, -1);

                    // // // in this case, 'after_typeref' will have already made a place in memory for this 
                    // if(last->kind = expression::binary_assignment) break;

                    // // this is a place in memory
                    // // Place* p = place::create();
                    // // p->type = last->type;
                    // // p->label = label;
                    // // TNode* save = stack_pop();
                    // // stack_push((TNode*)p);
                    // // stack_push(save);
                } break;
                default: advance_curt(); break; 
            }
            
        } break;
        case token::open_paren: advance_curt(); tuple_after_open_paren(); check_error; break;
        case token::equal:    
        case token::colon: before_expr(); check_error; break;
        
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
}
/*
         label: labelgroup ':' expr ( ';' | ']' | '}' )
    labelgroup: ID ( "," * ID )+

    here we are gathering a 'labelgroup', which is internally represented as a Tuple
    with the identifiers as its children

*/
void label_group_after_comma() { announce_stage;
    identifier_group(); check_error;

    // if we have come to a place where a comma was not followed by an identifier
    // we throw an error about it
    if((curt-1)->kind == token::comma) {
        diagnostic::parser::
            label_group_missing_id(curt);
    }

    if(curt->kind == token::colon) {
        label_after_colon(); check_error;
    } else {
        diagnostic::parser::
            label_missing_colon(curt);
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
    Expression* expr = expression::create();
    expr->kind = expression::identifier;
    expr->node.start = curt;
    expr->node.end = curt;
    stack_push((TNode*)expr);

    Token* save = curt;

    advance_curt();
    label_after_id(); check_error;

    Label* label;
    auto [idx, found] = map::find(parser->current_table->map, save->raw);
    if(found) {
        label = array::read(parser->current_table->map.values, idx);
    } else {
        label = label::create();
        map::add(parser->current_table->map, (String)save->raw, label);
    }

    node::insert_first((TNode*)label, stack_pop());

    TNode* cand = stack_pop();
    switch(cand->kind) {
        case node::place:
        case node::structure:
        case node::function:
        case node::module: {
            label->entity = (Entity*)cand;
            Entity* e = (Entity*)cand;
            label->aliased = e->label;
            e->label = label;
            cand->start = label->node.first_child->start;
            cand->end = label->node.last_child->end; 
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
                missing_semicolon(curt);
            return;
        }

        count++;
        advance_curt();
    }

    Module* m = parser->current_module;
    forI(count) {
        node::insert_first((TNode*)m, stack_pop());
    }

    set_start_end_from_children(m);
    stack_push((TNode*)m);
}

/*
    perform an initial scan over the labels marked by the lexer 
    we start parsing at each label and continue up until we are able 
    to resolve what it is and add a Symbol entry for it 

    this doesn't build any part of the AST, it just precreates labels so that we may 
    refer back to them during actual parsing to get more of an idea what identifiers are
    when we find them
*/
void prescan() {
    forI(parser->current_module->labels.count) {
        curt = array::readptr(code::get_token_array(parser->code), array::read(parser->current_module->labels, i));
        Label* l = label::create();
        l->node.start = curt;
        map::add(parser->current_module->table.map, (String)curt->raw, l);

        advance_curt();
        advance_curt();
        switch(curt->kind) {
            case token::open_paren: {
                // this can either be a variable of some tuple type, a function type, or a function definition
                // so we skip until we find the end of the tuple
                u32 nesting = 0;
                while(1) {
                    advance_curt();
                    if(curt->kind == token::open_paren) nesting++;
                    else if(curt->kind == token::close_paren) {
                        if(!nesting) break;
                        nesting--;
                    }
                }
                advance_curt();

                switch(curt->kind) {
                    case token::function_arrow: {
                        // we now have to skip until either a semicolon or opening brace is found 
                        // if a brace is found, this must be a function definition and so the label
                        // refers to a function entity, otherwise it is a label of function pointer type
                        while(1) {
                            advance_curt();
                            if(curt->kind == token::open_brace) {
                                // we assume this is a function definion, so we'll create a function entity to be 
                                // filled out later during actual parsing 
                                Function* f = function::create();
                                l->entity = f;
                                goto label_finished;
                            } else if(curt->kind == token::semicolon || curt->kind == token::equal) {
                                // this is probably a variable pointing to a function
                                // if '=' is found, then it's the same, only it is being initialized
                                Place* p = place::create();
                                l->entity = p;
                                goto label_finished;
                            }
                        }
                    } break;
                    case token::equal:
                    case token::semicolon: {
                        // this must be a label representing a variable of some tuple type
                        Place* p = place::create();
                        l->entity = p;
                        goto label_finished;
                    } break;
                    
                }

            } break;
            case token::equal: {
                // this is some implicitly typed runtime label
            } break;    
            case token::colon: {
                // this is some implicitly typed compile time label
                // we may run into 'struct', 'module', 'variant', or just an expression 
                advance_curt();
                switch(curt->kind) {
                    case token::structdecl: {
                        Structure* s = structure::create();
                        s->label = l;
                        StructuredType* t = type::structured::create(s);
                        l->entity = (Entity*)t;
                        t->label = l;
                        goto label_finished;
                    } break;
                    case token::moduledecl: {
                        l->entity = curt->module;
                        // we switch over to the new module and recurse
                        Token* save = curt;
                        push_module(curt->module);
                        prescan();
                        pop_module();
                        curt = save;
                        goto label_finished;
                    } break;
                    case token::open_paren: {
                        // this has to be a function definition
                        // for now, at least
                        // TODO(sushi) determine if the grammar can support Type objects being assigned like this
                        Function* f = function::create();
                        l->entity = f;
                        goto label_finished;
                    } break;
                    case token::using_: {
                        // this must be an alias, so we don't do anything because we can't reliably
                        // determine what entity to set the Label to yet
                        // this is handled later in label_after_colon
                    } break;
                }
            } break;
            default: {
                // in any other case, this is probably just a compile time variable declaration
                Place* p = place::create();
                l->entity = p;
                goto label_finished;
            } break;
        }
label_finished:;
    }
}


#undef announce_stage
} // namespace internal

void
execute(Code* code) {
    util::Stopwatch parser_time = util::stopwatch::start();

    messenger::dispatch(message::attach_sender(code,
        message::make_debug(message::verbosity::stages,
            String("beginning syntactic analysis"))));

    internal::parser = code->parser;
    internal::push_module(code->source->module);
    internal::stack = array::init<TNode*>(32);
    internal::prescan();
    internal::curt = array::readptr(code::get_token_array(code), 0);
    internal::start();

    util::println(
        node::util::print_tree<[](DString& c, TNode* n){to_string(c, n, true);}>(internal::stack.data[0]));
    
    // !Leak: a DString is generated by util::format_time, but messenger currently has no way to know 
	//        if a String needs to be cleaned up or not 
    messenger::dispatch(message::attach_sender(code,
        message::make_debug(message::verbosity::stages, 
            String("syntactic analysis finished in "), String(util::format_time(util::stopwatch::peek(parser_time))))));
}

} // namespace parser
} // namespace amu