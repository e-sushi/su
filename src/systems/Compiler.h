/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

#include "systems/Messenger.h"
#include "storage/Pool.h"
#include "representations/Entity.h"
#include "processors/Lexer.h"
#include "processors/Parser.h"
#include "processors/Sema.h"
#include "processors/GenTAC.h"
#include "processors/GenAIR.h"
#include "representations/Code.h"
#include "systems/VM.h"
#include "systems/Debugger.h"

namespace amu {

struct Compiler {
    util::Stopwatch compiler_time;

    FILE* log_file;

    struct {
        Pool<SourceCode>      source_code;
        Pool<VirtualCode>     virtual_code;
        Pool<Source>          sources;
        Pool<Lexer>           lexers;
        Pool<Parser>          parsers;
        Pool<Sema>            semas;
        Pool<GenTAC>          tac_gens;
        Pool<GenAIR>          air_gens;
        Pool<VM>              vm;
        Pool<Module>          modules;
        Pool<Label>           labels;
        Pool<VirtualLabel>    virtual_labels;
        Pool<Member>          members;
        Pool<Structure>       structures;
        Pool<Function>        functions;
        Pool<Stmt>            statements;
        Pool<Expr>            expressions;
        Pool<CompileTime>     comp_times;
        Pool<ScalarLiteral>   scalar_literals;
        Pool<StringLiteral>   string_literals;
        Pool<ArrayLiteral>    array_literals;
        Pool<TupleLiteral>    tuple_literals;
        Pool<Call>            calls;
        Pool<Block>           blocks; 
        Pool<VarRef>          varrefs; 
        Pool<For>             fors; 
        Pool<Var>             vars;
        Pool<Tuple>           tuples;
        Pool<Scalar>          scalars;
        Pool<Structured>      structured_types;
        Pool<Pointer>         pointer_types;
        Pool<StaticArray>     static_array_types;
        Pool<ViewArray>       view_array_types;
        Pool<DynamicArray>    dynamic_array_types;
        Pool<Range>           range_types;
        Pool<Variant>         variant_types;
        Pool<FunctionType>    function_types;
        Pool<TupleType>       tuple_types;
        Pool<MetaType>        meta_types;
		Pool<Debugger>        debuggers;
    }storage;

    // a global map of addresses to variables 
    Map<u8*, Var*> global_symbols;
    Array<Diagnostic> diagnostics;
    
    // anything that is specified on the command line
    // later on all of these things should be changable from within the language
    struct {
        String entry_path;

        u32 verbosity;
        b32 deliver_debug_immediately;

        b32 quiet;
        
        struct{
            String path;
            b32 exit;
            b32 human;
        }dump_tokens;

        struct{
            String path;
            Array<String> sources;
        }dump_diagnostics; 

    } options;
};

namespace compiler {

// initializes the compiler, storing all information in the global amu::compiler::instance
global void
init();

global void
deinit();


// the entry point of the compiler given a list of command line arguments
global void
begin(Array<String> args);

// attempts to compile a Code object up to the given level
// this doesn't clean up anything as it is expected that whatever is calling it
// needs information from whatever is generated here. All information generated
// here can be cleaned up by just calling code::destroy(<code ptr>)
b32
funnel(Code* code, code::level level);

// the global compiler object, referred to by anything in the program
// later on, we may allow creating multiple instances of the Compiler
// at the moment, I don't think I want this to be used directly,
// probably best to have an interface to it, but I'll never restrict it
// from direct use
extern Compiler instance;

// global compiler module, which is built up with information accessible to
// the language itself
extern Module* module;

} // namespace compiler 
} // namespace amu

#endif // AMU_COMPILER_H
