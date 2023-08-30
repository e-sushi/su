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

    while(pc < code->gen->air.count) {
        util::println(dstring::init("----------------------- ", pc));
        BC bc = array::read(code->gen->air, pc);
        util::println(to_string(&bc, code));
        switch(bc.instr) {
            case air::opcode::copy: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 = bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = src->_u32;
                }
            } break;

            case air::opcode::add: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 += bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 += src->_u32;
                }
            } break;

            case air::opcode::sub: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 -= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 -= src->_u32;
                }
            } break;

            case air::opcode::mul: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 *= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 *= src->_u32;
                }
            } break;

            case air::opcode::div: {
                Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
                if(bc.flags.right_is_const) {
                    dst->_u32 /= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 /= src->_u32;
                }
            } break;

            case air::opcode::jump_zero: {
                if(bc.flags.left_is_const) {
                    if(!bc.offset_a) pc += bc.offset_b - 1;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_a - 1);
                    if(!src->_u32) {
                        pc += bc.offset_b - 1;
                    }
                }
            } break;

            case air::opcode::jump: {
                pc += bc.offset_a - 1;
            } break;
        }
        forI(registers.count) {
            Register r = array::read(registers, i);
            util::println(dstring::init(r, " ", r._u32));
        }
        pc += 1;
    } 


    forI(code->gen->air.count) {
        
    }

    
} 

} // namespace amu
