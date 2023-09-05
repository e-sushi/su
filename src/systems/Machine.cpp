namespace amu {

Machine* Machine::
create(Code* entry) {
    Machine* out = pool::add(compiler::instance.storage.machines);
    // arbitrary amount of stack to allocate
    // this needs to be a compiler option later 
    out->stack = (u8*)memory::allocate(Megabytes(16));
    entry->machine = out;
    

    if(entry->is(code::function)) {
                       // WOW!
        out->frame.f = entry->parser->root->as<Label>()->entity->as<Function>();
        out->frame.ip = entry->air_gen->seq.data;
        out->frame.fp = out->stack;
        out->sp = out->stack;
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

    return;

    #define sized_op(op, sz, dst, src)                \
    switch(sz) {                                      \
        case byte: *(dst) op (u8)(src); break;        \
        case word: *(u16*)(dst) op (u16)(src); break; \
        case dble: *(u32*)(dst) op (u32)(src); break; \
        case quad: *(u64*)(dst) op (u64)(src); break; \
    }

    #define sized_op_flt(op, sz, dst, src)            \
    switch(sz) {                                      \
        case dble: *(f32*)(dst) op (f32)(src); break; \
        case quad: *(f64*)(dst) op (f64)(src); break; \
    }

    #define sized_op_assign(op, sz, dst, src)                        \
    switch(sz) {                                                     \
        case byte: *(dst) = *(dst) op (u8)(src); break;              \
        case word: *(u16*)(dst) = *(u16*)(dst) op (u16)(src); break; \
        case dble: *(u32*)(dst) = *(u32*)(dst) op (u32)(src); break; \
        case quad: *(u64*)(dst) = *(u64*)(dst) op (u64)(src); break; \
    }

    #define sized_op_assign_flt(op, sz, dst, src)                    \
    switch(sz) {                                                     \
        case dble: *(f32*)(dst) = *(f32*)(dst) op (f32)(src); break; \
        case quad: *(f64*)(dst) = *(f64*)(dst) op (f64)(src); break; \
    }

    b32 finished = 0;
    while(!finished) { 
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        util::println(DString::create("----------------------- ", frame.ip, " of ", frame.f->name()));
        util::println(to_string(*frame.ip));
        BC* instr = frame.ip;
        switch(instr->instr) {
            case air::op::push: {
                u8* dst = sp;
                if(frame.ip->flags.left_is_const) {
                    sized_op(=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    sized_op(=, instr->w, dst, *src);
                    sp += instr->w + *src;
                }
            } break;

            case air::op::pushn: sp += frame.ip->lhs; break;
            case air::op::popn:  sp -= frame.ip->lhs; break;

            case air::op::copy: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op(=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op(=, instr->w, dst, *src);
                }
            } break;

            case air::op::add: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op(+=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(+=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op(+=, instr->w, dst, *src);
                }
            } break;

            case air::op::sub: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op(-=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(-=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op(-=, instr->w, dst, *src);
                }
            } break;

            case air::op::mul: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op(*=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(*=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op(*=, instr->w, dst, *src);
                }
            } break;

            case air::op::div: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op(/=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_flt(/=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op(/=, instr->w, dst, *src);
                }
            } break;

            case air::op::jump_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(!frame.ip->lhs) frame.ip += frame.ip->rhs - 1;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    b32 cond = 0;
                    switch(instr->w) {
                        case byte: cond = cond == *src; break;
                        case word: cond = cond == *(u16*)src; break;
                        case dble: cond = cond == *(u32*)src; break;
                        case quad: cond = cond == *(u64*)src; break;
                    }
                    if(cond) frame.ip += frame.ip->rhs - 1;
                }
            } break;

            case air::op::jump_not_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(frame.ip->lhs) frame.ip += frame.ip->rhs - 1;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    b32 cond = 1;
                    switch(instr->w) {
                        case byte: cond = cond == *src; break;
                        case word: cond = cond == *(u16*)src; break;
                        case dble: cond = cond == *(u32*)src; break;
                        case quad: cond = cond == *(u64*)src; break;
                    }
                    if(cond) frame.ip += frame.ip->rhs - 1;
                }
            } break;

            case air::op::jump: {
                frame.ip += frame.ip->lhs - 1;
            } break;

            // TODO(sushi) this sucks do it better later 
            case air::op::eq: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(==, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(==, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(==, instr->w, dst, *src);
                }
            } break;

            case air::op::neq: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(!=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(!=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(!=, instr->w, dst, *src);
                }
            } break;

            case air::op::lt: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(<, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(<, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(<, instr->w, dst, *src);
                }
            } break;

            case air::op::gt: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(>, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(>, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(>, instr->w, dst, *src);
                }
            } break;

            case air::op::le: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(<=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(<=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(<=, instr->w, dst, *src);
                }
            } break;

            case air::op::ge: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    sized_op_assign(>=, instr->w, dst, frame.ip->rhs);
                } else if(frame.ip->flags.float_op) {
                    sized_op_assign_flt(>=, instr->w, dst, instr->lhs);
                    sp += instr->w + instr->lhs;
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    sized_op_assign(>=, instr->w, dst, *src);
                }
            } break;

            case air::op::call: {
                array::push(frames, frame);
                frame.f = frame.ip->f;
                frame.fp = sp - frame.ip->n_params;
                frame.ip = frame.f->code->air_gen->seq.data - 1;
                sp = frame.fp;
            } break;

            case air::op::ret: {
                if(!frames.count){
                    finished = true;
                    break;
                }
                CallFrame last = frame;
                sp = frame.fp + frame.ip->lhs; 
                frame = array::pop(frames);
                util::println(to_string(*frame.ip));
            } break;
        }

        u8* ss = frame.fp;
        forI(sp - frame.fp + 1) {
            util::print(DString::create("sp+", i, " ", *sp));
            if(sp == ss) {
                util::print(DString::create("<"));
            }
            util::println("");
            ss += 1;
        }
        frame.ip += 1;
    } 

    util::println(util::format_time(util::stopwatch::peek(start)));
} 

void Machine::
print_stack() {
    DString* out = DString::create();

    auto my_frames = array::init<CallFrame*>();
    forI(frames.count)
        array::push(my_frames, array::readptr(frames, i));
    array::push(my_frames, &frame);

    u32 frame_idx = 0;

    u8* p = stack;
    while(p < sp) {
        out->append("r", p-stack, " ", *p);
        if(frame_idx < my_frames.count && array::read(my_frames, frame_idx)->fp == p) {    
            out->append(" <-- fp of ", array::read(my_frames, frame_idx)->f->name());
            frame_idx++;
        }
        out->append("\n");
        p++;
    }
    util::println(out);
    out->deref();
}

} // namespace amu
