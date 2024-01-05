/* amu X64 assembler backend
Notes:


Index:
@registers
@binary
@interface

References:
Tiny C Compiler by Fabrice Bellard (https://bellard.org/tcc)
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
// @registers


// NOTE: sorted from more generic to more specific
// NOTE: rbx, rbp, and r12-r15 are callee-saved registers (saved across function calls)
enum{
	X64_RC_NONE  = 0,
	X64_RC_INT   = 0x00001, // generic integer
	X64_RC_FLOAT = 0x00002, // generic float
	X64_RC_RAX   = 0x00004,
	X64_RC_RDX   = 0x00008,
	X64_RC_RCX   = 0x00010,
	X64_RC_RSI   = 0x00020,
	X64_RC_RDI   = 0x00040,
	X64_RC_ST0   = 0x00080, // only used for f128
	X64_RC_R8    = 0x00100,
	X64_RC_R9    = 0x00200,
	X64_RC_R10   = 0x00400,
	X64_RC_R11   = 0x00800,
	X64_RC_XMM0  = 0x01000,
	X64_RC_XMM1  = 0x02000,
	X64_RC_XMM2  = 0x04000,
	X64_RC_XMM3  = 0x08000,
	X64_RC_XMM4  = 0x10000,
	X64_RC_XMM5  = 0x20000,
	X64_RC_XMM6  = 0x40000,
	X64_RC_XMM7  = 0x80000,
	X64_RC_IRET  = X64_RC_RAX,  // function return: integer register
	X64_RC_IRE2  = X64_RC_RDX,  // function return: second integer register
	X64_RC_FRET  = X64_RC_XMM0, // function return: float register
	X64_RC_FRE2  = X64_RC_XMM1, // function return: second float register
};

// NOTE: xmm6 and xmm7 are not tagged with RC_FLOAT since they are callee-saved on Windows
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
	X64_REG_RSP = 4,
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
	
	X64_REG_FLAG_MASK = 0x3f, // NOTE: all bits needed to construct X64_REG_MEM
	X64_REG_FLAG_CONSTANT = 0x30,
};

// Save `r` to the memory stack and mark it as being free
static void
X64_save_register(X64* assembler, u32 r){
	
}

// Find a free register matching the `register_class`. If none, save a matching register to the stack to free it up.
static u32
X64_get_register(X64* assembler, u32 register_class){
	int r;
	X64::StackValue* sv;
	
	// find a free register that matches
	for(r = 0; r < X64_register_classes_count; r += 1){
		if(X64_register_classes[r] & register_class){
			for(sv = assembler->stack; sv < assembler->stack + assembler->stack_count; sv++){
				if(r == (sv->reg & X64_REG_FLAG_MASK)){
					goto reg_already_in_use;
				}
			}
			return r;
		}
		reg_already_in_use: ;
	}
	
	// no register left; free the first one on the stack that matches
	for(sv = assembler->stack; sv < assembler->stack + assembler->stack_count; sv++){
		r = (sv->reg & X64_REG_FLAG_MASK);
		if(X64_register_classes[r] & register_class){
			X64_save_register(assembler, r);
			return r;
		}
	}
	
	Assert(!"Failed to get a valid X64 register; this should never happen.");
	return -1;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @binary


// heuristically chosen initial binary size roughly based on average of functions in the amu codebase
// TODO: make this configurable? make this calculated? median vs average?
#define X64_INITIAL_BINARY_SIZE 1024

// Reserve `count` bytes from the binary of `assembler`
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

// Write the single byte of `value` to the binary of `assembler`
static void
X64_write(X64* assembler, u8 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 1);
		cursor[0] = value;
	}
}
template<class T> void X64_write(X64* assembler, T value) = delete; // NOTE: disable implicit conversion

// Write the two bytes of `value` in little-endian to the binary of `assembler`
static void
X64_write_le16(X64* assembler, u16 value){
	if(value){
		u8* cursor = X64_push_binary(assembler, 2);
		cursor[0] = (u8)(value >> 0);
		cursor[1] = (u8)(value >> 8);
	}
}
template<class T> void X64_write_le16(X64* assembler, T value) = delete; // NOTE: disable implicit conversion

// Write the four bytes of `value` in little-endian to the binary of `assembler`
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
template<class T> void X64_write_le32(X64* assembler, T value) = delete; // NOTE: disable implicit conversion

// Write the eight bytes of `value` in little-endian to the binary of `assembler`
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
template<class T> void X64_write_le64(X64* assembler, T value) = delete; // NOTE: disable implicit conversion

// Write the instructions necessary to store the register `r` in the value `sv`
static void
X64_write_store(X64* assembler, u32 r, X64::StackValue* sv){
	
}

// Write the instructions necessary to load the register `r` from the value `sv`
static void
X64_write_load(X64* assembler, u32 r, X64::StackValue* sv){
	
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @interface


Assembler* X64::
create(Code* code){
	X64* assembler = (X64*)code->allocator.allocate(sizeof(X64));
	assembler->code = code;
	assembler->backend = Assembler::Backend::X64;
	assembler->binary = (u8*)code->allocator.allocate(X64_INITIAL_BINARY_SIZE);
	assembler->binary_space = X64_INITIAL_BINARY_SIZE;
	return assembler;
}

void X64::
run(){
	if(!this->code->air_gen){
		Assert(!"X64 requires that AIR bytecode must have been generated before beginning assembly.");
		return;
	}
	
	for(BC* bc = this->code->air_gen->seq.data; bc < this->code->air_gen->seq.data + this->code->air_gen->seq.count; bc++)
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
output(String path){
	// TODO: make these configurable
	b32 write_comments = true;
	u32 padding_width = 2;
	u32 instruction_width = 8;
	u32 args_width;
	
	
}

} // namespace amu