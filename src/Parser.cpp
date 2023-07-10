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

FORCE_INLINE void
__stack_push(TNode* n, String caller) {
    array::push(stack, n);
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug,
            String("pushed: "), caller)));
}

#define stack_push(n) __stack_push(n, __func__)

FORCE_INLINE TNode*
__stack_pop(String caller) {
    messenger::dispatch(message::attach_sender({parser->source, *curt},
        message::debug(message::verbosity::debug,
            String("popped: "), caller)));
    return array::pop(stack);
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
        curt++;
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
        op->start = curt-1;

        //readjust parents to make the binary op the new child of the parent node
        //and the ret node a child of the new binary op
        node::change_parent(parent, (TNode*)op);
        node::change_parent((TNode*)op, out);

        //evaluate next expression
        curt++;
        TNode* rhs = next_layer((TNode*)op);

        out = (TNode*)op;
    }
    return out;
}

void before_expr();
void label_after_colon();
void before_factor();

// state8:
// factor: ID *
// reduce ID -> expr:id
void reduce_identifier_to_identifier_expression() { announce_stage;
    Expression* e = compiler::create_expression();
    e->kind = expression::identifier;
    e->start = e->end = curt;
    stack_push((TNode*)e);
}

/* there doesn't need to do anything special for reducing a literal, because they all become the same sort of expression
   which just stores the token
    literal: int | float | string | char
*/
void reduce_literal_to_literal_expression() { announce_stage;
    Expression* e = compiler::create_expression();
    e->kind = expression::literal;
    e->start = e->end = curt;
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
    stack_push((TNode*)e);
}   


// state28:
// additive: additive '+' * term
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
//         case token::literal_integer: curt++; reduce_num_to_factor(); break;
//         case token::identifier:      curt++; reduce_identifier_to_type(); break;
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
        curt++;
        u32 count = 0;
        while(1) {
            before_factor();
            count++;
            if(curt->kind != token::comma) break;
            curt++;
        }

        Expression* e = compiler::create_expression();
        e->kind = expression::typeref;
        e->type.structure = compiler::builtins.functype;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);
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
                    case token::colon: curt+=2; label_after_colon(); break;
                    default: {
                        // otherwise this is assumed to be an expression handled by before_expr
                        before_expr();
                    } break;
                }

            } break;
            default: {
                before_expr();
            }break;
        }
        count += 1;
        if(curt->kind == token::comma) curt++;
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

    stack_push((TNode*)tuple);

    curt++;

    tuple_after_close_paren();
}

// state10:
// assignment: '=' * expr
void assignment_after_equal() { announce_stage;
    // just pass to before_expr 
}

/*
    comptime: * typeref ':' expr
            | typeref ':' * expr
            | * ':' expr
*/ 
void comptime_after_colon() { announce_stage;
    switch(curt->kind) {
        case token::identifier:      reduce_identifier_to_identifier_expression(); curt++; break;
        case token::colon:           curt++; comptime_after_colon(); break;
        case token::assignment:      curt++; assignment_after_equal(); break;
        case token::open_paren:      curt++; tuple_after_open_paren(); break;
        default: {
            if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                curt++;
            }else if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                curt++;
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
        factor: typeref *  
34*/ 
void after_typeref() { announce_stage;
    Token* save = curt;
    switch(curt->kind) {
        case token::colon: curt++; comptime_after_colon(); break;
        case token::assignment: {
            curt++; // assignment: typeref '=' * expr
            before_expr();
            // now we reduce to binary assignment
            Expression* e = compiler::create_expression();
            e->kind = expression::binary_assignment;
            e->start = save;
            e->end = curt;
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            stack_push((TNode*)e);
        } break;
    }
}

/*
    loop: switch | "loop" expr
*/
void loop() { announce_stage;
    if(curt->kind == token::loop) {
        curt++;
        before_expr();

        Expression* loop = compiler::create_expression();
        loop->kind = expression::loop;
        node::insert_last((TNode*)loop, stack_pop());
        stack_push((TNode*)loop);
        loop->start = ((Expression*)loop->node.first_child)->start;
        loop->end = ((Expression*)loop->node.last_child)->end;
    }
}

/*
    switch: conditional | "switch" '(' expr ')' '{' { expr "=>" expr } '}'
*/ 
void switch_() { announce_stage;
    if(curt->kind == token::switch_) {
        Token* save = curt;
        curt++;
        if(curt->kind != token::open_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_open_paren()));
            return;
        }

        curt++; // "switch" "(" * expr ")" ...

        before_expr(); // dont care what this returns cause any expression is good (for now)

        curt++; // "switch "(" expr * ")" ...

        if(curt->kind != token::close_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_close_paren()));
            return;
        }

        curt++; // "switch" "(" expr ")" * "{" ...

        if(curt->kind != token::open_brace) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_missing_open_brace()));
            return;
        }

        curt++; // "switch" "(" expr ")" "{" * { expr } "}"

        u32 count = 0;
        while(1) {
            if(curt->kind == token::close_brace) break;
            before_expr(); 
            
            curt++;
            if(curt->kind != token::match_arrow) {
                messenger::dispatch(message::attach_sender({parser->source, *curt},
                    diagnostic::parser::switch_missing_match_arrow_after_expr()));
                return;
            }

            curt++;
            before_expr();

            // if the last token is a close brace, we dont care about following up with a comma
            if(curt->kind == token::close_brace) {
                curt++;
                count++;
                continue;
            }

            curt++;
            if(curt->kind != token::comma) {
                messenger::dispatch(message::attach_sender({parser->source, *curt},
                    diagnostic::parser::switch_missing_comma_after_match_arm()));
                return;
            }
            curt++;
            count++;
        }

        if(!count) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::switch_empty_body()));
        }

        Expression* e = compiler::create_expression();
        e->kind = expression::switch_expr;
        e->start = save;
        e->end = curt;

        // reduce to switch expression
        forI(count+1){
            node::insert_first((TNode*)e, array::read(stack, -1));
        }

        array::pop(stack, count+1);
        stack_push((TNode*)e);

        curt++;
    }

    loop();
}

/*
    conditional: logi-or | "if" '(' expr ')' expr [ "else" expr ]
*/
void conditional() { announce_stage;
    if(curt->kind == token::if_) {
        curt++;

        if(curt->kind != token::open_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::if_missing_open_paren()));
            return;
        }

        curt++;

        before_expr();

        if(curt->kind != token::close_paren) {
            messenger::dispatch(message::attach_sender({parser->source, *curt},
                diagnostic::parser::if_missing_close_paren()));
        }

        curt++;

        before_expr();

        Expression* e = compiler::create_expression();
        e->kind = expression::conditional;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        if(curt->kind == token::else_) {
            curt++;
            before_expr();
            node::insert_last((TNode*)e, stack_pop());
        }
    }

    // switch_();
}

// TODO(sushi) this long chain of binary op handlers can probably all be combined into one function

/*
    logi-or: logi-and { "||" logi-and }
*/
void logi_or() { announce_stage;
    if(curt->kind == token::logi_or) {
        curt++;
        before_factor();
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_or;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        logi_or();
    }

    conditional();
}

/*
    logi-and: bit-or { "&&" bit-or }
*/
void logi_and() { announce_stage;
    if(curt->kind == token::logi_and) {
        curt++;
        before_factor();
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_and;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        logi_and();
    }

    logi_or();
}

/*
    bit-or: bit-xor { "|" bit-xor }
*/
void bit_or() { announce_stage;
    if(curt->kind == token::bit_or) {
        curt++;
        before_factor();
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_or;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        bit_or();
    }

    logi_and();
}

/*
    bit-xor: bit-and { "^" bit-and }
*/
void bit_xor() { announce_stage;
    if(curt->kind == token::bit_xor) {
        curt++;
        before_factor();
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_xor;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        bit_xor();
    }

    bit_or();
}


/*
    bit-and: equality { "&" equality } 
*/
void bit_and() { announce_stage;
    if(curt->kind == token::bit_and) {
        curt++;
        before_factor();
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_bit_and;

        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());

        stack_push((TNode*)e);

        bit_and();
    }

    bit_xor();
}

/*
    equality: relational { ( "!=" | "==" ) relational }
*/
void equality() { announce_stage;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::equal:
        case token::not_equal: {
            curt++;
            before_factor();
            Expression* e = compiler::create_expression();
            e->kind = kind == token::equal ? expression::binary_equal : expression::binary_not_equal;

            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            stack_push((TNode*)e);

            equality();
        } break;
    }

    bit_and();
}

/*
    relational: bit-shift { ( ">" | "<" | "<=" | ">=" ) bit-shift }
*/
void relational() { announce_stage;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::less_than:
        case token::less_than_or_equal:
        case token::greater_than:
        case token::greater_than_or_equal: {
            curt++;
            before_factor();
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

            stack_push((TNode*)e);

            relational();
        } break;
    }

    equality();
}

/*
    bit-shift: additive { "<<" | ">>" additive }
*/
void bit_shift() { announce_stage;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::bit_shift_left: 
        case token::bit_shift_right: {
            curt++; 
            before_factor();
            Expression* e = compiler::create_expression();
            e->kind = kind == token::bit_shift_left ? 
                      expression::binary_bit_shift_left :
                      expression::binary_bit_shift_right;
            
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            stack_push((TNode*)e);

            bit_shift();
        } break;
    }

    relational();
}

/*
    additive: term * { ("+" | "-" ) term }
*/
void additive() { announce_stage;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::plus:
        case token::negation: {
            curt++;
            before_factor();
            Expression* e = compiler::create_expression();
            e->kind = kind == token::plus ? expression::binary_plus : expression::binary_minus;
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            stack_push((TNode*)e);

            additive();
        } break;
    }

    bit_shift();
}

/*
    term: access * { ( "*" | "/" | "%" ) access }
*/
void term() { announce_stage;
    token::kind kind = curt->kind;
    switch(kind) {
        case token::modulo: 
        case token::division: 
        case token::multiplication: {
            curt++;
            before_factor();
            Expression* e = compiler::create_expression();
            e->kind = 
                    kind == token::modulo ? expression::binary_modulo 
                    : kind == token::division ? expression::binary_division
                    : expression::binary_multiply;
            
            node::insert_first((TNode*)e, stack_pop());
            node::insert_first((TNode*)e, stack_pop());

            stack_push((TNode*)e);

            term();
        } break;
    }

    additive();
}

/*
    access: factor * { "." factor }
*/
void access() { announce_stage;
    if(curt->kind == token::dot) {
        curt++;
        before_factor();
        // access: factor { "." factor * }
        Expression* e = compiler::create_expression();
        e->kind = expression::binary_access;
        node::insert_first((TNode*)e, stack_pop());
        node::insert_first((TNode*)e, stack_pop());
        e->start = ((Expression*)e->node.first_child)->start;
        e->end = ((Expression*)e->node.last_child)->end;
        stack_push((TNode*)e);
        access();
    }

    term();

    // NOTE(sushi) we can do this this way if we want to reduce the amount of nodes in the tree
    //             for example, apple.banana.orange right now is 
    //             (. (. apple banana) orange)
    //             but we can bring everything to the same parent
    //             (. apple banana orange)

    // if we gathered anything, reduce to an access expression
    // if(count) {
    //     Expression* e = compiler::create_expression();
    //     e->kind = expression::binary_access;

    //     forI(count+1) {
    //         node::insert_first((TNode*)e, array::read(stack, -i));
    //     }

    //     e->start = ((Expression*)e->node.first_child)->start;
    //     e->end = ((Expression*)e->node.last_child)->end;

    //     array::pop(stack, count+1);
    //     stack_push((TNode*)e);
    // } // otherwise do nothing
}

/*
    access: * factor { "." factor }
          | factor { "." * factor }
    factor: * (literal | id | tuple | type )
*/
void before_factor() { announce_stage;
    switch(curt->kind) {
        case token::identifier: reduce_identifier_to_identifier_expression(); curt++; break;
        case token::open_paren: curt++; tuple_after_open_paren(); break;
        case token::if_: conditional(); break;
        default: {
            if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                curt++;
            } else if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                curt++;
            }
        } break;
    }

    access();
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
        case token::identifier: reduce_identifier_to_identifier_expression(); curt++; break;
        case token::open_paren: curt++; tuple_after_open_paren(); break;
        default: {
            if(curt->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                curt++;
            }  
        }

    }

    TNode* last = array::read(stack, -1);

    switch(last->kind) {
        case node::expression: {
            Expression* expr = (Expression*)last;
            switch(expr->kind) {
                case expression::literal: {
                    access();
                } break;

                case expression::identifier: {

                } break;
            }
        }
    }
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
        case token::identifier:      reduce_identifier_to_identifier_expression(); curt++; break;
        case token::colon:           curt++; comptime_after_colon(); break;
        case token::assignment:      curt++; assignment_after_equal(); break;
        case token::open_paren:      curt++; tuple_after_open_paren(); break;
        default: {
            if(curt->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                curt++;
            }else if(curt->group == token::group_literal){
                reduce_literal_to_literal_expression();
                curt++;
            }
        } break;
    }

    TNode* last = array::read(stack, -1);

    switch(last->kind) {
        case node::expression: {
            Expression* expr = (Expression*)last;
            switch(expr->kind) {
                case expression::typeref: after_typeref(); break;
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
    //     case token::end_of_file: curt++; after_eof(); break;
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
//         case token::colon: curt++; label_after_colon(); break;
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
            stack_push((TNode*)group);
        }
        curt++;
        if(curt->kind == token::comma) curt++;
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
    switch(curt->kind) {
        case token::comma: curt++; label_group_after_comma(); break;
        case token::colon: curt++; label_after_colon(); break;
    }
}

/* 
        module: * { label } EOF 
         label: * ( ID | labelgroup ) ':' expr ( ';' | ']' | '}' )  
    labelgroup: ID ( "," ID )+ 
*/
void start() { announce_stage;
    while(1) {
        if(curt->kind != token::identifier) break;
        Expression* expr = compiler::create_expression();
        expr->kind = expression::identifier;
        stack_push((TNode*)expr);

        curt++;
        label_after_id();

        TNode* last = array::read(stack, -1); 

        switch(last->kind) {
            case node::expression: {
                // we determine if a semicolon is required or not by checking the current token
                if(curt->kind != token::close_brace && curt->kind != token::semicolon) {
                    messenger::dispatch(message::attach_sender({parser->source, *curt},
                        diagnostic::parser::missing_semicolon()));
                    return;
                }

                curt++;

                // reduce to a label 
                Label* label = compiler::create_label();
                node::insert_first((TNode*)label, stack_pop());
                node::insert_first((TNode*)label, stack_pop());

                label->token = ((Expression*)label->node.first_child)->start;

                stack_push((TNode*)label);

            } break;
        }
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