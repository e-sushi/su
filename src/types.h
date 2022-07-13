#pragma once
#ifndef SU_TYPES_H
#define SU_TYPES_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/string.h"
#include "core/threading.h"
#include "ctype.h"

//attempt at making str8 building thread safe
#define suStr8(...) to_str8_su(__VA_ARGS__)
template<class...T>
str8 to_str8_su(T... args){
	persist DPTracyLockable(mutex, tostr8_lock);
	tostr8_lock.lock();
	str8 ret = toStr8(args...);
	tostr8_lock.unlock();
	return ret;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Compile Options
enum ReturnCode {
	ReturnCode_Success                = 0,
	ReturnCode_No_File_Passed         = 1,
	ReturnCode_File_Not_Found         = 2,
	ReturnCode_File_Locked            = 3,
	ReturnCode_File_Invalid_Extension = 4,
	ReturnCode_Invalid_Argument       = 5,
	ReturnCode_Lexer_Failed           = 6,
	ReturnCode_Preprocessor_Failed    = 7,
	ReturnCode_Parser_Failed          = 8,
	ReturnCode_Assembler_Failed       = 9,
};

enum{
	W_NULL = 0,
	//// @level1 //// (warnings that are probably programmer errors, but are valid in rare cases)
	W_Level1_Start = W_NULL,

	WC_Overflow,
	//WC_Implicit_Narrowing_Conversion,

	W_Level1_End,
	//// @level2 //// (all the other warnings)
	W_Level2_Start = W_Level1_End,



	W_Level2_End,
	//// @level3 //// (warnings you might want to be aware of, but are valid in most cases)
	W_Level3_Start = W_Level2_End,

	WC_Unreachable_Code_After_Return,
	WC_Unreachable_Code_After_Break,
	WC_Unreachable_Code_After_Continue,
	//WC_Negative_Constant_Assigned_To_Unsigned_Variable,

	W_Level3_End,
	W_COUNT = W_Level3_End
};

enum OSOut {
	OSOut_Windows,
	OSOut_Linux,
	OSOut_OSX,
};

struct {
	u32 warning_level = 1;
	u32 verbosity = 0;
	u32 indent = 0;
	b32 supress_warnings   = false;
	b32 supress_messages   = false;
	b32 warnings_as_errors = false;
	b32 show_code = true;
	b32 log_immediatly           = true;
	b32 assert_compiler_on_error = true;
	OSOut osout = OSOut_Windows;
} globals;

/*	Verbosity
	0: Shows the names of files as they are processed 
	1: Shows the stages as they happen and the time it took for them to complete
	2: Shows parts of the stages as they happen
	3: Shows even more detail about whats happening in each stage
	4: Shows compiler debug information from each stage
*/

enum{
	Message_Log,
	Message_Error,
	Message_Warn,
};

mutex global_mem_lock;

void* su_memalloc(upt size){
	global_mem_lock.lock();
	void* ret = memalloc(size);
	global_mem_lock.unlock();
	return ret;
}

void su_memzfree(void* ptr){
	global_mem_lock.lock();
	memzfree(ptr);
	global_mem_lock.unlock();
}

//attempt at implementing a thread safe memory region 
//this is chunked, so memory never moves unless you use something like remove
//TODO(sushi) non-chunked variant
template<typename T>
struct suChunkedArena{
	Arena** arenas;
	u64 arena_count = 0;
	u64 arena_space = 16;
	u64 count = 0;
	mutex write_lock;
	mutex read_lock;

	void init(upt n_obj_per_chunk){
		arena_count = 1;
		arenas = (Arena**)su_memalloc(sizeof(Arena*)*arena_space);
		global_mem_lock.lock();
		arenas[arena_count-1] = memory_create_arena(sizeof(T)*n_obj_per_chunk); 
		global_mem_lock.unlock();
	}

	void deinit(){
		global_mem_lock.lock();
		forI(arena_count){
			memory_delete_arena(arenas[i]);
		}
		global_mem_lock.unlock();
		su_memzfree(arenas);
	}

	void add(const T& in){
		write_lock.lock();

		//make a new arena if needed
		if(arenas[arena_count-1]->used + sizeof(T) > arenas[arena_count-1]->size){
			global_mem_lock.lock();
			if(arena_space == arena_count){
				arena_space += 16;
				arenas = (Arena**)memrealloc(arenas, arena_space*sizeof(Arena*));
			}
			arenas[arena_count] = memory_create_arena(arenas[arena_count-1]->size);
			arena_count++;
			global_mem_lock.unlock();
		}
		count++;
		memcpy(arenas[arena_count-1]->cursor, &in, sizeof(T));
		arenas[arena_count-1]->used += sizeof(T);
		arenas[arena_count-1]->cursor += sizeof(T);
		write_lock.unlock();
	}

	T read(upt idx){
		persist u64 read_count = 0;
		read_lock.lock();
		read_count++;
		if(read_count==1){
			write_lock.lock();
		}
		read_lock.unlock();

		Assert(idx < count);

		u64 chunk_size = arenas[0]->size;
		u64 offset = idx * sizeof(T);
		u64 arenaidx = offset / chunk_size;

		T ret = *(T*)(arenas[arenaidx]->start + (offset - arenaidx * chunk_size));

		read_lock.lock();
		read_count--;
		if(!read_count){
			write_lock.unlock();
		}
		read_lock.unlock();

		return ret; 
	}

	void remove(upt idx){
		write_lock.lock();
		Assert(idx < count);

		u64 chunk_size = arenas[0]->size;
		u64 offset = idx * sizeof(T);
		u64 arenaidx = offset / chunk_size;
		T* removee = (T*)(arenas[arenaidx]->start + (offset-arenaidx*chunk_size));

		memmove(removee, removee+1, chunk_size - (offset - arenaidx * chunk_size));

		forI(arena_count - arenaidx){
			u64 idx = arenaidx + i;
			if(idx+1 < arena_count){
				//there is another arena ahead of this one, so we replace this one's last value with its first
				//and then move it back one
				memcpy(arenas[idx]->start + arenas[idx]->size-sizeof(T), arenas[idx+1]->start, sizeof(T));
				memmove(arenas[idx+1]->start, arenas[idx+1]->start+sizeof(T), arenas[idx+1]->used-sizeof(T));
				arenas[idx+1]->used -= sizeof(T);
			}else{
				//this is the last arena so just move its data back
				memmove(arenas[idx]->start, arenas[idx]->start+sizeof(T), arenas[idx]->used-sizeof(T));
				arenas[idx]->used -= sizeof(T);
				arenas[idx]->cursor -= sizeof(T);
			}
		}

		write_lock.unlock();
	}

};

template<typename T>
struct suArena{
	Arena* arena;
	mutex write_lock;
	mutex read_lock;


	void init(upt initial_size = 16){
		global_mem_lock.lock();
		arena = memory_create_arena(sizeof(T)*initial_size); 
		global_mem_lock.unlock();
	}

	void deinit(){
		global_mem_lock.lock();
		memory_delete_arena(arena);
		global_mem_lock.unlock();
	}

	void add(const T& in){
		write_lock.lock();
		if(arena->used + sizeof(T) > arena->size){
			global_mem_lock.lock();
			memory_grow_arena(arena, sizeof(T)*16);
			global_mem_lock.unlock();
		}
		memcpy(arena->cursor, &in, sizeof(T));
		arena->used += sizeof(T);
		arena->cursor += sizeof(T);
		write_lock.unlock();
	}

	T read(upt idx){
		persist u64 read_count = 0;
		read_lock.lock();
		read_count++;
		if(read_count==1){
			write_lock.lock();
		}
		read_lock.unlock();

		Assert(idx < arena->used / sizeof(T));

		T ret = *((T*)arena->start + idx);

		read_lock.lock();
		read_count--;
		if(!read_count){
			write_lock.unlock();
		}
		read_lock.unlock();

		return ret; 
	}

	void remove(upt idx){
		write_lock.lock();
		Assert(idx < arena->used / sizeof(T));

		memmove((T*)arena->start + idx, (T*)arena->start + idx + 1, arena->used - sizeof(T)*idx);
		arena->used -= sizeof(T);
		arena->cursor -= sizeof(T);

		write_lock.unlock();
	}

};




//sorted binary searched map for matching identifiers with
//their declarations. this map supports storing keys who have collided
//by storing them as neighbors and storing the unhashed key with the value
//this was delle's idea
//TODO(sushi) decide if we should store pair<str8,Decl*> in data or just use the str8 `identifier`
//            that is on Declaration already, this way we just pass a Declaration* and no str8
struct Declaration;
struct declmap{
	array<u64> hashes; 
	array<pair<str8, Declaration*>> data;

	declmap(){DPZoneScoped;
		hashes = array<u64>(deshi_allocator);
		data = array<pair<str8,Declaration*>>(deshi_allocator);
	}

	u32 find_key(u64 key){DPZoneScoped;
		spt index = -1;
		spt middle = -1;
		if(hashes.count){
			spt left = 0;
			spt right = hashes.count-1;
			while(left <= right){
				middle = left+((right-left)/2);
				if(hashes[middle] == key){
					index = middle;
					break;
				}
				if(hashes[middle] < key){
					left = middle+1;
					middle = left+((right-left)/2);
				}else{
					right = middle-1;
				}
			}
		}
		return index;
	}

	u32 find_key(str8 key){DPZoneScoped;
		return find_key(str8_hash64(key));
	}

	//note there is no overload for giving the key as a u64 because this struct requires
	//storing the original key value in the data.
	u32 add(str8 key, Declaration* val){DPZoneScoped;
		persist mutex add_lock;
		u64 key_hash = str8_hash64(key);
		spt index = -1;
		spt middle = 0;
		if(hashes.count){
			spt left = 0;
			spt right = hashes.count-1;
			while(left <= right){
				middle = left+((right-left)/2);
				if(hashes[middle] == key_hash){
					index = middle;
					break;
				}
				if(hashes[middle] < key_hash){
					left = middle+1;
					middle = left+((right-left)/2);
				}else{
					right = middle-1;
				}
			}
		}
		//if the index was found AND this is not a collision, we can just return 
		//but if the second check fails then this is a collision and we must still insert it as a neighbor 
		if(index != -1 && str8_equal_lazy(key, data[index].first)){
			add_lock.unlock();
			return index;
		}else{
			hashes.insert(key_hash, middle);
			data.insert({key, val}, middle);
			return middle;
		}
	}

	b32 has(str8 key){DPZoneScoped;

	}

	b32 has_collided(u64 key){DPZoneScoped;
		u32 index = find_key(key);
		if(index != -1 && 
		   index > 0            && hashes[index-1] != hashes[index] &&
	       index < hashes.count && hashes[index+1] != hashes[index]){
			return true;
		}
		return false;
	}

	//find the first instance of a key in the hashes array to simplify looking for a match
	//when things have collided. I'm not sure this is the best way to go about this.
	//this expects the duplicated key to have already been found 
	u32 find_key_first_entry(u64 idx){DPZoneScoped;
		while(idx!=0 && hashes[idx-1] == hashes[idx]) idx--;
		return idx;
	}

	//tests if the hash array at the found index has neighbors that are the same
	b32 has_collided(str8 key){DPZoneScoped;
		return has_collided(str8_hash64(key));
	}

	Declaration* at(str8 key){DPZoneScoped;
		u32 index = find_key(key);
		if(index != -1){
			if(hashes.count == 1){
				return data[0].second;
			}else if(index && index < hashes.count-1 && hashes[index-1] != hashes[index] && hashes[index+1] != hashes[index]){
				return data[index].second;
			}else if (!index && hashes.count > 1 && hashes[1] != hashes[0]){
				return data[index].second;
			}else if(index == hashes.count-1 && hashes[index-1] != hashes[index]){
				return data[index].second;
			}else{
				Log("declmap", "A collision happened in a declmap, which is NOT tested, so you should test this if it appears");
				TestMe;
				//a collision happened so we must find the right neighbor
				u32 idx = index;
				//iterate backwards as far as we can
				while(idx!=0 && hashes[idx-1] == hashes[idx]){
					idx--;
					if(str8_equal_lazy(data[idx].first, key)){
						return data[idx].second;
					}
				}
				idx = index;
				//iterate forwards as far as we can
				while(idx!=hashes.count-1 && hashes[idx+1] == hashes[idx]){
					idx++;
					if(str8_equal_lazy(data[idx].first, key)){
						return data[idx].second;
					}
				}
				return 0;
			}
		}
		if(index != -1){
			return data[index].second;
		}else{
			return 0;
		}
		return 0;
	}
};



//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Nodes
enum NodeType : u32 {
	NodeType_Program,
	NodeType_Structure,
	NodeType_Function,
	NodeType_Variable,
	NodeType_Scope,
	NodeType_Statement,
	NodeType_Expression,
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Registers
enum Registers{
	Register_NULL,
	
	Register0_64,  Register0_32,  Register0_16,  Register0_8,
	Register1_64,  Register1_32,  Register1_16,  Register1_8,
	Register2_64,  Register2_32,  Register2_16,  Register2_8,
	Register3_64,  Register3_32,  Register3_16,  Register3_8,
	Register4_64,  Register4_32,  Register4_16,  Register4_8,
	Register5_64,  Register5_32,  Register5_16,  Register5_8,
	Register6_64,  Register6_32,  Register6_16,  Register6_8,
	Register7_64,  Register7_32,  Register7_16,  Register7_8,
	Register8_64,  Register8_32,  Register8_16,  Register8_8,
	Register9_64,  Register9_32,  Register9_16,  Register9_8,
	Register10_64, Register10_32, Register10_16, Register10_8,
	Register11_64, Register11_32, Register11_16, Register11_8,
	Register12_64, Register12_32, Register12_16, Register12_8,
	Register13_64, Register13_32, Register13_16, Register13_8,
	Register14_64, Register14_32, Register14_16, Register14_8,
	Register15_64, Register15_32, Register15_16, Register15_8,
	
	//x64 names
	Register_RAX = Register0_64,  Register_EAX  = Register0_32,  Register_AX   = Register0_16,  Register_AL   = Register0_8,
	Register_RDX = Register1_64,  Register_EDX  = Register1_32,  Register_DX   = Register1_16,  Register_DL   = Register1_8,
	Register_RCX = Register2_64,  Register_ECX  = Register2_32,  Register_CX   = Register2_16,  Register_CL   = Register2_8,
	Register_RBX = Register3_64,  Register_EBX  = Register3_32,  Register_BX   = Register3_16,  Register_BL   = Register3_8,
	Register_RSI = Register4_64,  Register_ESI  = Register4_32,  Register_SI   = Register4_16,  Register_SIL  = Register4_8,
	Register_RDI = Register5_64,  Register_EDI  = Register5_32,  Register_DI   = Register5_16,  Register_DIL  = Register5_8,
	Register_RSP = Register6_64,  Register_ESP  = Register6_32,  Register_SP   = Register6_16,  Register_SPL  = Register6_8,
	Register_RBP = Register7_64,  Register_EBP  = Register7_32,  Register_BP   = Register7_16,  Register_BPL  = Register7_8,
	Register_R8  = Register8_64,  Register_R8D  = Register8_32,  Register_R8W  = Register8_16,  Register_R8B  = Register8_8,
	Register_R9  = Register9_64,  Register_R9D  = Register9_32,  Register_R9W  = Register9_16,  Register_R9B  = Register9_8,
	Register_R10 = Register10_64, Register_R10D = Register10_32, Register_R10W = Register10_16, Register_R10B = Register10_8,
	Register_R11 = Register11_64, Register_R11D = Register11_32, Register_R11W = Register11_16, Register_R11B = Register11_8,
	Register_R12 = Register12_64, Register_R12D = Register12_32, Register_R12W = Register12_16, Register_R12B = Register12_8,
	Register_R13 = Register13_64, Register_R13D = Register13_32, Register_R13W = Register13_16, Register_R13B = Register13_8,
	Register_R14 = Register14_64, Register_R14D = Register14_32, Register_R14W = Register14_16, Register_R14B = Register14_8,
	Register_R15 = Register15_64, Register_R15D = Register15_32, Register_R15W = Register15_16, Register_R15B = Register15_8,
	
	//usage
	Register_FunctionReturn     = Register_RAX,
	Register_BasePointer        = Register_RBP,
	Register_StackPointer       = Register_RSP,
	Register_FunctionParameter0 = Register_RDI,
	Register_FunctionParameter1 = Register_RSI,
	Register_FunctionParameter2 = Register_RDX,
	Register_FunctionParameter3 = Register_RCX,
	Register_FunctionParameter4 = Register_R8,
	Register_FunctionParameter5 = Register_R9,
};

global const char* registers_x64[] = {
	"%null",
	"%rax", "%eax",  "%ax",   "%al",
	"%rdx", "%edx",  "%dx",   "%dl",
	"%rcx", "%ecx",  "%cx",   "%cl",
	"%rbx", "%ebx",  "%bx",   "%bl",
	"%rsi", "%esi",  "%si",   "%sil",
	"%rdi", "%edi",  "%di",   "%dil",
	"%rsp", "%esp",  "%sp",   "%spl",
	"%rbp", "%ebp",  "%bp",   "%bpl",
	"%r8",  "%r8d",  "%r8w",  "%r8b",
	"%r9",  "%r9d",  "%r9w",  "%r9b",
	"%r10", "%r10d", "%r10w", "%r10b",
	"%r11", "%r11d", "%r11w", "%r11b",
	"%r12", "%r12d", "%r12w", "%r12b",
	"%r13", "%r13d", "%r13w", "%r13b",
	"%r14", "%r14d", "%r14w", "%r14b",
	"%r15", "%r15d", "%r15w", "%r15b",
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Builtin Types
typedef u32 DataType; enum { 
	DataType_NotTyped,
	DataType_Void,       // void
	DataType_Implicit,   // implicitly typed
	DataType_Signed8,    // s8
	DataType_Signed16,   // s16
	DataType_Signed32,   // s32 
	DataType_Signed64,   // s64
	DataType_Unsigned8,  // u8
	DataType_Unsigned16, // u16
	DataType_Unsigned32, // u32 
	DataType_Unsigned64, // u64 
	DataType_Float32,    // f32 
	DataType_Float64,    // f64 
	DataType_String,     // str
	DataType_Any,
	DataType_Structure,  // data type of types and functions
}; 

const char* dataTypeStrs[] = {
	"notype",
	"void",
	"impl",  
	"s8",  
	"s16",
	"s32",  
	"s64",  
	"u8", 
	"u16",
	"u32",
	"u64",
	"f32",   
	"f64",   
	"str",    
	"ptr",   
	"any",
}; 

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Lexer
enum{
	Token_Null = 0,
	Token_ERROR = 0,                // when something doesnt make sense during lexing
	Token_EOF,                      // end of file
	
	TokenGroup_Identifier,
	Token_Identifier = TokenGroup_Identifier,  // function, variable and struct names                 
	
	//// literal ////
	TokenGroup_Literal,
	Token_LiteralFloat = TokenGroup_Literal,
	Token_LiteralInteger,
	Token_LiteralCharacter,
	Token_LiteralString,
	
	//// control ////
	TokenGroup_Control,
	Token_Semicolon = TokenGroup_Control, // ;
	Token_OpenBrace,                      // {
	Token_CloseBrace,                     // }
	Token_OpenParen,                      // (
	Token_CloseParen,                     // )
	Token_OpenSquare,                     // [
	Token_CloseSquare,                    // ]
	Token_Comma,                          // ,
	Token_QuestionMark,                   // ?
	Token_Colon,                          // :
	Token_Dot,                            // .
	Token_At,                             // @
	Token_Pound,                          // #
	Token_Backtick,                       // `
	
	//// operators ////
	TokenGroup_Operator,
	Token_Plus = TokenGroup_Operator, // +
	Token_Increment,                  // ++
	Token_PlusAssignment,             // +=
	Token_Negation,                   // -
	Token_Decrement,                  // --
	Token_NegationAssignment,         // -=
	Token_Multiplication,             // *
	Token_MultiplicationAssignment,   // *=
	Token_Division,                   // /
	Token_DivisionAssignment,         // /=
	Token_BitNOT,                     // ~
	Token_BitNOTAssignment,           // ~=
	Token_BitAND,                     // &
	Token_BitANDAssignment,           // &=
	Token_AND,                        // &&
	Token_BitOR,                      // |
	Token_BitORAssignment,            // |=
	Token_OR,                         // ||
	Token_BitXOR,                     // ^
	Token_BitXORAssignment,           // ^=
	Token_BitShiftLeft,               // <<
	Token_BitShiftLeftAssignment,     // <<=
	Token_BitShiftRight,              // >>
	Token_BitShiftRightAssignment,    // >>=
	Token_Modulo,                     // %
	Token_ModuloAssignment,           // %=
	Token_Assignment,                 // =
	Token_Equal,                      // ==
	Token_LogicalNOT,                 // !
	Token_NotEqual,                   // !=
	Token_LessThan,                   // <
	Token_LessThanOrEqual,            // <=
	Token_GreaterThan,                // >
	Token_GreaterThanOrEqual,         // >=
	
	//// keywords ////
	TokenGroup_Keyword,
	Token_Return = TokenGroup_Keyword, // return
	Token_If,                          // if
	Token_Else,                        // else
	Token_For,                         // for
	Token_While,                       // while 
	Token_Break,                       // break
	Token_Continue,                    // continue
	Token_Defer,                       // defer
	Token_StructDecl,                  // struct
	Token_This,                        // this
	Token_Using,                       // using
	Token_As,                          // as
	
	//// types  ////
	TokenGroup_Type,
	Token_Void = TokenGroup_Type, // void
	Token_Signed8,                // s8
	Token_Signed16,               // s16 
	Token_Signed32,               // s32 
	Token_Signed64,               // s64
	Token_Unsigned8,              // u8
	Token_Unsigned16,             // u16
	Token_Unsigned32,             // u32 
	Token_Unsigned64,             // u64 
	Token_Float32,                // f32 
	Token_Float64,                // f64 
	Token_String,                 // str
	Token_Any,                    // any
	Token_Struct,                 // user defined type

	//// directives ////
	TokenGroup_Directive,
	Token_Directive_Import = TokenGroup_Directive,
	Token_Directive_Include,
	Token_Directive_Internal,
	Token_Directive_Run,

}; //typedef u32 Token_Type;

#define NAME(code) STRINGIZE(code)
const char* TokenTypes_Names[] = {
	"null/error",
	NAME(Token_EOF),
	NAME(Token_Identifier),
	
	NAME(Token_LiteralFloat),
	NAME(Token_LiteralInteger),
	NAME(Token_LiteralCharacter),
	NAME(Token_LiteralString),
	
	NAME(Token_Semicolon),
	NAME(Token_OpenBrace),
	NAME(Token_CloseBrace),
	NAME(Token_OpenParen),
	NAME(Token_CloseParen),
	NAME(Token_OpenSquare),
	NAME(Token_CloseSquare),
	NAME(Token_Comma),
	NAME(Token_QuestionMark),
	NAME(Token_Colon),
	NAME(Token_Dot),
	NAME(Token_At),
	NAME(Token_Pound),
	NAME(Token_Backtick),
	
	NAME(Token_Plus),
	NAME(Token_Increment),
	NAME(Token_PlusAssignment),
	NAME(Token_Negation),
	NAME(Token_Decrement),
	NAME(Token_NegationAssignment),
	NAME(Token_Multiplication),
	NAME(Token_MultiplicationAssignment),
	NAME(Token_Division),
	NAME(Token_DivisionAssignment),
	NAME(Token_BitNOT),
	NAME(Token_BitNOTAssignment),
	NAME(Token_BitAND),
	NAME(Token_BitANDAssignment),
	NAME(Token_AND),
	NAME(Token_BitOR),
	NAME(Token_BitORAssignment),
	NAME(Token_OR),
	NAME(Token_BitXOR),
	NAME(Token_BitXORAssignment),
	NAME(Token_BitShiftLeft),
	NAME(Token_BitShiftLeftAssignment),
	NAME(Token_BitShiftRight),
	NAME(Token_BitShiftRightAssignment),
	NAME(Token_Modulo),
	NAME(Token_ModuloAssignment),
	NAME(Token_Assignment),
	NAME(Token_Equal),
	NAME(Token_LogicalNOT),
	NAME(Token_NotEqual),
	NAME(Token_LessThan),
	NAME(Token_LessThanOrEqual),
	NAME(Token_GreaterThan),
	NAME(Token_GreaterThanOrEqual),
	
	NAME(Token_Return),
	NAME(Token_If),
	NAME(Token_Else),
	NAME(Token_For),
	NAME(Token_While),
	NAME(Token_Break),
	NAME(Token_Continue),
	NAME(Token_Defer),
	NAME(Token_StructDecl),
	NAME(Token_This),
	
	NAME(Token_Void),
	NAME(Token_Signed8),
	NAME(Token_Signed16),
	NAME(Token_Signed32),
	NAME(Token_Signed64),
	NAME(Token_Unsigned8),
	NAME(Token_Unsigned16),
	NAME(Token_Unsigned32),
	NAME(Token_Unsigned64),
	NAME(Token_Float32),
	NAME(Token_Float64),
	NAME(Token_String),
	NAME(Token_Any),
	NAME(Token_Struct),
};
#undef NAME

struct Token {
	Type type;
	Type group;
	str8 raw; 
	u64  raw_hash;
	
	str8 file;
	u32 l0, l1;
	u32 c0, c1;
	u8* line_start;

	u32 scope_depth;
	u32 idx;

	b32 is_global; //set true on tokens that are in global scope 
	b32 is_declaration; //set true on identifier tokens that are the identifier of a declaration
	Type decl_type;

	union{
		f64 f64_val;
		s64 s64_val;
		u64 u64_val;
	};
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Abstract Syntax Tree 
enum {
	Expression_IdentifierLHS,
	Expression_IdentifierRHS,
	
	Expression_Function_Call,
	
	//Special ternary conditional expression type
	Expression_TernaryConditional,
	
	//Types
	Expression_Literal,
	
	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,
	Expression_IncrementPrefix,
	Expression_IncrementPostfix,
	Expression_DecrementPrefix,
	Expression_DecrementPostfix,
	
	//Binary Operators
	Expression_BinaryOpPlus,
	Expression_BinaryOpMinus,
	Expression_BinaryOpMultiply,
	Expression_BinaryOpDivision,
	Expression_BinaryOpAND,
	Expression_BinaryOpBitAND,
	Expression_BinaryOpOR,
	Expression_BinaryOpBitOR,
	Expression_BinaryOpLessThan,
	Expression_BinaryOpGreaterThan,
	Expression_BinaryOpLessThanOrEqual,
	Expression_BinaryOpGreaterThanOrEqual,
	Expression_BinaryOpEqual,
	Expression_BinaryOpNotEqual,
	Expression_BinaryOpModulo,
	Expression_BinaryOpBitXOR,
	Expression_BinaryOpBitShiftLeft,
	Expression_BinaryOpBitShiftRight,
	Expression_BinaryOpAssignment,
	Expression_BinaryOpMemberAccess,
};

static const char* ExTypeStrings[] = {
	"idLHS: ",
	"idRHS: ",
	
	"fcall: ",
	
	"tern: ",
	
	"literal: ",
	
	"~",
	"!",
	"-",
	"++ pre",
	"++ post",
	"-- pre",
	"-- post",
	
	"+",
	"-",
	"*",
	"/",
	"&&",
	"&",
	"||",
	"|",
	"<",
	">",
	"<=",
	">=",
	"==",
	"!=",
	"%",
	"^",
	"<<",
	">>",
	"=",
	"accessor",
};

struct Struct;
struct Expression {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	str8 expstr;

	Type expr_type;
	Type data_type;
	Struct* struct_type;
	union {
		f32 float32;
		f64 float64;
		s8  int8;
		s16 int16;
		s32 int32;
		s64 int64;
		u8  uint8;
		u16 uint16;
		u32 uint32;
		u64 uint64;
		cstring str;
	};
};
#define ExpressionFromNode(x) CastFromMember(Expression, node, x)

enum {
	Statement_Unknown,
	Statement_Return,
	Statement_Expression,
	Statement_Declaration,
	Statement_Conditional,
	Statement_Else,
	Statement_For,
	Statement_While,
	Statement_Break,
	Statement_Continue,
	Statement_Struct,
};

struct Statement {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	Type type = Statement_Unknown;
};
#define StatementFromNode(x) CastFromMember(Statement, node, x)

struct Scope {
	TNode node;
	Token* token_start;
	Token* token_end;
	
	b32 has_return_statement = false;
};
#define ScopeFromNode(x) CastFromMember(Scope, node, x)

//represents information about an original declaration and is stored on 
//Struct,Function,and Variable.
//TODO(sushi) this setup is kind of scuffed, maybe just store the 3 structs on decl in a union so we can avoid having 2 layers of casting (eg. VariableFromDeclaration)
struct Struct;
struct Variable;
struct Function;
struct ParserThread;
struct Declaration{
	TNode node;
	Token* token_start;
	Token* token_end;
	
	//name of declaration as it was first found 
	str8 identifier;
	str8 declared_identifier;
	Type type;

	//set true when the declaration has been fully parsed
	b32 complete = 0;
	//set true when a thread starts working on this struct. this is necessary to block other threads from 
	//thinking a declaration is not complete and therefore doesnt exist
	b32 in_progress = 0;
	//used to identify which thread is working on this
	//and so a thread does not wait on itself to finish a declaration (ex. member functions using the type that is being worked on)
	ParserThread* working_thread = 0;
};

#define DeclarationFromNode(x) CastFromMember(Declaration, node, x)

struct Function {
	Declaration decl;

	Type data_type;
	Struct* struct_data;
	//this label is how a function is internally referred to in the case of overloading
	//it is where name mangling happens
	//format:
	//    func_name@argtype1,argtype2,...@rettype1,rettype2,...
	str8 internal_label;
	u32 positional_args = 0;
	map<str8, Declaration*> args;
	//TODO do this with a binary tree sort of thing instead later
	array<Function*> overloads;
};
#define FunctionFromDeclaration(x) CastFromMember(Function, decl, x)
#define FunctionFromNode(x) FunctionFromDeclaration(DeclarationFromNode(x))

struct Variable{
	Declaration decl;

	Type data_type;

	//number of times * appears on a variable's type specifier
	u32 pointer_depth;

	Struct* struct_data;
	union {
		f32 float32;
		f64 float64;
		s8  int8;
		s16 int16;
		s32 int32;
		s64 int64;
		u8  uint8;
		u16 uint16;
		u32 uint32;
		u64 uint64;
		str8 str;
	};
};
#define VariableFromDeclaration(x) CastFromMember(Variable, decl, x)
#define VariableFromNode(x) VariableFromDeclaration(DeclarationFromNode(x))

struct Struct {
	Declaration decl;

	Type type;
	
	declmap members;
};
#define StructFromDeclaration(x) CastFromMember(Struct, decl, x)
#define StructFromNode(x) StructFromDeclaration(DeclarationFromNode(x))


enum{
	Declaration_Unknown,
	Declaration_Function,
	Declaration_Variable,
	Declaration_Structure,
};



struct Program {
	Node node;
	str8 filename;
	//TODO add entrypoint string
};

enum {
	psFile,
	psDirective,
	psImport,
	psRun,
	psScope,
	psDeclaration,
	psStatement,
	psExpression,
	psConditional,
	psLogicalOR,  
	psLogicalAND, 
	psBitwiseOR,  
	psBitwiseXOR, 
	psBitwiseAND, 
	psEquality,   
	psRelational, 
	psBitshift,   
	psAdditive,   
	psTerm,       
	psFactor,     
};

const str8 psStrs[] = {
	STR8("File"),
	STR8("Directive"),
	STR8("Import"),
	STR8("Run"),
	STR8("Scope"),
	STR8("Declaration"),
	STR8("Statement"),
	STR8("Expression"),
	STR8("Conditional"),
	STR8("LogicalOR"),  
	STR8("LogicalAND"), 
	STR8("BitwiseOR"),  
	STR8("BitwiseXOR"), 
	STR8("BitwiseAND"), 
	STR8("Equality"),   
	STR8("Relational"), 
	STR8("Bitshift"),   
	STR8("Additive"),   
	STR8("Term"),       
	STR8("Factor"),     
};

enum{
	FileStage_Null,
	FileStage_Lexer,
	FileStage_Preprocessor,
	FileStage_Parser,
};

struct Token;
struct suFile;
struct suMessage{
	u32 verbosity;
	Token* token;
	Type type; //log, error, or warn	
	u32 indent;
	f64 time_made;
	str8 prefix;
	suFile* sufile;
	array<str8> message_parts;
};

struct suLogger{
	// instead of immediatly logging messages we collect them to format and display later
	// we do this to prevent threads' messages from overlapping each other, to prevent
	// actually printing anything during compiling, and so we can do much nicer formatting later on
	// this behavoir is disabled by globals.log_immediatly
	//TODO(sushi) implement an arena instead of array so that we know this will be thread safe
	array<suMessage> messages;

	suFile* sufile = 0;
	str8 owner_str_if_sufile_is_0 = {0,0};
	
	template<typename...T>
	void log(u32 verbosity, T... args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.verbosity < verbosity) return;
		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0), VTS_Default, ": ", args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Log;
			message.verbosity = verbosity;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}

	template<typename...T>
	void log(Token* t, u32 verbosity, T... args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.verbosity < verbosity) return;
		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0),  VTS_Default, "(",t->l0,",",t->c0,"): ", args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Log;
			message.verbosity = verbosity;
			message.token = t;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}

	template<typename...T>
	void error(Token* token, T...args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0), VTS_Default, "(",token->l0,",",token->c0,"): ", ErrorFormat("error: "), args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Error;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			message.token = token;
			messages.add(message);
		}
		if(globals.assert_compiler_on_error) DebugBreakpoint;
	}

	template<typename...T>
	void error(T...args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0), VTS_Default, ": ", ErrorFormat("error: "), args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Error;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
		if(globals.assert_compiler_on_error) DebugBreakpoint;
	}

	template<typename...T>
	void warn(Token* token, T...args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0), VTS_Default, "(",token->l0,",",token->c0,"): ", WarningFormat("error: "), args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Warn;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}
	
	template<typename...T>
	void warn(T...args){DPZoneScoped;
		if(globals.supress_messages) return;

		if(globals.log_immediatly){
			compiler.mutexes.log.lock();
			Log("", VTS_CyanFg, (sufile ? sufile->file->name : owner_str_if_sufile_is_0), WarningFormat("warning: "), args...);
			compiler.mutexes.log.unlock();
		}else{
			suMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Warn;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}
};

struct suFile{
	File* file;
	str8 file_buffer;
	Type stage;

	suLogger logger;

	struct{ // lexer
		array<Token> tokens;
		array<u32> declarations; // list of : tokens
		array<u32> imports;      // list of import tokens
		array<u32> internals;    // list of internal tokens
		array<u32> runs;         // list of run tokens
	}lexer;

	struct{ // preprocessor
		array<suFile*> imported_files;
		array<u32> exported_decl;
		array<u32> internal_decl;
		array<u32> runs;
	
	}preprocessor;

	struct{ // parser
		//node that all top-level declarations attach to 
		TNode base;

		//toplevel declarations known to the parser, these can be maps (i think :) because 
		//we should never allow a global variable to shadow another global variable
		declmap exported_decl;
		declmap imported_decl;
		declmap internal_decl;

		Declaration*
		find_identifier(str8 id){DPZoneScoped;
			if(Declaration* d = exported_decl.at(id)) return d;
			if(Declaration* d = imported_decl.at(id)) return d;
			if(Declaration* d = internal_decl.at(id)) return d;
			return 0;
		}

		//used by a parser parsing a different file
		Declaration*
		find_identifier_externally(str8 id){DPZoneScoped;
			if(Declaration* d = exported_decl.at(id)) return d;
			if(Declaration* d = imported_decl.at(id)) return d;
			return 0;
		}
	}parser;
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Lexer

struct Lexer {
	suFile* sufile;
	void lex();
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Preprocessor

struct Preprocessor {
	suFile* sufile;
	void preprocess();
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Parser

struct Parser;
struct ParserThread{
	suFile* sufile;
	
	Parser* parser;
	Type stage; //entry stage

	TNode* node;
	Token* curt;

	b32 is_internal;

	b32 finished=0;
	condvar cv;

	//stacks of known things 
	//TODO(sushi) replace with arenas
	struct{
		//stacks of declarations known in current scope
		//this array stores the number of declarations that have been pushed in the current scope
		//so we know how many to pop when going out of the scope
		array<u32> known_declarations_pushed;
		array<Declaration*> known_declarations;
		//stacks of elements that we are working with
		struct{
			array<Scope*>      scopes;
			array<Struct*>     structs;
			array<Variable*>   variables;
			array<Function*>   functions;
			array<Expression*> expressions;
		}nested;
	}stacks;

	//keeps track of what element we are working with 
	struct{
		Variable*   variable = 0;
		Expression* expression = 0;
		Struct*     structure = 0;
		Scope*      scope = 0;
		Function*   function = 0;
	}current;	
	
	Declaration* declare();
	TNode* define(TNode* node, Type stage);
	TNode* parse_import(Token* start);

	template<typename ...args> FORCE_INLINE b32
	curr_match(args... in){DPZoneScoped;
		return (((curt)->type == in) || ...);
	}

	template<typename ...args> FORCE_INLINE b32
	curr_match_group(args... in){DPZoneScoped;
		return (((curt)->group == in) || ...);
	}

	template<typename ...args> FORCE_INLINE b32
	next_match(args... in){DPZoneScoped;
		return (((curt + 1)->type == in) || ...);
	}

	template<typename ...args> FORCE_INLINE b32
	next_match_group(args... in){DPZoneScoped;
		return (((curt + 1)->group == in) || ...);
	}

	void init(){DPZoneScoped;
		stacks.nested.scopes =      array<Scope*>(deshi_allocator);
		stacks.nested.structs =     array<Struct*>(deshi_allocator);
		stacks.nested.variables =   array<Variable*>(deshi_allocator);
		stacks.nested.functions =   array<Function*>(deshi_allocator);
		stacks.nested.expressions = array<Expression*>(deshi_allocator);
	}

	void wait_for_dependency(str8 id){

	}
};

struct Parser {
	//maps a str8 indicating what identifier the thread is working on to a parser thread
	array<ParserThread> threads;

	//map of identifiers to global declarations
	declmap pending_globals;

	//file the parser is working in
	suFile* sufile;

	void parse();

	void spawn_parser(ParserThread pt);

	void wait_for_dependency(str8 id);
};


void parse_threaded_stub(void* pthreadinfo){DPZoneScoped;
	ParserThread* pt = (ParserThread*)pthreadinfo;
	pt->sufile = pt->parser->sufile;
	pt->define(pt->node, pt->stage);
	pt->finished = 1;
	pt->cv.notify_all();
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Compiler

struct CompilerRequest{
	Type stage;
	array<str8> filepaths;
};

struct Compiler{

	Stopwatch ctime;

	suLogger logger;

	map<str8, suFile> files;

	//locked when doing non-thread safe stuff 
	//such as loading a File, and probably when we use memory functions as well
	struct{
		DPTracyLockable(mutex, lexer);
		DPTracyLockable(mutex, preprocessor);
		DPTracyLockable(mutex, parser);
		DPTracyLockable(mutex, memory); //lock when using deshi's memory module
		DPTracyLockable(mutex, log); //lock when using logger
	}mutexes;

	void compile(CompilerRequest* request);

	suFile* start_lexer       (suFile* sufile);
	suFile* start_preprocessor(suFile* sufile);
	suFile* start_parser      (suFile* sufile);
	void    start_request(CompilerRequest* request);

	//used to completely reset all compiled information
	//this is mainly for performance testing, like running a compile on the same file
	//repeatedly to get an average time
	//or to test for memory leaks
	void reset(); 

}compiler;

struct CompilerThread{
	str8 filepath;
	Type stage;
	condvar wait;
	b32 finished;
	suFile* sufile;
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory

struct{
	Arena* functions;
	Arena* variables;
	Arena* structs;
	Arena* scopes;
	Arena* expressions;

	FORCE_INLINE
	Function* make_function(){DPZoneScoped;
		Function* ret = (Function*)functions->cursor;
		functions->cursor += sizeof(Function);
		functions->used += sizeof(Function);
		return ret;
	}

	FORCE_INLINE
	Variable* make_variable(){DPZoneScoped;
		Variable* ret = (Variable*)variables->cursor;
		variables->cursor += sizeof(Variable);
		variables->used += sizeof(Variable);
		return ret;
	}

	FORCE_INLINE
	Struct* make_struct(){DPZoneScoped;
		Struct* ret = (Struct*)structs->cursor;
		structs->cursor += sizeof(Struct);
		structs->used += sizeof(Struct);
		return ret;
	}

	FORCE_INLINE
	Scope* make_scope(){DPZoneScoped;
		Scope* ret = (Scope*)structs->cursor;
		scopes->cursor += sizeof(Scope);
		scopes->used += sizeof(Scope);
		return ret;
	}

	FORCE_INLINE
	Expression* make_expression(){DPZoneScoped;
		Expression* ret = (Expression*)expressions->cursor;
		expressions->cursor += sizeof(Expression);
		expressions->used += sizeof(Expression);
		return ret;
	}

	FORCE_INLINE
	void init(){DPZoneScoped;
		functions   = memory_create_arena(Kilobytes(512));
		variables   = memory_create_arena(Kilobytes(512));
		structs     = memory_create_arena(Kilobytes(512));
		scopes      = memory_create_arena(Kilobytes(512));
		expressions = memory_create_arena(Kilobytes(512));
	}

}arena;

#endif //SU_TYPES_H