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
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug,
            String("pushed: "), caller, String(" // "), 
                (n->start? n->start->raw : String("bad start")), String(" -> "), 
                (n->end? n->end->raw : String("bad end")))));
}

#define stack_push(n) __stack_push(n, __func__)

TNode*
__stack_pop(String caller) {
    TNode* ret = array::pop(stack);
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug,
            String("popped: "), caller, String(" // "), 
                (ret->start? ret->start->raw : String("bad start")), String(" -> "), 
                (ret->end? ret->end->raw : String("bad end")))));
    return ret;
}

#define stack_pop() __stack_pop(__func__)

FORCE_INLINE void
debug_announce_stage(String stage) {
    if(compiler::instance.options.verbosity < message::verbosity::debug) return;
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug, 
            String("parse level: "), stage)));
}

#define announce_stage debug_announce_stage(__func__)

template<typename... T> b32
next_match(T... args) {
    return (((curt+1)->kind == args) || ...);
}

expression::kind 
binop_token_to_expression(token::kind in){
	
	return expression::null;
}

template<TNode* (*next_layer)(TNode*), typename... T>
TNode* binop_parse(TNode* parent, TNode* ret, T... tokchecks){
    TNode* out = ret;
    while(next_match(tokchecks...)){
        advance_curt();
        //make binary op expression
        Expression* op = compiler::create_expression();
        switch(curt->kind){
            case token::multiplication:        op->kind = expression::binary_multiply; break;
            case token::division:              op->kind = expression::binary_division; break;
            case token::negation:              op->kind = expression::unary_negate; break;
            case token::plus:                  op->kind = expression::binary_plus; break;
            case token::logi_and:              op->kind = expression::binary_and; break;
            case token::logi_or:               op->kind = expression::binary_or; break;
            case token::less_than:             op->kind = expression::binary_less_than; break;
            case token::greater_than:          op->kind = expression::binary_greater_than; break;
            case token::less_than_or_equal:    op->kind = expression::binary_less_than_or_equal; break;
            case token::greater_than_or_equal: op->kind = expression::binary_greater_than_or_equal; break;
            case token::equal:                 op->kind = expression::binary_equal; break;
            case token::not_equal:             op->kind = expression::binary_not_equal; break;
            case token::bit_and:               op->kind = expression::binary_bit_and; break;
            case token::bit_or:                op->kind = expression::binary_bit_or; break;
            case token::bit_xor:               op->kind = expression::binary_bit_xor; break;
            case token::bit_shift_left:        op->kind = expression::binary_bit_shift_left; break;
            case token::bit_shift_right:       op->kind = expression::binary_bit_shift_right; break;
            case token::modulo:                op->kind = expression::binary_modulo; break;
            case token::bit_not:               op->kind = expression::unary_bit_comp; break;
            case token::logical_not:           op->kind = expression::unary_logi_not; break;
            default: Assert(0);
        }

        //readjust parents to make the binary op the new child of the parent node
        //and the ret node a child of the new binary op
        node::change_parent(parent, (TNode*)op);
        node::change_parent((TNode*)op, out);

        //evaluate next expression
        advance_curt();
        TNode* rhs = next_layer((TNode*)op);

        out = (TNode*)op;
    }
    return out;
}

#define check_error if(!array::read(stack, -1)) return;
#define push_error() do{                                                \
    messenger::dispatch(message::attach_sender({parser->source, *curt}, \
        message::debug(message::verbosity::debug,                       \
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


// state28:
// additive: additive '+' * term@
// term: * factor
//     | * term '*' factor
//     | * term '/' factor
// factor: * NUM
//       | * type
// tuple: * '(' tupleargs ')'
// func_type: * tuple "->" expr
// type: * tuple
//     | * func_type
//     | * ID
//     | * type decorators 
// void after_plus() { announce_stage;
//     switch(curt->kind) {
//         case token::literal_integer: advance_curt(); reduce_num_to_factor(); break;
//         case token::identifier:      advance_curt(); reduce_identifier_to_type(); break;
//     }
// }


// state15:
// expr: additive *
// additive: additive * '+' term
//         | additive * '-' term
// reduce additive -> expr
// void after_additive() { announce_stage;
//     switch(curt->type) {
//         case token::plus: 
//     }
// }


//state14:
// expr: ctime *
void reduce_comptime_to_expr() { announce_stage;

}

// state13:
// expr: assignment *
void reduce_assignment_to_expr() { announce_stage;

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
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::missing_function_return()));
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
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::tuple_expected_comma_or_close_paren()));
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
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::missing_semicolon()));
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
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_open_paren()));
            return;
        }

        advance_curt(); // "switch" "(" * expr ")" ...

        before_expr(); // dont care what this returns cause any expression is good (for now)

        if(curt->kind != token::close_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_close_paren()));
            push_error();
        }

        advance_curt(); // "switch" "(" expr ")" * "{" ...

        if(curt->kind != token::open_brace) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_open_brace()));
            push_error();
        }

        advance_curt(); // "switch" "(" expr ")" "{" * { expr } "}"

        u32 count = 0;
        while(1) {
            if(curt->kind == token::close_brace) break;
            before_expr(); 
            
            if(curt->kind != token::match_arrow) {
                messenger::dispatch(message::attach_sender({parser->source, *curt},
                    diagnostic::parser::switch_missing_match_arrow_after_expr()));
                push_error();
            }

            advance_curt();
            before_expr();

            if(curt->kind != token::comma && curt->kind != token::close_brace ) {
                messenger::dispatch(message::attach_sender({parser->source, *curt},
                    diagnostic::parser::switch_missing_comma_after_match_arm()));
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
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_empty_body()));
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

    loop();
}

/*
    conditional: logi-or | "if" '(' expr ')' expr [ "else" expr ]
*/
void conditional() { announce_stage;
    check_error;
    if(curt->kind == token::if_) {
        advance_curt();

        if(curt->kind != token::open_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::if_missing_open_paren()));
            push_error();
        }

        advance_curt();

        before_expr();

        if(curt->kind != token::close_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::if_missing_close_paren()));
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
        case token::directive_compiler_break: DebugBreakpoint; advance_curt();break;
        default: {
            if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                advance_curt();
            } else if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                advance_curt();
            } else {
                messenger::dispatch(message::attach_sender({parser->source, *curt},
                    diagnostic::parser::unexpected_token()));
                push_error();
            }
        } break;
    }
}

void struct_decl(ParserThread* thread) {
    Token* save = thread->curt;
    advance_curt(); // TODO(sushi) struct parameters
    if(thread->curt->kind != token::open_brace) {
        messenger::dispatch(message::attach_sender({thread->parser->source, *thread->curt},
            diagnostic::parser::missing_open_brace_for_struct()));
        push_error();
    }

    advance_curt();
    
    u32 count = 0;
    while(1) {
        if(thread->curt->kind == token::close_brace) break;
        if(thread->curt->kind == token::identifier) {
            label(thread); check_error;
            if(thread->curt->kind != token::semicolon) {
                messenger::dispatch(message::attach_sender({thread->parser->source, *thread->curt},
                    diagnostic::parser::missing_semicolon()));
            }
            advance_curt();
            count++;
            TNode* last = array::read(thread->stack, -1);
            if(last->last_child->kind == node::function) {
                messenger::dispatch(message::attach_sender({thread->parser->source, *last->start},
                    diagnostic::parser::struct_member_functions_not_allowed()));
                push_error();
            }
        } else {
            messenger::dispatch(message::attach_sender({thread->parser->source, *thread->curt},
                diagnostic::parser::struct_only_labels_allowed()));
            push_error();
        }
    }

    Structure* s = parser::create_structure(thread);
    s->node.start = save;
    s->node.end = thread->curt;

    forI(count) {
        node::insert_first((TNode*)s, stack_pop());
    }

    stack_push((TNode*)s);
}

        case token::assignment: {
            advance_curt();
            before_expr();
        } break;
    }
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
            Token* save = thread->curt;
            advance_curt();
            before_expr(thread); check_error;

            Expression* e = parser::create_expression(thread);
            e->kind = expression::unary_comptime;

            // if the candidate for this unary_comptime is an entity, we need to prioritize its node over 
            // the one we're about to place
            TNode* cand = stack_pop();
            switch(cand->kind) {
                case node::function:
                case node::structure:
                case node::module:
                case node::place: {
                    e->node.start = save;
                    e->node.end = cand->last_child->end;
                    for(TNode* n = cand->first_child;n;n=cand->first_child) {
                        node::change_parent((TNode*)e, n);
                    }
                    node::insert_last(cand, (TNode*)e);
                    stack_push(cand);
                } break;
                default: {
                    node::insert_first((TNode*)e, cand);
                    e->node.start = save;
                    e->node.end = e->node.last_child->end;
                    stack_push((TNode*)e);
                } break;
            }
        } break;

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

    after_expr();

    // switch(curt->kind) {
    //     case token::identifier: reduce_identifier_to_identifier_expression(); advance_curt(); break;
    //     case token::open_paren: advance_curt(); tuple_after_open_paren(); break;
    //     default: {
    //         if(curt->group == token::group_literal) {
    //             reduce_literal_to_literal_expression();
    //             advance_curt();
    //         }  
    //     }

    // }

    // TNode* last = array::read(stack, -1);

    // switch(last->kind) {
    //     case node::expression: {
    //         Expression* expr = (Expression*)last;
    //         switch(expr->kind) {
    //             case expression::literal: {
    //                 access();
    //             } break;

    //             case expression::identifier: {

    //             } break;
    //         }
    //     }
    // }
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
        case token::colon:           advance_curt(); comptime_after_colon(); break;
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
        messenger::dispatch(message::attach_sender({parser->source, *curt},
            diagnostic::parser::label_group_missing_id()));
    }

    if(curt->kind == token::colon) {
        label_after_colon();
    } else {
        messenger::dispatch(message::attach_sender({parser->source, *curt},
            diagnostic::parser::label_missing_colon()));
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
    label_after_id();
    check_error;

    // reduce to a label 
    Label* label = compiler::create_label();
    node::insert_first((TNode*)label, stack_pop());
    node::insert_first((TNode*)label, stack_pop());

    set_start_end_from_children(label);
    stack_push((TNode*)label);

}

/* 
        module: * { label } EOF 
         label: * ( ID | labelgroup ) ':' expr ( ';' | ']' | '}' )  
    labelgroup: ID ( "," ID )+ 
*/
void start() { announce_stage;
    while(1) {
        if(curt->kind != token::identifier) break;
        label(); check_error;

        if(curt->kind != token::close_brace && curt->kind != token::semicolon) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::missing_semicolon()));
            return;
        }

        advance_curt();
    }

    TNode* last = array::read(stack, -1);

    switch(last->kind) {
        case node::module: before_end(); break;
        case node::label:  before_eof(); break;
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
    internal::parser = &parser;
    internal::curt = array::readptr(lexer.tokens, 0);
    internal::stack = array::init<TNode*>(32);
    internal::start();
    //internal::file(&parser.source->module->node);
}

} // namespace parser
} // namespace amu