#pragma once
#ifndef SU_TYPES_H
#define SU_TYPES_H

#include "utils/defines.h"

//data type specifiers
enum DataType : u32 { 
	DataType_NotTyped,
	DataType_Implicit,   // implicitly typed
	DataType_Signed32,   // s32 
	DataType_Signed64,   // s64 
	DataType_Unsigned32, // u32 
	DataType_Unsigned64, // u64 
	DataType_Float32,    // f32 
	DataType_Float64,    // f64 
	DataType_String,     // str
	DataType_Pointer,    // type*
	DataType_Structure,  // data type of types and functions
}; //typedef u32 DataType;

//abstract node tree struct
struct Node {
	Node* next = 0;
	Node* prev = 0;

	Node* parent = 0;
	Node* first_child = 0;
	Node* last_child = 0;
	u32 child_count = 0;

	string debug_str;

	Node() {
		next = prev = this;
	}
};

inline Node* NewNode() {
	//TODO eventually these Nodes should be stored somewhere so they're not randomly alloced all over the place
	return new Node;
}

inline void NodeInsertNext(Node* to, Node* from, string debugstr = "") {
	if(to->next != to) from->next = to->next;
	from->prev = to;
	from->next->prev = from;
	to->next = from;
	from->parent = to->parent; //this maybe should just be done on insert child
	from->debug_str=debugstr;
}

inline void NodeInsertPrev(Node* to, Node* from, string debugstr = "") {
	if(to->prev != to) from->prev = to->prev;
	from->next = to;
	from->prev->next = from;
	to->prev = from;
	from->parent = to->parent;
	from->debug_str = debugstr;
}

inline void NodeRemove(Node* node, string debugstr = "") {
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

inline void NodeInsertChild(Node* parent, Node* child, string debugstr = "") {
	//TODO maybe we can avoid these checks ?
	if (!parent->first_child) parent->first_child = child;
	child->prev = parent->last_child;
	if (parent->last_child) parent->last_child->next = child;
	parent->last_child = child;
	child->parent = parent;
	child->debug_str=debugstr;
}
//TODO remove child node

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
		memcpy(cursor, &in, sizeof(T));
		cursor += sizeof(T);
		return cursor - sizeof(T);
	}
};

#endif