namespace amu {

GenAIR* GenAIR::
create(Code* code) {
    GenAIR* out = pool::add(compiler::instance.storage.air_gens);
    out->seq = array::init<BC>();
    out->code = code;
    code->air_gen = out;
    out->scoped_temps = array::init<u64>();
    array::push(out->scoped_temps, u64(0));
    return out;
}

void GenAIR::
generate() {
    start();

    util::println(code->identifier);
    u32 last_line_num = -1;
    forI(seq.count) {
        BC bc = array::read(seq, i);
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
            stack_offset = f->local_size;
            body();
            f->stack_size = stack_offset;
        } break;
        default: {
            TODO(dstring::init("unhandled start case: ", code::strings[code->kind]));
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
                        bc->offset_a = seq.count - from->bc_offset;
                    } else {
                        bc->offset_b = seq.count - from->bc_offset;
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
                stack_offset = tac->arg0._u64;
                BC* bc = array::push(seq);
                bc->instr = air::pushn;
                bc->offset_a = stack_offset;
                bc->comment = "push function's local stack";
                // ret_start = tac->arg1._u64;
            } break;

            case tac::jump_zero: {
                BC* bc = array::push(seq);
                bc->instr = air::jump_zero;
                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->offset_a = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::literal_u64: {
                        bc->flags.left_is_const = true;
                        bc->offset_a = tac->arg0._u64;
                    } break;
                    case arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset; 
                    } break;
                }
            } break;

            case tac::jump_not_zero: {
                BC* bc = array::push(seq);
                bc->instr = air::jump_not_zero;
                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->offset_a = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::literal_u64: {
                        bc->flags.left_is_const = true;
                        bc->offset_a = tac->arg0._u64;
                    } break;
                    case arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset; 
                    } break;
                }

            } break;

            case tac::jump: {
                BC* bc = array::push(seq);
                bc->instr = air::jump;
                if(tac->to && tac->to->bc_offset != -1) {
                    bc->offset_a = tac->to->bc_offset - seq.count + 1;
                }
            } break;

            case tac::temp: {
                // push_temp(tac);
                // FixMe; // map::add(offset_map, tac, (u32)registers.count);
                // // tac->arg0.kind = arg::reg;
                // // tac->arg0.reg_offset = registers.count;
                // // Register* r = array::push(registers);
            } break;

            case tac::addition:
            case tac::subtraction:
            case tac::multiplication:
            case tac::division: {
                push_temp(tac);
                
                BC* bc1 = array::push(seq);
                bc1->instr = 
                    (tac->op == tac::addition ?       air::add :
                     tac->op == tac::subtraction ?    air::sub :
                     tac->op == tac::multiplication ? air::mul :
                                                      air::div);
                bc1->offset_a = tac->temp_pos;

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc1->offset_b = tac->arg1.temporary->temp_pos;
                    } break;
                    case arg::literal_u64: {
                        bc1->flags.right_is_const = true;
                        bc1->offset_b = tac->arg1._u64;
                    } break;
                    case arg::var: {
                        bc1->offset_b = tac->arg1.var->reg_offset; 
                    } break;
                    case arg::reg: {
                        bc1->offset_b = tac->arg1.reg_offset;
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
                bc1->instr = 
                    (tac->op == tac::equal                ? air::eq  :
                     tac->op == tac::not_equal            ? air::neq :
                     tac->op == tac::less_than            ? air::lt  :
                     tac->op == tac::less_than_or_equal   ? air::le  :
                     tac->op == tac::greater_than         ? air::gt  :
                                                            air::ge);
                bc1->offset_a = tac->temp_pos;

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc1->offset_b = tac->arg1.temporary->temp_pos;
                    } break;
                    case arg::literal_u64: {
                        bc1->flags.right_is_const = true;
                        bc1->offset_b = tac->arg1._u64;
                    } break;
                    case arg::var: {
                        bc1->offset_b = tac->arg1.var->reg_offset; 
                    } break;
                    case arg::reg: {
                        bc1->offset_b = tac->arg1.reg_offset;
                    } break;
                }
            } break;
            
            case tac::logical_or: {

            } break;

            case tac::assignment: {
                BC* bc = array::push(seq);
                bc->instr = air::copy;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->offset_a = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset;
                    } break;
                    case arg::reg: {
                        bc->offset_a = tac->arg0.reg_offset;
                    } break;
                }

                switch(tac->arg1.kind) {
                    case arg::temporary: {
                        bc->offset_b = tac->arg1.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->offset_b = tac->arg1.var->reg_offset;
                    } break;
                    case arg::literal_u64: {
                        bc->flags.right_is_const = true;
                        bc->offset_b = tac->arg1._u64;
                    } break;
                    case arg::reg: {
                        bc->offset_b = tac->arg1.reg_offset;
                    } break;
                }

                bc->comment = "assignment";
            } break;

            case tac::block_value: {
                BC* bc = array::push(seq);
                bc->instr = air::copy;
                bc->offset_a = stack_offset;
                stack_offset += tac->temp_size;

                switch(tac->arg0.kind) {
                    case arg::temporary: {
                        bc->offset_b = tac->arg0.temporary->temp_pos;
                    } break;
                    case arg::var: {
                        bc->offset_b = tac->arg0.var->reg_offset;
                    } break;
                    case arg::literal_u64: {
                        bc->flags.right_is_const = true;
                        bc->offset_b = tac->arg0._u64;
                    } break;
                    case arg::reg: {
                        bc->offset_b = tac->arg0.reg_offset;
                    } break;
                }
            } break;

            case tac::param: {
                push_temp(tac);
            } break;

            case tac::call: {
                BC* bc = array::push(seq);
                bc->instr = air::call;
                bc->f = tac->arg0.func;
                bc->n_params = tac->arg1._u64;
                stack_offset = stack_offset - bc->n_params + tac->temp_size;
                tac->temp_pos = stack_offset - tac->temp_size;
                // manually remove argument temps because they are cleared when
                // the function returns 
                array::readref(scoped_temps, -1) -= bc->n_params - tac->temp_size;
                bc->comment = dstring::init("return value at pos ", tac->temp_pos);
            } break;

            case tac::ret: {
                u64 ret_size = 0;

                if(tac->arg0.kind != arg::none) {
                    BC* bc = array::push(seq);
                    bc->instr = air::copy;
                    bc->offset_a = 0;
                    
                    switch(tac->arg0.kind) {
                        case arg::literal_u64: {
                            bc->flags.right_is_const = true;
                            bc->offset_b = tac->arg0._u64;
                            ret_size = 1;
                        } break;

                        case arg::temporary: {
                            bc->offset_b = tac->arg0.temporary->temp_pos;
                            ret_size = tac->arg0.temporary->temp_size;
                        } break;
                    }
                }

                BC* ret = array::push(seq);
                ret->instr = air::ret;
                ret->flags.left_is_const = true;
                ret->offset_a = ret_size;
            } break;
        }
    }
}

u64 GenAIR::
new_reg(Type* t) {
    // u64 size = util::Max(1, t->size() / sizeof(Register));
    // u64 out = register_offset;
    // register_offset += size;
    // return out;
    return 0;
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
    out->instr = air::push;

    switch(tac->arg0.kind) {
        case arg::temporary: {
            out->offset_a = tac->arg0.temporary->temp_pos;
        } break;
        case arg::literal_u64: {
            out->flags.left_is_const = true;
            out->offset_a = tac->arg0._u64;
        } break;
        case arg::var: {
            out->offset_a = tac->arg0.var->reg_offset; 
        } break;
        case arg::reg: {
            out->offset_a = tac->arg0.reg_offset;
        } break;
    }

    tac->temp_pos = stack_offset;
    out->comment = dstring::init("temp with pos ", tac->temp_pos);
    stack_offset += tac->temp_size;
    array::readref(scoped_temps, -1) += tac->temp_size;
}

void GenAIR::
clean_temps() {
    u64& temp_count = array::readref(scoped_temps, -1);
    if(!temp_count) return;
    BC* last = array::readptr(seq, -1);
    if(last->instr == air::popn) {
        last->offset_a += temp_count;
    } else {
        BC* clean = array::push(seq);
        clean->instr = air::popn;
        clean->offset_a = temp_count;
        clean->flags.left_is_const = true;
    }
    stack_offset -= temp_count;
    temp_count = 0;
}

} // namespace amu 