namespace amu {

GenAIR* GenAIR::
create(Code* code) {
    GenAIR* out = pool::add(compiler::instance.storage.air_gens);
    out->seq = array::init<BC>();
    out->code = code;
    code->air_gen = out;
    out->scoped_temps = array::init<u64>();
    out->stack_things = map::init<BC*, StackThing>();
    array::push(out->scoped_temps, u64(0));
    return out;
}

void GenAIR::
generate() {
    start();

    util::println(code->identifier);
    u32 last_line_num = -1;
    TAC* last_tac = 0;
    forI(seq.count) {
        BC bc = array::read(seq, i);
        if(bc.tac && bc.tac != last_tac) {
            last_tac = bc.tac;
            util::println(to_string(bc.tac));
        }
        util::println(to_string(bc));
    }
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
            f->locals = map::init<s64, Var*>();
            
            // actually the first thing we need to do is see if we are returning something!
            stack_offset += f->type->return_type->size();

            // and then parameters!
            for(Label* n = f->type->parameters->first_child<Label>(); n; n = n->next<Label>()) {
                Var* v = n->entity->as<Var>();
                v->stack_offset = stack_offset;
                map::add(f->locals, (s64)stack_offset, v);
                stack_offset += v->type->size();
            }

            // the first thing we need to do is figure out the locals of this function and
            // setup their Vars to be positioned correctly on the stack
            forI(code->tac_gen->locals.count) {
                auto v = array::read(code->tac_gen->locals, i);
                v->stack_offset = stack_offset;
                if(v->type->is<Structured>()) {
                    // uhm i forget what I was going to do here
                }
                map::add(f->locals, (s64)v->stack_offset, v);
                stack_offset += v->type->size();
            }
            BC* bc = array::push(seq);
            bc->tac = array::read(code->tac_gen->seq, 0);
            bc->node = f->code->parser->root;
            bc->instr = air::pushn;
            bc->lhs = stack_offset;
            bc->flags.left_is_const = true;
            bc->comment = "make room for function locals";
            body();
        } break;
        case code::typedef_: { // typedefs dont generate anything for now 

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
        TAC* tac = array::read(tac_seq, i);
        if(tac->node->flags.break_air_gen) DebugBreakpoint;
        tac->bc_offset = seq.count;

        if(tac->from) {
            TAC* from = tac->from;
            while(from) {
                if(from->bc_offset != -1) {
                    BC* bc = array::readptr(seq, from->bc_offset);
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
                BC* bc = array::push(seq);
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::pushn;
                bc->lhs = stack_offset;
                bc->comment = "push function's local stack";
                // ret_start = tac->arg1.literal._u64;
            } break;

            case tac::jump_zero: {
                BC* bc = array::push(seq);
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
                }
            } break;

            case tac::jump_not_zero: {
                BC* bc = array::push(seq);
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
                }

            } break;

            case tac::jump: {
                BC* bc = array::push(seq);
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
                // // Register* r = array::push(registers);
            } break;

            case tac::addition:
            case tac::subtraction:
            case tac::multiplication:
            case tac::division: {
                push_temp(tac);
                
                BC* bc1 = array::push(seq);
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
                        bc1->rhs = tac->arg1.offset_var.var->stack_offset + tac->arg1.offset_var.member->offset;
                        if(tac->arg1.offset_var.member->type->is_any(scalar::float32, scalar::float64)) {
                            bc1->flags.float_op = true;
                        }
                        switch(tac->arg1.offset_var.member->type->size()) {
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
                        switch(tac->arg1.literal.kind) {
                            case scalar::float64: {
                                bc1->rhs_f = tac->arg1.literal._f64;
                                bc1->flags.float_op = true;
                                bc1->w = width::quad;
                            } break;
                            case scalar::float32: {
                                bc1->rhs = tac->arg1.literal._f32;
                                bc1->flags.float_op = true;
                                bc1->w = width::dble;
                            } break;
                            case scalar::signed64: // TODO(sushi) BC's l/rhs are 4 bytes not 8 bytes wide so this will not work correctly
                            case scalar::unsigned64: {
                                bc1->rhs = tac->arg1.literal._s64;
                                bc1->w = width::quad;
                            } break;
                            case scalar::signed32: 
                            case scalar::unsigned32: {
                                bc1->rhs = tac->arg1.literal._s32;
                                bc1->w = width::dble;
                            } break;
                            case scalar::signed16: 
                            case scalar::unsigned16: {
                                bc1->rhs = tac->arg1.literal._s16;
                                bc1->w = width::word;
                            } break;
                            case scalar::signed8: 
                            case scalar::unsigned8: {
                                bc1->rhs = tac->arg1.literal._s8;
                                bc1->w = width::byte;
                            } break;
                        }
                    } break;
                    case arg::var: {
                        bc1->rhs = tac->arg1.var->stack_offset; 
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

                BC* bc1 = array::push(seq);
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
                BC* bc = array::push(seq);
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::copy;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->lhs = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->lhs = tac->arg0.var->stack_offset;
                    } break;
                    case arg::member: {
                        bc->lhs = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.member->offset;
                    } break;
                    case arg::stack_offset: {
                        bc->lhs = tac->arg0.stack_offset;
                    } break;
                }

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc->rhs = tac->arg1.temporary->temp_pos;
                        switch(tac->arg1.temporary->temp_size) {
                            case 1: bc->w = width::byte; break; 
                            case 2: bc->w = width::word; break;
                            case 4: bc->w = width::dble; break;
                            case 8: bc->w = width::quad; break;
                        }
                    } break;
                    case arg::var: {
                        bc->rhs = tac->arg1.var->stack_offset;
                    } break;
                    case arg::literal: {
                        bc->flags.right_is_const = true;
                        switch(tac->arg1.literal.kind) {
                            case scalar::float64: {
                                bc->rhs_f = tac->arg1.literal._f64;
                                bc->flags.float_op = true;
                                bc->w = width::quad;
                            } break;
                            case scalar::float32: {
                                bc->rhs_f = tac->arg1.literal._f32;
                                bc->flags.float_op = true;
                                bc->w = width::dble;
                            } break;
                            case scalar::signed64: // TODO(sushi) BC's l/rhs are 4 bytes not 8 bytes wide so this will not work correctly
                            case scalar::unsigned64: {
                                bc->rhs = tac->arg1.literal._s64;
                                bc->w = width::quad;
                            } break;
                            case scalar::signed32: 
                            case scalar::unsigned32: {
                                bc->rhs = tac->arg1.literal._s32;
                                bc->w = width::dble;
                            } break;
                            case scalar::signed16: 
                            case scalar::unsigned16: {
                                bc->rhs = tac->arg1.literal._s16;
                                bc->w = width::word;
                            } break;
                            case scalar::signed8: 
                            case scalar::unsigned8: {
                                bc->rhs = tac->arg1.literal._s8;
                                bc->w = width::byte;
                            } break;
                        }
                    } break;
                    case arg::stack_offset: {
                        bc->rhs = tac->arg1.stack_offset;
                    } break;
                }

                bc->comment = "assignment";
            } break;

            case tac::block_value: {
                BC* bc = array::push(seq);
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::copy;
                bc->lhs = stack_offset;
                stack_offset += tac->temp_size;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->rhs = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->rhs = tac->arg0.var->stack_offset;
                    } break;
                    case arg::literal: {
                        bc->flags.right_is_const = true;
                        bc->rhs = tac->arg0.literal._u64;
                    } break;
                    case arg::stack_offset: {
                        bc->rhs = tac->arg0.stack_offset;
                    } break;
                }
            } break;

            case tac::param: {
                push_temp(tac);
            } break;

            case tac::call: {
                BC* bc = array::push(seq);
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::call;
                bc->f = tac->arg0.func;
                bc->n_params = tac->arg1.literal._u64;
                stack_offset = stack_offset - bc->n_params + tac->temp_size;
                tac->temp_pos = stack_offset - tac->temp_size;
                // manually remove argument temps because they are cleared when
                // the function returns 
                array::readref(scoped_temps, -1) -= bc->n_params - tac->temp_size;
                bc->comment = DString::create("return value at pos ", tac->temp_pos);
            } break;

            case tac::ret: {
                u64 ret_size = 0;

                if(tac->arg0.kind != arg::none) {
                    BC* bc = array::push(seq);
                    bc->tac = tac;
                    bc->node = tac->node;
                    bc->instr = air::copy;
                    bc->lhs = 0;
                    
                    switch(tac->arg0.kind) {
                        case arg::literal: {
                            bc->flags.right_is_const = true;
                            bc->rhs = tac->arg0.literal._u64;
                            ret_size = 1;
                        } break;

                        case arg::temporary: {
                            bc->rhs = tac->arg0.temporary->temp_pos;
                            ret_size = tac->arg0.temporary->temp_size;
                        } break;

                        case arg::var: {
                            bc->rhs = tac->arg0.var->stack_offset;
                            ret_size = tac->arg0.var->type->size();
                        } break;
                    }

                    switch(ret_size) {
                        case 1: bc->w = width::byte; break;
                        case 2: bc->w = width::word; break;
                        case 4: bc->w = width::dble; break;
                        case 8: bc->w = width::quad; break;
                    }
                }

                BC* ret = array::push(seq);
                ret->tac = tac;
                ret->node = tac->node;
                ret->instr = air::ret;
                ret->flags.left_is_const = true;
                ret->lhs = ret_size;
            } break;

            case tac::resz: {
                push_temp(tac);

                BC* bc = array::push(seq);
                bc->tac = tac;
                bc->node = tac->node;
                bc->instr = air::resz;
                bc->lhs = tac->temp_pos;
                
                switch(tac->arg1.kind) {
                    case arg::width: {
                        bc->w = tac->arg1.w;
                        if(tac->is_float) {
                            bc->flags.float_op = true;
                        } else {
                            bc->flags.right_is_const = true;
                        }
                    } break;
                }

            } break;
        }
    }
}

void GenAIR::
push_scope() {
    array::push(scoped_temps, u64(0));
}

void GenAIR::
pop_scope() {
    clean_temps();
    array::pop(scoped_temps);
}

void GenAIR::
push_temp(TAC* tac) {
    BC* out = array::push(seq);
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
        } break;
        case arg::var: {
            out->lhs = tac->arg0.var->stack_offset;
            out->rhs = tac->arg0.var->type->size();
        } break;
        case arg::member: {
            out->lhs = tac->arg0.offset_var.var->stack_offset + tac->arg0.offset_var.member->offset;
            out->rhs = tac->arg0.offset_var.member->type->size();
        } break;
        case arg::stack_offset: {
            out->lhs = tac->arg0.stack_offset;
            TestMe; // TODO(sushi) confirm this is correct
            out->rhs = tac->temp_size;
        } break;
    }

    out->rhs = tac->temp_size;

    tac->temp_pos = stack_offset;
    out->comment = DString::create("temp with pos ", tac->temp_pos);
    stack_offset += tac->temp_size;
    array::readref(scoped_temps, -1) += tac->temp_size;
}

void GenAIR::
clean_temps() {
    u64& temp_count = array::readref(scoped_temps, -1);
    if(!temp_count) return;
    BC* last = array::readptr(seq, -1);
    if(last->instr == air::popn) {
        last->lhs += temp_count;
    } else {
        BC* clean = array::push(seq);
        clean->instr = air::popn;
        clean->lhs = temp_count;
        clean->flags.left_is_const = true;
        clean->node = last->node;
    }
    stack_offset -= temp_count;
    temp_count = 0;
}

} // namespace amu 