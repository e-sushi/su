/* amu X64 assembler backend
Notes:
- The stack values array (X64::stack) is a list of all values we need to keep around for future
	instructions. Each stack value may be currently bound to a register or may have been pushed
	onto the actual stack to store a register's value for future use.
- Binary operator instructions (add, sub, ...) always operator on the two top stack values, with
	the destination argument being stack_top[-1] and the source argument being stack_top[0].


Index:
- @registers/stack
- @binary
- @interface

References:
- Tiny C Compiler by Fabrice Bellard (https://bellard.org/tcc)
- Intel64 and IA-32 Architectures Reference Manual by Intel (https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- Intel 8086 Family User's Manual (no official link)

TODO:
- X64_64bit
- X64_float
- X64_human_readable
*/

#include "backends/X64.h"
#include "processors/Assembler.h"
#include "processors/GenAIR.h"
#include "representations/AIR.h"
#include "representations/Code.h"
#include "storage/String.h"
#include <cstring> // memcpy

namespace amu{


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @registers/stack


// NOTE: sorted from more generic to more specific
// NOTE: rbx, rbp, and r12-r15 are callee-saved registers (saved across function calls)
enum{
	X64_RC_NONE  = 0,
	X64_RC_INT   = (1 << 0), // generic integer
	X64_RC_FLOAT = (1 << 1), // generic float
	X64_RC_RAX   = (1 << 2),
	X64_RC_RDX   = (1 << 3),
	X64_RC_RCX   = (1 << 4),
	X64_RC_RSI   = (1 << 5),
	X64_RC_RDI   = (1 << 6),
	X64_RC_ST0   = (1 << 7), // only used for f128
	X64_RC_R8    = (1 << 8),
	X64_RC_R9    = (1 << 9),
	X64_RC_R10   = (1 << 10),
	X64_RC_R11   = (1 << 11),
	X64_RC_XMM0  = (1 << 12),
	X64_RC_XMM1  = (1 << 13),
	X64_RC_XMM2  = (1 << 14),
	X64_RC_XMM3  = (1 << 15),
	X64_RC_XMM4  = (1 << 16),
	X64_RC_XMM5  = (1 << 17),
	X64_RC_XMM6  = (1 << 18),
	X64_RC_XMM7  = (1 << 19),
	X64_RC_IRET  = X64_RC_RAX,  // function return: integer register
	X64_RC_IRE2  = X64_RC_RDX,  // function return: second integer register
	X64_RC_FRET  = X64_RC_XMM0, // function return: float register
	X64_RC_FRE2  = X64_RC_XMM1, // function return: second float register
};

// NOTE: xmm6 and xmm7 are not tagged with X64_RC_FLOAT since they are callee-saved on Windows
static const u32 X64_register_classes[] = {
	X64_RC_RAX | X64_RC_INT,
	X64_RC_RCX | X64_RC_INT,
	X64_RC_RDX | X64_RC_INT,
	0, // rbx
	0, // rsp
	0, // rbp
	0, // rsi
	0, // rdi
	X64_RC_R8,
	X64_RC_R9,
	X64_RC_R10,
	X64_RC_R11,
	0, // r12
	0, // r13
	0, // r14
	0, // r15
	X64_RC_XMM0 | X64_RC_FLOAT,
	X64_RC_XMM1 | X64_RC_FLOAT,
	X64_RC_XMM2 | X64_RC_FLOAT,
	X64_RC_XMM3 | X64_RC_FLOAT,
	X64_RC_XMM4 | X64_RC_FLOAT,
	X64_RC_XMM5 | X64_RC_FLOAT,
	X64_RC_XMM6,
	X64_RC_XMM7,
	X64_RC_ST0
};
const u32 X64_register_classes_count = sizeof(X64_register_classes) / sizeof(u32);

enum{
	X64_REG_RAX = 0,
	X64_REG_RCX = 1,
	X64_REG_RDX = 2,
	//X64_REG_RBX = 3,
	X64_REG_RSP = 4,
	//X64_REG_RBP = 5,
	X64_REG_RSI = 6,
	X64_REG_RDI = 7,
	
	X64_REG_R8 = 8,
	X64_REG_R9 = 9,
	X64_REG_R10 = 10,
	X64_REG_R11 = 11,
	
	X64_REG_XMM0 = 16,
	X64_REG_XMM1 = 17,
	X64_REG_XMM2 = 18,
	X64_REG_XMM3 = 19,
	X64_REG_XMM4 = 20,
	X64_REG_XMM5 = 21,
	X64_REG_XMM6 = 22,
	X64_REG_XMM7 = 23,
	
	X64_REG_ST0 = 24,
	X64_REG_MEM = 32,
	
	X64_REG_FLAG_MASK      = 0x3f, // NOTE: all bits needed to construct X64_REG_MEM
	X64_REG_FLAG_CONSTANT  = 0x30,
	X64_REG_FLAG_LLOCAL    = 0x31,
	X64_REG_FLAG_LOCAL     = 0x32,
	X64_REG_FLAG_CMP       = 0x33,
	X64_REG_FLAG_JMP_TRUE  = 0x34,
	X64_REG_FLAG_JMP_FALSE = 0x35,
};

// Extract the 3 register bits from the register variable
// !ref: Intel 8086 Family User's Manual (Table 4-9. REG (Register Field Encoding))
#define X64_REG_VALUE(reg) ((reg) & 7)

// Returns true if the stack value is a 64bit type
static b32
X64_stack_value_is_64bit(X64::StackValue* sv){
	// TODO: X64_64bit
	return false;
}

// 
static void
X64_save_top_comparison_flags(X64* assembler){
	if(assembler->stack_top->reg == X64_REG_FLAG_CMP){
		
	}
}

// Swap the two top stack values
static void
X64_swap_top(X64* assembler){
	X64_save_top_comparison_flags(assembler);
	X64::StackValue temp = assembler->stack_top[0];
	assembler->stack_top[0] = assembler->stack_top[-1];
	assembler->stack_top[-1] = temp;
}

// Save `r` to the values stack and mark it as being free
static void
X64_save_register(X64* assembler, u32 r){
	
}

// Find a free register matching the `register_class`; if none, save a matching register to the stack to free it up
static u32
X64_get_register(X64* assembler, u32 register_class){
	int reg;
	X64::StackValue* sv;
	
	// find a free register that matches
	for(reg = 0; reg < X64_register_classes_count; reg += 1){
		if(X64_register_classes[reg] & register_class){
			for(sv = assembler->stack; sv < assembler->stack_top; sv++){
				if(reg == (sv->reg & X64_REG_FLAG_MASK)){
					goto reg_already_in_use;
				}
			}
			return reg;
		}
		reg_already_in_use: ;
	}
	
	// no register left; free the first one on the stack that matches
	// NOTE: we search from the bottom of the values to avoid using registers
	for(sv = assembler->stack; sv < assembler->stack_top; sv++){
		reg = (sv->reg & X64_REG_FLAG_MASK);
		if(X64_register_classes[reg] & register_class){
			X64_save_register(assembler, reg);
			return reg;
		}
	}
	
	Assert(!"Failed to get a valid X64 register; this should never happen.");
	return -1;
}

// Load the top stack value into a register matching `register_class` if it is not already, and return the register
static u32
X64_load_top_to_register(X64* assembler, u32 register_class){
	
	return 0;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @binary


// heuristically chosen initial binary size roughly based on average of functions in the amu codebase
// TODO: make this configurable? make this calculated? median vs average?
#define X64_INITIAL_BINARY_SIZE 1024

// Reserve `count` bytes from the binary
// NOTE: will grow the binary space of `assembler` by X64_INITIAL_BINARY_SIZE if necessary
static u8*
X64_push_binary(X64* assembler, u32 count){
	u32 new_count = assembler->binary_count + count;
	if(new_count > assembler->binary_space){
		u32 new_binary_space = assembler->binary_space + X64_INITIAL_BINARY_SIZE;
		if(    (assembler->binary == assembler->code->allocator.cursor - assembler->binary_space)
			&& (assembler->binary + new_binary_space <= assembler->code->allocator.start + Bump::slab_size))
		{
			assembler->code->allocator.allocate(X64_INITIAL_BINARY_SIZE);
		}else{
			u8* new_binary = (u8*)assembler->code->allocator.allocate(new_binary_space);
			memcpy(new_binary, assembler->binary, assembler->binary_space);
			assembler->binary = new_binary;
		}
		assembler->binary_space = new_binary_space;
	}
	
	u8* cursor = assembler->binary + assembler->binary_count;
	assembler->binary_count = new_count;
	return cursor;
}

// Write the single byte of `value` to the binary
static void
X64_write(X64* assembler, u8 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 1);
		cursor[0] = value;
	}
}

// Write the two bytes of `value` in little-endian to the binary
static void
X64_write_le16(X64* assembler, u16 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 2);
		cursor[0] = (u8)(value >> 0);
		cursor[1] = (u8)(value >> 8);
	}
}

// Write the four bytes of `value` in little-endian to the binary
static void
X64_write_le32(X64* assembler, u32 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 4);
		cursor[0] = (u8)(value >> 0);
		cursor[1] = (u8)(value >> 8);
		cursor[2] = (u8)(value >> 16);
		cursor[3] = (u8)(value >> 24);
	}
}

// Write the eight bytes of `value` in little-endian to the binary
static void
X64_write_le64(X64* assembler, u64 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 8);
		cursor[0] = (u8)(value >> 0);
		cursor[1] = (u8)(value >> 8);
		cursor[2] = (u8)(value >> 16);
		cursor[3] = (u8)(value >> 24);
		cursor[4] = (u8)(value >> 32);
		cursor[5] = (u8)(value >> 40);
		cursor[6] = (u8)(value >> 48);
		cursor[7] = (u8)(value >> 56);
	}
}

// Write the instructions necessary to store the register `reg` in the value `sv`
static void
X64_write_store(X64* assembler, u32 reg, X64::StackValue* sv){
	
}

// Write the instructions necessary to load the register `reg` from the value `sv`
static void
X64_write_load(X64* assembler, u32 reg, X64::StackValue* sv){
	
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @interface


Assembler* X64::
create(Code* code){
	X64* assembler = (X64*)code->allocator.allocate(sizeof(X64));
	assembler->code = code;
	assembler->backend = Assembler::Backend::X64;
	assembler->stack_top = assembler->stack;
	assembler->binary = (u8*)code->allocator.allocate(X64_INITIAL_BINARY_SIZE);
	assembler->binary_space = X64_INITIAL_BINARY_SIZE;
	return assembler;
}

void X64::
run(){
	X64* assembler = this;
	if(!assembler->code->air_gen){
		Assert(!"X64 requires that AIR bytecode must have been generated before beginning assembly.");
		return;
	}
	
	for(BC* bc = assembler->code->air_gen->seq.data; bc < assembler->code->air_gen->seq.data + assembler->code->air_gen->seq.count; bc++)
	{
		switch(bc->instr){
			case air::op::push:{
				
			}break;
			
			case air::op::pushn:{
				
			}break;
			
			case air::op::popn:{
				
			}break;
			
			case air::op::copy:{
				
			}break;
			
			case air::op::add:{
				StackValue* dst = &assembler->stack_top[-1];
				StackValue* src = &assembler->stack_top[ 0];
				if(bc->flags.right_is_const){
					switch(bc->rhs.kind){
						case ScalarValue::Kind::Signed8:
						{
							// Load the destination's stack value to an integer register
							X64_swap_top(assembler);
							u32 r = X64_load_top_to_register(assembler, X64_RC_INT);
							X64_swap_top(assembler);
							
							if(X64_stack_value_is_64bit(dst)){
								// TODO: X64_64bit
							}else{
								// Add sign-extended imm8 to r/m
								X64_write(assembler, 0x83); // ADD b
								X64_write(assembler, 0xc0 | X64_REG_VALUE(r));
								X64_write(assembler, src->_u8);
							}
						}break;
						case ScalarValue::Kind::Unsigned8:
						{
							
						}break;
						case ScalarValue::Kind::Signed16:
						case ScalarValue::Kind::Unsigned16:
						{
							
						}break;
						case ScalarValue::Kind::Signed32:
						case ScalarValue::Kind::Unsigned32:
						{
							
						}break;
						case ScalarValue::Kind::Signed64:
						case ScalarValue::Kind::Unsigned64:
						{
							// TODO: X64_64bit
						}break;
						case ScalarValue::Kind::Float32:
						{
							// TODO: X64_float
						}break;
						case ScalarValue::Kind::Float64:
						{
							// TODO: X64_float
						}break;
					}
				}else{
					
				}
			}break;
			
			case air::op::sub:{
				
			}break;
			
			case air::op::mul:{
				
			}break;
			
			case air::op::div:{
				
			}break;
			
			case air::op::mod:{
				
			}break;
			
			case air::op::eq:{
				
			}break;
			
			case air::op::neq:{
				
			}break;
			
			case air::op::lt:{
				
			}break;
			
			case air::op::gt:{
				
			}break;
			
			case air::op::le:{
				
			}break;
			
			case air::op::ge:{
				
			}break;
			
			case air::op::call:{
				
			}break;
			
			case air::op::ret:{
				
			}break;
			
			case air::op::jump:{
				
			}break;
			
			case air::op::jump_zero:{
				
			}break;
			
			case air::op::jump_not_zero:{
				
			}break;
			
			case air::op::ftou:{
				
			}break;
			
			case air::op::ftos:{
				
			}break;
			
			case air::op::itof:{
				
			}break;
			
			case air::op::resz:{
				
			}break;
			
			case air::op::ref:{
				
			}break;
			
			case air::op::deref:{
				
			}break;
			
			default:{
				//do nothing
			}break;
		}
	}
}

void X64::
output(String path, b32 human_readable){
	// TODO: make these configurable
	u32 padding_width = 2;
	u32 instruction_width = 8;
	u32 args_width = 16;
	
	X64* assembler = this;
	if(!assembler->code->air_gen){
		Assert(!"X64 requires that AIR bytecode must have been generated before beginning assembly.");
		return;
	}
	
	if(!assembler->binary_count){
		assembler->run();
	}
	
	FILE* file = fopen((char*)path.str, "wb");
	if(!file){
		Assert(!"failed to open file");
		return;
	}
	defer{ fclose(file); };
	
	if(!human_readable){
		fwrite(assembler->binary, 1, assembler->binary_count, file);
		return;
	}
	
	// TODO: X64_human_readable
}

} // namespace amu