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

    forI(code->gen->air.count) {
        BC bc = array::read(code->gen->air, i);
        util::println(to_string(&bc, code));
        Register* dst = array::readptr(registers, -(s32)bc.offset_a - 1);
        switch(bc.instr) {
            case air::opcode::copy: {
                if(bc.flags.right_is_const) {
                    dst->_u32 = bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 = src->_u32;
                }
            } break;

            case air::opcode::add: {
                if(bc.flags.right_is_const) {
                    dst->_u32 += bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 += src->_u32;
                }
            } break;

            case air::opcode::sub: {
                if(bc.flags.right_is_const) {
                    dst->_u32 -= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 -= src->_u32;
                }
            } break;

            case air::opcode::mul: {
                if(bc.flags.right_is_const) {
                    dst->_u32 *= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 *= src->_u32;
                }
            } break;

            case air::opcode::div: {
                if(bc.flags.right_is_const) {
                    dst->_u32 /= bc.offset_b;
                } else {
                    Register* src = array::readptr(registers, -(s32)bc.offset_b - 1);
                    dst->_u32 /= src->_u32;
                }
            } break;
        }
        forI(registers.count) {
            Register r = array::read(registers, i);
            util::println(dstring::init(r, " ", r._u32));
        }
        util::println("-----------------------");
    }

    
} 

} // namespace amu
