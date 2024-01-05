/*

    Three address code (TAC) representation of amu. This is the second to last representation of amu before
    reaching amu's intermediate representation (AIR). 

    The main information of TAC is a single operation and two or less operands. Operands are represented as
    Arg, which may reference an existing Var, Function, another TAC, a literal, or a register offset.

    The primary use of TAC is optimization. We optimize TAC over AIR because TAC does not yet deal with 
    the stack, constant offsets, and other hardcoded information that AIR includes. The only information
    TAC stores related to memory is how much space temporaries will take up (in units of sizeof(Register), for now)

    TAC is setup so that it can be freely reordered, unlike AIR. When you add, remove, or move TAC, other
    TAC in the execution sequence do not have to be adjusted with it. 

    TAC stores a lot of extra information meant to support debugging it and generating AIR, while AIR
    attempts to store as little as possible.

    
    Following TAC is optional optimization, which doesn't turn TAC into anything else, just reorganizes it. Then
    from TAC, AIR bytecode is generated. 

*/

#ifndef AMU_TAC_H
#define AMU_TAC_H

#include "representations/Function.h" // Function::display()
#include "representations/ScalarValue.h"
#include "representations/Variable.h" // Var::display()

namespace amu {

struct TAC;
struct ASTNode;

enum width {
    byte = 0, // 1 byte
    word = 1, // 2 bytes
    dble = 2, // 4 bytes
    quad = 3, // 8 bytes
};  

namespace tac {
enum op {
    // a placeholder TAC. This supports various kinds of control flow not knowing where
    // they are going to jump to. When we create a TAC, if the last TAC's op is 'nop', we return it
    // instead, so that jumps jump directly to the next instruction rather than a label
    nop,

    // a TAC that just creates a temp value for later TAC to refer to 
    temp,
	
    // indicates the start of a function
    // first operand denotes the size of the parameters + returns + local variables
    // second operand denotes an offset into this size where returns start
    func_start,

    addition,
    multiplication,
    subtraction,
    division,
    modulo,

    equal,
    not_equal,
    less_than,
    less_than_or_equal,
    greater_than,
    greater_than_or_equal,

    logical_and,
    logical_or,

    // assignment between 2 things
    assignment, 

    // get the address of the first first argument
    reference,

    // a parameter for an upcoming call
    param,

    // a call to a function
    call,

    // begin a coroutine, that is, a function that may 'yield' to the caller
    // and continues where it left off when called again.
    // these functions store and retrieve their state from the stack 
    coroutine_start,
    
    // yield from a coroutine, possibly with some value
    // this stores the functions entire stack on the heap
    // which is then retrieved when the function is called
    // again
    yield,

    // same as a function call, but expects to be used on a coroutine
    // and, besides the first call, will load its state from the heap
    call_coroutine,


    // markers for block start and end
    // this does not only represent literal blocks from amu
    // its primary use is to indicate scopes where temporaries will need to be 
    // cleaned up. The main example is 'loop', since loop can be used without a block
    // for example:
    //     loop a = a + 1;
    // this creates one temporary, which we need to pop everytime the loop ends, so
    // if there is no block in the source code, we insert one manually so that
    // the AIR knows to clean up any temporaries made within the loop 
    block_start,
    block_end,

    // the value given by a block if its last expression is not terminated by a semicolon
    block_value,

    // return from a function
    // takes a single optional argument which indicates something to return
    ret, 

    // a jump to another TAC
    jump,

    // conditional jumps
    jump_zero,
    jump_not_zero,
	
	// a TAC explicitly meant to be jumped to
	// this replaces the placeholder system I had setup before where a NOP TAC
	// was meant to be replaced by whatever was placed next. We ran into a lot 
	// of issues with that so I'm getting rid of it for now.
	jump_label,

    // I'm not sure how to handle this better atm
    // casts the first operand to float or vice versa with the size specified by the second operand 
    ftou,
    ftos,
    itof,
    resz,

    // casting to the same type but different size specified by the second operand
    resize, 

    // a TAC representing an array literal
    // temp_size is set to the total size of the array
    array_element,
    array_literal,

    // an index into whatever the first operand is 
    subscript,

    vm_break,

	intrinsic_rand_int,
	intrinsic_vm_stdout,
};
} // namespace tac

namespace arg {
enum kind {
    none,
    var,
    member,
    func,
    temporary,
    literal,
    width,
    stack_offset,
    cast,
	rand_int,
};

enum class refb {
    none,
    ref,
    deref,
};
} // namespace arg

struct Arg {
    arg::kind kind;
    union {
        Var* var; // a Var in memory that this Arg is referring to, aka an lvalue
        Function* func; // a function for call ops
        TAC* temporary; // a pointer to some TAC whose result this Arg references
        ScalarValue literal;
        u64 stack_offset;
        width w;
        ScalarValue::Kind scalar_kind;

        // a Variable with an offset and the Type of that offset
        // the reason we don't just store a Member here is because
        // that wouldn't work for nested access
        struct {
            Var* var;
            u64 offset;
            Type* type;
        } offset_var;
        
        struct {
            ScalarValue::Kind to;
            ScalarValue::Kind from;
        } cast;
    };

    b32 deref = 0;

    Arg() {memory.zero(this, sizeof(Arg));}
    Arg(Var* p) : kind(arg::var), var(p) {}
    Arg(Function* f) : kind(arg::func), func(f) {}
    Arg(TAC* t) : kind(arg::temporary), temporary(t) {}
    Arg(ScalarValue l) : kind(arg::literal), literal(l) {}
    Arg(width x) : kind(arg::width), w(x) {}
    Arg(const Arg& a) {memory.copy(this, (void*)&a, sizeof(Arg));}
    Arg operator=(const Arg& a) {memory.copy(this, (void*)&a, sizeof(Arg)); return *this;} 
};

struct TAC {
    tac::op op;
    Arg arg0, arg1;

    // TAC links for resolving jumps 
    // when a TAC is a jump, it will set 'to' to whatever TAC it wants to jump to
    // when a TAC is jumped to it sets 'from', which points to the start of a linked list
    // of TAC that all jump to that TAC
    TAC* from;
    TAC* next;
    TAC* to;

    b32 lvalue;

    // when a TAC generates a temporary, this will determine its size
    u64 temp_size;
    b32 is_float; 


    // ~~~~ info generated by GenAIR ~~~~
    // do NOT set these in GenTAC!


    // during AIR generation, we store where on the stack a TAC's temporary was placed
    u64 temp_pos;
    // set to the first BC that is generated from a TAC 
    u64 bc_offset;

    // for debug purposes, when a TAC is created its id is the number of TAC created
    // before it. need to move this somewhere better eventually 
    u64 id; 

    // the node the information of this TAC was retrieved from
    ASTNode* node;

    String comment;

    static TAC*
    create();
};

void to_string(DString& current, Arg* arg);
void to_string(DString& current, Arg& arg);

void to_string(DString& current, TAC* tac);

} // namespace amu

#endif // AMU_TAC_H
