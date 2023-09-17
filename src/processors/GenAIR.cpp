namespace amu {

GenAIR* GenAIR::
create(Code* code) {
    GenAIR* out = pool::add(compiler::instance.storage.air_gens);
    out->seq = Array<BC>::create();
    out->code = code;
    code->air_gen = out;
    out->scoped_temps = Array<u64>::create();
    out->stack_things = map::init<BC*, StackThing>();
    out->scoped_temps.push(u64(0));
    return out;
}

void GenAIR::
destroy() {
    seq.destroy();
    scoped_temps.destroy();
    map::deinit(stack_things);
}

void GenAIR::
generate() {
    start();

    util::println(code->identifier);
    u32 last_line_num = -1;
    TAC* last_tac = 0;
    forI(seq.count) {
        BC bc = seq.read(i);
        // if(bc.tac && bc.tac != last_tac) {
        //     last_tac = bc.tac;
        //     util::println(to_string(bc.tac));
        // }
        util::println(to_string(bc));
    }

    code->level = code::air;
}

void GenAIR::
start() {
    switch(code->kind) {
        case code::source: {
            for(auto* n = code->first_child<Code>(); n; n = n->next<Code>()) {
                if(!n->air_gen) create(n);
                n->air_gen->generate();
            }
        } break;
        case code::function: {
            Function* f = code->parser->root->as<Label>()->entity->as<Function>();
            f->frame.locals = Array<Var*>::create();
            
            // actually the first thing we need to do is see if we are returning something!
            stack_offset += f->type->return_type->size();

            // and then parameters!
            for(Label* n = f->type->parameters->first_child<Label>(); n; n = n->next<Label>()) {
                Var* v = n->entity->as<Var>();
                v->stack_offset = stack_offset;
                f->frame.locals.push(v);
                stack_offset += v->type->size();
            }

            // the first thing we need to do is figure out the locals of this function and
            // setup their Vars to be positioned correctly on the stack
            forI(code->tac_gen->locals.count) {
                auto v = code->tac_gen->locals.read(i);
                v->stack_offset = stack_offset;
                // if(v->type->is<Structured>()) {
                //     // uhm i forget what I was going to do here
                // }
                f->frame.locals.push(v);
                stack_offset += v->type->size();
            }
            BC* bc = seq.push();
            bc->tac = code->tac_gen->seq.read(0);
            bc->node = f->code->parser->root;
            bc->instr = air::pushn;
            bc->lhs = stack_offset;
            bc->flags.left_is_const = true;
            bc->comment = "make room for function locals";
            body();
            f->frame.ip = seq.readptr(0);
        } break;

        case code::typedef_: { // typedefs dont generate anything for now 

        } break;

        case code::expression: {
            auto e = code->parser->root->as<CompileTime>();
            forI(code->tac_gen->locals.count) {
                auto v = code->tac_gen->locals.read(i);
                v->stack_offset = stack_offset;
                e->frame.locals.push(v);
                stack_offset += v->type->size();
            }
            if(stack_offset) {
                BC* bc = seq.push();
                bc->tac = code->tac_gen->seq.read(0);
                bc->node = e;
                bc->instr = air::pushn;
                bc->lhs = stack_offset;
                bc->flags.left_is_const = true;
                bc->comment = "make room for compile time expr locals";
            }
            body();
        } break;

        case code::var_decl: {

        } break;

        default: {
            TODO(DString::create("unhandled start case: ", code::strings[code->kind]));
        } break;
    }
}

void GenAIR::
body() {

    // TODO(sushi) when we get around to implementing an optimization stage this will likely need to be changed 
    auto tac_seq = code->tac_gen->seq;
    forI(tac_seq.count) {
        TAC* tac = tac_seq.read(i);
        if(tac->node->flags.break_air_gen) DebugBreakpoint;
        tac->bc_offset = seq.count;

        if(tac->from) {
            TAC* from = tac->from;
            while(from) {
                if(from->bc_offset != -1) {
                    BC* bc = seq.readptr(from->bc_offset);
                    if(bc->instr == air::jump) {
                        bc->lhs = seq.count - from->bc_offset;
                    } else {
                        bc->rhs = seq.count - from->bc_offset;
                    }
                }
                from = from->next;
            }
        }

        switch(tac->op) {
            case tac::nop: {} break;
            
            case tac::block_start: {
                push_scope();
            } break;

            case tac::block_end: {
                clean_temps();
                pop_scope();
            } break;

            case tac::func_start: {
                stack_offset = tac->arg0.literal._u64;
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::pushn;
                bc->lhs = stack_offset;
                bc->comment = "push function's local stack";
                // ret_start = tac->arg1.literal._u64;
            } break;

            case tac::jump_zero: {
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::jump_zero;
                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->lhs = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::literal: {
                        bc->flags.left_is_const = true;
                        bc->lhs = tac->arg0.literal._u64;
                    } break;
                    case arg::var: {
                        bc->lhs = tac->arg0.var->stack_offset; 
                    } break;
                    case arg::member: {
                        bc->lhs = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.offset;
                        switch(tac->arg0.offset_var.type->size()) {
                            case 1: bc->w = width::byte; break;
                            case 2: bc->w = width::word; break;
                            case 4: bc->w = width::dble; break;
                            case 8: bc->w = width::quad; break;
                        }
                    } break;
                }
            } break;

            case tac::jump_not_zero: {
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::jump_not_zero;
                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->lhs = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::literal: {
                        bc->flags.left_is_const = true;
                        bc->lhs = tac->arg0.literal._u64;
                    } break;
                    case arg::var: {
                        bc->lhs = tac->arg0.var->stack_offset; 
                    } break;
                    case arg::member: {
                        bc->lhs = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.offset;
                        switch(tac->arg0.offset_var.type->size()) {
                            case 1: bc->w = width::byte; break;
                            case 2: bc->w = width::word; break;
                            case 4: bc->w = width::dble; break;
                            case 8: bc->w = width::quad; break;
                        }
                    } break;
                }

            } break;

            case tac::jump: {
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::jump;
                if(tac->to && tac->to->bc_offset != -1) {
                    bc->lhs = tac->to->bc_offset - seq.count + 1;
                }
            } break;

            case tac::temp: {
                // push_temp(tac);
                // FixMe; // map::add(offset_map, tac, (u32)registers.count);
                // // tac->arg0.kind = arg::stack_offset;
                // // tac->arg0.stack_offset = registers.count;
                // // Register* r = registers.push();
            } break;

            case tac::addition:
            case tac::subtraction:
            case tac::multiplication:
            case tac::division: {
                push_temp(tac);
                
                BC* bc1 = seq.push();
                bc1->tac = tac;
                bc1->node = tac->node;
                bc1->instr = 
                    (tac->op == tac::addition ?       air::add :
                     tac->op == tac::subtraction ?    air::sub :
                     tac->op == tac::multiplication ? air::mul :
                                                      air::div);
                bc1->lhs = tac->temp_pos;

                switch(tac->arg1.kind) {
                    case arg::member: {
                        bc1->rhs = tac->arg1.offset_var.var->stack_offset + tac->arg1.offset_var.offset;
                        if(tac->arg1.offset_var.type->is_any(scalar::float32, scalar::float64)) {
                            bc1->flags.float_op = true;
                        }
                        switch(tac->arg1.offset_var.type->size()) {
                            case 1: bc1->w = width::byte; break;
                            case 2: bc1->w = width::word; break;
                            case 4: bc1->w = width::dble; break;
                            case 8: bc1->w = width::quad; break;
                        }
                        bc1->comment = "add with member";
                    } break;
                    case arg::temporary: {
                        bc1->rhs = tac->arg1.temporary->temp_pos;
                    } break;
                    case arg::literal: {
                        bc1->flags.right_is_const = true;
                        bc1->rhs = tac->arg1.literal;
                    } break;
                    case arg::var: {
                        TODO("fix var arithmetic");
                        // auto v = tac->arg1.var;
                        // if(v->is_compile_time) {
                        //     if(!code->compile_time) {
                        //         s64 lhs = bc1->lhs;
                        //         forI(v->type->size()) {
                        //             bc1->flags.right_is_const = true;
                        //             bc1->w = width::byte;
                        //             bc1->rhs = *(v->memory + i);
                        //             bc1->lhs = lhs + i;
                        //             if(i != v->type->size() - 1) {
                        //                 bc1 = seq.push();
                        //                 bc1->instr = air::copy;
                        //                 bc1->node = tac->node;
                        //             }
                        //         }
                        //     } else {
                        //         bc1->flags.right_is_ptr = true;
                        //         bc1->rhs = (s64)v->memory;
                        //     }
                        // } else {
                        //     bc1->rhs = v->stack_offset;
                        // }
                    } break;
                    case arg::stack_offset: {
                        bc1->rhs = tac->arg1.stack_offset;
                    } break;
                }
            } break;
            
            case tac::equal:
            case tac::not_equal:
            case tac::less_than:
            case tac::less_than_or_equal:
            case tac::greater_than:
            case tac::greater_than_or_equal: {
                push_temp(tac);

                BC* bc1 = seq.push();
                bc1->tac = tac;
                bc1->node = tac->node;
                bc1->instr = 
                    (tac->op == tac::equal                ? air::eq  :
                     tac->op == tac::not_equal            ? air::neq :
                     tac->op == tac::less_than            ? air::lt  :
                     tac->op == tac::less_than_or_equal   ? air::le  :
                     tac->op == tac::greater_than         ? air::gt  :
                                                            air::ge);
                bc1->lhs = tac->temp_pos;

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc1->rhs = tac->arg1.temporary->temp_pos;
                    } break;
                    case arg::literal: {
                        bc1->flags.right_is_const = true;
                        bc1->rhs = tac->arg1.literal._u64;
                    } break;
                    case arg::var: {
                        bc1->rhs = tac->arg1.var->stack_offset; 
                    } break;
                    case arg::stack_offset: {
                        bc1->rhs = tac->arg1.stack_offset;
                    } break;
                }
            } break;
            
            case tac::logical_or: {

            } break;

            case tac::assignment: {
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::copy;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        if(tac->arg0.temporary->lvalue){
                            bc->flags.deref_left = true;
                        }
                        bc->copy.dst = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        if(tac->arg0.var->is_compile_time) {
                            bc->flags.left_is_ptr = true;
                            bc->copy.dst = (s64)tac->arg0.var->memory;
                        } else {
                            bc->copy.dst = tac->arg0.var->stack_offset;
                        }
                    } break;
                    case arg::member: {
                        bc->copy.dst = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.offset;
                    } break;
                    case arg::stack_offset: {
                        bc->copy.dst = tac->arg0.stack_offset;
                    } break;
                }

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc->copy.src = tac->arg1.temporary->temp_pos;
                        bc->copy.size = tac->arg1.temporary->temp_size;
                    } break;
                    case arg::var: {
                        if(tac->arg1.var->is_compile_time) {
                            if(!code->compile_time) {
                                bc->copy.src = tac->arg1.var->stack_offset;
                                bc->copy.size = tac->arg1.var->type->size();
                            } else {
                                bc->flags.right_is_ptr = true;
                                bc->rhs = (s64)tac->arg1.var->memory;
                            }
                        } else {
                            bc->rhs = tac->arg1.var->stack_offset;
                        }
                    } break;
                    case arg::literal: {
                        bc->flags.right_is_const = true;
                        bc->copy.literal = tac->arg1.literal;
                        bc->flags.float_op = tac->arg1.literal.is_float();
                    } break;
                    case arg::stack_offset: {
                        bc->copy.src = tac->arg1.stack_offset;
                        Assert(0); // ??? 
                    } break;
                }
                bc->comment = "assignment";
            } break;

            case tac::block_value: {
                clean_temps();
                push_temp(tac);
            } break;

            case tac::param: {
                push_temp(tac);
            } break;

            case tac::call: {
                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::call;
                bc->f = tac->arg0.func;
                bc->n_params = tac->arg1.literal._u64;
                stack_offset = stack_offset - bc->n_params + tac->temp_size;
                tac->temp_pos = stack_offset - tac->temp_size;
                // manually remove argument temps because they are cleared when
                // the function returns 
                scoped_temps.readref(-1) -= bc->n_params - tac->temp_size;
                bc->comment = DString::create("return value at pos ", tac->temp_pos);
            } break;

            case tac::ret: {
                u64 ret_size = 0;

                if(tac->arg0.kind != arg::none) {
                    BC* bc = seq.push();
                    bc->tac = tac;
                    bc->node = tac->node;
                    bc->instr = air::copy;
                    bc->copy.dst = 0;
                    
                    switch(tac->arg0.kind) {
                        case arg::literal: {
                            bc->flags.right_is_const = true;
                            bc->copy.literal = tac->arg0.literal;
                            ret_size = bc->copy.literal.size();
                        } break;

                        case arg::temporary: {
                            bc->copy.src = tac->arg0.temporary->temp_pos;
                            ret_size = tac->arg0.temporary->temp_size;
                        } break;

                        case arg::var: {
                            bc->copy.src = tac->arg0.var->stack_offset;
                            ret_size = tac->arg0.var->type->size();
                        } break;
                    }
                }

                BC* ret = seq.push();
                ret->tac = tac;
                ret->node = tac->node;
                ret->instr = air::ret;
                ret->flags.left_is_const = true;
                ret->lhs = ret_size;
            } break;

            case tac::resz: {
                BC* temp = seq.push();
                temp->tac = tac;
                temp->node = tac->node;
                temp->instr = air::push;
                temp->lhs = u64(0);
                temp->flags.left_is_const = true;
                temp->rhs = tac->temp_size;

                tac->temp_pos = stack_offset;

                scoped_temps.readref(-1) += temp->rhs._u64;
                stack_offset += temp->rhs._u64;

                BC* bc = seq.push();
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::resz;
                bc->resz.dst = tac->temp_pos;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->resz.src = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->resz.src = tac->arg0.var->stack_offset;
                    } break;
                }
                
                bc->resz.from = tac->arg1.cast.from;
                bc->resz.to = tac->arg1.cast.to;
            } break;

            case tac::array_element: {
                push_temp(tac);
            } break;

            case tac::array_literal: {
                tac->temp_pos = tac->arg0.temporary->temp_pos;
            } break;

            case tac::subscript: {
                BC* out = 0;

                switch(tac->arg0.kind) {
                    case arg::var: {
                        auto v = tac->arg0.var;
                        switch(tac->arg1.kind) {
                            case arg::var: {
                                auto vr = tac->arg1.var;
                                BC* temp = seq.push();
                                temp->tac = tac;
                                temp->node = tac->node;
                                temp->instr = air::push;
                                temp->flags.left_is_const = true;
                                temp->lhs = v->stack_offset;
                                temp->rhs = u64(8);
                                u64 temp_pos = stack_offset;
                                stack_offset += 8;
                                scoped_temps.readref(-1) += 8;
                                tac->temp_pos = temp_pos;

                                BC* add = seq.push();
                                add->tac = tac;
                                add->node = tac->node;
                                add->instr = air::add;
                                add->lhs = temp_pos;
                                add->rhs = vr->stack_offset;

                                BC* mult = seq.push();
                                mult->tac = tac;
                                mult->node = tac->node;
                                mult->instr = air::mul;
                                mult->lhs = temp_pos;
                                mult->flags.right_is_const = true;
                                mult->rhs = tac->temp_size;

                                if(!tac->lvalue) {
                                    out = seq.push();
                                    out->tac = tac;
                                    out->node = tac->node;
                                    out->instr = air::push;
                                    out->flags.deref_left = true;
                                    out->lhs = temp_pos;
                                    out->rhs = tac->temp_size;
                                } else {
                                    out = mult;
                                }
                                
                            } break;
                            case arg::temporary: {
                                TAC* t = tac->arg1.temporary;

                                BC* mult = seq.push();
                                mult->tac = tac;
                                mult->node = tac->node;
                                mult->instr = air::mul;
                                mult->lhs = t->temp_pos;
                                mult->flags.right_is_const = true;
                                mult->rhs = tac->temp_size;
                                tac->temp_pos = t->temp_pos;

                                if(!tac->lvalue) {
                                    out = seq.push();
                                    out->tac = tac;
                                    out->node = tac->node;
                                    out->instr = air::push;
                                    out->flags.deref_left = true;
                                    out->lhs = t->temp_pos;
                                    out->rhs = tac->temp_size;
                                } else {
                                    // out = mult;
                                }
                            } break;
                            case arg::literal: {
                                out = seq.push();
                                out->tac = tac;
                                out->node = tac->node;
                                out->instr = air::push;
                                out->lhs = v->stack_offset + tac->arg1.literal._s64 * tac->temp_size;
                                tac->temp_pos = stack_offset;
                                
                                if(tac->lvalue) {
                                    out->flags.left_is_const = true;
                                    out->rhs = u64(8);

                                } else {
                                    out->rhs = tac->temp_size;
                                }

                                stack_offset += out->rhs._u64;
                                scoped_temps.readref(-1) += out->rhs._u64;
                            } break;
                        }
                    } break;
                    default: {
                        TODO("handle subscripting things other than direct arrays");
                    } break;
                }

                // if(out) {
                //     tac->temp_pos = stack_offset;
                //     out->comment = DString::create("temp with pos ", tac->temp_pos);
                //     stack_offset += tac->temp_size;
                //     scoped_temps.readref(-1) += tac->temp_size;
                // }
            } break; 
        }
    }
}

void GenAIR::
push_scope() {
    scoped_temps.push(u64(0));
}

void GenAIR::
pop_scope() {
    clean_temps();
    scoped_temps.pop();
}

void GenAIR::
push_temp(TAC* tac) {
    BC* out = seq.push();
    out->tac = tac;
    out->node = tac->node;
    out->instr = air::push;

    switch(tac->arg0.kind) {
        case arg::temporary: {
            out->lhs = tac->arg0.temporary->temp_pos;
            out->rhs = tac->arg0.temporary->temp_size;
        } break;
        case arg::literal: {
            out->flags.left_is_const = true;
            out->lhs = tac->arg0.literal._u64;
            out->rhs = tac->arg0.literal.size();
        } break;
        case arg::var: {
            auto v = tac->arg0.var;
            if(v->is_compile_time) {
                TODO("fix var temps");
                // if(!code->compile_time) {
                //     s64 lhs = out->lhs;
                //     forI(v->type->size()) {
                //         out->flags.left_is_const = true;
                //         out->w = width::byte;
                //         out->lhs = *(v->memory + i);
                //         out->rhs = 1;
                //         if(i != v->type->size() - 1) {
                //             out = seq.push();
                //             out->instr = air::push;
                //             out->node = tac->node;
                //         }
                //     }
                // } else {
                //     out->flags.left_is_ptr = true;
                //     out->lhs = (s64)v->memory;
                //     out->rhs = v->type->size();
                // }
            } else {
                out->lhs = tac->arg0.var->stack_offset;
                out->rhs = tac->arg0.var->type->size();
            }
        } break;
        case arg::member: {
            out->lhs = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.offset;
            out->rhs = tac->arg0.offset_var.type->size();
        } break;
        case arg::stack_offset: {
            out->lhs = tac->arg0.stack_offset;
            TestMe; // TODO(sushi) confirm this is correct
            out->rhs = tac->temp_size;
        } break;
    }

    tac->temp_pos = stack_offset;
    out->comment = DString::create("temp with pos ", tac->temp_pos);
    stack_offset += tac->temp_size;
    scoped_temps.readref(-1) += tac->temp_size;
}

void GenAIR::
clean_temps() {
    u64& temp_count = scoped_temps.readref(-1);
    if(!temp_count) return;
    BC* last = seq.readptr(-1);
    if(last->instr == air::popn) {
        last->lhs._u64 += temp_count;
    } else {
        BC* clean = seq.push();
        clean->instr = air::popn;
        clean->lhs = temp_count;
        clean->flags.left_is_const = true;
        clean->node = last->node;
    }
    stack_offset -= temp_count;
    temp_count = 0;
}

} // namespace amu 