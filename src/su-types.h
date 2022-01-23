#pragma once
#ifndef SU_TYPES_H
#define SU_TYPES_H

#include "utils/defines.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
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

enum OSOut {
	OSOut_Windows,
	OSOut_Linux,
	OSOut_OSX,
};

struct {
	u32   warning_level    = 1;
	b32   verbose_print    = false;
	b32   supress_warnings = false;
	b32   supress_errors   = false;
	b32   supress_messages = false;
	OSOut osout            = OSOut_Windows;
} globals;


///////////////////////////////////////////////////////////////////////////////////////////////////
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
	Register_RCX = Register1_64,  Register_ECX  = Register1_32,  Register_CX   = Register1_16,  Register_CL   = Register1_8,
	Register_RDX = Register2_64,  Register_EDX  = Register2_32,  Register_DX   = Register2_16,  Register_DL   = Register2_8,
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

global_ const char* registers_x64[] = {
	"%null",
	"%rax", "%eax",  "%ax",   "%al",
	"%rcx", "%ecx",  "%cx",   "%cl",
	"%rdx", "%edx",  "%dx",   "%dl",
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


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Builtin Types
enum DataType : u32 { 
	DataType_NotTyped,
	DataType_Void,       // void
	DataType_Implicit,   // implicitly typed
	DataType_Signed8,    // s8
	DataType_Signed32,   // s32 
	DataType_Signed64,   // s64
	DataType_Unsigned8,  // u8
	DataType_Unsigned32, // u32 
	DataType_Unsigned64, // u64 
	DataType_Float32,    // f32 
	DataType_Float64,    // f64 
	DataType_String,     // str
	DataType_Pointer,    // type*
	DataType_Any,
	DataType_Structure,  // data type of types and functions
}; //typedef u32 DataType;

const char* dataTypeStrs[] = {
	"notype",
	"void",
	"impl",  
	"s8",   
	"s32",  
	"s64",  
	"u8", 
	"u32",
	"u64",
	"f32",   
	"f64",   
	"str",    
	"ptr",   
	"any",
	"struct", 
}; 

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Nodes
enum NodeType : u32 {
	NodeType_Program,
	NodeType_Function,
	NodeType_Scope,
	NodeType_Declaration,
	NodeType_Statement,
	NodeType_Expression,
};

//abstract node tree struct
struct Node {
	Node* next = 0;
	Node* prev = 0;
	Node* parent = 0;
	Node* first_child = 0;
	Node* last_child = 0;
	u32   child_count = 0;
	
	NodeType type;
	
	//debug vars
	string comment;
};

#define for_node(node) for(Node* it = node; it != 0; it = it->next)
#define for_node_reverse(node) for(Node* it = node; it != 0; it = it->prev)

inline void insert_after(Node* target, Node* node){
	if(target->next) target->next->prev = node;
	node->next = target->next;
	node->prev = target;
	target->next = node;
}

inline void insert_before(Node* target, Node* node){
	if(target->prev) target->prev->next = node;
	node->prev = target->prev;
	node->next = target;
	target->prev = node;
}

inline void remove_horizontally(Node* node){
	if(node->next) node->next->prev = node->prev;
	if(node->prev) node->prev->next = node->next;
	node->next = node->prev = 0;
}

void insert_last(Node* parent, Node* child){
	if(parent == 0){ child->parent = 0; return; }
	
	child->parent = parent;
	if(parent->first_child){
		insert_after(parent->last_child, child);
		parent->last_child = child;
	}else{
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

void insert_first(Node* parent, Node* child){
	if(parent == 0){ child->parent = 0; return; }
	
	child->parent = parent;
	if(parent->first_child){
		insert_before(parent->first_child, child);
		parent->first_child = child;
	}else{
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

void remove(Node* node){
	//remove self from parent
	if(node->parent){
		if(node->parent->child_count > 1){
			if(node == node->parent->first_child) node->parent->first_child = node->next;
			if(node == node->parent->last_child)  node->parent->last_child  = node->prev;
		}else{
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	
	//add children to parent (and remove self from children)
	if(node->child_count > 1){
		for(Node* child = node->first_child; child != 0; child = child->next){
			insert_last(node->parent, child);
		}
	}
	
	//remove self horizontally
	remove_horizontally(node);
	
	//reset self  //TODO not necessary if we are deleting this node, so exclude this logic in another function NodeDelete?
	node->parent = node->first_child = node->last_child = 0;
	node->child_count = 0;
}

void change_parent(Node* new_parent, Node* node){
	//if old parent, remove self from it 
	if(node->parent){
		if(node->parent->child_count > 1){
			if(node == node->parent->first_child) node->parent->first_child = node->next;
			if(node == node->parent->last_child)  node->parent->last_child  = node->prev;
		}else{
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	
	//remove self horizontally
	remove_horizontally(node);
	
	//add self to new parent
	insert_last(new_parent, node);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
struct Arena {
	u8* data = 0;
	u8* cursor = 0;
	upt size = 0;
	
	void init(upt bytes) {
		data = (u8*)calloc(1, bytes);
		cursor = data;
		size = bytes;
	}
	
	template<typename T>
		void* add(const T& in) {
		if (cursor - data < sizeof(T) + size) {
			data = (u8*)calloc(1, size); 
			cursor = data;
		}
		*((T*)cursor) = in;
		cursor += sizeof(T);
		return cursor - sizeof(T);
	}
};

#endif //SU_TYPES_H