namespace amu {


namespace tac {

TAC*
add_tac(Gen* gen) {
    TAC* out = pool::add(gen->tac_pool);
    out->id = gen->tac_count++;
    return out;
}

b32 label(Label* l, u64 stack_offset);
void block(Code* code, Block* e);
Arg expression(Code* code, Expr* e);

struct ConditionalState {
    Array<TAC*> truelist; // list of cond_jumps that need to be backpatched with a TAC to jump to
    Array<TAC*> falselist; // list of plain jumps that need to be backpatched with a TAC To jump to
    TAC* result;
    Place* temp;
};

Arg
conditional(Code* code, Expr* cond, ConditionalState* state) {
    Arg condition = expression(code, (Expr*)cond->node.first_child);

    TAC* cond_jump = add_tac(code->gen);
    cond_jump->op = tac::conditional_jump;
    cond_jump->arg0 = condition;

    array::push(state->falselist, cond_jump);
    array::push(code->gen->tac, cond_jump);

    Expr* first = (Expr*)cond->node.first_child->next;
    Expr* second = (Expr*)cond->node.last_child;

    // if this is a conditional then it is its own if/else ladder that does not
    // need to use the current state
    Arg f = expression(code, first);
    // if(first->kind == expr::conditional) f = conditional(code, first, state);
    // else f = expression(code, first);
    {
        TAC* assign = add_tac(code->gen);
        assign->op = tac::assignment;
        assign->arg0.kind = arg::place;
        assign->arg0.place = state->temp;
        assign->arg1 = f;
        array::push(code->gen->tac, assign);
    }

    if(second != first) {
        // add the if/else exit jump
        TAC* jump = add_tac(code->gen);
        jump->op = tac::jump;

        array::push(code->gen->tac, jump);

        // the last cond_jump can be resolved to the instruction following the jump 
        TAC* resolved = array::pop(state->falselist);
        u64 next_index = code->gen->tac.count;

        array::push(state->truelist, jump);
        
        Arg s;
        if(second->kind == expr::conditional) s = conditional(code, second, state);
        else {
            s = expression(code, second);
            TAC* assign = add_tac(code->gen);
            assign->op = tac::assignment;
            assign->arg0.kind = arg::place;
            assign->arg0.place = state->temp;
            assign->arg1 = s;
            array::push(code->gen->tac, assign);
        }

        resolved->arg1.temporary = array::read(code->gen->tac, next_index);
        resolved->arg1.kind = arg::kind::temporary;

        // we've reached the end of an if/else ladder
        // so we can fill out the true body jumps
        if(second->kind != expr::conditional) {
            // we make a label TAC for the jumps, because we dont know yet
            // what the next instruction is actually going to be
            // this should be cleaned up eventually, ideally removing the need for making this at all
            TAC* end = add_tac(code->gen);
            end->op = tac::nop;
            array::push(code->gen->tac, end);
            forI(state->truelist.count) {
                TAC* resolve = array::read(state->truelist, i);
                resolve->arg0 = end;
            }
        }
    } else {
        // we're at the end, so we don't need a jump and we can backfill all previous jumps
        // and the hanging cond_jump
        TAC* end = add_tac(code->gen);
        end->op = tac::nop;
        array::push(code->gen->tac, end);
        forI(state->truelist.count) {
            TAC* resolve = array::read(state->truelist, i);
            resolve->arg0 = end;
        }
        array::pop(state->falselist)->arg1 = end;
    }
    

    Arg out = {};
    out.kind = arg::temporary;
    out.temporary = state->result;
    return out;
}

Arg
expression(Code* code, Expr* e) {
    switch(e->kind) {
        case expr::placeref: {
            auto pr = (PlaceRef*)e;
            return pr->place;
        } break;
        case expr::binary_plus: {
            Arg lhs = expression(code, (Expr*)e->node.first_child);
            Arg rhs = expression(code, (Expr*)e->node.last_child);

            TAC* add = add_tac(code->gen);
            add->op = tac::op::addition;
            add->arg0 = lhs;
            add->arg1 = rhs;

            array::push(code->gen->tac, add);

            return add;
        } break;
        case expr::cast: {
            return expression(code, (Expr*)e->node.first_child);
        } break;
        case expr::literal: {
            TODO("handle literals other than unsigned ints");
            return e->node.start->u64_val;
        } break;
        case expr::binary_assignment: {
            Arg lhs = expression(code, (Expr*)e->node.first_child);
            Arg rhs = expression(code, (Expr*)e->node.last_child);

            TAC* tac = add_tac(code->gen);
            tac->op = tac::assignment;
            tac->arg0 = lhs;
            tac->arg1 = rhs;

            array::push(code->gen->tac, tac);

            return tac;
        } break;
        case expr::unary_assignment: {
            // what this is being used for is handled by whatever called this
            return expression(code, (Expr*)e->node.last_child);
        } break;
        case expr::call: {
            ScopedArray<Arg> returns = array::init<Arg>();

            auto ce = (Call*)e;
            for(TNode* n = ce->arguments->node.last_child; n; n = n->prev) {
                array::push(returns, expression(code, (Expr*)n));
            }

            forI(returns.count) {
                TAC* tac = add_tac(code->gen);
                tac->op = tac::param;
                Arg ret = array::read(returns, i);

                tac->arg0 = ret;

                array::push(code->gen->tac, tac);
            }

            TAC* tac = add_tac(code->gen);
            tac->op = tac::call;
            tac->arg0.kind = tac::arg::func;
            tac->arg0.func = ce->callee;

            array::push(code->gen->tac, tac);

            return tac;
        } break;
        case expr::block: {
            block(code, (Block*)e);
            return array::read(code->gen->tac, -1);
        } break;
        case expr::conditional: {
            ConditionalState state = {};
            
            state.truelist = array::init<TAC*>();
            state.falselist = array::init<TAC*>();

            state.temp = Place::create();
            state.temp->type = e->type;

            state.result = add_tac(code->gen);
            state.result->op = tac::temp;
            state.result->arg0.kind = tac::arg::place;
            state.result->arg0.place = state.temp;

            array::push(code->gen->tac, state.result);
            
            return conditional(code, e, &state);

            // // keep a list of TAC that needs to be back filled once we resolve where its jump should be 
            // Array<TAC*> backfills 
            
            // ScopedArray<Expr*> conditional_stack = array::init<Expr*>();
            // Expr* current_conditional = 0;

            // array::push(conditional_stack, current_conditional);
            // current_conditional = e;

            // b32 is_returning = current_conditional->flags.conditional.returning;

            // // a TAC where the result of a returning if chain will be stored 
            // TAC* result 

            // array::push(code->gen->tac, result);

            
        } break;
    }

    // an expression didn't return anything or this is a
    // completely unhandled expression kind
    Assert(0); 
    return {};
}

void
statement(Code* code, Statement* s) {
    switch(s->kind) {
        case statement::label: {
            auto l = (Label*)s->node.first_child;
            
            switch(l->entity->node.kind) {
                case node::place: {
                    auto p = (Place*)l->entity;
                    Arg er = expression(code, (Expr*)l->node.last_child);

                    TAC* tac = add_tac(code->gen);
                    tac->op = tac::assignment;
                    tac->arg0.kind = tac::arg::place;
                    tac->arg0.place = p;

                    tac->arg1 = er;

                    array::push(code->gen->tac, tac);
                } break;
                default: {
                    Assert(0); // unhandled label kind 
                } break;
            }
        } break;
        case statement::expression: {
            Arg er = expression(code, (Expr*)s->node.first_child);
            if(er.kind != arg::temporary) {
                Assert(0); // what to do here?
            }
        } break;
        case statement::block_final: {
            if(s->node.first_child) {
                // we are returning a value
                Arg er = expression(code, (Expr*)s->node.first_child);

                TAC* ret = add_tac(code->gen);
                ret->op = tac::block_value;

                ret->arg0 = er;

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
block(Code* code, Block* e) {
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

    block(code, (Block*)l->node.last_child->last_child);

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

b32
generate(Code* code) {
    if(!code->gen) code->gen = gen::create(code);
    if(!start(code)) return false;

    util::println(code->name);
    forI(code->gen->tac.count) {
        util::println(to_string(array::read(code->gen->tac, i)));
    }

    return true;
}

} // namespace tac


namespace gen {

Gen*
create(Code* code) {
    Gen* out = pool::add(compiler::instance.storage.gens);
    out->tac_pool = pool::init<TAC>(128);
    out->tac = array::init<TAC*>();
    code->gen = out;
    return out;
}

namespace air {

} // namespace air
} // namespace gen


void
to_string(DString& current, tac::Arg arg) {
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
        case tac::nop: {
            dstring::append(current, "nop");
        } break;
        case tac::temp: {
            dstring::append(current, "temp");
        } break;
        case tac::stack_push: {
            dstring::append(current, "stack_push ", tac->arg0);
        } break;
        case tac::stack_pop: {
            dstring::append(current, "stack_pop ", tac->arg0);
        } break;
        case tac::addition: {
            dstring::append(current, tac->arg0, " + ", tac->arg1);
        } break;
        case tac::assignment: {
            dstring::append(current, tac->arg0, " = ", tac->arg1);
        } break;
        case tac::param: {
            dstring::append(current, "param ", tac->arg0);
        } break;
        case tac::call: {
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
        case tac::ret: {
            dstring::append(current, "return ");
            if(tac->arg0.kind) {
                dstring::append(current, tac->arg0);
            }
        } break;
        case tac::jump: {
            dstring::append(current, "jump ");
            if(tac->arg0.kind) {
                dstring::append(current, tac->arg0);
            } else {
                dstring::append(current, "...");
            }
        } break;
        case tac::conditional_jump: {
            dstring::append(current, "cond_jump ", tac->arg0, " ");
            if(tac->arg1.kind) {
                dstring::append(current, tac->arg1);
            } else {
                dstring::append(current, "...");
            }
        } break;
       
    }
}

} // namespace amu