#pragma once
#ifndef AMU_TYPES_H
#define AMU_TYPES_H

#include "kigu/arrayT.h"
#include "kigu/common.h"
#include "kigu/string.h"
#include "core/threading.h"
#include "ctype.h"

#define SetThreadName(...) threader_set_thread_name(amuStr8(__VA_ARGS__))

//attempt at making str8 building thread safe
#define amuStr8(...) to_str8_amu(__VA_ARGS__)
template<class...T>
str8 to_str8_amu(T... args){DPZoneScoped;
	persist mutex tostr8_lock = mutex_init();
	mutex_lock(&tostr8_lock);
	mutex_lock(&global_mem_lock);
	str8b str; str8_builder_init(&str, {0}, deshi_temp_allocator);
	constexpr auto arg_count{sizeof...(T)};
	str8 arr[arg_count] = {to_str8(args, deshi_temp_allocator)...};
	forI(arg_count) str8_builder_append(&str, arr[i]);
	mutex_unlock(&global_mem_lock);
	mutex_unlock(&tostr8_lock);
	return str.fin;
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
	
	WC_Empty_Import_Directive,

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

/*	Verbosity
	0: Does not display any information other than warnings, errors, and notes that may come with them
	1: Shows each files name as it is reached in compilation 
	2: Shows the stages as they happen and the time it took for them to complete
	3: Shows parts of the stages as they happen
	4: Shows even more detail about whats happening in each stage
	5: Shows compiler debug information from each stage
*/

enum{
	Verbosity_Always,
	Verbosity_FileNames,
	Verbosity_Stages,
	Verbosity_StageParts,
	Verbosity_Detailed,
	Verbosity_Debug,
};

struct Format{
	color fg;
	color bg;
	str8 prefix;
	str8 suffix;
};

struct {
	u32 warning_level = 1;
	u32 verbosity = Verbosity_Debug;
	u32 indent = 0;
	b32 supress_warnings   = false;
	b32 supress_messages   = false;
	b32 warnings_as_errors = true;
	b32 show_code = true;
	b32 log_immediatly           = true;
	b32 assert_compiler_on_error = true;
	b32 disable_colors           = false;
	OSOut osout = OSOut_Windows;

	struct{
		Format function = {
			color{0x82aaff},
			color{0xffffff},
			str8{0},str8{0}
		};
		//TODO(sushi) expand on this idea

	}formatting;
} globals;

enum{
	Message_Log,
	Message_Error,
	Message_Warn,
	Message_Note
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
	NodeType_Module,
	NodeType_Label,
};

//thread safe version of TNode
//this only accounts for modifying a node, not reading it
//but I believe that modification and reading shouldnt happen at the same time, so its probably fine
//TODO(sushi) decide if this is necessary, the only place threading becomes a problem with this is syntax_analyzer adding stuff to its base node
//            but that can be deferred.
struct amuNode{
	Type  type;
	Flags flags;
	
	amuNode* next = 0;
	amuNode* prev = 0;
	amuNode* parent = 0;
	amuNode* first_child = 0;
	amuNode* last_child = 0;
	u32      child_count = 0;
	
	mutex lock;

	str8 debug;
};

/*
	before:
		O
	   / \	
	  O	- T <- target 
		N <- node

	after:
	   O
	  / \ 
     O - T - N  
*/
global inline void insert_after(amuNode* target, amuNode* node) { DPZoneScoped;
	mutex_lock(&target->lock);
	mutex_lock(&node->lock);
	defer{
		mutex_unlock(&node->lock);
		mutex_unlock(&target->lock);
	};
	if (target->next) target->next->prev = node;
	node->next = target->next;
	node->prev = target;
	target->next = node;
}

/*
	before:
		O
	   / \	
	  O	- T <- target 
		N <- node

	after:
		  O
	   /     \  
      O - N - T
	NOTE this doesn't connect it to the target's parent! use the insert_first/last functions if a parent exists.
*/
global inline void insert_before(amuNode* target, amuNode* node) { DPZoneScoped;
	mutex_lock(&target->lock);
	mutex_lock(&node->lock);
	defer{
		mutex_unlock(&target->lock);
		mutex_unlock(&node->lock);
	};
	if (target->prev) target->prev->next = node;
	node->prev = target->prev;
	node->next = target;
	target->prev = node;
}

/*
	before:
		O
	   / \
	  O - N <- node

	after:
		O
	   / \   
      O   N  
	NOTE still connected to parent! this causes an incorrect structure, so you must remove from parent as well
*/
global inline void remove_horizontally(amuNode* node) { DPZoneScoped;
	mutex_lock(&node->lock);
	defer {mutex_unlock(&node->lock);};
	if (node->next) node->next->prev = node->prev;
	if (node->prev) node->prev->next = node->next;
	node->next = node->prev = 0;
}

/*
	before:
		O <- parent
	   / \	
	  A	- B  
		N <- node

	after:
		O
	 /  |  \	
    A - B - N 
*/
global void insert_last(amuNode* parent, amuNode* child) { DPZoneScoped;
	if (parent == 0) { child->parent = 0; return; }
	if(parent==child){DebugBreakpoint;}

	mutex_lock(&parent->lock);
	mutex_lock(&child->lock);
	defer {
		mutex_unlock(&parent->lock);
		mutex_unlock(&child->lock);
	};
	
	child->parent = parent;
	if (parent->first_child) {
		insert_after(parent->last_child, child);
		parent->last_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

/*
	before:
		O <- parent
	   / \	
	  A	- B  
		N <- node

	after:
		O
	 /  |  \	
    N - A - B 
*/

global void insert_first(amuNode* parent, amuNode* child) { DPZoneScoped;
	mutex_lock(&parent->lock);
	mutex_lock(&child->lock);
	defer{
		mutex_unlock(&parent->lock);
		mutex_unlock(&child->lock);
	};
	if (parent == 0) { child->parent = 0; return; }
	
	child->parent = parent;
	if (parent->first_child) {
		insert_before(parent->first_child, child);
		parent->first_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
	
}

global void change_parent(amuNode* new_parent, amuNode* node);
global void remove(amuNode* node);
global void insert_above(amuNode* below, amuNode* above){DPZoneScoped;
	mutex_lock(&below->lock);
	mutex_lock(&above->lock);
	defer{
		mutex_unlock(&below->lock);
		mutex_unlock(&above->lock);
	};

	amuNode copy = *below;
	remove(below);
	copy.parent->child_count++;

	if(copy.parent){
		above->parent = copy.parent;
		if(copy.next && copy.prev){
			insert_after(copy.prev, above);
		}else if( copy.next && !copy.prev){
			insert_before(copy.next, above);
			copy.parent->first_child = above;
		}else if(!copy.next &&  copy.prev){
			insert_after(copy.prev, above);
			copy.parent->last_child = above;
		}else{
			copy.parent->first_child = copy.parent->last_child = above;
		}
	}

	//insert_after(below, above);
	//above->parent = below->parent;
	change_parent(above, below);
	//if(!above->next) above->parent->last_child = above;
}

global void change_parent(amuNode* new_parent, amuNode* node) { DPZoneScoped;
	mutex_lock(&node->lock);
	defer { mutex_unlock(&node->lock); };

	//if old parent, remove self from it 
	if (node->parent) {
		mutex_lock(&node->parent->lock);
		defer{mutex_unlock(&node->parent->lock);};
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
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

global void move_to_parent_first(amuNode* node){ DPZoneScoped;
	mutex_lock(&node->lock);
	defer { mutex_unlock(&node->lock); };
	if(!node->parent) return;
	
	amuNode* parent = node->parent;
	if(parent->first_child == node) return;
	if(parent->last_child == node) parent->last_child = node->prev;

	remove_horizontally(node);
	node->next = parent->first_child;
	parent->first_child->prev = node;
	parent->first_child = node;
}

global void move_to_parent_last(amuNode* node){ DPZoneScoped;
	mutex_lock(&node->lock);
	defer{mutex_unlock(&node->lock);};
	if(!node->parent) return;
	
	amuNode* parent = node->parent;
	if(parent->last_child == node) return;
	if(parent->first_child == node) parent->first_child = node->next;

	remove_horizontally(node);
	node->prev = parent->last_child;
	parent->last_child->next = node;
	parent->last_child = node;
}

global void remove(amuNode* node) { DPZoneScoped;
	mutex_lock(&node->lock);
	defer{mutex_unlock(&node->lock);};
	//add children to parent (and remove self from children)
	for(amuNode* it = node->first_child; it != 0; ) {
		amuNode* next = it->next;
		change_parent(node->parent, it);
		it = next;
	}
	
	//remove self from parent
	if (node->parent) {
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	node->parent = 0;
	
	//remove self horizontally
	remove_horizontally(node);
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory

mutex global_mem_lock = mutex_init();

void* amu_memalloc(upt size){
	mutex_lock(&global_mem_lock);
	void* ret = memalloc(size);
	mutex_unlock(&global_mem_lock);
	return ret;
}

void amu_memzfree(void* ptr){
	mutex_lock(&global_mem_lock);
	memzfree(ptr);
	mutex_unlock(&global_mem_lock);
}

//attempt at implementing a thread safe memory region 
//this is chunked, so memory never moves unless you use something like remove
template<typename T>
struct amuChunkedArena{
	Arena** arenas;
	u64 arena_count = 0;
	u64 arena_space = 16;
	u64 count = 0;
	mutex write_lock;
	mutex read_lock;

	void init(upt n_obj_per_chunk){
		write_lock = mutex_init();
		read_lock = mutex_init();
		arena_count = 1;
		arenas = (Arena**)amu_memalloc(sizeof(Arena*)*arena_space);
		mutex_lock(&global_mem_lock);
		arenas[arena_count-1] = memory_create_arena(sizeof(T)*n_obj_per_chunk); 
		mutex_unlock(&global_mem_lock);
	}

	void deinit(){
		mutex_lock(&global_mem_lock);
		forI(arena_count){
			memory_delete_arena(arenas[i]);
		}
		mutex_unlock(&global_mem_lock);
		amu_memzfree(arenas);
	}

	T* add(const T& in){
		mutex_lock(&write_lock);

		//make a new arena if needed
		if(arenas[arena_count-1]->used + sizeof(T) > arenas[arena_count-1]->size){
			mutex_lock(&global_mem_lock);
			if(arena_space == arena_count){
				arena_space += 16;
				arenas = (Arena**)memrealloc(arenas, arena_space*sizeof(Arena*));
			}
			arenas[arena_count] = memory_create_arena(arenas[arena_count-1]->size);
			arena_count++;
			mutex_unlock(&global_mem_lock);
		}
		count++;
		memcpy(arenas[arena_count-1]->cursor, &in, sizeof(T));
		T* ret = (T*)arenas[arena_count-1]->cursor;
		arenas[arena_count-1]->used += sizeof(T);
		arenas[arena_count-1]->cursor += sizeof(T);
		mutex_unlock(&write_lock);
		return ret;
	}

	T read(upt idx){
		persist u64 read_count = 0;
		mutex_lock(&read_lock);
		read_count++;
		if(read_count==1){
			mutex_lock(&write_lock);
		}
		mutex_unlock(&read_lock);

		Assert(idx < count);

		u64 chunk_size = arenas[0]->size;
		u64 offset = idx * sizeof(T);
		u64 arenaidx = offset / chunk_size;

		T ret = *(T*)(arenas[arenaidx]->start + (offset - arenaidx * chunk_size));

		mutex_lock(&read_lock);
		read_count--;
		if(!read_count){
			mutex_unlock(&write_lock);
		}
		mutex_unlock(&read_lock);

		return ret; 
	}

	void remove(upt idx){
		mutex_lock(&write_lock);
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

		mutex_unlock(&write_lock);
	}

};

template<typename T>
struct amuArena{
	T* data;
	mutex write_lock;
	mutex read_lock;
	u64 count = 0;
	u64 space = 0;

	void init(upt initial_size = 16){DPZoneScoped;
		write_lock = mutex_init();
		read_lock = mutex_init();
		mutex_lock(&global_mem_lock);
		space = initial_size;
		data = (T*)memalloc(sizeof(T)*space); 
		mutex_unlock(&global_mem_lock);
	}

	void deinit(){DPZoneScoped;
		mutex_deinit(&write_lock);
		mutex_deinit(&read_lock);
		mutex_lock(&global_mem_lock);
		memzfree(data);
		mutex_unlock(&global_mem_lock);
		count = 0;
		space = 0;
	}

	void add(const T& in){DPZoneScoped;
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		if(count == space){
			mutex_lock(&global_mem_lock);
			space += 16;
			data = (T*)memrealloc(data, sizeof(T)*space);
			mutex_unlock(&global_mem_lock);
		}
		memcpy(data+count, &in, sizeof(T));
		count++;
	}

	//TODO(sushi) need to implement a system for allowing an arbitrary amount of threads to read while blocking writing
	T read(upt idx) {DPZoneScoped;
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};

		Assert(idx < count);

		T ret = *(data + idx);

		return ret; 
	}

	void modify(upt idx, const T& val){DPZoneScoped;
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		Assert(idx < count)
		memcpy(data+idx, &val, sizeof(T));
	}

	void insert(upt idx, const T& val){DPZoneScoped;
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		Assert(idx <= count);
		if(idx == count){
			add(val);
		} else if(count == space){
			mutex_lock(&global_mem_lock);
			space += 16;
			data = (T*)memrealloc(data, sizeof(T)*space);
			mutex_unlock(&global_mem_lock);
			memmove(data+idx+1, data+idx, (count-idx)*sizeof(T));
			memcpy(data+idx, &val, sizeof(T));
			count++;
		}else{
			memmove(data+idx+1, data+idx, (count-idx)*sizeof(T));
			memcpy(data+idx, &val, sizeof(T));
			count++;
		}
	}

	T operator [](u64 idx){DPZoneScoped;
		return read(idx);
	}

	T* readptr(u64 idx){
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		//NOTE(sushi) its probably possible that once the thread gets into here 
		//            that another thread has removed enough elements to make this idx invalid
		//            the solution is to just avoid situations where this would happen
		//TODO(sushi) I don't remember what I meant by avoiding 'situations like this', clear this up or make it impossible
		Assert(idx < count);

		T* ret = data + idx;

		return ret;
	}

	void remove(upt idx){DPZoneScoped;
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		Assert(idx < count);
		count--;
		memmove(data + idx, data + idx + 1, (count - idx) * sizeof(T));
	}

	void remove_unordered(upt idx){
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		Assert(idx < count);
		T endval = data[count-1];
		count--;
		if(count){
			data[idx] = endval;
		}
	}

	void clear(){
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		count = 0;
	}

	T pop(u64 _count = 1){ 
		mutex_lock(&write_lock);
		defer{mutex_unlock(&write_lock);};
		Assert(_count <= count);
		T ret;
		forI(_count){
			if(i==_count-1) memcpy(&ret, data+count-1, sizeof(T));
			count--;
		}
		return ret;
	}

	amuArena<T> copy(){
		amuArena<T> nu;
		nu.init(count);
		nu.count = count;
		CopyMemory(nu.data,data,sizeof(T)*count);
		return nu;
	}

	inline T* begin(){ return &data[0]; }
	inline T* end()  { return &data[count]; }
	inline const T* begin()const{ return &data[0]; }
	inline const T* end()  const{ return &data[count]; }
};


struct Label;
struct LabelTable {
	LabelTable* prev; // tables are nested 
	amuArena<Label*> set; // labels already store their own hash on themselves
	amuArena<Label*> ordered; // NOTE(sushi) I am not sure if this is necessary/useful yet
};


// sorted map
//their mutex_deinit(&declarations); this map supports storing keys who have collided
//by storing them as neighbors and storing the unhashed key with the value
//this was delle's idea
//TODO(sushi) decide if we should store pair<str8,Decl*> in data or just use the str8 `identifier`
//            that is on Declaration already, this way we just pass a Declaration* and no str8
//TODO(sushi) make a generic sorted binary searched map for amu
struct Declaration;
struct nodemap{
	amuArena<u64> hashes; 
	amuArena<pair<str8, amuNode*>> data;

	void init(){DPZoneScoped;
		hashes.init(256);
		data.init(256);
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
	u32 add(str8 key, amuNode* val){DPZoneScoped;
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
			return index;
		}else{
			hashes.insert(middle, key_hash);
			data.insert(middle, {key, val});
			return middle;
		}
	}

	b32 has(str8 key){DPZoneScoped;
		return (find_key(key) == -1 ? 0 : 1);
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

	amuNode* at(str8 key){DPZoneScoped;
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
				Log("nodemap", "A collision happened in a nodemap, which is NOT tested, so you should test this if it appears");
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
		return 0;
	}

	amuNode* atIdx(u64 idx){
		Assert(idx < data.count);
		return data[idx].second;
	}


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

global str8 registers_x64[] = {
	STR8("%null"),
	STR8("%rax"), STR8("%eax"),  STR8("%ax"),   STR8("%al"),
	STR8("%rdx"), STR8("%edx"),  STR8("%dx"),   STR8("%dl"),
	STR8("%rcx"), STR8("%ecx"),  STR8("%cx"),   STR8("%cl"),
	STR8("%rbx"), STR8("%ebx"),  STR8("%bx"),   STR8("%bl"),
	STR8("%rsi"), STR8("%esi"),  STR8("%si"),   STR8("%sil"),
	STR8("%rdi"), STR8("%edi"),  STR8("%di"),   STR8("%dil"),
	STR8("%rsp"), STR8("%esp"),  STR8("%sp"),   STR8("%spl"),
	STR8("%rbp"), STR8("%ebp"),  STR8("%bp"),   STR8("%bpl"),
	STR8("%r8"),  STR8("%r8d"),  STR8("%r8w"),  STR8("%r8b"),
	STR8("%r9"),  STR8("%r9d"),  STR8("%r9w"),  STR8("%r9b"),
	STR8("%r10"), STR8("%r10d"), STR8("%r10w"), STR8("%r10b"),
	STR8("%r11"), STR8("%r11d"), STR8("%r11w"), STR8("%r11b"),
	STR8("%r12"), STR8("%r12d"), STR8("%r12w"), STR8("%r12b"),
	STR8("%r13"), STR8("%r13d"), STR8("%r13w"), STR8("%r13b"),
	STR8("%r14"), STR8("%r14d"), STR8("%r14w"), STR8("%r14b"),
	STR8("%r15"), STR8("%r15d"), STR8("%r15w"), STR8("%r15b"),
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// LexicalAnalyzer
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
	Token_FunctionArrow,                  // ->
	
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
	Token_ModuleDecl,                  // module
	Token_This,                        // this
	Token_Using,                       // using
	Token_As,                          // as
	Token_Operator,                    // operator
	
	//// types  ////
	TokenGroup_Type,
	Token_Void = TokenGroup_Type, // void
	//NOTE(sushi) the order of these entries matter, primarily for type conversion reasons. see SemanticAnalyzer::init
	Token_Unsigned8,              // u8
	Token_Unsigned16,             // u16
	Token_Unsigned32,             // u32 
	Token_Unsigned64,             // u64 
	Token_Signed8,                // s8
	Token_Signed16,               // s16 
	Token_Signed32,               // s32 
	Token_Signed64,               // s64
	Token_Float32,                // f32 
	Token_Float64,                // f64 
	Token_String,                 // str
	Token_Any,                    // any
	Token_Struct,                 // user defined type

	//aliases, plus one extra for indicating pointers
	DataType_Void       = Token_Void,
	DataType_Unsigned8  = Token_Unsigned8,
	DataType_Unsigned16 = Token_Unsigned16,
	DataType_Unsigned32 = Token_Unsigned32,
	DataType_Unsigned64 = Token_Unsigned64,
	DataType_Signed8    = Token_Signed8,
	DataType_Signed16   = Token_Signed16,
	DataType_Signed32   = Token_Signed32,
	DataType_Signed64   = Token_Signed64,
	DataType_Float32    = Token_Float32,
	DataType_Float64    = Token_Float64,
	DataType_String     = Token_String,
	DataType_Any        = Token_Any,
	DataType_Struct     = Token_Struct,
	DataType_Ptr,

	//// directives ////
	TokenGroup_Directive,
	Token_Directive_Import = TokenGroup_Directive,
	Token_Directive_Include,
	Token_Directive_Internal,
	Token_Directive_Run,

}; //typedef u32 Token_Type;

#define NAME(code) STRINGIZE(code)
const char* TokenTypes_Names[] = {
	NAME(Token_Null),
	NAME(Token_ERROR),
	NAME(Token_EOF),

	NAME(TokenGroup_Identifier),
	NAME(Token_Identifier),

	NAME(TokenGroup_Literal),
	NAME(Token_LiteralFloat),
	NAME(Token_LiteralInteger),
	NAME(Token_LiteralCharacter),
	NAME(Token_LiteralString),

	NAME(TokenGroup_Control),
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

	NAME(TokenGroup_Operator),
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

	NAME(TokenGroup_Keyword),
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
	NAME(Token_Using),
	NAME(Token_As),

	NAME(TokenGroup_Type),
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

	NAME(TokenGroup_Directive),
	NAME(Token_Directive_Import),
	NAME(Token_Directive_Include),
	NAME(Token_Directive_Internal),
	NAME(Token_Directive_Run),
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
	Expression_NULL,

	Expression_Identifier,
	
	Expression_IdentifierModule,
	Expression_IdentifierStruct,
	Expression_IdentifierVariable,
	Expression_IdentifierFunction,

	Expression_FunctionCall,
	
	//Special ternary conditional expression type
	Expression_TernaryConditional,

	Expression_InitializerList,
	
	//Types
	Expression_Literal,
	Expression_Type,
	
	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,
	Expression_IncrementPrefix,
	Expression_IncrementPostfix,
	Expression_DecrementPrefix,
	Expression_DecrementPostfix,
	Expression_Cast,
	//this is a special expression type that is only made when we are sure that we have the correct type information.
	//in Expression_Cast we must attach another expression to tell it what identifier to cast to, but in this case
	//we can just use the TypedValue information on Expression to figure out what type we are casting to.
	//there is a separation between these because in parsing it is possible that we come across a cast to a struct that has not
	//been parsed yet and so doesnt have a Struct associated with it, so we must indiciate what we are casting to through the id.
	Expression_CastImplicit, 
	Expression_Reinterpret,

	
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
	Expression_BinaryOpAs,
};

static str8 ExTypeStrings[] = {
	STR8("null"),//Expression_NULL,

	STR8("id"),//Expression_Identifier,

	STR8("module_id"), 
	STR8("struct_id"), 
	STR8("variable_id"), 
	STR8("function_id"), 
	
	STR8("call"),//Expression_FunctionCall,
	
	//Special ternary conditional expression type
	STR8("tern"),//Expression_TernaryConditional,

	STR8("initializer"),
	
	//Types
	STR8("literal"),//Expression_Literal,
	STR8("type"),//Expression_Type,
	
	//Unary Operators
	STR8("~"),//Expression_UnaryOpBitComp,
	STR8("!"),//Expression_UnaryOpLogiNOT,
	STR8("-"),//Expression_UnaryOpNegate,
	STR8("++"),//Expression_IncrementPrefix,
	STR8("++"),//Expression_IncrementPostfix,
	STR8("--"),//Expression_DecrementPrefix,
	STR8("--"),//Expression_DecrementPostfix,
	STR8("<>"),//Expression_Cast,
	STR8("<>i"),//Expression_CastImplicit, 
	STR8("<!>"),//Expression_Reinterpret,

	
	//Binary Operators
	STR8("+"),//Expression_BinaryOpPlus,
	STR8("-"),//Expression_BinaryOpMinus,
	STR8("*"),//Expression_BinaryOpMultiply,
	STR8("/"),//Expression_BinaryOpDivision,
	STR8("&&"),//Expression_BinaryOpAND,
	STR8("&"),//Expression_BinaryOpBitAND,
	STR8("||"),//Expression_BinaryOpOR,
	STR8("|"),//Expression_BinaryOpBitOR,
	STR8("<"),//Expression_BinaryOpLessThan,
	STR8(">"),//Expression_BinaryOpGreaterThan,
	STR8("<="),//Expression_BinaryOpLessThanOrEqual,
	STR8(">="),//Expression_BinaryOpGreaterThanOrEqual,
	STR8("=="),//Expression_BinaryOpEqual,
	STR8("!="),//Expression_BinaryOpNotEqual,
	STR8("%"),//Expression_BinaryOpModulo,
	STR8("^"),//Expression_BinaryOpBitXOR,
	STR8("<<"),//Expression_BinaryOpBitShiftLeft,
	STR8(">>"),//Expression_BinaryOpBitShiftRight,
	STR8("="),//Expression_BinaryOpAssignment,
	STR8("."),//Expression_BinaryOpMemberAccess,
	STR8("as"),//Expression_BinaryOpAs,
};

Type binop_token_to_expression(Type in){
	switch(in){
		case Token_Multiplication:     return Expression_BinaryOpMultiply;
		case Token_Division:           return Expression_BinaryOpDivision;
		case Token_Negation:           return Expression_BinaryOpMinus;
		case Token_Plus:               return Expression_BinaryOpPlus;
		case Token_AND:                return Expression_BinaryOpAND;
		case Token_OR:                 return Expression_BinaryOpOR;
		case Token_LessThan:           return Expression_BinaryOpLessThan;
		case Token_GreaterThan:        return Expression_BinaryOpGreaterThan;
		case Token_LessThanOrEqual:    return Expression_BinaryOpLessThanOrEqual;
		case Token_GreaterThanOrEqual: return Expression_BinaryOpGreaterThanOrEqual;
		case Token_Equal:              return Expression_BinaryOpEqual;
		case Token_NotEqual:           return Expression_BinaryOpNotEqual;
		case Token_BitAND:             return Expression_BinaryOpBitAND;
		case Token_BitOR:              return Expression_BinaryOpBitOR;
		case Token_BitXOR:             return Expression_BinaryOpBitXOR;
		case Token_BitShiftLeft:       return Expression_BinaryOpBitShiftLeft;
		case Token_BitShiftRight:      return Expression_BinaryOpBitShiftRight;
		case Token_Modulo:             return Expression_BinaryOpModulo;
		case Token_BitNOT:             return Expression_UnaryOpBitComp;
		case Token_LogicalNOT:         return Expression_UnaryOpLogiNOT;
	}
	return Expression_NULL;
}

//held by anything that can represent a value at compile time, currently variables and expressions
//this helps with doing compile time evaluations between variables and expressions
struct Struct;
struct Declaration;
struct TypedValue{
	Struct* structure;
	u32 pointer_depth; // a value of -1 indicates that this value is a placeholder
	b32 implicit = 0; //special indicator for variables that were declared with no type and depend on the expression they are assigned
	str8 type_name;
	union {
		f32  float32;
		f64  float64;
		s8   int8;
		s16  int16;
		s32  int32;
		s64  int64;
		u8   uint8;
		u16  uint16;
		u32  uint32;
		u64  uint64;
		str8 string;
	};
};

struct Expression {
	amuNode node;
	Token* token_start;
	Token* token_end;
	
	Type type;

	TypedValue data;
};

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
	Statement_Block,
	Statement_Using,
	Statement_Import,
};

struct Statement {
	amuNode node;
	Token* token_start;
	Token* token_end;
	
	Type type;

	LabelTable ltable;
};

struct Scope {
	amuNode node;
	Token* token_start;
	Token* token_end;
	
	b32 has_return_statement;
};

struct Module;
struct Struct;
struct Variable;
struct Function;
struct SyntaxAnalyzerThread;
struct Entity;

// the handle of anything in amu
struct Label {
	amuNode node;
	str8 identifier;
	u64 identifier_hash;

	Entity* entity; // entity this label belongs to 
	Label* original; // if this label is an alias of another label, we point to it with this
};

enum{
	Entity_Unknown,
	Entity_Variable,
	Entity_Function,
	Entity_Structure,
	Entity_Module,
	Entity_Loop,
};

// represents anything that may have a label in amu
struct Entity {
	amuNode node;
	Type type;

	Label* label; // the label for this entity. 0 indicates an anonymous entity.
 
	str8 internal_label; // the label of this entity that is distinct from the label assigned to it.

	Token* token_start;
	Token* token_end;
};

struct Function {
	Entity entity;
	//an array of overloads
	//this is only used on the base Function that represents overloads
	amuArena<Function*> overloads;

	// list of arguments this function takes
	amuArena<Variable*> args;
	u32 default_count; // how many of these arguments have default values

	TypedValue data;

	//this label is how a function is internally referred to in the case of overloading
	//it is where name mangling happens
	//format:
	//    func_name@argtype1,argtype2,...@rettype1,rettype2,...
	str8 internal_label;

	//set when the function is to be inlined 
	b32 inlined;
};

struct Variable{
	Entity entity; 	

	u32 initialized; // set true when this variable is given a value where it is declared

	//number of times * appears on a variable's type specifier
	u32 pointer_depth;

	TypedValue data;
};

struct Struct {
	Entity entity;
	u64 size; //size of struct in bytes

	//set to 0 if this is a user defined struct
	//otherwise this struct is something that is built in
	//and is indiciated by the DataType_* types
	Type type;

	
	nodemap members;

	//map of conversions defined for this structure
	//the key is the name of the declaration it is a conversion to
	nodemap conversions;

	nodemap operators;
};

struct Module{
	Entity entity;
	// in order to nicely support overloading, we store a main function node
	// representing all of the functions here. the main function is just the 
	// first one we come across.
	nodemap functions;
};

struct Program {
	Node node;
	str8 filename;
	//TODO add entrypoint string
};

enum {
	psFile,
	psDirective,
	psLabel,
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
	psAccess, 
	psFactor,
	psInitializer,
	psType,
};

enum{
	FileStage_Null,
	FileStage_Lexer,
	FileStage_Preprocessor,
	FileStage_Parser,
	FileStage_Validator,
	FileStage_ERROR, // set when the file has errored in some stage and cannot continue
};

struct Token;
struct amuFile;
struct amuMessage{
	u32 verbosity;
	Token* token;
	Type type; //log, error, or warn	
	u32 indent;
	f64 time_made;
	str8 prefix;
	amuFile* amufile;
	arrayT<str8> message_parts;
};

struct amuLogger{
	// instead of immediatly logging messages we collect them to format and display later
	// we do this to prevent threads' messages from overlapping each other, to prevent
	// actually printing anything during compiling, and so we can do much nicer formatting later on
	// this behavoir is disabled by globals.log_immediatly
	//TODO(sushi) implement an arena instead of array so that we know this will be thread safe
	arrayT<amuMessage> messages;

	amuFile* amufile = 0;
	str8 owner_str_if_sufile_is_0 = {0,0};
	
	template<typename...T>
	void log(u32 verbosity, T... args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.verbosity < verbosity) return;
		if(globals.log_immediatly){
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, (amufile ? amufile->file->name : owner_str_if_sufile_is_0), VTS_Default, ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, t->file,  VTS_Default, "(",t->l0,",",t->c0,"): ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, token->file, VTS_Default, "(",token->l0,",",token->c0,"): ", ErrorFormat("error"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, (amufile ? amufile->file->name : owner_str_if_sufile_is_0), VTS_Default, ": ", ErrorFormat("error"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, token->file, VTS_Default, "(",token->l0,",",token->c0,"): ", WarningFormat("warning"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, (amufile ? amufile->file->name : owner_str_if_sufile_is_0), VTS_Default, ": ", WarningFormat("warning"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
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
	void note(Token* token, T...args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.log_immediatly){
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, token->file, VTS_Default, "(",token->l0,",",token->c0,"): ", MagentaFormat("note"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Note;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}
	
	template<typename...T>
	void note(T...args){DPZoneScoped;
		if(globals.supress_messages) return;
		if(globals.log_immediatly){
			mutex_lock(&compiler.mutexes.log);
			str8 out = to_str8_amu(VTS_CyanFg, (amufile ? amufile->file->name : owner_str_if_sufile_is_0), ": ", MagentaFormat("note"), ": ", args...);
			Log("", out);
			mutex_unlock(&compiler.mutexes.log);
		}else{
			amuMessage message;
			message.time_made = peek_stopwatch(compiler.ctime);
			message.type = Message_Note;
			constexpr auto arg_count{sizeof...(T)};
			str8 arr[arg_count] = {to_str8(args, deshi_allocator)...};
			message.message_parts.resize(arg_count);
			memcpy(message.message_parts.data, arr, sizeof(str8)*arg_count);
			messages.add(message);
		}
	}
};

struct amuFile{
	File* file;
	str8 file_buffer;
	Type stage;
	//set true if a compiler thread is started for this file
	//to prevent multiple compiler threads from starting on the same file
	b32 being_processed = 0;

	amuLogger logger;

	//for files to wait on other files to finish certain stages
	//NOTE(sushi) this may only be necessary for validator
	struct{
		condvar lex;
		condvar preprocess;
		condvar parse;
		condvar validate;
	}cv;

	struct{ // lexer
		amuArena<Token> tokens;
		amuArena<u32> declarations; // list of : tokens
		amuArena<u32> imports;      // list of import tokens
		amuArena<u32> internals;    // list of internal tokens
		amuArena<u32> runs;         // list of run tokens
		
		amuArena<u32> labels; // list of : tokens

		b32 failed;
	}lexical_analyzer;

	struct{ // preprocessor
		amuArena<amuFile*> imported_files;
		// these declarations are tokens that the preprocessor THINKS should be either internal or exported,
		// but it could be wrong. this is resolved by the syntax_analyzer.
		amuArena<u32> exported_decl;
		amuArena<u32> internal_decl;
		amuArena<u32> runs;

		b32 failed;
	}preprocessor;

	struct{ // syntax_analyzer
		//arrays organizing toplevel declarations and import directives
		amuArena<Statement*> import_directives;
		//TODO(sushi) it may not be necessary to have exported and imported in separate arrays anymore
		amuArena<Declaration*> exported_decl;
		amuArena<Declaration*> imported_decl;
		amuArena<Declaration*> internal_decl;

		b32 failed;
	}syntax_analyzer;
		

	struct{
		// TODO(sushi) remove this struct if no other vars are added to it.
		b32 failed;
	}semantic_analyzer;

	// the module representing this file. this is where the AST of the file lives 
	Module* module;
	LabelTable* table_root;

	void init(){
		lexical_analyzer.tokens.init();
		lexical_analyzer.declarations.init();
		lexical_analyzer.imports.init();
		lexical_analyzer.internals.init();
		lexical_analyzer.labels.init();
		lexical_analyzer.runs.init();
		preprocessor.imported_files.init();
		preprocessor.exported_decl.init();
		preprocessor.internal_decl.init();
		preprocessor.runs.init();
		syntax_analyzer.import_directives.init();
		syntax_analyzer.exported_decl.init();
		syntax_analyzer.imported_decl.init();
		syntax_analyzer.internal_decl.init();
		cv.lex = condition_variable_init();
		cv.preprocess = condition_variable_init();
		cv.parse = condition_variable_init();
		cv.validate = condition_variable_init();
	}
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// LexicalAnalyzer

struct LexicalAnalyzer {
	amuFile* amufile;
	void lex();
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Preprocessor

struct Preprocessor {
	amuFile* amufile;
	void preprocess();
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// SyntaxAnalyzer

struct SyntaxAnalyzer;
struct SyntaxAnalyzerThread{
	amuFile* amufile;
	
	SyntaxAnalyzer* syntax_analyzer;
	Type stage; //entry stage

	amuNode* node;
	Token* curt;

	// to give better error messages, we store a stack of labels so that we may refer back to them 
	amuArena<Label*> label_stack;
	Label* current_label;

	LabelTable* current_table;

	b32 is_internal;
	b32 parsing_func_args = 0;

	b32 finished=0;
	condvar cv;
	
	Declaration* declare();
	amuNode* define(amuNode* node, Type stage);

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

	template<typename ...args> FORCE_INLINE b32
	prev_match(args... in){DPZoneScoped;
		return (((curt-1)->type == in) || ...);
	}

	template<typename ...args> FORCE_INLINE b32
	prev_match_group(args... in){DPZoneScoped;
		return (((curt-1)->group == in) || ...);
	}

	template<typename... T>
	amuNode* binop_parse(amuNode* node, amuNode* ret, Type next_stage, T... tokchecks);
};

struct SyntaxAnalyzer {
	amuArena<SyntaxAnalyzerThread> threads;

	//file the syntax_analyzer is working in
	amuFile* amufile;

	void analyze();
};

void semantic_analyzer_threaded_stub(void* pthreadinfo){DPZoneScoped;
	SetThreadName("syntax_analyzer thread started.");
	SyntaxAnalyzerThread* pt = (SyntaxAnalyzerThread*)pthreadinfo;
	pt->amufile = pt->syntax_analyzer->amufile;
	pt->define(pt->node, pt->stage);
	pt->finished = 1;
	condition_variable_notify_all(&pt->cv);
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// SemanticAnalyzer

struct SemanticAnalyzer{
	amuFile* amufile;

	//keeps track of what element we are currently working in
	struct{
		Variable*   variable = 0;
		Expression* expression = 0;
		Struct*     structure = 0;
		Scope*      scope = 0;
		Function*   function = 0;
		Statement*  statement = 0;
		Module*     module = 0;
	}current;

	struct{
		//stacks of declarations known in current scope
		//this array stores the index of the last declaration that was pushed before a new scope begins
		amuArena<u32> known_declarations_scope_begin_offsets;
		amuArena<Declaration*> known_declarations;
		//stacks of elements that we are working with
		struct{
			amuArena<Scope*>      scopes;
			amuArena<Struct*>     structs;
			amuArena<Variable*>   variables;
			amuArena<Function*>   functions;
			amuArena<Expression*> expressions;
			amuArena<Statement*>  statements;
			amuArena<Module*>     modules;
		}nested;
	}stacks;

	LabelTable* current_table;

	void init(){DPZoneScoped;
		stacks.nested.scopes.init();
		stacks.nested.structs.init();
		stacks.nested.variables.init();
		stacks.nested.functions.init();
		stacks.nested.expressions.init();
		stacks.nested.statements.init();
		stacks.nested.modules.init();
		stacks.known_declarations.init();
		stacks.known_declarations_scope_begin_offsets.init();
	}

	//starts the validation stage
	void         start();
	//recursive validator function
	amuNode*     validate(amuNode* node);
	//checks if a variable is going to conflict with another variable in its scope
	//return false if the variable conflicts with another
	b32          check_shadowing(Declaration* d);
	//finds a declaration by iterating the known declarations array backwards
	Declaration* find_decl(str8 id);
	Declaration* find_typename(str8 id);
	//checks if a conversion between 2 types is possible
	b32 can_type_convert(TypedValue* tv0, TypedValue* tv1);
	void pop_known_decls();
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Compiler

struct CompilerRequest{
	Type stage;
	arrayT<str8> filepaths;
};

struct CompilerReport{
	b32 failed;

	amuArena<amuFile*> units; // array of amuFiles created by this compile request
};

struct  Compiler{
	Stopwatch ctime;

	amuLogger logger;

	b32 failed;

	str8 working_dir = STR8("tests/_/");

	//TODO(sushi) we probably want to just chunk arenas for suFiles instead of randomly allocating them.
	map<str8, amuFile*> files;
	
	struct{
		//we represent builtin scalars as structs internally because it is possible for the user
		//to define conversions between them and other structs and it makes some things easier to implement
		union{
			Struct* arr[14];
			struct{
				Struct* void_;
				Struct* unsigned8;
				Struct* unsigned16;
				Struct* unsigned32;
				Struct* unsigned64;
				Struct* signed8;
				Struct* signed16;
				Struct* signed32;
				Struct* signed64;
				Struct* float32;
				Struct* float64;
				Struct* ptr;
				Struct* any;
				Struct* str;
			};
		}types;
		union{
			Label* arr[14];
			struct{
				Label* void_;
				Label* unsigned8;
				Label* unsigned16;
				Label* unsigned32;
				Label* unsigned64;
				Label* signed8;
				Label* signed16;
				Label* signed32;
				Label* signed64;
				Label* float32;
				Label* float64;
				Label* ptr;
				Label* any;
				Label* str;
			};
		}labels;
	}builtin;

	//locked when doing non-thread safe stuff 
	//such as loading a File, and probably when we use memory functions as well
	struct{
		mutex lexer;
		mutex preprocessor;
		mutex syntax_analyzer;
		mutex log; //lock when using logger
		mutex compile_request;
	}mutexes;

	CompilerReport compile(CompilerRequest* request, b32 wait = 1);

	amuFile* start_lexer       (amuFile* amufile);
	amuFile* start_preprocessor(amuFile* amufile);
	amuFile* start_parser      (amuFile* amufile);
	amuFile* start_validator   (amuFile* amufile);

	void init();

	//used to completely reset all compiled information
	//this is mainly for performance testing, like running a compile on the same file
	//repeatedly to get an average time
	//or to test for memory leaks
	void reset(); 

	Compiler(){}

}compiler;

struct CompilerThread{
	str8 filepath;
	Type stage;
	condvar wait;
	b32 finished;
	amuFile* amufile;
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Helpers

str8 type_token_to_str(Type type){
    switch(type){
        case Token_Void:       return STR8("void");
        case Token_Signed8:    return STR8("s8");
        case Token_Signed16:   return STR8("s16");
        case Token_Signed32:   return STR8("s32");
        case Token_Signed64:   return STR8("s64");
        case Token_Unsigned8:  return STR8("u8");
        case Token_Unsigned16: return STR8("u16");
        case Token_Unsigned32: return STR8("u32");
        case Token_Unsigned64: return STR8("u64");
        case Token_Float32:    return STR8("f32");
        case Token_Float64:    return STR8("f64");
        case Token_String:     return STR8("str");
        case Token_Any:        return STR8("any");
        case Token_Struct:     return STR8("struct-type");
    }
    return STR8("UNKNOWN DATA TYPE");
}

str8 get_typename(Variable* v){ 
	if(v->data.structure->entity.label) return v->data.structure->entity.label->identifier;
	return v->data.structure->entity.internal_label;
}
str8 get_typename(Expression* e){ 
	if(e->data.structure->entity.label) return e->data.structure->entity.label->identifier;
	return e->data.structure->entity.internal_label; 
}
str8 get_typename(Struct* s){ 
	if(s->entity.label) return s->entity.label->identifier;
	return s->entity.internal_label;
}
str8 get_typename(Function* f){ 
	if(f->data.structure->entity.label) return f->data.structure->entity.label->identifier;
	return f->data.structure->entity.internal_label; 
}

// reimplement this if it ever is needed
// str8 get_typename(amuNode* n){
// 	switch(n->type){
// 		case NodeType_Variable:   return get_typename((Variable*)n);
// 		case NodeType_Expression: return get_typename((Expression*)n);
// 		case NodeType_Structure:  return get_typename((Struct*)n);
// 		default: Assert(false,"Invalid node type given to get_typename()"); return {0};
// 	}
// 	return {0};
// }

u64 builtin_sizes(Type type){
	switch(type){
		case Token_Unsigned8:  return sizeof(u8);
		case Token_Unsigned16: return sizeof(u16);
		case Token_Unsigned32: return sizeof(u32);
		case Token_Unsigned64: return sizeof(u64);
		case Token_Signed8:    return sizeof(s8);
		case Token_Signed16:   return sizeof(s16);
		case Token_Signed32:   return sizeof(s32);
		case Token_Signed64:   return sizeof(s64);
		case Token_Float32:    return sizeof(f32);
		case Token_Float64:    return sizeof(f64);
		case Token_String:     return sizeof(void*) + sizeof(u64);
		case Token_Any:        return sizeof(void*);
	}
	return -1;
}

Struct* builtin_from_type(Type type){
	switch(type){
		case DataType_Void:       return compiler.builtin.types.void_;
		case DataType_Unsigned8:  return compiler.builtin.types.unsigned8;
		case DataType_Unsigned16: return compiler.builtin.types.unsigned16;
		case DataType_Unsigned32: return compiler.builtin.types.unsigned32;
		case DataType_Unsigned64: return compiler.builtin.types.unsigned64;
		case DataType_Signed8:    return compiler.builtin.types.signed8;
		case DataType_Signed16:   return compiler.builtin.types.signed16;
		case DataType_Signed32:   return compiler.builtin.types.signed32;
		case DataType_Signed64:   return compiler.builtin.types.signed64;
		case DataType_Float32:    return compiler.builtin.types.float32;
		case DataType_Float64:    return compiler.builtin.types.float64;
		case DataType_Ptr:        return compiler.builtin.types.ptr;
		case DataType_Any:        return compiler.builtin.types.any;
		case DataType_String:     return compiler.builtin.types.str;
	}
	Assert(false);
	return 0;
}

b32 is_builtin_type(Expression* e){ return e->data.structure->type; }
b32 is_builtin_type(Variable* v){ return v->data.structure->type; }
b32 is_builtin_type(Struct* s){ return s->type; }

b32 is_float(Expression* e){ return e->data.structure->type == DataType_Float32 || e->data.structure->type == DataType_Float64; }
b32 is_float(Variable* v){ return v->data.structure->type == DataType_Float32 || v->data.structure->type == DataType_Float64; }
b32 is_float(Struct* s){ return s->type == DataType_Float32 || s->type == DataType_Float64; }
b32 is_int(Expression* e){ return e->data.structure->type >= DataType_Unsigned8 && e->data.structure->type <= DataType_Signed64; }
b32 is_int(Variable* v){ return v->data.structure->type >= DataType_Unsigned8 && v->data.structure->type <= DataType_Signed64; }
b32 is_int(Struct* s){ return s->type >= DataType_Unsigned8 && s->type <= DataType_Signed64; }
//b32 is_scalar(Expression* e){ return e->data.structure->type < }


//laziness
FORCE_INLINE b32 types_match(Variable* v0,   Variable* v1)   { return v0->data.structure == v1->data.structure; }
FORCE_INLINE b32 types_match(Variable* v,    Expression* e)  { return v->data.structure == e->data.structure; }
FORCE_INLINE b32 types_match(Variable* v,    Struct* s)      { return v->data.structure == s; }
FORCE_INLINE b32 types_match(Expression* e0, Expression* e1) { return e0->data.structure == e1->data.structure; }
FORCE_INLINE b32 types_match(Expression* e,  Variable* v)    { return e->data.structure == v->data.structure; }
FORCE_INLINE b32 types_match(Expression* e,  Struct* s)      { return e->data.structure == s; }
FORCE_INLINE b32 types_match(Struct* s0,     Struct* s1)     { return s0 == s1; }
FORCE_INLINE b32 types_match(Struct* s,      Variable* v)    { return v->data.structure == s; }
FORCE_INLINE b32 types_match(Struct* s,      Expression* e)  { return e->data.structure == s; }

b32 is_expression_type(amuNode* n, Type type){
	if(n->type != NodeType_Expression) return false;
	if(((Expression*)n)->type == type) return true;
	return false;
}

//allocates a string into temp memory
str8 show(Function* f, b32 display_var_names = 1){
	Assert(!f->overloads.count, "gen_sig_func was passed an overload base");

	// TODO(sushi) setup showing anonymous functions
	if(!f->entity.label){
		FixMe; 
	}

	str8b b; str8_builder_init(&b, STR8(""), deshi_temp_allocator);
	str8_builder_append(&b, f->entity.label->identifier);
	str8_builder_append(&b, STR8("("));
	forI(f->args.count){
		Variable* arg = f->args[i];
		if(display_var_names){
			str8_builder_append(&b, arg->entity.label->identifier);
			str8_builder_append(&b, STR8(":"));
		}
		str8_builder_append(&b, get_typename(arg));
		if(i != f->args.count-1){
			str8_builder_append(&b, STR8(","));
		}
	}

	str8_builder_append(&b, STR8("):"));
	str8_builder_append(&b, get_typename(f));

	return b.fin;
}

str8 show(Expression* e){
	str8 out = e->token_start->raw;
	out.count = (e->token_end->raw.str - e->token_start->raw.str) + 1;
	return out;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory

struct{
	amuChunkedArena<Function>   functions;
	amuChunkedArena<Variable>   variables;
	amuChunkedArena<Struct>     structs;
	amuChunkedArena<Scope>      scopes;
	amuChunkedArena<Expression> expressions;
	amuChunkedArena<Statement>  statements;
	amuChunkedArena<Module>     modules;
	amuChunkedArena<Label>      labels;
	amuChunkedArena<LabelTable> label_tables;

	//TODO(sushi) bypass debug message assignment in release build
	FORCE_INLINE
	Function* make_function(str8 debugmsg = STR8("")){DPZoneScoped;
		Function* function = functions.add(Function());
		//compiler.logger.log(Verbosity_Debug, "Making a function with debug message ", debugmsg);
		function->entity.node.lock = mutex_init();
		function->entity.node.type = NodeType_Function;
		function->entity.node.debug = debugmsg;
		return function;
	}

	FORCE_INLINE
	Variable* make_variable(str8 debugmsg = STR8("")){DPZoneScoped;
		Variable* variable = variables.add(Variable());
		//compiler.logger.log(Verbosity_Debug, "Making a variable with debug message ", debugmsg);
		variable->entity.node.lock = mutex_init();
		variable->entity.node.type = NodeType_Variable;
		variable->entity.node.debug = debugmsg;
		return variable;
	}

	FORCE_INLINE
	Struct* make_struct(str8 debugmsg = STR8("")){DPZoneScoped;
		Struct* structure = structs.add(Struct());
		//compiler.logger.log(Verbosity_Debug, "Making a structure with debug message ", debugmsg);
		structure->entity.node.lock = mutex_init();
		structure->entity.node.type = NodeType_Structure;
		structure->entity.node.debug = debugmsg;
		structure->members.init();
		structure->conversions.init();
		return structure;
	}

	FORCE_INLINE
	Scope* make_scope(str8 debugmsg = STR8("")){DPZoneScoped;
		Scope* scope = scopes.add(Scope());
		//compiler.logger.log(Verbosity_Debug, "Making a scope with debug message ", debugmsg);
		scope->node.lock = mutex_init();
		scope->node.type = NodeType_Scope;
		scope->node.debug = debugmsg;
		return scope;
	}

	FORCE_INLINE
	Expression* make_expression(str8 debugmsg = STR8("")){DPZoneScoped;
		Expression* expression = expressions.add(Expression());
		//compiler.logger.log(Verbosity_Debug, "Making an expression with debug message ", debugmsg);
		expression->node.lock = mutex_init();
		expression->node.type = NodeType_Expression;
		expression->node.debug = debugmsg;
		return expression;
	}

	FORCE_INLINE
	Statement* make_statement(str8 debugmsg = STR8("")){DPZoneScoped;
		Statement* statement = statements.add(Statement());
		//compiler.logger.log(Verbosity_Debug, "Making a statement with debug message ", debugmsg);
		statement->node.lock = mutex_init();
		statement->node.type = NodeType_Statement;
		statement->node.debug = debugmsg;
		return statement;
	}

	FORCE_INLINE
	Module* make_module(str8 debugmsg = STR8("")){DPZoneScoped;
		Module* module = modules.add(Module());
		//compiler.logger.log(Verbosity_Debug, "Making a ns with debug message ", debugmsg);
		module->entity.node.lock = mutex_init();
		module->entity.node.type = NodeType_Module;
		module->entity.node.debug = debugmsg;
		return module;
	}

	FORCE_INLINE
	Label* make_label(str8 debugmsg = STR8("")){DPZoneScoped;
		Label* label = labels.add(Label());
		//compiler.logger.log(Verbosity_Debug, "Making a ns with debug message ", debugmsg);
		label->node.lock = mutex_init();
		label->node.type = NodeType_Label;
		label->node.debug = debugmsg;
		return label;
	}

	LabelTable* make_label_table(){DPZoneScoped;
		LabelTable* label_table = label_tables.add(LabelTable());
		label_table->set.init();
		return label_table;
	}

	FORCE_INLINE
	void init(){DPZoneScoped;
		functions.init(256);   
		variables.init(256);   
		structs.init(256);     
		scopes.init(256);      
		expressions.init(256); 
		statements.init(256);
		modules.init(256);
		labels.init(256);
	}

}arena;

// copies a node and all of its children recursively
// TODO(sushi) it's VERY possible something goes wrong here. several of our structures contain data
//             that are mutable. I do not want to do deep copies at the moment, but it may be necesary
// NOTE(sushi) due to the aforementioned issue, this should only be used where you know data on the copied
//             branch is not going to be modified. 
amuNode* copy_branch(amuNode* node){
	amuNode* out = 0;
	switch(node->type){
		case NodeType_Program:{
			TestMe;
		}break;
		case NodeType_Structure:{
			Struct* s = arena.make_struct(node->debug);
			CopyMemory((u8*)s+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Struct)-sizeof(amuNode));
			out = (amuNode*)s;
		}break;
		case NodeType_Function:{
			Function* f = arena.make_function(node->debug);
			CopyMemory((u8*)f+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Function)-sizeof(amuNode));
			out = (amuNode*)f;
		}break;
		case NodeType_Variable:{
			Variable* v = arena.make_variable(node->debug);
			CopyMemory((u8*)v+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Variable)-sizeof(amuNode));
			out = (amuNode*)v;
		}break;
		case NodeType_Scope:{
			Scope* s = arena.make_scope(node->debug);
			CopyMemory((u8*)s+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Scope)-sizeof(amuNode));
			out = (amuNode*)s;
		}break;
		case NodeType_Statement:{
			Statement* s = arena.make_statement(node->debug);
			CopyMemory((u8*)s+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Scope)-sizeof(amuNode));
			out = (amuNode*)s;
		}break;
		case NodeType_Expression:{
			Expression* e = arena.make_expression(node->debug);
			CopyMemory((u8*)e+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Expression)-sizeof(amuNode));
			out = (amuNode*)e;
		}break;
		case NodeType_Module:{
			Module* m = arena.make_module(node->debug);
			CopyMemory((u8*)m+sizeof(amuNode), (u8*)node+sizeof(amuNode), sizeof(Module)-sizeof(amuNode));
			out = (amuNode*)m;
		}break;
		default: FixMe;
	}
	for_node(node->first_child){
		insert_last(out, copy_branch(it));
	}
	return out;
}


LabelTable label_table_init(){DPZoneScoped;
	LabelTable out;
	out.set.init(256);
	return out;
}

pair<spt,b32> label_table_find(LabelTable* table, u64 key){
	spt index = -1;
	spt middle = -1;
	if(table->set.count){
		spt left = 0;
		spt right = table->set.count-1;
		while(left <= right){
			middle = left+((right-left)/2);
			if(table->set[middle]->identifier_hash == key){
				index = middle;
				break;
			}
			if(table->set[middle]->identifier_hash < key){
				left = middle+1;
				middle = left+((right-left)/2);
			}else{
				right = middle-1;
			}
		}
	}
	return {middle, index == -1};
}

pair<spt,b32> label_table_find(LabelTable* table, str8 key){
	return label_table_find(table, str8_hash64(key));
}

pair<spt,b32> label_table_find(LabelTable* table, Label* label){
	return label_table_find(table, label->identifier_hash);
}

spt label_table_add(LabelTable* table, Label* label){
	auto [index, found] = label_table_find(table, label->identifier_hash);
	if(!found) table->set.insert(index, label);
	return index;
}

b32 label_table_has(LabelTable* table, Label* label){
	return label_table_find(table, label).second;
}

Label* label_table_at(LabelTable* table, str8 id){
	auto [index, found] = label_table_find(table, id);
	if(!found) return 0;
	return table->set[index];
}

Label* label_table_at(LabelTable* table, u32 idx){
	Assert(idx < table->set.count);
	return table->set[idx];
}

#endif //AMU_TYPES_H