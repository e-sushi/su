namespace amu {

Machine* Machine::
create(Code* code) {
    Machine* out = pool::add(compiler::instance.storage.machines);
    out->registers = array::init<Register>();
    out->code = code;
    code->machine = out;
    return out;
}

void Machine::
run() {
    // push the registers on the stack backwards so that we don't have to readjust 
    // where bytecode offsets point to 
    forI_reverse(code->gen->registers.count) {
        array::push(registers, array::read(code->gen->registers, i));
    }

    u32 pc = 0;

    // return;

    while(pc < code->gen->air.count) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        util::println(dstring::init("----------------------- ", pc));
        BC bc = array::read(code->gen->air, pc);
        util::println(to_string(&bc, code));
        switch(bc.instr) {
            case air::op::copy: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = src->_u32;
                }
            } break;

            case air::op::add: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 += bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 += src->_u32;
                }
            } break;

            case air::op::sub: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 -= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 -= src->_u32;
                }
            } break;

            case air::op::mul: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 *= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 *= src->_u32;
                }
            } break;

            case air::op::div: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 /= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 /= src->_u32;
                }
            } break;

            case air::op::jump_zero: {
                if(bc.flags.left_is_const) {
                    if(!bc.offset_a) pc += bc.offset_b - 1;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_a - 1);
                    if(!src->_u32) {
                        pc += bc.offset_b - 1;
                    }
                }
            } break;

            case air::op::jump_not_zero: {
                if(bc.flags.left_is_const) {
                    if(bc.offset_a) pc += bc.offset_b - 1;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_a - 1);
                    if(src->_u32) {
                        pc += bc.offset_b - 1;
                    }
                }
            } break;

            case air::op::jump: {
                pc += bc.offset_a - 1;
            } break;

            case air::op::eq: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 == bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 == src->_u32;
                }
            } break;

            case air::op::neq: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 != bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 != src->_u32;
                }
            } break;

            case air::op::lt: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 < bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 < src->_u32;
                }
            } break;

            case air::op::gt: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 > bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 > src->_u32;
                }
            } break;

            case air::op::le: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 <= bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 <= src->_u32;
                }
            } break;

            case air::op::ge: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = dst->_u32 >= bc.offset_b;    
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = dst->_u32 >= src->_u32;
                }
            } break;
        }
        forI(registers.count) {
            Register r = array::read(registers, i);
            util::println(dstring::init(r, " ", r._u32));
        }
        pc += 1;
    } 
} 

} // namespace amu
