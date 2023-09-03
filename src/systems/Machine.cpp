namespace amu {

Machine* Machine::
create(Code* entry) {
    Machine* out = pool::add(compiler::instance.storage.machines);
    out->registers = array::init<Register>();
    entry->machine = out;
    

    if(entry->is(code::function)) {
                       // WOW!
        out->frame.f = entry->parser->root->as<Label>()->entity->as<Function>();
        out->frame.ip = entry->air_gen->seq.data;
        out->registers = array::init<Register>();
        forI(128) {
            array::push(out->registers);
        }
        out->frame.fp = out->registers.data;
        out->stack_top = out->registers.data;
    } else {
        // TODO(sushi) we'll need to implement a fake CallFrame or just be able to 
        //             use some kind of alternative
        NotImplemented;
    }

    return out;
}



void Machine::
run() {

    auto start = util::stopwatch::start();

    // return;

    b32 finished = 0;
    while(!finished) { 
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        util::println(dstring::init("----------------------- ", frame.ip, " of ", frame.f->name()));
        util::println(to_string(*frame.ip));
        switch(frame.ip->instr) {
            case air::op::push: {
                Register* dst = stack_top;
                if(frame.ip->flags.left_is_const) {
                    dst->_u32 = frame.ip->offset_a;
                } else {
                    Register* src = frame.fp + frame.ip->offset_a;
                    dst->_u32 = src->_u32;
                }
                stack_top += 1;
            } break;

            case air::op::pushn: {
                stack_top += frame.ip->offset_a;
            } break;

            case air::op::popn: {
                stack_top -= frame.ip->offset_a;
            } break;

            case air::op::pop: {
                stack_top--;
            } break;

            case air::op::copy: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = src->_u32;
                }
            } break;

            case air::op::add: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 += frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 += src->_u32;
                }
            } break;

            case air::op::sub: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 -= frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 -= src->_u32;
                }
            } break;

            case air::op::mul: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 *= frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 *= src->_u32;
                }
            } break;

            case air::op::div: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 /= frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 /= src->_u32;
                }
            } break;

            case air::op::jump_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(!frame.ip->offset_a) frame.ip += frame.ip->offset_b - 1;
                } else {
                    Register* src = frame.fp + frame.ip->offset_a;
                    if(!src->_u32) {
                        frame.ip += frame.ip->offset_b - 1;
                    }
                }
            } break;

            case air::op::jump_not_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(frame.ip->offset_a) frame.ip += frame.ip->offset_b - 1;
                } else {
                    Register* src = frame.fp + frame.ip->offset_a;
                    if(src->_u32) {
                        frame.ip += frame.ip->offset_b - 1;
                    }
                }
            } break;

            case air::op::jump: {
                frame.ip += frame.ip->offset_a - 1;
            } break;

            case air::op::eq: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 == frame.ip->offset_b;    
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 == src->_u32;
                }
            } break;

            case air::op::neq: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 != frame.ip->offset_b;    
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 != src->_u32;
                }
            } break;

            case air::op::lt: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 < frame.ip->offset_b;    
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 < src->_u32;
                }
            } break;

            case air::op::gt: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 > frame.ip->offset_b;
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 > src->_u32;
                }
            } break;

            case air::op::le: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 <= frame.ip->offset_b;    
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 <= src->_u32;
                }
            } break;

            case air::op::ge: {
                Register* dst = frame.fp + frame.ip->offset_a;
                if(frame.ip->flags.right_is_const) {
                    dst->_u32 = dst->_u32 >= frame.ip->offset_b;    
                } else {
                    Register* src = frame.fp + frame.ip->offset_b;
                    dst->_u32 = dst->_u32 >= src->_u32;
                }
            } break;

            case air::op::call: {
                array::push(frames, frame);
                frame.f = frame.ip->f;
                frame.fp = stack_top - frame.ip->n_params;
                frame.ip = frame.f->code->air_gen->seq.data - 1;
                stack_top = frame.fp;
            } break;

            case air::op::ret: {
                if(!frames.count){
                    finished = true;
                    break;
                }
                CallFrame last = frame;
                stack_top = frame.fp + frame.ip->offset_a; 
                frame = array::pop(frames);
                util::println(to_string(*frame.ip));
            } break;
        }

        Register* sp = frame.fp;
        forI(stack_top - frame.fp + 1) {
            util::print(dstring::init("r", i, " ", sp->_u32));
            if(stack_top == sp) {
                util::print(dstring::init("<"));
            }
            util::println("");
            sp += 1;
        }
        frame.ip += 1;
    } 

    util::println(util::format_time(util::stopwatch::peek(start)));
} 

void Machine::
print_stack() {
    DString out = dstring::init();

    auto my_frames = array::init<CallFrame*>();
    forI(frames.count)
        array::push(my_frames, array::readptr(frames, i));
    array::push(my_frames, &frame);

    u32 frame_idx = 0;

    Register* p = registers.data;
    while(p < stack_top) {
        dstring::append(out, "r", p-registers.data, " ", p->_u64);
        if(frame_idx < my_frames.count && array::read(my_frames, frame_idx)->fp == p) {    
            dstring::append(out, " <-- fp of ", array::read(my_frames, frame_idx)->f->name());
            frame_idx++;
        }
        dstring::append(out, "\n");
        p++;
    }
    util::println(out);
    dstring::deinit(out);
}

} // namespace amu
