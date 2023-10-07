#include "Common.h"
#include "util.h"
#include "basic/Memory.h"
#include "storage/Array.h"
#include "signal.h"

#include "basic/Memory.cpp"
#include "storage/Array.cpp"

using namespace amu;

// gives the bottom of a stack we allocate for a context
// 7 pointers because we save the  general registers 
// and we need to store the return address
#define STACK_BOTTOM(ptr,sz) ((ptr)+(sz)-7*sizeof(void*))

__attribute__((section(".text")))
static u8 get_context_code_x86_64_sysv[] = {
	0x4c,0x8b,0x04,0x24,      // mov    r8,QWORD PTR [rsp]
	0x4c,0x89,0x07,           // mov    QWORD PTR [rdi],r8
	0x4c,0x8b,0x44,0x24,0x08, // mov    r8,QWORD PTR [rsp+0x8]
	0x4c,0x89,0x47,0x08,      // mov    QWORD PTR [rdi+0x8],r8
	0x48,0x89,0x5f,0x10,      // mov    QWORD PTR [rdi+0x10],rbx
	0x48,0x89,0x6f,0x18,      // mov    QWORD PTR [rdi+0x18],rbp
	0x4c,0x89,0x67,0x20,      // mov    QWORD PTR [rdi+0x20],r12
	0x4c,0x89,0x6f,0x28,      // mov    QWORD PTR [rdi+0x28],r13
	0x4c,0x89,0x77,0x30,      // mov    QWORD PTR [rdi+0x30],r14
	0x4c,0x89,0x7f,0x38,      // mov    QWORD PTR [rdi+0x38],r15
	0x31,0xc0,                // xor    eax,eax
	0xc3,                     // ret
};

__attribute__((section(".text")))
static u8 end_context_code_x86_64_sysv[] = {
	0x5c,      // pop    rsp
	0x41,0x5f, // pop    r15
	0x41,0x5e, // pop    r14
	0x41,0x5d, // pop    r13
	0x41,0x5c, // pop    r12
	0x5d,      // pop    rbp
	0x5b,      // pop    rbx
	0xc3,      // ret
};

__attribute__((section(".text")))
static u8 swap_context_code_x86_64_sysv[] = {
	0x4c,0x8b,0x04,0x24,           // mov    r8,QWORD PTR [rsp]
	0x41,0x50,                     // push   r8
	0x53,                          // push   rbx
	0x55,                          // push   rbp
	0x41,0x54,                     // push   r12
	0x41,0x55,                     // push   r13
	0x41,0x56,                     // push   r14
	0x41,0x57,                     // push   r15
	0x48,0x89,0xf2,                // mov    rdx,rsi
	0x48,0x89,0xe0,                // mov    rax,rsp
	0x48,0x89,0xfc,                // mov    rsp,rdi
	0x41,0x5f,                     // pop    r15
	0x41,0x5e,                     // pop    r14
	0x41,0x5d,                     // pop    r13
	0x41,0x5c,                     // pop    r12
	0x5d,                          // pop    rbp
	0x5b,                          // pop    rbx
	0x41,0x59,                     // pop    r9
	0x41,0x50,                     // push   r8
	0xff,0x35,0x03,0x00,0x00,0x00, // push   QWORD PTR [rip+0x3]        # 67 <exit>
	0x41,0xff,0xe1,                // jmp    r9
};
__attribute__((section(".text")))
static u8 exit_code_x86_64_sysv[] = {
	0x5c,      // pop    rsp
	0x41,0x5f, // pop    r15
	0x41,0x5e, // pop    r14
	0x41,0x5d, // pop    r13
	0x41,0x5c, // pop    r12
	0x5d,      // pop    rbp
	0x5b,      // pop    rbx
	0xc3,      // ret
};

__attribute__((section(".text")))
static u8 execute_into_code_x86_64_sysv[] = {
	0x53,           // push   rbx
	0x55,           // push   rbp
	0x41,0x54,      // push   r12
	0x41,0x55,      // push   r13
	0x41,0x56,      // push   r14
	0x41,0x57,      // push   r15
	0x48,0x89,0xe3, // mov    rbx,rsp
	0x48,0x89,0xf4, // mov    rsp,rsi
	0x48,0x89,0xde, // mov    rsi,rbx
	0x41,0x5f,      // pop    r15
	0x41,0x5e,      // pop    r14
	0x41,0x5d,      // pop    r13
	0x41,0x5c,      // pop    r12
	0x5d,           // pop    rbp
	0x5b,           // pop    rbx
	0xff,0x22,      // jmp    QWORD PTR [rdx]
};

__attribute__((section(".text")))
static u8 get_ip_code_x86_64_sysv[] = {
	0x48,0x8b,0x04,0x24, // mov    rax,QWORD PTR [rsp]
	0xc3,                // ret
};

struct swap_p {
	void* sp;
	void* parm;
};

struct ipsp {
	void* ip;
	void* sp;
};

struct continuation {
	void* stack_pointer;
	void* parameters;

};

struct trampoline_args {
	void* f;
	void* sp;
};

//static void (*get_context)(void*) = (void (*)(void*))get_context_code_x86_64_sysv;
//static swap_p (*swap_context)(void*, void*) = (swap_p (*)(void*, void*))swap_context_code_x86_64_sysv;
//static swap_p (*execute_into)(void*, void*, void*) = (swap_p (*)(void*, void*, void*))execute_into_code_x86_64_sysv;
//static void* (*get_ip)() = (void* (*)())get_ip_code_x86_64_sysv;

extern "C" swap_p swap_context(void*,void*);

struct ctx {
	void* rip,* rsp;
	void* rbx,* rbp,* r12,* r13,* r14,* r15;
};


struct scheduler {
	Array<continuation> queue;
};

scheduler gscheduler;

// yield to the given continuation
void 
yield(continuation c) {
	
}

continuation
new_continuation(void* f) {
	auto stack = (u8*)malloc(4096);
	auto bottom = STACK_BOTTOM(stack, 4096);
	*((void**)(bottom - 1)) = f;
	continuation out;
	out.stack_pointer = bottom;
	out.parameters = 0;
	return out;
}

void*
make_context(void* f) {
	auto top = (u8*)malloc(4096);
	auto bottom = STACK_BOTTOM(top, 4096);
	bottom = (u8*)((upt)bottom & -16L);
	bottom -= 128;
	bottom -= 7*sizeof(void*);
	*((void**)bottom + 6) = f;
	return bottom;
}

void hello() {
	printf("hello\n");
}

template<typename F, typename... Args>
continuation callcc(F f, Args&&... args) {
	auto c = new_continuation((void*)f);
}

int main() {
	auto c = make_context((void*)hello);
	
	auto s = swap_context(c, 0);

	printf("hi!\n");

	return 0;
}
