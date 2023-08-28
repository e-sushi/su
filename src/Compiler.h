/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

#include "Messenger.h"
#include "storage/Pool.h"
#include "Entity.h"
#include "Result.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Generator.h"
#include "Code.h"

namespace amu {

struct Compiler {
    util::Stopwatch compiler_time;

    FILE* log_file;

    // in order to make cleaning up the compiler easy, storage of all things in amu 
    // have their handle inside the compiler struct
    // this way when the compiler needs to be deinitialized, it doesn't need to ask other
    // things to clean up
    //
    // this is experimental and depending on how much state needs cleaned up in other 
    // components of the compiler, I may just go back to storing this locally  
    struct {
        Pool<SourceCode>         source_code;
        Pool<VirtualCode>        virtual_code;
        Pool<Source>             sources;
        Pool<Lexer>              lexers;
        Pool<Parser>             parsers;
        Pool<Sema>               semas;
        Pool<Gen>                gens;
        Pool<Module>             modules;
        Pool<Label>              labels;
        Pool<Structure>          structures;
        Pool<Function>           functions;
        Pool<Statement>          statements;
        Pool<Expression>         expressions;
        Pool<CallExpression>     call_expressions;
        Pool<BlockExpression>    block_expressions; 
        Pool<PlaceRefExpression> placeref_expressions; 
        Pool<Place>              places;
        Pool<Tuple>              tuples;
        Pool<Type>               types;
        Pool<ScalarType>         builtin_types;
        Pool<StructuredType>     structured_types;
        Pool<PointerType>        pointer_types;
        Pool<ArrayType>          array_types;
        Pool<VariantType>        variant_types;
        Pool<FunctionType>       function_types;
        Pool<TupleType>          tuple_types;
        Pool<MetaType>           meta_types;
    }storage;

    Array<Diagnostic> diagnostics;
    
    // anything that is specified on the command line
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

global void
begin(Array<String> args);

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