/*

    NOTES
    -----

    When an Entity is created, eg. a Function, Place, etc. It is pushed onto the stack 
    *after* the actual node that represents it is placed


*/

namespace amu {
namespace parser{

Parser*
create(Code* code) {
    Parser* out = pool::add(compiler::instance.storage.parsers);
    out->code = code;
    out->module_stack = array::init<Module*>();
    out->current_module = 0;
    out->table_stack = array::init<LabelTable*>();
    out->current_table = 0;
    out->stack = array::init<TNode*>();
    code->parser = out;
    return out;
}

void
destroy(Parser* parser) {
    array::deinit(parser->module_stack);
    array::deinit(parser->table_stack);
    array::deinit(parser->stack);
    pool::remove(compiler::instance.storage.parsers, parser);
}


// Parser* parser;
// Token* curt;
// Array<TNode*> stack;

// // when an Entity is constructed, a Label usually needs to consume it 
// Entity* constructed_entity;

// FORCE_INLINE void 
// advance_curt() {
//     curt++;
//     if(curt->kind == token::directive_compiler_break) {
//         DebugBreakpoint; curt++;
//     }
// }

// // we will have already hit the break token, so dont break again
// #define backtrack() do {                                \
//     curt--;                                             \
//     if(curt->kind == token::directive_compiler_break) { \
//         curt--;                                         \
//     }                                                   \
// } while(0)

// FORCE_INLINE Token* 
// lookahead(u32 n) {
//     Token* out = curt + n;
//     while(out->kind == token::directive_compiler_break) out++;
//     return out;
// }

// void
// __stack_push(TNode* n, String caller) {
//     array::push(stack, n);
//     // messenger::dispatch(message::attach_sender(curt,
//     //     message::make_debug(message::verbosity::debug,
//     //         String("pushed: "), caller, String(" // "), 
//     //             (n->start? n->start->raw : String("bad start")), String(" -> "), 
//     //             (n->end? n->end->raw : String("bad end")))));
// }

// #define stack_push(n) __stack_push(n, __func__)

// TNode*
// __stack_pop(String caller) {
//     TNode* ret = array::pop(stack);
//     // messenger::dispatch(message::attach_sender(curt,
//     //     message::make_debug(message::verbosity::debug,
//     //         String("popped: "), caller, String(" // "), 
//     //             (ret->start? ret->start->raw : String("bad start")), String(" -> "), 
//     //             (ret->end? ret->end->raw : String("bad end")))));
//     return ret;
// }

// #define stack_pop() __stack_pop(__func__)

// FORCE_INLINE void
// push_table(LabelTable* table) {
//     array::push(parser->table_stack, parser->current_table);
//     table->last = parser->current_table;
//     parser->current_table = table;
// }

// FORCE_INLINE void
// pop_table() {
//     parser->current_table = array::pop(parser->table_stack);
// }

// FORCE_INLINE void 
// push_module(Module* m) {
//     array::push(parser->module_stack, parser->current_module);
//     parser->current_module = m;
//     push_table(&m->table);
// }

// FORCE_INLINE void 
// pop_module() {
//     parser->current_module = array::pop(parser->module_stack);
//     pop_table();
// }



// FORCE_INLINE void
// debug_announce_stage(String stage) {
//     if(compiler::instance.options.verbosity < message::verbosity::debug) return;
//     messenger::dispatch(message::attach_sender(curt,
//         message::make_debug(message::verbosity::debug, 
//             String("parse level: "), stage)));
// }

// #define announce_stage //debug_announce_stage(__func__)

// #define check_error if(!array::read(stack, -1)) return;
// #define push_error() do{                                                \
//     messenger::dispatch(message::attach_sender(curt, \
//         message::make_debug(message::verbosity::debug,                       \
//             String(__func__), String(ErrorFormat(" pushed error")))));  \
//     return (void)array::push(stack, (TNode*)0);                         \
// }while(0)


// #define set_start_end_from_children(n)                  \
// do{                                                     \
// ((TNode*)n)->start = (((TNode*)n)->first_child)->start; \
// ((TNode*)n)->end = (((TNode*)n)->last_child)->end;      \
// }while(0)                                    


// Label* 
// search_for_label(LabelTable* table, u64 hash) {
//     while(1) {
//         auto [idx, found] = map::find(table->map, hash);
//         if(found) return array::read(table->map.values, idx);
//         if(!table->last) return 0;
//         table = table->last;
//     }
// }


// void before_expr();
// void label_after_colon();
// void factor();
// void after_typeref(); 
// void label_after_id();
// void label();
// void logi_or();
// void logi_and();
// void bit_or();
// void bit_xor();
// void bit_and();
// void equality();
// void relational();
// void bit_shift();
// void additive();
// void term();
// void access();
// void block();
// void start();


// // parses identifiers separated by commas and groups them under a tuple 
// // this must be called after the first identifier and comma have been parsed
// // and the first identfier must have been pushed onto the stack
// void identifier_group() { announce_stage;
//     while(1) {
//         if(curt->kind != token::identifier) break; 
//         Expression* expr = Expr::create();
//         expr->kind = expr::identifier;
//         expr->node.start = expr->node.end = curt;
        
//         TNode* last = array::read(stack, -1);
//         if(last->kind == node::tuple) {
//             // a label group was already created, so just append to it
//             node::insert_last(last, (TNode*)expr);
//             last->end = curt;
//         } else {
//             // this is the second label, so the last must be another identifier
//             // make the label group tuple
//             Tuple* group = tuple::create();
//             group->kind = tuple::label_group;
//             node::change_parent((TNode*)group, stack_pop());
//             node::insert_last((TNode*)group, (TNode*)expr);
//             set_start_end_from_children(group);
//             stack_push((TNode*)group);
//         }
//         advance_curt();
//         if(curt->kind == token::comma) advance_curt();
//         else break;
//     }

// }

// // state8:
// // factor: ID *
// // reduce ID -> expr:id
// void reduce_identifier_to_identifier_expression() { announce_stage;
//     Expression* e = Expr::create();
//     e->kind = expr::identifier;
//     TNode* n = (TNode*)e;
//     n->start = n->end = curt;
//     stack_push(n);
// }

// /* there doesn't need to do anything special for reducing a literal, because they all become the same sort of expression
//    which just stores the token
//     literal: int | float | string | char

//     TODO(sushi) this may only be used in one or two places so it can be removed if so 
// */
// void reduce_literal_to_literal_expression() { announce_stage;
//     Expression* e = Expr::create();
//     e->kind = expr::literal;
//     TNode* n = (TNode*)e;
//     n->start = n->end = curt;
//     switch(curt->kind) {
//         case token::literal_character: {
//             e->type = &type::scalar::unsigned8;
//         } break;
//         case token::literal_float: {
//             e->type = &type::scalar::float64;
//         } break;
//         case token::literal_integer: {
//             e->type = &type::scalar::signed64;
//         } break;
//         case token::literal_string: {
//             e->type = type::array::create(&type::scalar::unsigned8, curt->raw.count);
//         } break;
//     }
//     stack_push(n);
// }


// /*
//     typeref: "void" * 
//            | "u8" *  | "u16" *  | "u32" *  | "u64" * 
//            | "s8" *  | "s16" *  | "s32" *  | "s64" * 
//            | "f32" * | "f64" *
// */
// void reduce_builtin_type_to_typeref_expression() { announce_stage;
//     Expression* e = Expr::create();
//     e->kind = expr::typeref;
//     switch(curt->kind) {
//         case token::void_:      e->type = &type::scalar::void_; break; 
//         case token::unsigned8:  e->type = &type::scalar::unsigned8; break;
//         case token::unsigned16: e->type = &type::scalar::unsigned16; break;
//         case token::unsigned32: e->type = &type::scalar::unsigned32; break;
//         case token::unsigned64: e->type = &type::scalar::unsigned64; break;
//         case token::signed8:    e->type = &type::scalar::signed8; break;
//         case token::signed16:   e->type = &type::scalar::signed16; break;
//         case token::signed32:   e->type = &type::scalar::signed32; break;
//         case token::signed64:   e->type = &type::scalar::signed64; break;
//         case token::float32:    e->type = &type::scalar::float32; break;
//         case token::float64:    e->type = &type::scalar::float64; break;
//     }
//     e->node.start = curt;
//     e->node.end = curt;
//     stack_push((TNode*)e);
// }

// /* 
//     tuple: '(' * ( label | expr ) { ( label | expr ) "," } [ "," ] ')' 
//     label: * ID ':' ...
//      expr: ...
// */
// void tuple_after_open_paren() { announce_stage;
//     Token* save = curt-1;
    
//     Tuple* tuple = tuple::create();
//     tuple->kind = tuple::unknown;

//     u32 count = 0;
//     b32 found_label = 0;
//     b32 valued = 0;
//     while(1) { // NOTE(sushi) label lists are not supported in tuples, so if a comma is encountered, it's assumed to be starting a new tuple item
//         if(curt->kind == token::close_paren) break;
//         switch(curt->kind) {
//             case token::identifier: {
//                 // need to figure out if this is an expression or label 
//                 switch(lookahead(1)->kind) {
//                     case token::colon:{
//                         if(!found_label) {
//                             tuple->table.map = map::init<String, Label*>();
//                             push_table(&tuple->table);
//                         }
//                         found_label = true;
//                         label(); check_error;
//                     } break;
//                     default: {
//                         if(found_label) {
//                             diagnostic::parser::tuple_positional_arg_but_found_label(curt);
//                             push_error();
//                         }
//                         // otherwise this is assumed to be an expression handled by before_expr
//                         before_expr(); check_error;
//                     } break;
//                 }
//             } break;
//             default: {
//                 before_expr();
//             }break;
//         }
//         count += 1;
//         if(curt->kind == token::comma) advance_curt();
//         else if(curt->kind != token::close_paren) { // there was some error in consuming an expr/label
//             diagnostic::parser::
//                 tuple_expected_comma_or_close_paren(curt);
//             return;
//         }
//     }

//     forI(count){
//         node::insert_first((TNode*)tuple, stack_pop());
//     }

//     if(count) set_start_end_from_children(tuple);
//     else{
//         tuple->node.start = save;
//         tuple->node.end = curt;
//     }   
//     stack_push((TNode*)tuple);

//     // check for function type definition
//     if(lookahead(1)->kind == token::function_arrow) {
//         advance_curt(); advance_curt();
//         u32 count = 0;
//         while(1) {
//             factor();
//             count++;
//             if(curt->kind != token::comma) break;
//             advance_curt();
//         }

//         if(!count) {
//             diagnostic::parser::
//                 missing_function_return_type(curt);
//             push_error();
//         }

//         Expression* e = Expr::create();
//         e->kind = expr::typeref;
//         e->type = type::function::create();
//         auto ft = (FunctionType*)e->type;

//         if(count > 1) {
//             Tuple* t = tuple::create();
//             t->kind = tuple::multireturn;
            
//             // need to collect types to form a TupleType
//             auto types = array::init<Type*>(count);

//             forI(count) {
//                 auto n = (Type*)stack_pop();
//                 array::push(types, n);
//                 node::insert_first((TNode*)t, (TNode*)n);
//             }

//             ft->return_type = type::tuple::create(types);
//             ft->returns = (TNode*)t;
//             node::insert_last((TNode*)e, (TNode*)t);

//             set_start_end_from_children(t);
//         } else {
//             auto t = (Type*)stack_pop();
//             ft->return_type = t;
//             ft->returns = (TNode*)t;
//             node::insert_first((TNode*)e, ft->returns);
//         }

//         ft->parameters = stack_pop();
//         node::insert_first((TNode*)e, ft->parameters);

//         set_start_end_from_children(e);
//         // this is a function entity declaration
//         if(curt->kind == token::open_brace) {
//             advance_curt();
//             block(); check_error;
//             node::insert_last((TNode*)e, stack_pop());
//             e->node.end = e->node.last_child->end;
//             Function* f = function::create();
//             f->node.start = e->node.start;
//             f->node.end = e->node.end;
//             f->type = ft;
//             stack_push((TNode*)f);
//         }
        
//         stack_push((TNode*)e);
        
        

//     } else advance_curt();

//     if(found_label) pop_table();
// }

// /*
//     block: '{' { stmt } '}'
// */
// void block() {
//     BlockExpression* e = block_Expr::create();
//     e->node.start = curt-1;
//     push_table(&e->table);

//     u32 count = 0;
//     while(1) {
//         // this is set to false when a nested block ends, so that we skip checking for a semicolon
//         b32 need_semicolon = true; 
//         statement::kind skind = statement::unknown;
//         if(curt->kind == token::close_brace) break;
//         switch(curt->kind) {
//             case token::identifier: {
//                 if((curt+1)->kind == token::colon) {
//                     label(); check_error;
//                     skind = statement::label;
//                 } else {
//                     before_expr(); check_error;
//                     skind = statement::expression;
//                 }
//             } break;
//             case token::directive_print_meta_type: {
//                 // special directive to emit the meta type of the given identifier
//                 advance_curt();
//                 if(curt->kind != token::identifier) {
//                     diagnostic::parser::expected_identifier(curt);
//                     push_error();
//                 }

//                 Label* l = search_for_label(parser->current_table, curt->hash);
//                 if(!l) {
//                     diagnostic::parser::unknown_identifier(curt);
//                     push_error();
//                 }

//                 switch(l->entity->node.kind) {
//                     case node::place: {
//                         messenger::dispatch(message::attach_sender(curt, 
//                             message::make_debug(message::verbosity::debug, message::plain("place"))));
//                     } break;
//                 }
//                 advance_curt();
//                 if(curt->kind != token::semicolon) {
//                     diagnostic::parser::missing_semicolon(curt);
//                     push_error();
//                 }
//                 advance_curt();
//                 continue; // this isn't an actual statement, so prevent making one
//             } break;
//             case token::directive_print_type: {
//                 // special directive to emit the type of the given identifier
//                 advance_curt();
//                 if(curt->kind != token::identifier) {
//                     diagnostic::parser::expected_identifier(curt);
//                     push_error();
//                 }

//                 Label* l = search_for_label(parser->current_table, curt->hash);
//                 if(!l) {
//                     diagnostic::parser::unknown_identifier(curt);
//                     push_error();
//                 }


//                 String out;
//                 switch(l->entity->node.kind) {
//                     case node::place: {
//                         out = type::name(((Place*)l->entity)->type);                        
//                     } break;
//                     case node::function: {
//                         out = type::name(((Function*)l->entity)->type);
//                     } break;
//                 }
//                 messenger::dispatch(message::attach_sender(curt,
//                             message::make_debug(message::verbosity::debug, out)));
//                 advance_curt();
//                 if(curt->kind != token::semicolon) {
//                     diagnostic::parser::missing_semicolon(curt);
//                     push_error();
//                 }
//                 advance_curt();
//                 continue; // this isn't an actual statement, so prevent making one
//             } break;
//             default: {
//                 before_expr(); check_error;
//                 skind = statement::expression;
//                 if(curt->kind == token::close_brace) {
//                     // this must be the end of a block expression, so we dont need a semicolon
//                     need_semicolon = false;
//                 }
//             } break;
//         }
//         if(need_semicolon && curt->kind != token::semicolon) {
//             if(curt->kind != token::close_brace) {
//                 diagnostic::parser::
//                     missing_semicolon(curt);
//                 push_error();
//             }
//             // if there's a close brace, this must be the last expression of a block
//             // so we dont't do anything, allowing the expression to be attached as the last
//             // node of the block below
//             count++;
//             break;
//         } else { // this is just a normal statement
//             Statement* s = statement::create();
//             s->kind = skind;

//             node::insert_first((TNode*)s, stack_pop());
//             s->node.start = s->node.first_child->start;
//             s->node.end = curt;

//             stack_push((TNode*)s);

//             advance_curt();
//             count++;
//         }
//     }

//     forI(count) {
//         node::insert_first((TNode*)e, stack_pop());
//     }
//     e->node.end = curt;
//     stack_push((TNode*)e);
//     pop_table();
// }

// /*
//     comptime: * typeref ':' expr
//             | typeref ':' * expr
//             | * ':' expr
// */ 
// void comptime_after_colon() { announce_stage;
//     switch(curt->kind) {
//         case token::identifier:      reduce_identifier_to_identifier_expression(); advance_curt(); break;
//         case token::colon:           advance_curt(); comptime_after_colon(); check_error; break;
//         case token::open_paren:      advance_curt(); tuple_after_open_paren(); check_error; break;
//         default: {
//             if(curt->group == token::group_type) {
//                 reduce_builtin_type_to_typeref_expression();
//                 advance_curt();
//             }else if(curt->group == token::group_literal) {
//                 reduce_literal_to_literal_expression();
//                 advance_curt();
//             }
//         }break;
//     }

//     TNode* last = array::read(stack, -1);

//     switch(last->kind) {
//         case node::expression: {
//             Expression* expr = (Expression*)last;
//             switch(expr->kind) {
//                 case expr::binary_plus: {
                    
//                 } break;
//             }
//         } break;    
//     }
// }

// /*
//     assignment: typeref * '=' expr
//          ctime: typeref * ':' expr
//         factor: typeref * [ block | '*' ]
// */ 
// void after_typeref() { announce_stage;
//     Token* save = curt;
//     switch(curt->kind) {
//         case token::colon: advance_curt(); comptime_after_colon(); break;
//         case token::equal: {
//             advance_curt(); // assignment: typeref '=' * expr
//             before_expr(); check_error;
//             // now we reduce to binary assignment
//             Expression* e = Expr::create();
//             e->kind = expr::binary_assignment;
//             e->node.start = save;
//             e->node.end = curt;
//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());
//             // always take the type of the left side because it controls the type of the expression
//             e->type = ((Expression*)e->node.first_child)->type;

//             set_start_end_from_children(e);

//             // this is a place in memory, so we need to create a Place entity for it 
//             auto p = Place::create();
//             p->node.start = e->node.start;
//             p->node.end = e->node.end;
//             p->type = e->type;

//             stack_push((TNode*)e);
//             stack_push((TNode*)p);

//         } break;
//         case token::open_brace: {
//             advance_curt();
//             block(); check_error;
//             node::insert_last(array::read(stack, -2), stack_pop());
//             TNode* typeref = array::read(stack, -1);
//             typeref->end = typeref->last_child->end;
//         } break;
//         case token::asterisk: {
//             Expression* last = (Expression*)array::read(stack, -1);
//             last->type = type::pointer::create(last->type);
//             advance_curt();
//             after_typeref(); check_error;
//         } break;
//     }
// }

// /*
//     loop: "loop" expr
// */
// void loop() { announce_stage;
//     check_error;
//     if(curt->kind == token::loop) {
//         advance_curt();
//         before_expr();

//         Expression* loop = Expr::create();
//         loop->kind = expr::loop;
//         node::insert_last((TNode*)loop, stack_pop());
//         set_start_end_from_children(loop);
//         stack_push((TNode*)loop);
//     }
// }

// /*
//     for: "for" ( "(" label ";" expr ";"" expr ")" | "(" id { "," id } "in" expr ")" ) expr
// */
// void for_() {
//     Token* save = curt;

//     advance_curt();
//     if(curt->kind != token::open_paren) {
//         diagnostic::parser::
//             for_missing_open_paren(curt);
//         push_error();
//     }

//     advance_curt();
//     if(curt->kind != token::identifier) {
//         diagnostic::parser::for_expected_some_identfier(curt);
//         push_error();
//     }

//     b32 c_style = false;
    
//     switch((curt+1)->kind) {
//         case token::in:
//         case token::comma: { // this is a list of identifiers that must be followed by 'in'
//             Expression* expr = Expr::create();
//             expr->kind = expr::identifier;
//             expr->node.start = curt;
//             expr->node.end = curt;
//             stack_push((TNode*)expr);
//             advance_curt();

//             if(curt->kind == token::comma){
//                 advance_curt();
//                 identifier_group(); check_error;
//                 // TODO(sushi) maybe look into properly supporting this later
//                 if(curt->kind == token::colon) { 
//                     diagnostic::parser::for_label_group_not_allowed(curt);
//                     push_error();
//                 }
//             }

//             if(curt->kind != token::in) {
//                 diagnostic::parser::for_expected_in(curt);
//                 push_error();
//             }

//             advance_curt();
//             before_expr(); check_error;
//         } break;
//         case token::colon: { 
//             c_style = true;
//             label();
//             if(curt->kind != token::semicolon) {
//                 diagnostic::parser::missing_semicolon(curt);
//                 push_error();
//             }

//             advance_curt();
//             before_expr(); check_error;

//             if(curt->kind != token::semicolon) {
//                 diagnostic::parser::missing_semicolon(curt);
//                 push_error();
//             }

//             advance_curt();
//             before_expr(); check_error;

//         } break;
        
//     }   

//     if(curt->kind != token::close_paren) {
//         diagnostic::parser::for_missing_close_paren(curt);
//         push_error();
//     }
    
//     advance_curt();

//     // we should be before some expression now 
//     before_expr(); check_error;

//     Expression* e = Expr::create();
//     e->kind = expr::for_;

//     node::insert_first((TNode*)e, stack_pop());
//     node::insert_first((TNode*)e, stack_pop());
//     if(c_style) {
//         node::insert_first((TNode*)e, stack_pop());
//     }
//     node::insert_first((TNode*)e, stack_pop());

//     e->node.start = save;
//     e->node.end = e->node.last_child->end;

//     stack_push((TNode*)e);
// }

// /*
//     switch: conditional | "switch" '(' expr ')' '{' { expr "=>" expr } '}'
// */ 
// void switch_() { announce_stage;
//     advance_curt();
//     if(curt->kind != token::open_paren) {
//         diagnostic::parser::
//             switch_missing_open_paren(curt);
//         return;
//     }

//     advance_curt(); // "switch" "(" * expr ")" ...

//     before_expr(); // dont care what this returns cause any expression is good (for now)

//     if(curt->kind != token::close_paren) {
//         diagnostic::parser::
//             switch_missing_close_paren(curt);
//         push_error();
//     }

//     advance_curt(); // "switch" "(" expr ")" * "{" ...

//     if(curt->kind != token::open_brace) {
//         diagnostic::parser::
//             switch_missing_open_brace(curt);
//         push_error();
//     }

//     advance_curt(); // "switch" "(" expr ")" "{" * { expr } "}"

//     u32 count = 0;
//     while(1) {
//         if(curt->kind == token::close_brace) break;
//         before_expr(); 
        
//         if(curt->kind != token::match_arrow) {
//             diagnostic::parser::
//                 switch_missing_match_arrow_after_expr(curt);
//             push_error();
//         }

//         advance_curt();
//         before_expr();

//         if(curt->kind != token::comma && curt->kind != token::close_brace ) {
//             diagnostic::parser::
//                 switch_missing_comma_after_match_arm(curt);
//             push_error();
//         }
//         advance_curt();
//         count++;

//         Expression* e = Expr::create();
//         e->kind = expr::switch_case;
        
//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);

//         stack_push((TNode*)e);

//     }

//     if(!count) {
//         diagnostic::parser::
//             switch_empty_body(curt);
//     }

//     Expression* e = Expr::create();
//     e->kind = expr::switch_expr;

//     // reduce to switch expression
//     forI(count+1){
//         node::insert_first((TNode*)e, stack_pop());
//     }

//     set_start_end_from_children(e);
//     stack_push((TNode*)e);
// }

// /*
//     conditional: logi-or | "if" '(' expr ')' expr [ "else" expr ]
// */
// void conditional() { announce_stage;
//     check_error;
//     if(curt->kind == token::if_) {
//         advance_curt();

//         if(curt->kind != token::open_paren) {
//             diagnostic::parser::
//                 if_missing_open_paren(curt);
//             push_error();
//         }

//         advance_curt();

//         before_expr();

//         if(curt->kind != token::close_paren) {
//             diagnostic::parser::
//                 if_missing_close_paren(curt);
//             push_error();
//         }

//         advance_curt();

//         before_expr();

//         Expression* e = Expr::create();
//         e->kind = expr::conditional;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         if(curt->kind == token::else_) {
//             advance_curt();
//             before_expr();
//             node::insert_last((TNode*)e, stack_pop());
//         }
//     }
// }

// // TODO(sushi) this long chain of binary op handlers can probably all be combined into one function

// void assignment() { announce_stage;
//     if(curt->kind == token::equal) {
//         advance_curt();
//         logi_or();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         bit_and(); check_error;
//         bit_xor(); check_error;
//         bit_or(); check_error;
//         logi_and(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_assignment;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         assignment();
//     }

// }

// /*
//     logi-or: logi-and { "||" logi-and }
// */
// void logi_or() { announce_stage;
//     if(curt->kind == token::logi_or) {
//         advance_curt();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         bit_and(); check_error;
//         bit_xor(); check_error;
//         bit_or(); check_error;
//         logi_and(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_or;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         logi_or();
//     }
// }

// /*
//     logi-and: bit-or { "&&" bit-or }
// */
// void logi_and() { announce_stage;
//     check_error;
//     if(curt->kind == token::double_ampersand) {
//         advance_curt();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         bit_and(); check_error;
//         bit_xor(); check_error;
//         bit_or(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_and;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         logi_and();
//     }
// }

// /*
//     bit-or: bit-xor { "|" bit-xor }
// */
// void bit_or() { announce_stage;
//     check_error;
//     if(curt->kind == token::vertical_line) {
//         advance_curt();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         bit_and(); check_error;
//         bit_xor(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_bit_or;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         bit_or();
//     }
// }

// /*
//     bit-xor: bit-and { "^" bit-and }
// */
// void bit_xor() { announce_stage;
//     check_error;
//     if(curt->kind == token::caret) {
//         advance_curt();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         bit_and(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_bit_xor;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         bit_xor();
//     }
// }


// /*
//     bit-and: equality { "&" equality } 
// */
// void bit_and() { announce_stage;
//     check_error;
//     if(curt->kind == token::ampersand) {
//         advance_curt();
//         factor(); check_error;
//         access(); check_error;
//         term(); check_error;
//         additive(); check_error;
//         bit_shift(); check_error;
//         relational(); check_error;
//         equality(); check_error;
//         Expression* e = Expr::create();
//         e->kind = expr::binary_bit_and;

//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());

//         set_start_end_from_children(e);
//         stack_push((TNode*)e);

//         bit_and();
//     }
// }

// /*
//     equality: relational { ( "!=" | "==" ) relational }
// */
// void equality() { announce_stage;
//     check_error;
//     token::kind kind = curt->kind;
//     switch(kind) {
//         case token::double_equal:
//         case token::explanation_mark_equal: {
//             advance_curt();
//             factor(); check_error;
//             access(); check_error;
//             term(); check_error;
//             additive(); check_error;
//             bit_shift(); check_error;
//             relational(); check_error;
//             Expression* e = Expr::create();
//             e->kind = kind == token::double_equal ? expr::binary_equal : expr::binary_not_equal;

//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());

//             set_start_end_from_children(e);
//             stack_push((TNode*)e);

//             equality();
//         } break;
//     }
// }

// /*
//     relational: bit-shift { ( ">" | "<" | "<=" | ">=" ) bit-shift }
// */
// void relational() { announce_stage;
//     check_error;
//     token::kind kind = curt->kind;
//     switch(kind) {
//         case token::less_than:
//         case token::less_than_equal:
//         case token::greater_than:
//         case token::greater_than_equal: {
//             advance_curt();
//             factor(); check_error;
//             access(); check_error;
//             term(); check_error;
//             additive(); check_error;
//             bit_shift(); check_error;
//             Expression* e = Expr::create();
//             e->kind = kind == token::less_than ?
//                       expr::binary_less_than :
//                       kind == token::less_than_equal ? 
//                       expr::binary_less_than_or_equal :
//                       kind == token::greater_than ? 
//                       expr::binary_greater_than :
//                       expr::binary_greater_than_or_equal;

//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());

//             set_start_end_from_children(e);
//             stack_push((TNode*)e);

//             relational();
//         } break;
//     }
// }

// /*
//     bit-shift: additive { "<<" | ">>" additive }
// */
// void bit_shift() { announce_stage;
//     check_error;
//     token::kind kind = curt->kind;
//     switch(kind) {
//         case token::double_less_than: 
//         case token::double_greater_than: {
//             advance_curt(); 
//             factor(); check_error;
//             access(); check_error;
//             term(); check_error;
//             additive(); check_error;
//             Expression* e = Expr::create();
//             e->kind = kind == token::double_less_than ? 
//                       expr::binary_bit_shift_left :
//                       expr::binary_bit_shift_right;
            
//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());

//             set_start_end_from_children(e);
//             stack_push((TNode*)e);

//             bit_shift();
//         } break;
//     }
// }

// /*
//     additive: term * { ("+" | "-" ) term }
// */
// void additive() { announce_stage;
//     check_error;
//     token::kind kind = curt->kind;
//     switch(kind) {
//         case token::plus:
//         case token::minus: {
//             advance_curt();
//             factor(); check_error;
//             access(); check_error;
//             term(); check_error;
//             Expression* e = Expr::create();
//             e->kind = kind == token::plus ? expr::binary_plus : expr::binary_minus;
//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());

//             set_start_end_from_children(e);
//             stack_push((TNode*)e);

//             additive();
//         } break;
//     }
// }

// /*
//     term: access * { ( "*" | "/" | "%" ) access }
// */
// void term() { announce_stage;
//     check_error;
//     token::kind kind = curt->kind;
//     switch(kind) {
//         case token::percent: 
//         case token::solidus: 
//         case token::asterisk: {
//             advance_curt();
//             factor(); check_error; 
//             access(); check_error;
//             Expression* e = Expr::create();
//             e->kind = 
//                     kind == token::percent ? expr::binary_modulo 
//                     : kind == token::solidus ? expr::binary_division
//                     : expr::binary_multiply;
            
//             node::insert_first((TNode*)e, stack_pop());
//             node::insert_first((TNode*)e, stack_pop());

//             set_start_end_from_children(e);
//             stack_push((TNode*)e);

//             term();
//         } break;
//     }
// }

// /*
//     access: factor * { "." factor }
// */
// void access() { announce_stage;
//     check_error;
//     if(curt->kind == token::dot) {
//         advance_curt();
//         // when we parse accesses, we don't care about figuring out if the identifier is 
//         // correct because we don't know what is being accessed and even if we did, it would
//         // possibly not have been parsed yet 
//         if(curt->kind != token::identifier) {
//             diagnostic::parser::
//                 expected_identifier(curt);
//             push_error();
//         }

//         reduce_identifier_to_identifier_expression();

//         Expression* e = Expr::create();
//         e->kind = expr::binary_access;
//         node::insert_first((TNode*)e, stack_pop());
//         node::insert_first((TNode*)e, stack_pop());
//         set_start_end_from_children(e);
//         stack_push((TNode*)e);
//         advance_curt();
//         access();
//     }
// }

// /*
//     access: * factor { "." factor }
//           | factor { "." * factor }
//     factor: * (literal | id | tuple | type )
// */
// void factor() { announce_stage;
//     check_error;
//     switch(curt->kind) {
//         case token::identifier: {
//             reduce_identifier_to_identifier_expression();
//             Label* label = search_for_label(parser->current_table, curt->hash); 
//             if(!label) {
//                 diagnostic::parser::unknown_identifier(curt);
//                 push_error();
//             }
            
//             // in order to determine how we will treat this identfier syntactically, we figure out what sort of 
//             // entity the label points to
//             switch(label->entity->node.kind) {
//                 case node::function: {
//                     if(lookahead(1)->kind == token::open_paren) {
//                         // must be a function call
//                         Call* e = call_Expr::create();
//                         e->kind = expr::call;
//                         e->node.start = curt;
//                         e->callee = (Function*)label->entity;
//                         advance_curt(); advance_curt();
//                         tuple_after_open_paren(); check_error;          
//                         e->node.end = curt;
//                         e->arguments = (Tuple*)stack_pop();
//                         node::insert_last((TNode*)e, (TNode*)e->arguments); // append argument tuple
//                         node::insert_last((TNode*)e, stack_pop()); // append identfier found above
//                         stack_push((TNode*)e);

//                     } else {
//                         NotImplemented;
//                         // // probably just a reference to a function entity
//                         // Expression* last = (Expression*)array::read(stack, -1);
//                         // last->kind = expr::entity_func;
//                         // last->entity = label->entity;
//                         // advance_curt();
//                     }
//                 } break;
//                 case node::structure: {
//                     Expression* last = (Expression*)array::read(stack, -1);
//                     last->kind = expr::typeref;
//                     last->type = (Type*)label->entity;
//                     advance_curt();
//                     after_typeref(); check_error;
//                 } break;
//                 case node::place: {
//                     Expression* last = (Expression*)array::read(stack, -1);
//                     last->kind = expr::identifier;
//                     last->type = ((Place*)label->entity)->type;
//                     advance_curt();
//                 } break;
//             }
//         } break;
//         case token::open_paren: advance_curt(); tuple_after_open_paren(); break;
//         case token::if_:        conditional(); break;
//         case token::switch_:    switch_(); break;
//         case token::for_:       for_(); break;
//         case token::open_brace: advance_curt(); block(); break;
//         case token::ampersand: {
//             Expression* ref = Expr::create();
//             ref->kind = expr::unary_reference;
//             ref->node.start = curt;

//             advance_curt();
//             factor(); check_error;
            
//             auto last = (Expression*)stack_pop();
//             ref->type = type::pointer::create(last->type);
//             ref->node.end = last->node.end;
//             node::insert_last((TNode*)ref, (TNode*)last);
//             stack_push((TNode*)ref);
//         } break;
//         default: {
//             if(curt->group == token::group_literal) {
//                 reduce_literal_to_literal_expression();
//                 advance_curt();
//             } else if(curt->group == token::group_type) {
//                 reduce_builtin_type_to_typeref_expression();
//                 advance_curt();
//             } else {
//                 diagnostic::parser::
//                     unexpected_token(curt, curt);
//                 push_error();
//             }
//         } break;
//     }
// }

// void struct_decl() {
//     Token* save = curt;
//     advance_curt(); // TODO(sushi) struct parameters
//     if(curt->kind != token::open_brace) {
//         diagnostic::parser::
//             missing_open_brace_for_struct(curt);
//         push_error();
//     }

//     advance_curt();
    
//     Structure* s = structure::create();
//     s->node.start = save;
//     s->node.end = curt;

//     push_table(&s->table);

//     u32 count = 0;
//     while(1) {
//         if(curt->kind == token::close_brace) break;
//         if(curt->kind == token::identifier) {
//             label(); check_error;
//             if(curt->kind != token::semicolon) {
//                 diagnostic::parser::
//                     missing_semicolon(curt);
//             }
//             advance_curt();
//             count++;
//             TNode* last = array::read(stack, -1);
//             if(last->last_child->kind == node::function) {
//                 diagnostic::parser::
//                     struct_member_functions_not_allowed(last->start);
//                 push_error();
//             }
//         } else {
//             diagnostic::parser::
//                 struct_only_labels_allowed(curt);
//             push_error();
//         }
//     }

//     pop_table();

//     forI(count) {
//         node::insert_first((TNode*)s, stack_pop());
//     }

//     Structured* t = type::structured::create(s);


//     stack_push((TNode*)t);
//     stack_push((TNode*)t);

// }

// /* general expr handler, since we will come across this alot
//            expr: * ( loop | switch | assignment | ctime | conditional ) .
//            loop: * "loop" expr
//          switch: * "switch" '(' expr ')' '{' { expr } '}'
//      assignment: * [ expr | typeref ] . '=' expr .
//           ctime: * [ typeref ] ':' expr
//     conditional: * ( logical-or | "if" '(' expr ')' expr [ "else" expr ] 
//      logical-or: * logical-and { "||" logical-and }
//     logical-and: * bit-or { "&&" bit-or }            
//          bit-or: * bit-xor { "|" bit-xor }                                 
//         bit-and: * equality { "&" equality }                              
//        equality: * relational { ("!=" | "==" ) relational }              
//       elational: * bit-shift { ( "<" | ">" | "<=" | ">=" ) bit-shift } 
//            term: * factor { ( '*' | '/' | '%' ) factor }
//          factor: * ( literal | ID | tuple | type )
//           tuple: * '(' tupleargs ')'
//         typeref: * ( func_type | factor decorators | "void" | "u8" | "u16" | "u32" | "u64" | "s8" | "s16" | "s32" | "s64" | "f32" | "f64" )
//         literal: * ( string | float | int | char )
// */
// void before_expr() { announce_stage;
//     switch(curt->kind) {
//         case token::equal: {
//             Token* save = curt;
//             advance_curt();
//             before_expr(); check_error;

//             Expression* e = Expr::create();
//             e->kind = expr::unary_assignment;

//             Expression* last = (Expression*)stack_pop();

//             node::insert_first((TNode*)e, (TNode*)last);
//             e->node.start = save;
//             e->node.end = e->node.last_child->end;
//             e->type = last->type;

//             auto p = Place::create();
//             p->type = last->type;

//             stack_push((TNode*)p);
//             stack_push((TNode*)e);
//         } break;
//         case token::colon: {
//             Token* save = curt;
//             advance_curt();
//             before_expr(); check_error;

//             Expression* e = Expr::create();
//             e->kind = expr::unary_comptime;

//             node::insert_first((TNode*)e, stack_pop());
//             e->node.start = save;
//             e->node.end = e->node.last_child->end;
//             stack_push((TNode*)e);
//         } break;
//         case token::structdecl: {
//             struct_decl(); check_error;
//         } break;
//         case token::moduledecl: {
//             push_module(curt->module);
//             advance_curt();
//             advance_curt();
//             start();
//             pop_module();
//         } break;
//         case token::return_: {
//             Token* save = curt;
//             advance_curt();
//             before_expr();

//             Expression* e = Expr::create();
//             e->kind = expr::return_;

//             node::insert_first((TNode*)e, stack_pop());
//             e->node.start = save;
//             e->node.end = e->node.last_child->end;

//             stack_push((TNode*)e);
//         }break;
//         case token::using_: {
//             // this should just be an alias
//             Expression* e = Expr::create();
//             e->kind = expr::using_;
//             e->node.start = curt;

//             // TODO(sushi) this needs to be changed to take in an expression that may result in an identifier
//             advance_curt();
//             if(curt->kind != token::identifier) {
//                 diagnostic::parser::expected_identifier(curt);
//                 push_error();
//             }

//             e->node.end = curt;

//             Expression* i = Expr::create();
//             i->kind = expr::identifier;
//             i->node.start = i->node.end = curt;

//             node::insert_last((TNode*)e, (TNode*)i);

//             Label* label = search_for_label(parser->current_table, curt->hash);
//             if(!label) {
//                 diagnostic::parser::unknown_identifier(curt);
//                 push_error();
//             }

//             stack_push((TNode*)label->entity);
//             advance_curt();
//         } break;
//         default: factor(); check_error;
//     }

//     // loop and see if any operators are being used, if so call their entry point
//     b32 search = true;
//     while(search) {
//         switch(curt->kind) {
//             case token::dot: access(); check_error; break;
//             case token::asterisk:
//             case token::solidus: term(); check_error; break;
//             case token::plus:
//             case token::minus: additive(); check_error; break;
//             case token::double_less_than:
//             case token::double_greater_than: bit_shift(); check_error; break;
//             case token::double_equal: 
//             case token::explanation_mark_equal: equality(); check_error; break;
//             case token::less_than:
//             case token::less_than_equal:
//             case token::greater_than:
//             case token::greater_than_equal: relational(); check_error; break;
//             case token::ampersand: bit_and(); check_error; break;
//             case token::caret: bit_xor(); check_error; break;
//             case token::vertical_line: bit_or(); check_error; break;
//             case token::double_ampersand: logi_and(); check_error; break;
//             case token::logi_or: logi_or(); check_error; break;
//             case token::equal: assignment(); check_error; break;
//             //case token::open_brace: advance_curt(); block(); check_error; break;
//             default: search = false;
//         }
//     }

//     // after_expr(); TODO(sushi)
// }

// void label_after_colon() { announce_stage;
//     switch(curt->kind) {
//         case token::identifier: {
//             reduce_identifier_to_identifier_expression(); 
//             Label* label = search_for_label(parser->current_table, curt->hash);
//             if(!label) {
//                 diagnostic::parser::unknown_identifier(curt);
//                 push_error();
//             }

//             switch(label->entity->node.kind) {
//                 case node::type: {
//                     Expression* last = (Expression*)array::read(stack, -1);
//                     last->kind = expr::typeref;
//                     last->type = (Type*)label->entity;
//                     advance_curt();
//                     after_typeref(); check_error;

//                     last = (Expression*)array::read(stack, -1);

//                     // // in this case, 'after_typeref' will have already made a place in memory for this 
//                     if(last->kind == expr::binary_assignment) break;

//                     // this is a place in memory
//                     auto p = Place::create();
//                     p->type = last->type;
//                     p->label = label;
//                     TNode* save = stack_pop();
//                     stack_push((TNode*)p);
//                     stack_push(save);
//                 } break;
//                 default: advance_curt(); break; 
//             }
            
//         } break;
//         case token::open_paren: advance_curt(); tuple_after_open_paren(); check_error; break;
//         case token::equal:    
//         case token::colon: before_expr(); check_error; break;
        
//         default: {
//             if(curt->group == token::group_type) {
//                 reduce_builtin_type_to_typeref_expression();
//                 // this will be a typed place in memory
//                 auto last = (Expression*)stack_pop();
//                 auto p = Place::create();
//                 p->type = last->type;

//                 stack_push((TNode*)p);
//                 stack_push((TNode*)last);
                
//                 advance_curt();
//             }else if(curt->group == token::group_literal){
//                 reduce_literal_to_literal_expression();
//                 advance_curt();
//             }
//         } break;
//     }

//     check_error;

//     TNode* last = array::read(stack, -1);

//     switch(last->kind) {
//         case node::expression: {
//             Expression* expr = (Expression*)last;
//             switch(expr->kind) {
//                 case expr::typeref: after_typeref(); check_error; break;
//             }
//         } break;
//     }
// }
// /*
//          label: labelgroup ':' expr ( ';' | ']' | '}' )
//     labelgroup: ID ( "," * ID )+

//     here we are gathering a 'labelgroup', which is internally represented as a Tuple
//     with the identifiers as its children

// */
// void label_group_after_comma() { announce_stage;
//     identifier_group(); check_error;

//     // if we have come to a place where a comma was not followed by an identifier
//     // we throw an error about it
//     if((curt-1)->kind == token::comma) {
//         diagnostic::parser::
//             label_group_missing_id(curt);
//     }

//     if(curt->kind == token::colon) {
//         label_after_colon(); check_error;
//     } else {
//         diagnostic::parser::
//             label_missing_colon(curt);
//     }
// }

// /*
//          label: ( ID * | idgroup ) ':' expr ( ';' | ']' | '}' )
//     labelgroup: ID * ( "," ID )+
// */
// void label_after_id() { announce_stage;
//     check_error;

//     switch(curt->kind) {
//         case token::comma: advance_curt(); label_group_after_comma(); break;
//         case token::colon: advance_curt(); label_after_colon(); break;
//     }
// }

// void label() {
//     Expression* expr = Expr::create();
//     expr->kind = expr::identifier;
//     expr->node.start = curt;
//     expr->node.end = curt;
//     stack_push((TNode*)expr);

//     Token* save = curt;

//     advance_curt();
//     label_after_id(); check_error;

//     Label* label;
//     auto [idx, found] = map::find(parser->current_table->map, save->raw);
//     if(found) {
//         label = array::read(parser->current_table->map.values, idx);
//     } else {
//         label = label::create();
//         map::add(parser->current_table->map, (String)save->raw, label);
//     }

//     node::insert_first((TNode*)label, stack_pop());

//     TNode* cand = stack_pop();
//     switch(cand->kind) {
//         case node::place:
//         case node::type:
//         case node::function:
//         case node::module: {
//             label->entity = (Entity*)cand;
//             Entity* e = (Entity*)cand;
//             label->aliased = e->label;
//             e->label = label;
//             cand->start = label->node.first_child->start;
//             cand->end = label->node.last_child->end; 
//             node::insert_first((TNode*)label, stack_pop());
//         } break;
//         default: node::insert_first((TNode*)label, cand);
//     }

//     set_start_end_from_children(label);
//     stack_push((TNode*)label);
// }

// /* 
//         module: * { label } EOF 
//          label: * ( ID | labelgroup ) ':' expr ( ';' | ']' | '}' )  
//     labelgroup: ID ( "," ID )+ 
// */
// void start() { announce_stage;
//     u32 count = 0;
//     while(1) {
//         if(curt->kind != token::identifier) break;
//         label(); check_error;

//         if(curt->kind != token::close_brace && curt->kind != token::semicolon) {
//             diagnostic::parser::
//                 missing_semicolon(curt);
//             return;
//         }

//         count++;
//         advance_curt();
//     }

//     Module* m = parser->current_module;
//     forI(count) {
//         node::insert_first((TNode*)m, stack_pop());
//     }

//     set_start_end_from_children(m);
//     stack_push((TNode*)m);
// }

// /*
//     perform an initial scan over the labels marked by the lexer 
//     we start parsing at each label and continue up until we are able 
//     to resolve what it is and add a Symbol entry for it 

//     this doesn't build any part of the AST, it just precreates labels so that we may 
//     refer back to them during actual parsing to get more of an idea what identifiers are
//     when we find them
// */
// void prescan() {
//     forI(parser->code->lexer->labels.count) {
//         curt = array::readptr(code::get_token_array(parser->code), array::read(parser->code->lexer->labels, i));
//         Label* l = label::create();
//         l->node.start = curt;
//         map::add(parser->current_module->table.map, (String)curt->raw, l);

//         advance_curt();
//         advance_curt();
//         switch(curt->kind) {
//             case token::open_paren: {
//                 // this can either be a variable of some tuple type, a function type, or a function definition
//                 // so we skip until we find the end of the tuple
//                 u32 nesting = 0;
//                 while(1) {
//                     advance_curt();
//                     if(curt->kind == token::open_paren) nesting++;
//                     else if(curt->kind == token::close_paren) {
//                         if(!nesting) break;
//                         nesting--;
//                     }
//                 }
//                 advance_curt();

//                 switch(curt->kind) {
//                     case token::function_arrow: {
//                         // we now have to skip until either a semicolon or opening brace is found 
//                         // if a brace is found, this must be a function definition and so the label
//                         // refers to a function entity, otherwise it is a label of function pointer type
//                         while(1) {
//                             advance_curt();
//                             if(curt->kind == token::open_brace) {
//                                 // we assume this is a function definion, so we'll create a function entity to be 
//                                 // filled out later during actual parsing 
//                                 Function* f = function::create();
//                                 l->entity = f;
//                                 goto label_finished;
//                             } else if(curt->kind == token::semicolon || curt->kind == token::equal) {
//                                 // this is probably a variable pointing to a function
//                                 // if '=' is found, then it's the same, only it is being initialized
//                                 auto p = Place::create();
//                                 l->entity = p;
//                                 goto label_finished;
//                             }
//                         }
//                     } break;
//                     case token::equal:
//                     case token::semicolon: {
//                         // this must be a label representing a variable of some tuple type
//                         auto p = Place::create();
//                         l->entity = p;
//                         goto label_finished;
//                     } break;
                    
//                 }

//             } break;
//             case token::equal: {
//                 // this is some implicitly typed runtime label
//             } break;    
//             case token::colon: {
//                 // this is some implicitly typed compile time label
//                 // we may run into 'struct', 'module', 'variant', or just an expression 
//                 advance_curt();
//                 switch(curt->kind) {
//                     case token::structdecl: {
//                         Structure* s = structure::create();
//                         s->label = l;
//                         Structured* t = type::structured::create(s);
//                         l->entity = (Entity*)t;
//                         t->label = l;
//                         goto label_finished;
//                     } break;
//                     case token::moduledecl: {
//                         l->entity = curt->module;
//                         // we switch over to the new module and recurse
//                         Token* save = curt;
//                         push_module(curt->module);
//                         prescan();
//                         pop_module();
//                         curt = save;
//                         goto label_finished;
//                     } break;
//                     case token::open_paren: {
//                         // this has to be a function definition
//                         // for now, at least
//                         // TODO(sushi) determine if the grammar can support Type objects being assigned like this
//                         Function* f = function::create();
//                         l->entity = f;
//                         goto label_finished;
//                     } break;
//                     case token::using_: {
//                         // this must be an alias, so we don't do anything because we can't reliably
//                         // determine what entity to set the Label to yet
//                         // this is handled later in label_after_colon
//                     } break;
//                 }
//             } break;
//             default: {
//                 // in any other case, this is probably just a compile time variable declaration
//                 auto p = Place::create();
//                 l->entity = p;
//                 goto label_finished;
//             } break;
//         }
// label_finished:;
//     }
// }

/* @ascent

    The second stage of parsing: recursive ascent
    This probably needs renamed cause it doesn't really seem to be ascent anymore
    we are still going downwards and we typically know what we're parsing


    This stage parses smaller details of Code objects in a 
    mostly bottom up manner.
*/

namespace ascent {

b32 tuple(Code* code, code::TokenIterator& iter);
b32 label_after_colon(Code* code, code::TokenIterator& iter);
b32 label_group_after_comma(Code* code, code::TokenIterator& iter);
b32 label_after_id(Code* code, code::TokenIterator& iter);
b32 label_get(Code* code, code::TokenIterator& iter);
b32 label(Code* code, code::TokenIterator& iter);
b32 statement(Code* code, code::TokenIterator& iter);
b32 expression(Code* code, code::TokenIterator& iter);
b32 struct_decl(Code* code, code::TokenIterator& token);
b32 factor(Code* code, code::TokenIterator& token);
b32 conditional(Code* code, code::TokenIterator& token);
b32 logi_or(Code* code, code::TokenIterator& token);
b32 logi_and(Code* code, code::TokenIterator& token);
b32 bit_or(Code* code, code::TokenIterator& token);
b32 bit_xor(Code* code, code::TokenIterator& token);
b32 bit_and(Code* code, code::TokenIterator& token);
b32 equality(Code* code, code::TokenIterator& token);
b32 relational(Code* code, code::TokenIterator& token);
b32 bit_shift(Code* code, code::TokenIterator& token);
b32 additive(Code* code, code::TokenIterator& token);
b32 term(Code* code, code::TokenIterator& token);
b32 access(Code* code, code::TokenIterator& token);

b32 
reduce_literal_to_literal_expression(Code* code, code::TokenIterator& token) {
    Expr* e = Expr::create(expr::literal);
    e->node.start = e->node.end = token.current();
    switch(token.current_kind()) {
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
            e->type = StaticArray::create(&type::scalar::unsigned8, token.current()->raw.count);
        } break;
    }
    stack::push(code, e);
    return true;
}

b32 
reduce_builtin_type_to_typeref_expression(Code* code, code::TokenIterator& token) { 
    Expr* e = Expr::create(expr::typeref);
    e->node.start = e->node.end = token.current();
    switch(token.current_kind()) {
        case token::void_:      e->type = &type::void_; break; 
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
    stack::push(code, e);
    return true;
}

b32
typeref(Code* code, code::TokenIterator& token) {
    Token* start = token.current();
    switch(token.current_kind()) {
        case token::equal: {
            token.increment();
            if(!expression(code, token)) return false;

            Expr* e = Expr::create(expr::binary_assignment);
            e->node.start = start;
            e->node.end = token.current();
            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));
            // always take the type of the lhs because it controls the type of the rhs expression
            e->type = ((Expr*)e->node.first_child)->type;
            stack::push(code, e);
        } break;

        case token::open_brace: {
            TODO("type restricted blocks <typeref> \"{\" ... \"}\"");
        } break;

        case token::asterisk: {
            auto last = (Expr*)stack::last(code);
            last->type = Pointer::create(last->type);
            token.increment();
            if(!typeref(code, token)) return false;
        } break;
    }

    return true;
}

// expects to start at the opening brace and returns after the closing brace
// TODO(sushi) it may be easier (or necessary) to just do a scan to the matching brace
//             when parsing blocks, instead of the weird logic we try to do 
//             to allow omitting semicolons after labels that end with blocks 
b32
block(Code* code, code::TokenIterator& token) {
    auto e = Block::create();
    e->node.start = token.current();
    e->table.last = stack::current_table(code);
    stack::push_table(code, &e->table);

    token.increment();

    u32 count = 0;
    while(1) {
        b32 need_semicolon = true;
        b32 last_expr = false;
        if(token.is(token::close_brace)) break;
        if(token.is(token::semicolon)){ // empty statements are fine, but ignored
            token.increment(); 
            continue; 
        }
        Statement* s = statement::create();
        s->kind = statement::unknown;

        switch(token.current_kind()) {
            case token::identifier: {
                if(token.next_kind() == token::colon) {
                    if(!label(code, token)) return false;
                    s->kind = statement::label;
                    if(token.prev_is(token::close_brace)) {
                        need_semicolon = false;
                    }
                } else {
                    if(!expression(code, token)) return false;
                    s->kind = statement::expression;
                    // TODO(sushi) these rules are sketchy and i feel like they will break at some point 
                    if(token.prev_is(token::close_brace)) {
                        need_semicolon = false;
                        if(token.is(token::close_brace))
                            last_expr = true;
                    } else if(token.is(token::close_brace)) {
                        need_semicolon = false;
                        last_expr = true;
                    }
                }
            } break;

            default: {
                if(!expression(code, token)) return false;
                s->kind = statement::expression;
                // TODO(sushi) these rules are sketchy and i feel like they will break at some point 
                if(token.prev_is(token::close_brace)) {
                    need_semicolon = false;
                    if(token.is(token::close_brace))
                        last_expr = true;
                } else if(token.is(token::close_brace)) {
                    need_semicolon = false;
                    last_expr = true;
                }
            } break;
        }
        if(need_semicolon && !token.is(token::semicolon)) {
            if(!token.is(token::close_brace)) {
                diagnostic::parser::missing_semicolon(token.current());
                return false;
            }
            count++;
            node::insert_first(s, stack::pop(code));
            s->node.start = s->node.first_child->start;
            s->node.end = token.current();
            s->kind = statement::block_final;
            stack::push(code, s);
            break;
        } else {
            node::insert_first(s, stack::pop(code));
            s->node.start = s->node.first_child->start;
            s->node.end = token.current();

            count++;
            stack::push(code, s);
            
            if(last_expr) {
                s->kind = statement::block_final;
                break;
            } else if(need_semicolon) token.increment();
        }
    }

    forI(count) {
        node::insert_first(e, stack::pop(code));
    }
    e->node.end = token.current();
    token.increment();
    stack::push(code, e);
    stack::pop_table(code);
    return true;
}


b32
factor(Code* code, code::TokenIterator& token) {
    switch(token.current_kind()) {
        case token::identifier: {
            Label* l = symbol::search(code, token.current()->hash);
            if(!l) {
                diagnostic::parser::unknown_identifier(token.current());
                return false;
            }

            switch(l->entity->node.kind) {
                case node::place: {
                    auto e = PlaceRef::create();
                    e->node.start = e->node.end = token.current();
                    e->place = (Place*)l->entity;
                    token.increment();
                    stack::push(code, e);
                } break;
                case node::function: {
                    if(token.next_is(token::open_paren)) {
                        // must be a function call
                        auto id = Expr::create(expr::identifier);
                        id->node.start = id->node.end = token.current();
                        auto call = Call::create();
                        call->node.start = token.current();
                        call->callee = (Function*)l->entity;
                        token.increment();
                        if(!tuple(code, token)) return false;
                        call->node.end = token.current();
                        call->arguments = (Tuple*)stack::pop(code);
                        node::insert_last(call, id);
                        node::insert_last(call, call->arguments);
                        stack::push(code, call);
                    } else {
                        TODO("plain function reference");
                        NotImplemented;
                    }
                } break;
                default: {
                    util::println(dstring::init(
                        token.current(), "unhandled identifier reference: ", node::strings[l->entity->node.kind]));
                } break;
            }

        } break;
        
        case token::open_paren: {
            if(!tuple(code, token)) return false;
        } break;

        case token::if_: {
            if(!conditional(code, token)) return false;
        } break;

        case token::switch_: {
            TODO("switch parsing");
        } break;

        case token::for_: {
            TODO("for parsing");
        } break;

        case token::open_brace: {
            if(!block(code, token)) return false;
        } break;

        case token::ampersand: {
            Expr* e = Expr::create(expr::unary_reference);
            e->node.start = token.current();

            token.increment();
            if(!factor(code, token)) return false;

            node::insert_last(e, stack::pop(code));
            e->type = Pointer::create(Type::resolve(e->node.last_child));
            e->node.end = e->node.last_child->end;
            stack::push(code, e);
        } break;

        default: {
            if(token.current()->group == token::group_literal) {
                reduce_literal_to_literal_expression(code, token);
                token.increment();
            } else if(token.current()->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression(code, token);
                token.increment();
            } else {
                diagnostic::parser::unexpected_token(token.current(), token.current());
                return false;
            }
        } break;
    }
    return true;
}

b32
conditional(Code* code, code::TokenIterator& token) {
    Expr* e = Expr::create(expr::conditional);
    e->node.start = token.current();

    token.increment();

    if(!token.is(token::open_paren)) {
        diagnostic::parser::
            if_missing_open_paren(token.current());
        return false;
    }

    token.increment();

    if(!expression(code,token)) return false;

    if(!token.is(token::close_paren)) {
        diagnostic::parser::
            if_missing_close_paren(token.current());
        return false;
    }

    token.increment();

    if(!expression(code, token)) return false;

    node::insert_first(e, stack::pop(code));
    node::insert_first(e, stack::pop(code));

    stack::push(code, e);

    if(token.is(token::else_)) {
        token.increment();
        if(!expression(code, token)) return false;
        node::insert_last(e, stack::pop(code));
    }

    e->node.end = token.current();

    return true;
}

b32
assignment(Code* code, code::TokenIterator& token) {
    if(token.is(token::equal)) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        if(!bit_and(code, token)) return false;
        if(!bit_xor(code, token)) return false;
        if(!bit_or(code, token)) return false;
        if(!logi_and(code, token)) return false;
        if(!logi_or(code, token)) return false;

        Expr* e = Expr::create(expr::binary_assignment);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;

        stack::push(code, e);

        if(!assignment(code, token)) return false;
    }
    return true;
}

/*
    logi-or: logi-and { "||" logi-and }
*/
b32 logi_or(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::logi_or) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        if(!bit_and(code, token)) return false;
        if(!bit_xor(code, token)) return false;
        if(!bit_or(code, token)) return false;
        if(!logi_and(code, token)) return false;
        auto e = Expr::create(expr::binary_or);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
        stack::push(code, e);

        if(!logi_or(code, token)) return false;
    }
    return true;
}

/*
    logi-and: bit-or { "&&" bit-or }
*/
b32 logi_and(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::double_ampersand) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        if(!bit_and(code, token)) return false;
        if(!bit_xor(code, token)) return false;
        if(!bit_or(code, token)) return false;
        auto e = Expr::create(expr::binary_and);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
        stack::push(code, e);

        if(!logi_and(code, token)) return false;
    }
    return true;
}

/*
    bit-or: bit-xor { "|" bit-xor }
*/
b32 bit_or(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::vertical_line) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        if(!bit_and(code, token)) return false;
        if(!bit_xor(code, token)) return false;
        auto e = Expr::create(expr::binary_bit_or);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
        stack::push(code, e);

        if(!bit_or(code, token)) return false;
    }
    return true;
}

/*
    bit-xor: bit-and { "^" bit-and }
*/
b32 bit_xor(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::caret) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        if(!bit_and(code, token)) return false;
        auto e = Expr::create(expr::binary_bit_xor);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
        stack::push(code, e);

        if(!bit_xor(code, token)) return false;
    }
    return true;
}


/*
    bit-and: equality { "&" equality } 
*/
b32 bit_and(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::ampersand) {
        token.increment();
        if(!factor(code, token)) return false;
        if(!access(code, token)) return false;
        if(!term(code, token)) return false;
        if(!additive(code, token)) return false;
        if(!bit_shift(code, token)) return false;
        if(!relational(code, token)) return false;
        if(!equality(code, token)) return false;
        auto e = Expr::create(expr::binary_bit_and);

        node::insert_first(e, stack::pop(code));
        node::insert_first(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
        stack::push(code, e);

        if(!bit_and(code, token)) return false;
    }
    return true;
}

/*
    equality: relational { ( "!=" | "==" ) relational }
*/
b32 equality(Code* code, code::TokenIterator& token) { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::double_equal:
        case token::explanation_mark_equal: {
            token.increment();
            if(!factor(code, token)) return false;
            if(!access(code, token)) return false;
            if(!term(code, token)) return false;
            if(!additive(code, token)) return false;
            if(!bit_shift(code, token)) return false;
            if(!relational(code, token)) return false;
            auto e = Expr::create(
                kind == token::double_equal 
                    ? expr::binary_equal 
                    : expr::binary_not_equal);

            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));

            e->node.start = e->node.first_child->start;
            e->node.end = e->node.last_child->end;
            stack::push(code, e);

            if(!equality(code, token)) return false;
        } break;
    }
    return true;
}

/*
    relational: bit-shift { ( ">" | "<" | "<=" | ">=" ) bit-shift }
*/
b32 relational(Code* code, code::TokenIterator& token) { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::less_than:
        case token::less_than_equal:
        case token::greater_than:
        case token::greater_than_equal: {
            token.increment();
            if(!factor(code, token)) return false;
            if(!access(code, token)) return false;
            if(!term(code, token)) return false;
            if(!additive(code, token)) return false;
            if(!bit_shift(code, token)) return false;
            auto e = Expr::create(
                kind == token::less_than ?
                      expr::binary_less_than :
                      kind == token::less_than_equal ? 
                      expr::binary_less_than_or_equal :
                      kind == token::greater_than ? 
                      expr::binary_greater_than :
                      expr::binary_greater_than_or_equal);

            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));

            e->node.start = e->node.first_child->start;
            e->node.end = e->node.last_child->end;
            stack::push(code, e);

            if(!relational(code, token)) return false;
        } break;
    }
    return true;
}

/*
    bit-shift: additive { "<<" | ">>" additive }
*/
b32 bit_shift(Code* code, code::TokenIterator& token) { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::double_less_than: 
        case token::double_greater_than: {
            token.increment(); 
            if(!factor(code, token)) return false;
            if(!access(code, token)) return false;
            if(!term(code, token)) return false;
            if(!additive(code, token)) return false;
            auto e = Expr::create(
                kind == token::double_less_than ? 
                      expr::binary_bit_shift_left :
                      expr::binary_bit_shift_right);
            
            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));

            e->node.start = e->node.first_child->start;
            e->node.end = e->node.last_child->end;
            stack::push(code, e);

            if(!bit_shift(code, token)) return false;
        } break;
    }
    return true;
}

/*
    additive: term * { ("+" | "-" ) term }
*/
b32 additive(Code* code, code::TokenIterator& token) { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::plus:
        case token::minus: {
            token.increment();
            if(!factor(code, token)) return false;
            if(!access(code, token)) return false;
            if(!term(code, token)) return false;
            auto e = Expr::create(
                kind == token::plus 
                    ? expr::binary_plus 
                    : expr::binary_minus);

            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));

            e->node.start = e->node.first_child->start;
            e->node.end = e->node.last_child->end;
            stack::push(code, e);

            if(!additive(code, token)) return false;
        } break;
    }
    return true;
}

/*
    term: access * { ( "*" | "/" | "%" ) access }
*/
b32 term(Code* code, code::TokenIterator& token) { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::percent: 
        case token::solidus: 
        case token::asterisk: {
            token.increment();
            if(!factor(code, token)) return false;
            if(!access(code, token)) return false;
            auto e = Expr::create(
                kind == token::percent ? expr::binary_modulo : 
                kind == token::solidus ? expr::binary_division : 
                    expr::binary_multiply);
            
            node::insert_first(e, stack::pop(code));
            node::insert_first(e, stack::pop(code));

            e->node.start = e->node.first_child->start;
            e->node.end = e->node.last_child->end;
            stack::push(code, e);

            if(!term(code, token)) return false;
        } break;
    }
    return true;
}

/*
    access: factor * { "." factor }
*/
b32 access(Code* code, code::TokenIterator& token) { 
    if(token.current_kind() == token::dot) {
        token.increment();
        // when we parse accesses, we don't care about figuring out if the identifier is 
        // correct because we don't know what is being accessed and even if we did, it would
        // possibly not have been parsed yet 
        if(token.current_kind() != token::identifier) {
            diagnostic::parser::
                expected_identifier(token.current());
            return false;
        }

        auto id = Expr::create(expr::identifier);
        id->node.start = id->node.end = token.current();

        auto e = Expr::create(expr::binary_access);
        node::insert_first((TNode*)e, stack::pop(code));
        node::insert_first((TNode*)e, id);
        stack::push(code, e);
        token.increment();
        if(!access(code, token)) return false;
    }
    return true;
}

b32
expression(Code* code, code::TokenIterator& token) {
    switch(token.current_kind()) {
        case token::equal: {
            auto e = Expr::create(expr::unary_assignment);
            e->node.start = token.current();

            token.increment();
            if(!expression(code, token)) return false;

            node::insert_first(e, stack::pop(code));
            e->node.end = e->node.last_child->end;

            stack::push(code, e);
        } break;

        case token::colon: {
            auto e = Expr::create(expr::unary_comptime);
            e->node.start = token.current();

            token.increment();

            if(!expression(code, token)) return false;

            node::insert_first(e, stack::pop(code));
            e->node.end = e->node.last_child->end;
            e->type = ((Expr*)e->node.first_child)->type;

            TODO("implement comptime code funneling");

            stack::push(code, e);
        } break;

        case token::structdecl: {
            if(!struct_decl(code, token)) return false;
        } break;

        case token::moduledecl: {
            TODO("module decl parsing");
        } break;

        case token::return_: {
            auto e = Expr::create(expr::return_);
            e->node.start = token.current();

            token.increment();
            if(!expression(code, token)) return false;

            node::insert_first(e, stack::pop(code));
            e->node.end = e->node.last_child->end;

            stack::push(code, e);
        } break;

        case token::using_: {
            TODO("using expression parsing");
        } break;

        default: if(!factor(code, token)) return false; 
    }

     // loop and see if any operators are being used, if so call their entry point
    b32 search = true;
    while(search) {
        switch(token.current_kind()) {
            case token::dot: if(!access(code, token)) return false; break;
            case token::asterisk:
            case token::solidus: if(!term(code, token)) return false; break;
            case token::plus:
            case token::minus: if(!additive(code, token)) return false; break;
            case token::double_less_than:
            case token::double_greater_than: if(!bit_shift(code, token)) return false; break;
            case token::double_equal: 
            case token::explanation_mark_equal: if(!equality(code, token)) return false; break;
            case token::less_than:
            case token::less_than_equal:
            case token::greater_than:
            case token::greater_than_equal: if(!relational(code, token)) return false; break;
            case token::ampersand: if(!bit_and(code, token)) return false; break;
            case token::caret: if(!bit_xor(code, token)) return false; break;
            case token::vertical_line: if(!bit_or(code, token)) return false; break;
            case token::double_ampersand: if(!logi_and(code, token)) return false; break;
            case token::logi_or: if(!logi_or(code, token)) return false; break;
            case token::equal: if(!assignment(code, token)) return false; break;
            //case token::open_brace: advance_curt(); if(!block(code, token)) return false; break;
            default: search = false;
        }
    }

    return true;
}

// expects to be started at the opening parenthesis and returns 
// at the token following the closing parenthesis
// if the tuple is followed by '->', then this must be a function type
// that may be followed by a definition of a function. 
// In the case of a function definition the token is left at
// the closing brace
b32
tuple(Code* code, code::TokenIterator& token) {
    Tuple* tuple = tuple::create();
    tuple->node.start = token.current();

    token.increment();

    u32 count = 0;
    u32 found_label = 0;
    while(1) {
        if(token.is(token::close_paren)) break;
        switch(token.current_kind()) {
            case token::identifier: { // need to determine if this is a label or expression
                switch(token.next_kind()) {
                    case token::colon: {
                        if(!found_label) {
                            tuple->table.last = stack::current_table(code);
                            stack::push_table(code, &tuple->table);
                            found_label = 1;
                        }
                        if(!label(code, token)) return false;
                    } break;
                    default: {
                        if(found_label) {
                            diagnostic::parser::tuple_positional_arg_but_found_label(token.current());
                            return false;
                        }
                        if(!expression(code, token)) return false;
                    } break;
                }
            } break;
            default: {
                if(!expression(code, token)) return false;
            } break;
        }
        count += 1;
        if(token.is(token::comma)) token.increment();
        else if(!token.is(token::close_paren)) {
            diagnostic::parser::
                tuple_expected_comma_or_close_paren(token.current());
            return false;
        }
    }

    if(found_label)
        stack::pop_table(code);
    

    forI(count) 
        node::insert_first(tuple, stack::pop(code));

    tuple->node.end = token.current();

    token.increment();

    if(!token.is(token::function_arrow)){
        stack::push(code, tuple);
        return true;
    } 

    // this must be a function type
    token.increment();

    count = 0;
    while(1) {
        if(!factor(code, token)) return false;
        count++;
        if(!token.is(token::comma)) break;
        token.increment();
    }

    if(!count) {
        diagnostic::parser::missing_function_return_type(token.current());
        return false;
    }

    Expr* e = Expr::create(expr::typeref);

    if(count > 1) {
        Tuple* t = tuple::create();
        t->kind = tuple::multireturn;

        ScopedArray<Type*> types = array::init<Type*>();

        forI(count) {
            auto n = (Type*)stack::pop(code);
            array::push(types, n);
            node::insert_first(t, n);
        }

        node::insert_last(e, t);

        t->node.start = t->node.first_child->start;
        t->node.end = t->node.last_child->start;
    } else {
        auto t = (Type*)stack::pop(code);
        node::insert_first(e, t);
    }

    // push the argument tuple under the typeref we will be returning
    // and load its table onto the table stack for the block we're about to parse
    node::insert_first(e, tuple);

    if(token.is(token::open_brace)) {
        // the symbol table of the block will go through the tuple's symbol table, regardless of if
        // it generated one or not 
        tuple->table.last = stack::current_table(code);
        stack::push_table(code, &tuple->table);

        if(!block(code, token)) return false;

        stack::pop_table(code);

        node::insert_last(e, stack::pop(code));

        e->node.start = e->node.first_child->start;
        e->node.end = e->node.last_child->end;
    }

    auto ft = FunctionType::create();
    ft->parameters = e->node.first_child;
    ft->returns = e->node.first_child->next;
    ft->return_type = Type::resolve(ft->returns); // kind of redundant 
    e->type = (Type*)ft;

    stack::push(code, e);

    return true;
}

b32 
label_after_colon(Code* code, code::TokenIterator& token) {
    switch(token.current_kind()) {
        case token::identifier: {
            // this should be a typeref 
            Label* l = symbol::search(code, token.current()->hash);
            if(!l) {
                diagnostic::parser::
                    unknown_identifier(token.current());
                return false;
            }

            switch(l->entity->node.kind) {
                case node::type: {
                    Expr* e = Expr::create(expr::typeref, (Type*)l->entity);
                    e->node.start = e->node.end = token.current();
                    stack::push(code, e);

                    token.increment();
                    if(!typeref(code, token)) return false;
                } break;
            }
        } break;

        case token::colon:
        case token::equal: {
            if(!expression(code, token)) return false;
        } break;

        case token::open_paren: {
            if(!tuple(code, token)) return false;
        } break;

        default: {
            if(token.current()->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression(code, token);
                token.increment();
                typeref(code, token);
            } else {
                diagnostic::parser::unexpected_token(token.current(), token.current());
                return false;
            }
        } break;
    }
    return true;
}

b32 
label_group_after_comma(Code* code, code::TokenIterator& token) {
     while(1) {
        if(token.current_kind() != token::identifier) break; 
        Expr* expr = Expr::create(expr::identifier);
        expr->node.start = expr->node.end = token.current();
        
        TNode* last = stack::last(code);
        if(last->kind == node::tuple) {
            // a label group was already created, so just append to it
            node::insert_last(last, (TNode*)expr);
            last->end = token.current();
        } else {
            // this is the second label, so the last must be another identifier
            // make the label group tuple
            Tuple* group = tuple::create();
            group->kind = tuple::label_group;
            node::insert_last((TNode*)group, stack::pop(code));
            node::insert_last((TNode*)group, (TNode*)expr);
            group->node.start = group->node.first_child->start;
            group->node.end = group->node.last_child->end;
            stack::push(code, (TNode*)group);
        }
        
        token.increment();
        
        if(token.current_kind() == token::comma) token.increment(); 
        else break;
    }

    if(token.prev_kind() == token::comma) {
        diagnostic::parser::
            label_group_missing_id(token.current());
    }

    if(token.current_kind() == token::colon) {
        if(!label_after_colon(code, token)) return false;
    } else {
        diagnostic::parser::
            label_missing_colon(token.current());
        return false;
    }
    return true;
}

// figure out if this is a single label or a group
b32
label_after_id(Code* code, code::TokenIterator& token) {
    // switch(token.current_kind()) {
    //     case token::colon:  else break;
    //     case token::comma: if(!token.increment()) return false; else if(!label_group_after_comma(code, token)) return false; else break;
    // }
    NotImplemented;
    return true;
}

// just parses <id> { "," <id> } : and pushes a label on the stack representing it 
// returns with the tokenator at the token following the colon
b32 
label_get(Code* code, code::TokenIterator& token) {
    Expr* expr = Expr::create(expr::identifier);
    expr->node.start = token.current();
    expr->node.end = token.current();
    stack::push(code, (TNode*)expr);

    token.increment();
    switch(token.current_kind()) {
        case token::colon: break;
        case token::comma: {
            token.increment();
            if(!label_group_after_comma(code, token)) return false;
        } break;
        default: {
            diagnostic::parser::expected_colon_for_label(token.current());
            return false;
        } break;
    }

    Label* l = label::create();
    l->node.start = expr->node.start;
    node::insert_first(l, stack::pop(code));
    stack::push(code, l);
    token.increment();
    return true;
}

// parses a label and whatever comes after it 
b32
label(Code* code, code::TokenIterator& token) {
    if(!label_get(code, token)) return false;
    
    Label* l = (Label*)stack::pop(code);
    table::add(code, l->node.start->raw, l);
    
    if(!label_after_colon(code, token)) return false;

    TNode* cand = stack::pop(code);
    switch(cand->kind) {
        case node::expression: {
            auto expr = (Expr*)cand;
            switch(expr->kind) {
                case expr::unary_assignment: {
                    auto p = Place::create();
                    p->label = l;
                    p->type = expr->type;
                    l->entity = (Entity*)p;
                    node::insert_last(l, cand);
                } break;
                case expr::unary_comptime: {
                    // it's possible this a function def
                    switch(expr->type->kind) {
                        case type::kind::function: {
                            auto f = Function::create();
                            f->label = l;
                            f->type = (FunctionType*)expr->type;
                            f->node.start = f->type->parameters->start;
                            l->entity = (Entity*)f;
                            node::insert_last(l, cand);
                        } break;
                    }
                } break;
                case expr::binary_assignment: {
                    auto p = Place::create();
                    p->label = l;
                    l->entity = (Entity*)p;
                    node::insert_last(l, cand);
                } break;
                case expr::typeref: {
                    // this is probably when we have 
                    // <id> : <type> ;
                    Type* t = expr->type;
                    switch(t->kind) {
                        case type::kind::scalar: {
                            auto p = Place::create();
                            p->label = l;
                            p->type = t;
                            l->entity = (Entity*)p;
                            node::insert_last(l, cand);
                        } break;

                        case type::kind::structured: {
                            auto p = Place::create();
                            p->label = l;
                            p->type = t;
                            l->entity = (Entity*)p;
                            node::insert_last(l, cand);
                        } break;

                        case type::kind::function: {
                            // if the last child of the typeref is not a block, then
                            // we do not create a function entity, this is simply a variable
                            // representing a function
                            if(cand->last_child->kind != node::expression || 
                               ((Expr*)cand->last_child)->kind != expr::block) {
                                auto p = Place::create();
                                p->label = l;
                                p->type = t;
                                l->entity = (Entity*)p;
                                node::insert_last(l, cand);
                            } else {
                                auto f = Function::create();
                                f->label = l;
                                f->type = (FunctionType*)t;
                                f->node.start = f->type->parameters->start;
                                l->entity = (Entity*)f;
                                node::insert_last(l, cand);
                            }
                        } break;
                    }
                } break;
                default: {
                    util::println(
                        dstring::init("unhandled label case: ", expr::strings[expr->kind]));
                    return false;
                }
            }
        } break;
        case node::statement: {
            DebugBreakpoint;
        } break;
        default: {
            util::println(
                    dstring::init("unhandled label case: ", node::strings[cand->kind]));
            return false;
        }
    }

    l->node.start = l->node.first_child->start;
    l->node.end = l->node.last_child->end;

    stack::push(code, l);
    return true;
}

/*  parses a structure definition
    expected to be started at the 'struct' token
    leaves each declaration inside the struct on a typeref expression
    for example, the struct 
    
    struct {
        a: u32;
        b: u32;
    }

    will leave

    (expr:typeref
        (label<'a'> ...)
        (label<'b'> ...))

    on the stack
*/
b32 
struct_decl(Code* code, code::TokenIterator& token) {
    token.increment();

    if(!token.is(token::open_brace)) {
        diagnostic::parser::
            missing_open_brace_for_struct(token.current());
        return false;
    }

    token.increment();

    auto e = Expr::create(expr::typeref);
    
    while(1) {
        if(token.is(token::close_brace)) break;
        if(token.is(token::identifier)) {
            if(!label(code, token)) return false;
            if(!token.is(token::semicolon)) {
                diagnostic::parser::
                    missing_semicolon(token.current());
                return false;
            }
            token.increment();
            if(stack::last(code)->kind == node::function) {
                diagnostic::parser::
                    struct_member_functions_not_allowed(stack::last(code)->start);
                return false;
            }
        } else {
            diagnostic::parser::
                struct_only_labels_allowed(token.current());
            return false;
        }
        node::insert_last(e, stack::pop(code));
    }

    stack::push(code, e);
    return true;
}

b32
prescanned_type(Code* code, code::TokenIterator& token) {
    Label* l = (Label*)code->parser->root;
    Type* type = (Type*)l->entity;

    switch(type->kind) {
        case type::kind::structured: {
            auto stype = (Structured*)type;
            Structure* s = stype->structure;
            
            token.skip_until(token::structdecl);
            if(!struct_decl(code, token)) return false;

            stack::push_table(code, &s->table);

            auto e = (Expr*)stack::pop(code);
            for(TNode* n = e->node.first_child; n; n = n->next) {
                auto l = (Label*)e->node.first_child;
                table::add(code, l->node.start->raw, l);
            }
            // reuse the typeref to place in the AST 
            e->type = type;
            node::insert_last(l, e);
        } break;
    }
    return true;
}

// parses a function definition, a function type followed by a block
// expects to be started at the opening parenthesis of the parameter tuple
// and returns with the token after the block's close brace
// leaves a typeref expression on the stack with the form:
// (expr:typeref
//      tuple         -- function parameters
//      tuple/typeref -- return information
//      block)        -- definition
// b32
// function(Code* code, code::TokenIterator& token) {
//     if(!tuple(code, token)) return false;
    
   

//     return true;
// }

// this function is called from ascent::start, so it only parses functions that 
// have been prescanned by the first stage of parsing. A Function entity and Label
// will have already been created
b32
prescanned_function(Code* code, code::TokenIterator& token) {
    Label* l = (Label*)code->parser->root;
    Function* f = (Function*)l->entity;

    f->type = FunctionType::create();

    token.skip_until(token::open_paren);

    if(!tuple(code, token)) return false;

    auto e = (Expr*)stack::pop(code);

    f->type = (FunctionType*)e->type;

    node::insert_last(l, e);

    return true;
}

b32
statement(Code* code, code::TokenIterator& token) {
    Statement* s = statement::create();
    switch(token.current_kind()) {
        case token::identifier: {
            if(token.next_kind() == token::colon) {
                if(!label(code, token)) return false;
                s->kind = statement::label;
                if(!token.is(token::semicolon)) {  
                    if(!token.is(token::close_brace)) {
                        diagnostic::parser::missing_semicolon(token.current());
                        return false;
                    }
                }
            } else {
                if(!expression(code, token)) return false;
                s->kind = statement::expression;
            }
        } break;
    }
    return true;
}

b32
start(Code* code) {
    Assert(code->parser, "a parser must have been made for a Code object created in the first stage.");
    switch(code->kind) {
        case code::function: {
            code::TokenIterator token(code);
            if(!prescanned_function(code, token)) return false;
        } break;
        case code::typedef_: {
            code::TokenIterator token(code);
            if(!prescanned_type(code, token)) return false;
        } break;
        case code::source: {
            for(TNode* n = code->node.first_child; n; n = n->next) {
                Code* c = (Code*)n;
                if(!start(c)) return false;
                node::insert_last(code->parser->root, c->parser->root);
            }
            // when we're done, we need to join all of the children's nodes into the source's
            
        } break;
    }
    return true;
}

} // namespace ascent

/* @descent
    Functions relating to the first stage of parsing: recursive descent

    In this stage we generate an overview of the structure of the given Code object
    which involves creating sub Code objects representing entities such as modules,
    typedefs, functions, and variables.

    These Code objects are then parsed in the recursive ascent stage in a depth first order.
*/
namespace descent {

b32 expression(Code* code, code::TokenIterator& token);
b32 label(Code* code, code::TokenIterator& token);
b32 module(Code* code, code::TokenIterator& token);
b32 source(Code* code, code::TokenIterator& token);

b32
label(Code* code, code::TokenIterator& token) {
    Token* id = token.current();
    switch(token.increment()->kind) {
        case token::colon: {
            switch(token.increment()->kind) {
                case token::colon: {
                    switch(token.increment()->kind) {
                        case token::open_paren: {
                            Token* open_paren = token.current(); 
                            token.skip_to_matching_pair();
                            switch(token.increment()->kind) {
                                case token::function_arrow: {
                                    token.skip_until(token::open_brace, token::semicolon);
                                    if(token.current_kind() == token::open_brace) {
                                        // this is a function definition, so we segment it into its own
                                        // Code object, create a Label and Function for it, construct the AST
                                        // for the Label and then push it onto whatever table we are 
                                        // currently working with
                                        token.skip_to_matching_pair();

                                        Code* nu = code::from(code, id, token.current());
                                        nu->kind = code::function;

                                        auto f = Function::create();
                                        f->node.start = open_paren;
                                        f->node.end = token.current();

                                        Label* l = label::create();
                                        l->node.start = id;
                                        l->node.end = token.current();

                                        l->entity = (Entity*)f;
                                        f->label = l;

                                        auto e = Expr::create(expr::identifier);
                                        e->node.start = e->node.end = id;

                                        node::insert_first(l, e);
                                        
                                        nu->parser = parser::create(nu);
                                        nu->parser->root = (TNode*)l;

                                        table::add(code, id->raw, l);

                                        node::insert_last(code, nu);
                                    }
                                } break;
                                case token::semicolon: {
                                    // TODO(sushi) tuple constants
                                    NotImplemented;
                                } break;
                            }
                        } break;
                        case token::structdecl: {
                            if(token.increment()->kind != token::open_brace) {
                                diagnostic::parser::expected_open_brace(token.current());
                                return false;
                            }
                            Token* start = token.current();
                            token.skip_to_matching_pair();
                            
                            Code* nu = code::from(code, id, token.current());
                            nu->kind = code::typedef_;

                            Label* l = label::create();
                            l->node.start = id;
                            l->node.end = token.current();

                            auto s = Structure::create();
                            s->node.start = start;
                            s->node.end = token.current();

                            auto stype = Structured::create(s);
                            stype->node.start = s->node.start;
                            stype->node.end = s->node.end;

                            l->entity = (Entity*)stype;
                            stype->label = l;

                            auto e = Expr::create(expr::identifier);
                            e->node.start = e->node.end = id;

                            node::insert_first(l, e);

                            nu->parser = parser::create(nu);
                            nu->parser->root = (TNode*)l;

                            table::add(code, id->raw, l);

                            node::insert_last(code, nu);
                        } break;
                        case token::moduledecl: {
                            if(token.increment()->kind != token::open_brace) {
                                diagnostic::parser::expected_open_brace(token.current());
                                return false;
                            }
                            token.skip_to_matching_pair();
                            Code* nu = code::from(code, id, token.current());
                            nu->kind = code::module;
                            node::insert_last((TNode*)code, (TNode*)nu);
                            auto temp = code::TokenIterator(code);
                            module(nu, temp);
                        } break;
                        default: {
                            // this must be a compile time var declaration, so parse for an expression

                        } break;
                    }
                } break;
                case token::open_paren: {
                    // TODO(sushi) segment function variables
                    NotImplemented;
                } break;
            }
        } break;
        case token::comma: {
            // TODO(sushi) segmenting for multi labels
            NotImplemented; 
        } break;
    }
    return true;
}

b32 
module(Code* code, code::TokenIterator& token) {
    auto m = Module::create();
    m->table.last = code->parser->current_table;
    stack::push_table(code, &m->table);
    defer { stack::pop_table(code); };

    while(1) {
        switch(token.current_kind()) {
            case token::identifier: label(code, token); break;
            default: return false;
        }
    }
}

b32
source(Code* code, code::TokenIterator& token) {
    // all source files are modules, so we create one for it here
    auto m = Module::create();
    stack::push_table(code, &m->table);
    defer { stack::pop_table(code); };

    code->parser->root = (TNode*)m;

    while(1) {
        switch(token.current_kind()) {
            case token::identifier: {
                label(code, token);
            } break;
        }
        if(token.increment()->kind == token::end_of_file) return true;
    }
}

b32
start(Code* code) {
    switch(code->kind) {
        case code::source: {
            code::TokenIterator token(code);
            source(code, token);
        } break;
        case code::function: {
            // code::TokenIterator token = code::token_tokenator(code);
            // function(code, token);
        } break;
        case code::statement: {

        } break;
    }
    return true;
}

} // namespace decent

#undef announce_stage

namespace stack {

void
push(Code* c, TNode* node) {
    array::push(c->parser->stack, c->parser->last);
    c->parser->last = node;
}

TNode*
pop(Code* c) {
    TNode* out = c->parser->last;
    c->parser->last = array::pop(c->parser->stack);
    return out;
}

TNode*
last(Code* c) {
    return c->parser->last;
}

void
push_table(Code* c, LabelTable* table) {
    array::push(c->parser->table_stack, c->parser->current_table);
    c->parser->current_table = table;
}

void
pop_table(Code* c) {
    c->parser->current_table = array::pop(c->parser->table_stack);
}

LabelTable*
current_table(Code* c) {
    return c->parser->current_table;
}

DString
display(Code* c) {
    if(!c->parser->stack.count) return dstring::init("empty stack");
    DString out = dstring::init("stack of ", c->name, ":\n");
    forI(c->parser->stack.count) {
        if(!i) continue; // first element is always 0 due to storing the last element separate
        TNode* n = array::read(c->parser->stack, i);
        to_string(out, n, true);
        dstring::append(out, "\n");
    }
    to_string(out, c->parser->last, true);
    return out;
}

} // namespace stack

namespace table {

void 
add(Code* c, String id, Label* l) {
    map::add(c->parser->current_table->map, id, l);
}

} // namespace table 

namespace symbol {

Label*
search(Code* code, u64 hashed_id) {
    LabelTable* table = stack::current_table(code);
   
    while(table) {
         forI(table->map.keys.count) {
            u64 hash = array::read(table->map.keys, i).hash;
            util::println(to_string(hash));
        }
        auto [idx, found] = map::find(table->map, hashed_id);
        if(found) {
            return array::read(table->map.values, idx);
        }
        table = table->last;
    }
    return 0;
}

} // namespace symbol


void
parse(Code* code) {
    //util::Stopwatch parser_time = util::stopwatch::start();

    // messenger::dispatch(message::attach_sender(code,
    //     message::make_debug(message::verbosity::stages,
    //         String("beginning syntactic analysis"))));

    if(!code->parser) {
        code->parser = create(code);
    }

    descent::start(code); 
    ascent::start(code);

    util::println(node::util::print_tree(code->parser->root));

    // internal::parser = code->parser;
    // internal::push_module(code->source->module);
    // internal::stack = array::init<TNode*>(32);
    // internal::prescan();
    // internal::curt = array::readptr(code::get_token_array(code), 0);
    // internal::start();

    // util::println(
    //     node::util::print_tree<[](DString& c, TNode* n){to_string(c, n, true);}>(internal::stack.data[0]));
    
    // !Leak: a DString is generated by util::format_time, but messenger currently has no way to know 
	//        if a String needs to be cleaned up or not 
    // messenger::dispatch(message::attach_sender(code,
    //     message::make_debug(message::verbosity::stages, 
    //         String("syntactic analysis finished in "), String(util::format_time(util::stopwatch::peek(parser_time))))));
}

} // namespace parser
} // namespace amu