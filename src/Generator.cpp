namespace amu {
namespace gen {

Gen*
create(Code* code) {
    Gen* out = pool::add(compiler::instance.storage.gens);
    out->tac_pool = pool::init<TAC>(128);
    out->tac = array::init<TAC*>();
    code->gen = out;
    return out;
}

TAC*
add_tac(Gen* gen) {
    TAC* out = pool::add(gen->tac_pool);
    out->id = gen->tac_count++;
    return out;
}

namespace internal {

// expressions use this to determine if they need to consume an Arg or not
// since Expressions can give standalone arguments, unlike other entities
struct ExprRet {
    b32 is_tac;
    Arg arg;
    Array<TAC*> assignment_backfill;
};

b32 label(Label* l, u64 stack_offset);
void block(Code* code, BlockExpression* e);

ExprRet
expression(Code* code, Expression* e) {
    ExprRet out = {}; 
    switch(e->kind) {
        case expression::placeref: {
            auto pr = (PlaceRefExpression*)e;
            out.arg.kind = tac::arg::place;
            out.arg.place = pr->place;
        } break;
        case expression::binary_plus: {
            ExprRet lhs = expression(code, (Expression*)e->node.first_child);
            ExprRet rhs = expression(code, (Expression*)e->node.last_child);

            TAC* add = add_tac(code->gen);
            add->op = tac::op::addition;
            out.is_tac = true;

            if(!lhs.is_tac) add->arg0 = lhs.arg;
            else {
                add->arg0.kind = tac::arg::temporary;
                add->arg0.temporary = array::read(code->gen->tac, -1);
            }

            if(!rhs.is_tac) add->arg1 = rhs.arg;
            else{
                add->arg1.kind = tac::arg::temporary;
                add->arg1.temporary = array::read(code->gen->tac, -1);
            }  

            array::push(code->gen->tac, add);
        } break;
        case expression::cast: {
            return expression(code, (Expression*)e->node.first_child);
        } break;
        case expression::literal: {
            TODO("handle literals other than unsigned ints");
            out.arg.kind = tac::arg::literal;
            out.arg.literal = e->node.start->u64_val;
        } break;
        case expression::binary_assignment: {
            out.is_tac = true;
            
            ExprRet lhs = expression(code, (Expression*)e->node.first_child);
            ExprRet rhs = expression(code, (Expression*)e->node.last_child);

            TAC* tac = add_tac(code->gen);
            tac->op = tac::assignment;

            if(!lhs.is_tac) tac->arg0 = lhs.arg;
            else {
                tac->arg0.kind = tac::arg::temporary;
                tac->arg0.temporary = array::read(code->gen->tac, -1);
            } 

            if(!rhs.is_tac) tac->arg1 = rhs.arg;
            else {
                tac->arg1.kind = tac::arg::temporary;
                tac->arg1.temporary = array::read(code->gen->tac, -1);
            }

            array::push(code->gen->tac, tac);
        } break;
        case expression::unary_assignment: {
            // what this is being used for is handled by whatever called this
            return expression(code, (Expression*)e->node.last_child);
        } break;
        case expression::call: {
            out.is_tac = true;

            ScopedArray<ExprRet> returns = array::init<ExprRet>();

            auto ce = (CallExpression*)e;
            for(TNode* n = ce->arguments->node.last_child; n; n = n->prev) {
                array::push(returns, expression(code, (Expression*)n));
            }

            forI(returns.count) {
                TAC* tac = add_tac(code->gen);
                tac->op = tac::param;
                
                ExprRet ret = array::read(returns, i);
                if(!ret.is_tac) tac->arg0 = ret.arg;
                else {
                    tac->arg0.kind = tac::arg::temporary;
                    tac->arg0.temporary = array::read(code->gen->tac, -1);
                }
                array::push(code->gen->tac, tac);
            }

            TAC* tac = add_tac(code->gen);
            tac->op = tac::call;
            tac->arg0.kind = tac::arg::func;
            tac->arg0.func = ce->callee;

            array::push(code->gen->tac, tac);
            
        } break;
        case expression::block: {
            block(code, (BlockExpression*)e);
            out.is_tac = true;
        } break;
        case expression::conditional: {
            // generation for conditionals is not recursive because we need to keep track of jump TAC to backfill 
            // and, if the conditional is returning, a temp variable to fill with the resulting value of each branch
            // TODO(sushi) the temp var kind of sucks, we should probably just back fill all of the returns
            //             with whatever is taking the value of the if conditional

            // keep a list of TAC that needs to be back filled once we resolve where its jump should be 
            Array<TAC*> backfills = array::init<TAC*>();
            
            ScopedArray<Expression*> conditional_stack = array::init<Expression*>();
            Expression* current_conditional = 0;


            array::push(conditional_stack, current_conditional);
            current_conditional = e;

            b32 is_returning = current_conditional->flags.conditional.returning;

            // if this conditional returns something we return an array of TAC 
            // that are to be filled out by whatever is calling this 
            Array<TAC*> external_backfill;
            
            if(is_returning) {
                external_backfill = array::init<TAC*>();
            }

            while(1) {
                ExprRet cond = expression(code, (Expression*)current_conditional->node.first_child);

                TAC* cond_jump = add_tac(code->gen);
                cond_jump->op = tac::conditional_jump;
                
                if(!cond.is_tac) cond_jump->arg0 = cond.arg;
                else {
                    cond_jump->arg0.kind = tac::arg::temporary;
                    cond_jump->arg0.temporary = array::read(code->gen->tac, -1);
                }

                array::push(backfills, cond_jump);
                array::push(code->gen->tac, cond_jump);

                Expression* first = (Expression*)current_conditional->node.first_child->next;
                Expression* second = (Expression*)current_conditional->node.last_child;

                if(first->kind == expression::conditional) {
                    // no need to worry about backfilling in this case 
                    expression(code, first);
                }

                ExprRet f = expression(code, first);

                TAC* jump = add_tac(code->gen);
                jump->op = tac::jump;
                array::push(backfills, jump);

                if(second == first) {

                } 

            }
        } break;

        default: {
            Assert(0); // unhandled expression 
        } break;
    }
    return out;
}

void
statement(Code* code, Statement* s) {
    switch(s->kind) {
        case statement::label: {
            auto l = (Label*)s->node.first_child;
            
            switch(l->entity->node.kind) {
                case node::place: {
                    auto p = (Place*)l->entity;
                    ExprRet er = expression(code, (Expression*)l->node.last_child);

                    TAC* tac = add_tac(code->gen);
                    tac->op = tac::assignment;
                    tac->arg0.kind = tac::arg::place;
                    tac->arg0.place = p;

                    if(!er.is_tac) tac->arg1 = er.arg;
                    else {
                        tac->arg1.kind = tac::arg::temporary;
                        tac->arg1.temporary = array::read(code->gen->tac, -1);
                    }

                    array::push(code->gen->tac, tac);
                } break;
                default: {
                    Assert(0); // unhandled label kind 
                } break;
            }
        } break;
        case statement::expression: {
            ExprRet er = expression(code, (Expression*)s->node.first_child);
            if(!er.is_tac) {
                Assert(0); // what to do here?
            }
        } break;
        case statement::block_final: {
            if(s->node.first_child) {
                // we are returning a value
                ExprRet er = expression(code, (Expression*)s->node.first_child);

                TAC* ret = add_tac(code->gen);
                ret->op = tac::block_value;

                if(!er.is_tac) ret->arg0 = er.arg;
                else{
                    ret->arg0.kind = tac::arg::temporary;
                    ret->arg0.temporary = array::read(code->gen->tac, -1);
                } 

                array::push(code->gen->tac, ret);
            } else {
                // we are just returning from the function
                TAC* ret = add_tac(code->gen);
                ret->op = tac::op::ret;
                array::push(code->gen->tac, ret);
            }
        } break;
    }
}

void
block(Code* code, BlockExpression* e) {
    TAC* tac = add_tac(code->gen);
    tac->op = tac::block_start;
    array::push(code->gen->tac, tac);
    for(TNode* n = e->node.first_child; n; n = n->next) {
        statement(code, (Statement*)n);
    }

    if(array::read(code->gen->tac, -1)->op != tac::block_value) {
         tac = add_tac(code->gen);
        tac->op = tac::block_end;
        array::push(code->gen->tac, tac);
    }
}

void
function(Code* code) {
    auto l  = (Label*)code->parser->root;
    auto f  = (Function*)l->entity;
    auto ft = f->type;

    block(code, (BlockExpression*)l->node.last_child->last_child);

    // if(gr.local_space) {
    //     TAC* tac = add_tac(code->gen);
    //     tac->op = tac::stack_push;
    //     tac->arg0.kind = tac::arg::literal;
    //     tac->arg0.literal = gr.local_space;

    //     array::insert(gr.tac, 0, tac);

    //     tac = add_tac(code->gen);
    //     tac->op = tac::stack_pop;
    //     tac->arg0.kind = tac::arg::literal;
    //     tac->arg0.literal = gr.local_space;

    //     array::push(gr.tac, tac);
    // }
} 

void
label(Code* code, Label* l) {
    switch(l->entity->node.kind) {
        case node::function: {
            function(code);
        } break;
        case node::place: {
        } break;
    }
}

// void
// module(TNode* node) {
//     for(TNode* n = node->first_child; n; n = n->next) {
//         label((Label*)n);
//     }
// }

b32
start(Code* code) {
    switch(code->kind) {
        case code::source: {
            for(TNode* n = code->node.first_child; n; n = n->next) {
                if(!generate((Code*)n)) return false;
            }
        } break;
        case code::function: {
            function(code);
        } break;
        default: {
            TODO(dstring::init("unhandled start case: ", code::strings[code->kind]));
            return false;
        } break;
    }
    return true;
}

} // namespace internal

b32
generate(Code* code) {
    if(!code->gen) code->gen = create(code);
    if(!internal::start(code)) return false;
    return true;
}

} // namespace gen


void
to_string(DString& current, Arg arg) {
    switch(arg.kind) {
        case tac::arg::literal: {
            dstring::append(current, arg.literal);
        } break;
        case tac::arg::place: {
            dstring::append(current, label::resolve((TNode*)arg.place)); // laziness
        } break;
        case tac::arg::func: {
            dstring::append(current, label::resolve((TNode*)arg.func)); // laziness
        } break;
        case tac::arg::temporary: {
            dstring::append(current, "(", arg.temporary->id, ")");
        } break;
    }
}

void
to_string(DString& current, TAC* tac) {
    dstring::append(current, "(", tac->id, ") ~ ");
    switch(tac->op) {
         case tac::op::stack_push: {
            dstring::append(current, "stack_push ", tac->arg0);
        } break;
        case tac::op::stack_pop: {
            dstring::append(current, "stack_pop ", tac->arg0);
        } break;
        case tac::op::addition: {
            dstring::append(current, tac->arg0, " + ", tac->arg1);
        } break;
        case tac::assignment: {
            dstring::append(current, tac->arg0, " = ", tac->arg1);
        } break;
        case tac::op::param: {
            dstring::append(current, "param ", tac->arg0);
        } break;
        case tac::op::call: {
            dstring::append(current, "call ", tac->arg0);
        } break;
        case tac::block_start: {
            dstring::append(current, "block_start");
        } break;
        case tac::block_end: {
            dstring::append(current, "block_end");
        } break;
        case tac::block_value: {
            dstring::append(current, "block_value ", tac->arg0);
        } break;
        case tac::op::ret: {
            dstring::append(current, "return ");
            if(tac->arg0.kind) {
                dstring::append(current, tac->arg0);
            }
        } break;
       
    }
}

} // namespace amu