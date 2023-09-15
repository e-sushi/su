#ifndef AMU_PARSER_H
#define AMU_PARSER_H

#include "storage/Map.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "representations/Label.h"
#include "representations/Code.h"

namespace amu {



struct Parser {
    Code* code;

    ASTNode* root;

    // this is really ugly, split them up into different types 
    // or just prefix the names 
    struct TableStack {
        Array<LabelTable*> stack;
        LabelTable* last;

        void push(LabelTable* l);
        void pop();
        void add(String id, Label* l);
        Label* search(u64 hashed_id);
    } table;

    struct NodeStack {
        Array<ASTNode*> stack;
        ASTNode* current;

        void push(ASTNode* n);
        ASTNode* pop();
    } node;

    // array of Tokens representing identifiers that refer to something
    // unknown when we come across it in parsing 
    Array<Token> unknowns;

    code::TokenIterator token;


    // ~~~~~~ interface ~~~~~~~


    // creates a Parser for the given Code and returns it 
    static Parser*
    create(Code* code);

    void
    destroy();

    // begins parsing and returns true if successful
    b32
    parse();

    // displays the current NodeStack 
    DString*
    display_stack();


    // actual stages of the Parser as methods so that we don't have to keep manually 
    // passing a Code object and TokenIterator around. These aren't meant to be 
    // manually accessed and I put them behind private just to keep them from polluting
    // Parser's namespace or whatever you'd call it in my IDE 
private:
    // these stages perform a sort of prescan of a given Code object
    // and only emit new, nested Code objects from the starting object
    // and dispatches a new Parser on them
    b32 prescan_start();
    b32 prescan_source();
    b32 prescan_module();
    b32 prescan_label();
    b32 prescan_expression();
    
    // these stages perform 'actual' parsing of the current Code object
    // in that they will build an AST for it. It's possible that new Code
    // objects are created and Parsed as well, though
    b32 start();
    b32 prescanned_function();
    b32 prescanned_type();
    b32 prescanned_var_decl();
    b32 struct_decl();
    b32 label();
    b32 label_get();
    b32 label_group_after_comma();
    b32 label_after_colon();
    b32 tuple();
    b32 expression();
    b32 access();
    b32 term();
    b32 additive();
    b32 bit_shift();
    b32 relational();
    b32 equality();
    b32 bit_and();
    b32 bit_xor();
    b32 bit_or();
    b32 logi_and();
    b32 logi_or();
    b32 assignment();
    b32 conditional();
    b32 loop();
    b32 factor();
    b32 block();
    b32 typeref();
    b32 reduce_builtin_type_to_typeref_expression();
    b32 reduce_literal_to_literal_expression();
};

namespace parser {

Parser*
create(Code* source);

void
destroy(Parser* parser);

void
parse(Code* code);

namespace stack {

void
push(Code* c, ASTNode* node);

// NOTE(sushi) this is somewhat dangerous cause you can accidentally pass something like 
//             a function pointer to this and it will work
//             but it's useful so i will keep it for now :)
//             if our types inherited TNode, we could use std::is_base_of
//             but i dont want to implement all of that rn
template<typename T> FORCE_INLINE void
push(Code* c, T* node) { push(c, (ASTNode*)node); }

ASTNode*
pop(Code* c);

ASTNode*
last(Code* c);

void
push_table(Code* c, LabelTable* table);

void
pop_table(Code* c);

LabelTable*
current_table(Code* c);

DString
display(Code* c);

} // namespace stack

namespace table {
// adds the given label to whatever table is currently active
void 
add(Code* c, String id, Label* l);
} // namespace table

namespace symbol {

Label*
search(Code* code, u64 hashed_id);

} // namespace symbol


} // namespace parser
} // namespace amu 

#endif // AMU_PARSER_H