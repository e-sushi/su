namespace amu {

GenTAC* GenTAC::
create(Code* code) {
    GenTAC* out = pool::add(compiler::instance.storage.tac_gens);
    out->code = code;
    code->tac_gen = out;
    out->pool = pool::init<TAC>(64);
    out->seq = Array<TAC*>::create();
    out->loop_start_stack = Array<TAC*>::create();
    out->loop_end_stack = Array<TAC*>::create();
    out->temps = Array<Array<TAC*>>::create();
    return out;
}

void GenTAC::
destroy() {
    pool::deinit(pool);
    seq.destroy();
    loop_start_stack.destroy();
    loop_end_stack.destroy();
    temps.destroy();
}

void GenTAC::
generate() {
    start();

    util::println(code->identifier);
    u32 last_line_num = -1;
    forI(seq.count) {
        TAC* tac = seq.read(i);
        if(last_line_num == -1 || last_line_num != tac->node->start->l0) {
            util::println(tac->node->first_line(true, true));
            last_line_num = tac->node->start->l0;
        }
        util::println(to_string(seq.read(i)));
    }

    code->level = code::tac;
}

void GenTAC::
start() {
    switch(code->kind) {
        case code::source: {
            for(auto* n = code->first_child<Code>(); n; n = n->next<Code>()) {
                if(!n->tac_gen) n->tac_gen = create(n);
                n->tac_gen->generate();
            }
        } break;
        case code::function: {
            function();
        } break;
        case code::typedef_: {
            // TAC doesn't need to be generated for structures for now 
            // we might want to generate initialization TAC for structures later once we support that
            // so that it can be inserted immediately
        } break;
        case code::expression: {
            auto e = code->parser->root->as<CompileTime>();
            TAC* start = 0;
            if(e->first_child()->is_not<Block>()) {
                start = make_and_place();
                start->op = tac::block_start;
                start->node = e;
            }
            temps.push(Array<TAC*>::create());
            Arg arg = expression(e->first_child<Expr>());
            if(start) {
                TAC* end = make_and_place();
                end->op = tac::ret;
                end->arg0 = arg;
                end->node = e;
            } else if(seq.read(-1)->op == tac::block_value) {
                seq.read(-1)->op = tac::ret;
            } else {
                TAC* ret = make_and_place();
                ret->op = tac::ret;
                ret->node = e;
            }
        } break;
        case code::var_decl: {
            // this is a global variable so for now we'll just make room for it on the stack 
            // and initialize it to whatever its value needs to be 
            TODOsoft("handle runtime global vars");
        } break;
        default: {
            TODO(DString::create("unhandled start case: ", code::strings[code->kind]));
        } break;
    }
}

void GenTAC::
label(Label* l) {
    switch(l->entity->kind) {
        case entity::func: {
            function();
        } break;
        case entity::var: {
            TODO("var label");
        } break;
    }
}

void GenTAC::
function() {
    auto l  = code->parser->root->as<Label>();
    auto f  = l->entity->as<Function>();
    auto ft = f->type;

    f->frame.identifier = DString::create(ScopedDeref(f->dump()).x);

    messenger::qdebug(code, String("generating TAC for func "), ScopedDStringRef(f->display()).x->fin);

    block(l->last_child()->last_child<Block>());

    // if a function ends with a block_value, we want to turn it into a return
    TAC* last = seq.read(-1);
    if(last->op == tac::block_value) {
        last->op = tac::ret;
    } else {
        TAC* ret = make_and_place(); // wow
        auto p = l->last_child()->last_child();
        ret->node = (p->last_child()? p->last_child() : p);
        ret->op = tac::ret;
    }
} 

void GenTAC::
block(Block* e) {
    TAC* tac = make_and_place();
    tac->op = tac::block_start;
    temps.push(Array<TAC*>::create());
    for(auto n = e->first_child<Stmt>(); n; n = n->next<Stmt>())
        statement(n);
    tac->node = e;

    if(seq.read(-1)->op != tac::block_value) {
        tac = make_and_place();
        tac->op = tac::block_end;
        tac->node = (e->last_child()? e->last_child() : e);
    }
}

void GenTAC::
statement(Stmt* s) {
    switch(s->kind) {
        case stmt::label: {
            auto l = s->first_child<Label>();
            switch(l->entity->kind) {
                case entity::var: {
                    auto v = l->entity->as<Var>();
                    // a compile time variable should not appear in runtime code
                    if(!code->compile_time && v->is_compile_time) {
                        return;
                    }
                    locals.push(v);

                    Arg arg;
                    if(l->last_child()->is(expr::binary_assignment)) {
                        // we need to manually extract the rhs to avoid dealing with the typeref
                        // that will be on the left
                        // TODO(sushi) by the time we get here, these parts of the AST should probably have been removed
                        //             so that we dont have to deal with them this far into compilation 
                        arg = expression(l->last_child()->last_child<Expr>());
                    } else if(l->last_child()->is(expr::typeref)) {
                        // this is a variable decl of the form
                        //     <id> : <type> ;
                        // so we just 0 fill it 
                        arg.literal = s64(0);
                    } else {
                        arg = expression(l->last_child<Expr>());
                    }

                    TAC* tac = make_and_place();
                    tac->op = tac::assignment;
                    tac->arg0.kind = arg::var;
                    tac->arg0.var = v;
                    tac->arg1 = arg;
                    tac->node = s;

                    // initialize special types such as StaticArrays 
                    switch(v->type->kind) {
                        case type::kind::structured: {

                        } break;
                    }
                } break;
                default: {
                    TODO("unhandled label kind"); // unhandled label kind 
                } break;
            }
        } break;
        case stmt::expression: {
            if(s->flags.break_air_gen) {
                s->first_child()->flags.break_air_gen = true;
            }
            expression(s->first_child<Expr>());
        } break;
        case stmt::block_final: {
            if(s->first_child()) {
                // we are returning a value
                Arg arg = expression(s->first_child<Expr>());

                if(arg.kind == arg::none && s->first_child()->is(expr::conditional)) {
                    // if there was no return and the child is a conditional, then this was mistakenly marked
                    // as a block final due to ending with 2 close braces
                    // TODO(sushi) fix the Parser mistakenly marking this as a block final
                } else {
                    TAC* ret = make_and_place();
                    ret->op = tac::block_value;
                    ret->arg0 = arg;
                    ret->node = s->first_child();
                }
            } else {
                // we are just returning from the function
                TAC* ret = make_and_place();
                ret->op = tac::op::ret;
                ret->node = s;
            }
        } break;
    }
}

Arg GenTAC::
expression(Expr* e) {
    switch(e->kind) {
        case expr::varref: {
            return e->as<VarRef>()->var;
        } break;

        case expr::binary_plus:
        case expr::binary_minus:
        case expr::binary_multiply:
        case expr::binary_division:
        case expr::binary_equal:
        case expr::binary_not_equal:
        case expr::binary_less_than:
        case expr::binary_less_than_or_equal:
        case expr::binary_greater_than:
        case expr::binary_greater_than_or_equal: {
            Arg lhs = expression(e->first_child<Expr>());
            Arg rhs = expression(e->last_child<Expr>());

            TAC* add = make_and_place(); // this sucks 
            add->op = (e->kind == expr::binary_plus                  ? tac::addition :
                       e->kind == expr::binary_minus                 ? tac::subtraction :
                       e->kind == expr::binary_multiply              ? tac::multiplication :
                       e->kind == expr::binary_division              ? tac::division :
                       e->kind == expr::binary_equal                 ? tac::equal : 
                       e->kind == expr::binary_not_equal             ? tac::not_equal :
                       e->kind == expr::binary_less_than             ? tac::less_than :
                       e->kind == expr::binary_less_than_or_equal    ? tac::less_than_or_equal :
                       e->kind == expr::binary_greater_than          ? tac::greater_than :
                                                                       tac::greater_than_or_equal);
            add->arg0 = lhs;
            add->arg1 = rhs;

            new_temp(add, e->type);

            add->node = e;
            return add;
        } break;

        case expr::binary_access: {
            Arg arg;
            arg.kind = arg::member;
            arg.offset_var.type = e->member->type;
            arg.offset_var.offset = 0;

            Expr* step = e;
            while(1) {
                arg.offset_var.offset += step->member->offset;
                if(step->first_child()->is<VarRef>()) {
                    arg.offset_var.var = step->first_child<VarRef>()->var;
                    break;
                } 
                step = step->first_child<Expr>();
            }

            return arg;
        } break;

        case expr::cast: {
            Arg arg = expression(e->first_child<Expr>());
            TAC* cast = make_and_place();
            new_temp(cast, e->type);
            cast->node = e;
            if(e->type->is<Scalar>() && e->first_child<Expr>()->type->is<Scalar>()) {
                auto to = e->type->as<Scalar>();
                auto from = e->first_child<Expr>()->type->as<Scalar>();
                switch(to->kind) {
                    case scalar::unsigned64: {
                        cast->temp_size = 4;
                        cast->arg1 = width::quad;
                        switch(from->kind) {
                            case scalar::unsigned32: {
                                cast->op = tac::resz;
                                cast->arg0 = arg;
                            } break;
                            case scalar::unsigned16: {
                                cast->op = tac::resz;
                                cast->arg0 = arg;
                            } break;
                        }
                    } break;
                    case scalar::float32: {
                        cast->temp_size = 4;
                        cast->arg1 = width::dble;
                        switch(from->kind) {
                            case scalar::float64: {
                                 // NOTE(sushi) even if we're casting to a smaller type, the temp 
                                //             has to be large enough to hold the original value.
                                //             Ideally we setup casting values that are already in
                                //             temporaries in place soon, but for now this is fine
                                //             since it's going to be cleared eventually anyways
                                cast->temp_size = 8;
                                cast->op = tac::resz;
                                cast->arg0 = arg;
                                cast->is_float = true;
                            } break;
                        }
                    } break;
                    case scalar::float64: {
                        cast->temp_size = 8;
                        cast->arg1 = width::quad;
                        switch(from->kind) {
                            case scalar::float32: {
                                cast->op = tac::resz;
                                cast->arg0 = arg;
                                cast->is_float = true;
                            } break;
                        }
                    } break;
                }
            } else {
                TODO("handle non-scalar casts");
            }

            // if(arg.kind == arg::temporary) return arg;
            // else return cast;
            return cast;
        } break;

        case expr::literal_scalar: {
            return e->as<ScalarLiteral>()->value;
        } break;

        case expr::literal_string: {
            TODO("handle string literals");
        } break;

        case expr::literal_array: {
            TAC* first = 0;
            for(Expr* elem = e->first_child<Expr>(); elem; elem = elem->next<Expr>()) {
                TAC* t = make_and_place();
                t->op = tac::array_element;
                t->arg0 = expression(elem);
                t->temp_size = elem->type->size();
                t->node = elem;
                if(!first) first = t;
            }

            TAC* arr = make_and_place();
            arr->op = tac::array_literal;
            arr->temp_size = e->type->size();
            arr->node = e;
            arr->arg0 = first;

            return arr;
        } break;

        case expr::subscript: {
            Arg lhs = expression(e->first_child<Expr>());
            Arg rhs = expression(e->last_child<Expr>());
            
            TAC* tac = make_and_place();
            tac->op = tac::subscript;
            tac->arg0 = lhs;
            tac->arg1 = rhs;
            tac->temp_size = e->last_child<Expr>()->type->size();
            tac->node = e;

            return tac;
        } break;

        case expr::literal_tuple: {
            TODO("handle tuple literals");
        } break;

        case expr::binary_assignment: {
            Arg lhs = expression(e->first_child<Expr>());
            Arg rhs = expression(e->last_child<Expr>());

            TAC* tac = make_and_place();
            tac->op = tac::assignment;
            tac->arg0 = lhs;
            tac->arg1 = rhs;

            tac->node = e;

            return tac;
        } break;
        case expr::unary_assignment: {
            // what this is being used for is handled by whatever called this
            return expression(e->last_child<Expr>());
        } break;
        case expr::call: {
            u64 param_size = 0;
            auto ce = e->as<Call>();

            TAC* ret = make();
            ret->op = tac::param;
            ret->node = e;
            ret->arg0.literal = u64(0);
            ret->comment = DString::create("return slot for ", ce->callee->display());
            new_temp(ret, e->type);

            auto params = Array<TAC*>::create();

            for(auto n = ce->arguments->last_child<Expr>(); n; n = n->prev<Expr>()) {
                Arg ret = expression(n);
                TAC* tac = make();
                tac->op = tac::param;
                tac->node = e;
                tac->arg0 = ret;
                tac->comment = DString::create("arg in call to ", ce->callee->display());
                new_temp(tac, n->type);
                param_size += tac->temp_size;
                params.push(tac);
            }

            place(ret);
            forI(params.count) {
                place(params.read(i));
            }
            TAC* tac = make_and_place();
            tac->op = tac::call;
            tac->arg0.kind = arg::func;
            tac->arg0.func = ce->callee;
            tac->arg1.literal = param_size + ret->temp_size;
            tac->temp_size = ret->temp_size;
            tac->node = e;

            params.destroy();

            return tac;
        } break;
        case expr::block: {
            block(e->as<Block>());
            return seq.read(-1);
        } break;
        case expr::conditional: {
            TAC* temp = 0;

            if(!e->first_child()->next<Expr>()
                 ->type->is_any<Void,Whatever>()) {
                temp = make_and_place();
                temp->op = tac::temp;
                temp->node = e;

                new_temp(temp, e->type);
            }   
            
            Expr* curr = e;

            TAC* last_condjump = 0;
            auto truejumps = Array<TAC*>::create();

            while(1) {
                auto cond       = curr->first_child<Expr>();
                auto true_body  = curr->first_child()->next<Expr>();;
                auto false_body = curr->last_child<Expr>();

                Arg cond_eval = expression(cond);

                // create a conditional jump 
                TAC* condjump = make_and_place();
                condjump->op = tac::jump_zero;
                condjump->arg0 = cond_eval;
                condjump->node = cond;

                // generate code for true body
                Arg true_eval = expression(true_body); 

                if(temp) {
                    TAC* assign = make_and_place();
                    assign->op = tac::assignment;
                    assign->arg0 = temp;
                    assign->arg1 = true_eval;
                    assign->node = e;
                }

                // generate the jump out of the true body
                if(true_body != false_body) {
                    TAC* truejump = make_and_place();
                    truejump->op = tac::jump;
                    truejump->node = (true_body->last_child()? true_body->last_child() : true_body);
                    truejumps.push(truejump);
                }

                TAC* placeholder = make_and_place();
                placeholder->op = tac::nop;
                placeholder->from = condjump;

                condjump->arg1.kind = arg::temporary;
                condjump->arg1.temporary = placeholder;

                if(true_body == false_body) { // there is no false body
                    
                    if(!truejumps.count) break;

                    TAC* last_jump = truejumps.pop();
                    last_jump->arg0.temporary = placeholder;
                    last_jump->arg0.kind = arg::temporary;
                    placeholder->from->next = last_jump;

                    while(truejumps.count) {
                        TAC* j = truejumps.pop();
                        j->arg0.temporary = placeholder;
                        j->arg0.kind = arg::temporary;
                        last_jump->next = j;
                        last_jump = j;
                    }
                    if(temp) return temp;
                    return {};
                }

                if(false_body->is(expr::conditional)) {
                    curr = false_body;
                    continue;
                } 

                // we've reached an else with no following if
                Arg false_eval = expression(false_body);

                if(temp) {
                    TAC* assign = make_and_place();
                    assign->op = tac::assignment;
                    assign->arg0 = temp;
                    assign->arg1 = false_eval;
                    assign->node = e;
                }

                placeholder = make_and_place();
                placeholder->op = tac::nop;

                TAC* last_jump = truejumps.pop();
                last_jump->arg0.kind = arg::temporary;
                last_jump->arg0.temporary = placeholder;
                placeholder->from = last_jump;

                while(truejumps.count) {
                    TAC* j = truejumps.pop();
                    j->arg0.kind = arg::temporary;
                    j->arg0.temporary = placeholder;
                    last_jump->next = j;
                    last_jump = j;
                }

                break;
            }
            if(temp) return temp;
            return {};
        } break;

        case expr::loop: {
            TAC* end = make();
            end->node = e;

            u32 count = seq.count;
            TAC* nop = make_and_place();
            nop->node = e;

            loop_start_stack.push(nop);
            loop_end_stack.push(end);
            temps.push(Array<TAC*>::create());

            b32 make_block = e->first_child()->is_not<Block>();
            if(make_block) {
                // if this loop is not scoped, we must manually place blocks
                // so that any temporaries made inside of it can be cleaned up 
                // in AIR generation later 
                TAC* t = make_and_place();
                t->op = tac::block_start;
                t->node = e;
            }

            Arg arg = expression(e->first_child<Expr>());

            if(make_block) {
                TAC* t = make_and_place();
                t->op = tac::block_end;
                t->node = e;
            }

            TAC* tac = make_and_place();
            tac->op = tac::jump;
            tac->arg0.kind = arg::temporary;
            tac->arg0.temporary = seq.read(count);
            tac->to = nop;
            tac->node = e;

            place(end);

            return Arg();
        } break;

        case expr::break_: {
            TAC* jump = make_and_place();
            jump->op = tac::jump;
            jump->node = e;
            TAC* loop_end = loop_end_stack.read(-1);
            jump->arg0 = loop_end;
            jump->next = loop_end->from;
            loop_end->from = jump;
            return jump;
        } break;

        case expr::binary_or: {
            // logical or currently returns only 0 or 1
            // and is short circuiting
            // but later on I want to set it up to return the first true 
            // value so that we can do things like 
            // a := func() || other_func() || return;
            TAC* temp = make_and_place();
            temp->op = tac::temp;
            temp->node = e;
            new_temp(temp, &scalar::_u64);

            auto jump_stack = Array<TAC*>::create();

            auto left_stack = Array<Expr*>::create();
            left_stack.push(e);
            auto left = e->first_child<Expr>();
            while(1) {
                if(left->is_not(expr::binary_or)) break;
                left_stack.push(left);
                left = left->first_child<Expr>();
            }

            Arg left_result;

            left = left_stack.pop();

            // get deepest lhs expression result 
            Arg lhs = expression(left->first_child<Expr>());

            TAC* ljump = make_and_place();
            ljump->op = tac::jump_not_zero;
            ljump->arg0 = lhs;
            ljump->node = e;

            jump_stack.push(ljump);

            b32 user_exit = 0;

            // gather the rhs of each level
            while(1) {
                Arg rhs = expression(left->last_child<Expr>());

                // the user has already setup the exit for us 
                if(left->last_child()->is_any(expr::break_, expr::return_)) {
                    user_exit = true;
                    break;
                }   

                TAC* rjump = make_and_place();
                rjump->op = tac::jump_not_zero;
                rjump->arg0 = rhs;
                rjump->node = e;

                jump_stack.push(rjump);

                if(!left_stack.count) break;
                left = left_stack.pop();
            }

            TAC* fail_jump = 0; 
            if(!user_exit) {
                fail_jump = make_and_place();
                fail_jump->op = tac::jump;
                fail_jump->node = e;
            }

            TAC* success = make_and_place();
            success->op = tac::assignment;
            success->arg0.kind = arg::temporary;
            success->arg0.temporary = temp;
            success->arg1.literal = u64(1);
            success->node = e;

            if(fail_jump) {
                TAC* fin = make_and_place();
                fin->op = tac::nop;
                fin->node = e;
                fin->from = fail_jump;

                fail_jump->arg0.kind = arg::temporary;
                fail_jump->arg0.temporary = fin;
            }


            if(jump_stack.count) {
                TAC* last_jump = jump_stack.pop();
                success->from = last_jump;
                last_jump->arg1.kind = arg::temporary;
                last_jump->arg1.temporary = success;

                while(jump_stack.count) {
                    TAC* b = jump_stack.pop();
                    b->arg1.kind = arg::temporary;
                    b->arg1.temporary = success;
                    last_jump->next = b;
                    last_jump = b;
                }
            }
            return temp;
        } break;

        case expr::binary_and: {
            // logical and also returns 0 or 1
            // and is short circuiting
            // however I'm not sure if it would be possible
            // to choose a branch to return from to achieve 
            // an effect similar to what I want to do with logical or
            // maybe it returns the last branch that fails? especially if we 
            // want to handle an error
            // a := func?() && other_func?() && another_func?() && return;
            // if any func failed, a will have its value, otherwise the function
            // will have returned
            TAC* temp = make_and_place();
            temp->op = tac::temp;
            temp->node = e;
            new_temp(temp, &scalar::_u64);

            auto jump_stack = Array<TAC*>::create();

            auto left_stack = Array<Expr*>::create();
            left_stack.push(e);
            auto left = e->first_child<Expr>();
            while(1) {
                if(left->is_not(expr::binary_and)) break;
                left_stack.push(left);
                left = left->first_child<Expr>();
            }

            Arg left_result;

            left = left_stack.pop();

            // get deepest lhs expression result 
            Arg lhs = expression(left->first_child<Expr>());

            TAC* ljump = make_and_place();
            ljump->op = tac::jump_zero;
            ljump->arg0 = lhs;
            ljump->node = e;

            jump_stack.push(ljump);

            b32 user_exit = 0;

            // gather the rhs of each level
            while(1) {
                Arg rhs = expression(left->last_child<Expr>());

                // the user has already setup the exit for us 
                if(left->last_child()->is_any(expr::break_, expr::return_)) {
                    user_exit = true;
                    break;
                }   

                TAC* rjump = make_and_place();
                rjump->op = tac::jump_zero;
                rjump->arg0 = rhs;
                rjump->node = e;

                jump_stack.push(rjump);

                if(!left_stack.count) break;
                left = left_stack.pop();
            }

            TAC* success = make_and_place();
            success->op = tac::assignment;
            success->arg0.kind = arg::temporary;
            success->arg0.temporary = temp;
            success->arg1.literal = u64(1);
            success->node = e;

            TAC* fail_jump = make_and_place();

            if(jump_stack.count) {
                TAC* last_jump = jump_stack.pop();
                fail_jump->from = last_jump;
                last_jump->arg1.kind = arg::temporary;
                last_jump->arg1.temporary = fail_jump;

                while(jump_stack.count) {
                    TAC* b = jump_stack.pop();
                    b->arg1.kind = arg::temporary;
                    b->arg1.temporary = fail_jump;
                    last_jump->next = b;
                    last_jump = b;
                }
            }
            return temp;
        } break;

        case expr::unary_comptime: {
            return expression(e->last_child<Expr>());
        } break;
    }

    // an expression didn't return anything or this is a
    // completely unhandled expression kind
    TODO(DString::create("unhandled expression kind: ", expr::strings[e->kind]));
    return {};
}

TAC* GenTAC::
make() {
    if(seq.count) {
        TAC* last = seq.read(-1);
        if(last->op == tac::nop)
            return last;
    } 

    TAC* out = pool::add(pool);
    out->id = count++;
    out->bc_offset = -1;
    return out;
}

void GenTAC::
place(TAC* t) {
    seq.push(t);
}

TAC* GenTAC::
make_and_place() {
    TAC* out = make();
    // TODO(sushi) do this better
    if(!seq.count || seq.read(-1)->op != tac::nop) 
        place(out);
    return out;
}

void GenTAC::
new_temp(TAC* tac, Type* t) {
    tac->temp_size = t->size();
    temps.readref(-1).push(tac);
}

} // namespace amu