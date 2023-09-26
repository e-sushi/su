#include "representations/AIR.h"
namespace amu {

VM* VM::
create(Code* entry) {
    VM* out = pool::add(compiler::instance.storage.vm);
    // arbitrary amount of stack to allocate
    // this needs to be a compiler option later 
    out->stack = (u8*)memory::allocate(Megabytes(10));
    entry->machine = out;
    
    if(entry->is(code::function)) {
        out->frame = entry->parser->root->as<Label>()->entity->as<Function>()->frame;
    } else if(entry->is(code::expression)) {
        // we create a frame for this expr
        out->frame.identifier = DString::create("ExprFrame<", (void*)entry->parser->root, ">");
    } else if(entry->is(code::var_decl)) {
        out->frame.identifier = DString::create("GlobalVarFrame<", (void*)entry->parser->root, ">");
    }
    out->frame.ip = entry->air_gen->seq.data;
    out->frame.fp = out->stack;
    out->sp = out->frame.fp;
	out->finished = false;

    return out;
}

void VM::
destroy() {
    memory::free(stack);
    pool::remove(compiler::instance.storage.vm, this);
}

BC* VM::
step() {
	if(finished) return 0;

	auto move_sp_direct = [&](u8* pos) {
        sp = pos;
    };

    auto move_sp = [&](s64 offset) {
        move_sp_direct(sp + offset);
    };

    // return;

    u64 instr_count = 0;
// std::this_thread::sleep_for(std::chrono::milliseconds(50));
	BC* instr = frame.ip;
	// util::println(DString::create("----------------------- ", instr, " of ", frame.identifier, " with sp ", sp - frame.fp));
//	util::println(ScopedDeref(DString::create("-------------------------------- sp: ", sp - frame.fp)).x);
//	util::println(instr->node->underline());
//	util::println(to_string(instr->tac));
//	util::println(ScopedDeref(to_string(*instr)).x);
	// print_stack_positions();
	switch(instr->instr) {
		case air::op::push: {
			u8* dst = sp;
			if(instr->flags.left_is_const) {
				switch(instr->lhs.kind) {
					case scalar::float32: *(f32*)dst = instr->lhs._f32; break;
					case scalar::float64: *(f64*)dst = instr->lhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = instr->lhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = instr->lhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = instr->lhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = instr->lhs._u64; break;
				}
				move_sp(instr->rhs._u64);
			} else {
				u8* src = 0;
				if(instr->flags.left_is_ptr) {
					if(instr->flags.deref_left) {
						src = (u8*)*(u64*)instr->lhs._u64;
					} else {
						src = (u8*)instr->lhs._u64;
					}
				} else {
					if(instr->flags.deref_left) {
						src = (u8*)*(u64*)(frame.fp + instr->lhs._u64);
					} else {
						src = frame.fp + instr->lhs._u64;
					}
				}
				memory::copy(dst, src, instr->rhs._u64);
				move_sp(instr->rhs._u64);
			}
		} break;

		case air::op::pushn: {
			// this causes a segfault so WHATEVER for now 
			// memory::zero(sp, instr->lhs._u64); 
			move_sp(instr->lhs._u64);
		} break;

		case air::op::popn: {
			move_sp(-instr->lhs._u64); 
			
		} break;

		case air::op::copy: {
			u8* dst = 0;
			if(instr->flags.left_is_ptr) {
				if(instr->flags.deref_left) {
					dst = (u8*)*(u64*)instr->lhs._u64;
				} else {
					dst = (u8*)instr->copy.dst;
				}
			} else {
				if(instr->flags.deref_left) {
					dst = (u8*)*(u64*)(frame.fp + instr->copy.dst);
				} else {
					dst = frame.fp + instr->copy.dst;
				}
			}
			if(instr->flags.right_is_const) {
				switch(instr->copy.literal.kind) {
					case scalar::float32: *(f32*)dst = instr->copy.literal._f32; break;
					case scalar::float64: *(f64*)dst = instr->copy.literal._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = instr->copy.literal._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = instr->copy.literal._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = instr->copy.literal._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = instr->copy.literal._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->copy.src;
				} else {
					src = frame.fp + instr->copy.src;
				}
				if(instr->flags.deref_right) {
					src = (u8*)*(u64*)src;
				}
				memory::copy(dst, src, instr->copy.size);
			}
		} break;

		case air::op::add: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst += instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst += instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst += instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst += instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst += instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst += instr->rhs._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->rhs._u64;
				} else if(instr->flags.deref_right) {
					src = (u8*)*(u64*)(frame.fp + instr->rhs._u64);
				} else {
					src = frame.fp + instr->rhs._u64;
				}
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst += *(f32*)src; break;
					case scalar::float64: *(f64*)dst += *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst += *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst += *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst += *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst += *(u64*)src; break;
				}
			}
		} break;

		case air::op::sub: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst -= instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst -= instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst -= instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst -= instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst -= instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst -= instr->rhs._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->rhs._u64;
				} else if(instr->flags.deref_right) {
					src = (u8*)*(u64*)(frame.fp + instr->rhs._u64);
				} else {
					src = frame.fp + instr->rhs._u64;
				}
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst -= *(f32*)src; break;
					case scalar::float64: *(f64*)dst -= *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst -= *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst -= *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst -= *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst -= *(u64*)src; break;
				}
			}
		} break;

		case air::op::mul: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst *= instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst *= instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst *= instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst *= instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst *= instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst *= instr->rhs._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->rhs._u64;
				} else if(instr->flags.deref_right) {
					src = (u8*)*(u64*)(frame.fp + instr->rhs._u64);
				} else {
					src = frame.fp + instr->rhs._u64;
				}
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst *= *(f32*)src; break;
					case scalar::float64: *(f64*)dst *= *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst *= *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst *= *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst *= *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst *= *(u64*)src; break;
				}
			}
		} break;

		case air::op::div: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst /= instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst /= instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst /= instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst /= instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst /= instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst /= instr->rhs._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->rhs._u64;
				} else if(instr->flags.deref_right) {
					src = (u8*)*(u64*)(frame.fp + instr->rhs._u64);
				} else {
					src = frame.fp + instr->rhs._u64;
				}
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst /= *(f32*)src; break;
					case scalar::float64: *(f64*)dst /= *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst /= *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst /= *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst /= *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst /= *(u64*)src; break;
				}
			}
		} break;

		case air::op::mod: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::signed8: *(s8*)dst %= instr->rhs._s8; break;
					case scalar::unsigned8: *(u8*)dst %= instr->rhs._u8; break;
					case scalar::signed16: *(s16*)dst %= instr->rhs._s16; break;
					case scalar::unsigned16: *(u16*)dst %= instr->rhs._u16; break;
					case scalar::signed32: *(s32*)dst %= instr->rhs._s32; break;
					case scalar::unsigned32: *(u32*)dst %= instr->rhs._u32; break;
					case scalar::signed64: *(s64*)dst %= instr->rhs._s64; break;
					case scalar::unsigned64: *(u64*)dst %= instr->rhs._u64; break;
				}
			} else {
				u8* src;
				if(instr->flags.right_is_ptr) {
					src = (u8*)instr->rhs._u64;
				} else if(instr->flags.deref_right) {
					src = (u8*)*(u64*)(frame.fp + instr->rhs._u64);
				} else {
					src = frame.fp + instr->rhs._u64;
				}
				switch(instr->rhs.kind) {
					case scalar::signed8: *(s8*)dst %= *(s8*)src; break;
					case scalar::unsigned8: *(u8*)dst %= *(u8*)src; break;
					case scalar::signed16: *(s16*)dst %= *(s16*)src; break;
					case scalar::unsigned16: *(u16*)dst %= *(u16*)src; break;
					case scalar::signed32: *(s32*)dst %= *(s32*)src; break;
					case scalar::unsigned32: *(u32*)dst %= *(u32*)src; break;
					case scalar::signed64: *(s64*)dst %= *(s64*)src; break;
					case scalar::unsigned64: *(u64*)dst %= *(u64*)src; break;
				}
			}
		} break;

		case air::op::jump_zero: {
			if(instr->flags.left_is_const) {
				if(!instr->lhs._u64) frame.ip += instr->rhs._u64 - 1;
			} else {
				u8* src = 0;
				if(instr->flags.left_is_ptr) {
					src = (u8*)instr->lhs._u64;
				} else {
					src = frame.fp + instr->lhs._u64;
				}
				if(instr->flags.deref_left) {
					src = (u8*)*(u64*)src;
				}
				b32 cond = 0;
				switch(instr->w) {
					case byte: cond = cond == *src; break;
					case word: cond = cond == *(u16*)src; break;
					case dble: cond = cond == *(u32*)src; break;
					case quad: cond = cond == *(u64*)src; break;
				}
				if(cond) frame.ip += instr->rhs._u64 - 1;
			}
		} break;

		case air::op::jump_not_zero: {
			if(instr->flags.left_is_const) {
				if(instr->lhs._u64) frame.ip += instr->rhs._u64 - 1;
			} else {
				u8* src = 0;
				if(instr->flags.left_is_ptr) {
					src = (u8*)instr->lhs._u64;
				} else {
					src = frame.fp + instr->lhs._u64;
				}
				if(instr->flags.deref_left) {
					src = (u8*)*(u64*)src;
				}
				b32 cond = 1;
				switch(instr->w) {
					case byte: cond = cond == *src; break;
					case word: cond = cond == *(u16*)src; break;
					case dble: cond = cond == *(u32*)src; break;
					case quad: cond = cond == *(u64*)src; break;
				}
				if(cond) frame.ip += instr->rhs._u64 - 1;
			}
		} break;

		case air::op::jump: {
			frame.ip += instr->lhs._u64 - 1;
		} break;

		// TODO(sushi) this sucks do it better later 
		case air::op::eq: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst == instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst == instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst == instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst == instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst == instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst == instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst == *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst == *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst == *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst == *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst == *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst == *(u64*)src; break;
				}
			}
		} break;

		case air::op::neq: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst != instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst != instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst != instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst != instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst != instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst != instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst != *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst != *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst != *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst != *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst != *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst != *(u64*)src; break;
				}
			}
		} break;

		case air::op::lt: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst < instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst < instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst < instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst < instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst < instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst < instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst < *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst < *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst < *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst < *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst < *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst < *(u64*)src; break;
				}
			}
		} break;

		case air::op::gt: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst > instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst > instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst > instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst > instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst > instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst > instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst > *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst > *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst > *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst > *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst > *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst > *(u64*)src; break;
				}
			}
		} break;

		case air::op::le: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst <= instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst <= instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst <= instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst <= instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst <= instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst <= instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst <= *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst <= *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst <= *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst <= *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst <= *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst <= *(u64*)src; break;
				}
			}
		} break;

		case air::op::ge: {
			u8* dst = frame.fp + instr->lhs._u64;
			if(instr->flags.right_is_const) {
				switch(instr->rhs.kind) {
					case scalar::float32: *(f32*)dst = *(f32*)dst >= instr->rhs._f32; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst >= instr->rhs._f64; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst >= instr->rhs._u8; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst >= instr->rhs._u16; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst >= instr->rhs._u32; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst >= instr->rhs._u64; break;
				}
			} else {
				u8* src = frame.fp + instr->rhs._u64;
				switch(instr->rhs.kind) { 
					case scalar::float32: *(f32*)dst = *(f32*)dst >= *(f32*)src; break;
					case scalar::float64: *(f64*)dst = *(f64*)dst >= *(f64*)src; break;
					case scalar::signed8: 
					case scalar::unsigned8: *(u8*)dst = *(u8*)dst >= *(u8*)src; break;
					case scalar::signed16: 
					case scalar::unsigned16: *(u16*)dst = *(u16*)dst >= *(u16*)src; break;
					case scalar::signed32: 
					case scalar::unsigned32: *(u32*)dst = *(u32*)dst >= *(u32*)src; break;
					case scalar::signed64: 
					case scalar::unsigned64: *(u64*)dst = *(u64*)dst >= *(u64*)src; break;
				}
			}
		} break;

		case air::op::call: {
			frames.push(frame);
			u8* next_fp = sp - instr->n_params;
			frame = instr->f->frame;
			frame.fp = next_fp;
			frame.stack_depth = 0;
			move_sp_direct(frame.fp);
			return instr; // we don't want to increment the ip
		} break;

		case air::op::ret: {
			if(!frames.count){
				finished = true;
				break;
			}
			Frame last = frame;
			move_sp_direct(frame.fp + instr->lhs._u64);
			frame = frames.pop();
		} break;

		case air::op::resz: {
			u8* dst = 0;
			if(instr->flags.left_is_ptr) {
				dst = (u8*)instr->resz.dst;
			} else if(instr->flags.deref_right) {
				dst = (u8*)*(u64*)(frame.fp + instr->resz.dst);
			} else {
				dst = frame.fp + instr->resz.dst;
			}
		
			u8* src = 0;
			if(instr->flags.right_is_ptr) {
				src = (u8*)instr->resz.src;
			} else if(instr->flags.deref_right) {
				src = (u8*)*(u64*)(frame.fp + instr->resz.src);
			} else {
				src = frame.fp + instr->resz.src;
			}
			#define inner(blah)                                        \
				switch(instr->resz.to) {                               \
					case scalar::unsigned64: *(u64*)dst = blah; break; \
					case scalar::unsigned32: *(u32*)dst = blah; break; \
					case scalar::unsigned16: *(u16*)dst = blah; break; \
					case scalar::unsigned8 : *(u8*)dst  = blah; break; \
					case scalar::signed64: *(s64*)dst = blah; break;   \
					case scalar::signed32: *(s32*)dst = blah; break;   \
					case scalar::signed16: *(s16*)dst = blah; break;   \
					case scalar::signed8 : *(s8*)dst  = blah; break;   \
					case scalar::float64: *(f64*)dst = blah; break;    \
					case scalar::float32: *(f32*)dst = blah; break;    \
				}
			if(instr->flags.left_is_const) {
				switch(instr->resz.from) {
					case scalar::unsigned64: inner((u64)instr->resz.src); break;
					case scalar::unsigned32: inner((u32)instr->resz.src); break;
					case scalar::unsigned16: inner((u16)instr->resz.src); break;
					case scalar::unsigned8: inner((u8)instr->resz.src); break;
					case scalar::signed64: inner((s64)instr->resz.src); break;
					case scalar::signed32: inner((s32)instr->resz.src); break;
					case scalar::signed16: inner((s16)instr->resz.src); break;
					case scalar::signed8: inner((s8)instr->resz.src); break;
					case scalar::float64: inner((f64)instr->resz.src); break;
					case scalar::float32: inner((f32)instr->resz.src); break;
				}
			} else {
				switch(instr->resz.from) {
					case scalar::unsigned64: inner(*(u64*)src); break;
					case scalar::unsigned32: inner(*(u32*)src); break;
					case scalar::unsigned16: inner(*(u16*)src); break;
					case scalar::unsigned8: inner(*(u8*)src); break;
					case scalar::signed64: inner(*(s64*)src); break;
					case scalar::signed32: inner(*(s32*)src); break;
					case scalar::signed16: inner(*(s16*)src); break;
					case scalar::signed8: inner(*(s8*)src); break;
					case scalar::float64: inner(*(f64*)src); break;
					case scalar::float32: inner(*(f32*)src); break;
				}
			}
		} break;

		case air::ref: {
			u8* dst = 0;
			if(instr->flags.left_is_ptr) {
				dst = (u8*)instr->lhs._u64;
			} else {
				dst = frame.fp + instr->lhs._u64;
			}
			if(instr->flags.deref_left) {
				dst = (u8*)*(u64*)dst;
			}
			u8* src = 0;
			if(instr->flags.right_is_ptr) {
				src = (u8*)instr->rhs._u64;
			} else {
				src = frame.fp + instr->rhs._u64;
			}
			if(instr->flags.deref_right) {
				src = (u8*)*(u64*)src;
			}
			*(u64*)dst = (u64)src;
			
		} break;

		case air::vm_break: {
			// util::println(ScopedDeref(DString::create("-------------------------------- sp: ", sp - frame.fp)).x);
			// util::println(instr->node->underline());
			// util::println(ScopedDeref(to_string(*instr)).x);
			// print_frame_vars();
			// DebugBreakpoint;
		} break;

		case air::intrinsic_rand_int: {
			// NOTE(sushi) since we're using C's rand, we don't actually get
			//             a u64, but since I consider this temporary idrc for now
			u64* dst = (u64*)(frame.fp + instr->lhs._u64);
			*dst = rand();
		} break;

		default: {
			TODO(DString::create("unhandled instruction: ", to_string(*instr)));
		} break;
	}

	frame.ip += 1;
	instr_count += 1;
	//print_frame_vars();

	if(sp - frame.fp > Megabytes(10)) {
		util::println("reached end of stack (10mb)");
		return 0;
	}
 
	return instr;
}


BC* VM::
run() {
    //auto start = util::stopwatch::start();

	while(!finished) {
		auto instr = step();
		if(!instr) return 0;
		if(instr->instr == air::vm_break) {
			return instr;
		}
	}

	return 0;

    // auto time = util::stopwatch::peek(start);
    // util::println(DString::create("took ", util::format_time(time/1e6), " to execute ", instr_count, " instructions, giving ", util::format_metric(f64(instr_count)/time*1e9), " IPS"));
} 

void VM::
print_stack() {
    DString* out = DString::create();

    auto my_frames = Array<Frame*>::create();
    forI(frames.count)
        my_frames.push(frames.readptr(i));
    my_frames.push(&frame);

    u32 frame_idx = 0;

    u8* p = stack;
    while(p < sp) {
        out->append("r", p-stack, " ", *p);
        if(frame_idx < my_frames.count && my_frames.read(frame_idx)->fp == p) {    
            out->append(" <-- fp of ", my_frames.read(frame_idx)->identifier);
            frame_idx++;
        }
        out->append("\n");
        p++;
    }
    util::println(out);
    out->deref();
}

void VM::
print_frame_vars() {
    DString* out = DString::create();

    forI(frame.locals.count) {
        Var* v = frame.locals.read(i);
        auto val = v->type->print_from_address(frame.fp + v->stack_offset);
        // val->indent(2);
        out->append(ScopedDeref(v->display()).x, "(", v->stack_offset, "): ", ScopedDeref(val).x, "\n");
    }

    util::println(out->fin);
    out->deref();
}

} // namespace amu
