/* amu X64 assembler backend
Notes:


Index:
@

References:
Tiny C Compiler by Fabrice Bellard (https://bellard.org/tcc)
*/

#include "backends/X64.h"
#include "processors/Assembler.h"
#include "processors/GenAIR.h"
#include "representations/AIR.h"
#include "representations/Code.h"
#include "storage/String.h"

namespace amu {

#define ARRAY_COUNT(a)

// TODO: convert these to runtime settings?
#define X64_WRITE_COMMENTS true
#define X64_PADDING_WIDTH 2
#define X64_INSTRUCTION_WIDTH 8
#define X64_ARGS_WIDTH 16

// heuristically chosen initial binary size roughly based on average of functions in this codebase
// TODO: make this configurable? make this calculated? median vs average?
#define X64_INITIAL_BINARY_SIZE 1024

Assembler* X64::
create(Code* code){
	X64* assembler = (X64*)code->allocator.allocate(sizeof(X64));
	assembler->code = code;
	assembler->backend = Assembler::Backend::X64;
	assembler->binary = Array<u8>::create(X64_INITIAL_BINARY_SIZE);
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
	
}

} // namespace amu