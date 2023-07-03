#ifdef FIX_MY_IDE_PLEASE
#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/node.h"

#define DESHI_DISABLE_IMGUI
#include "core/logger.h"
#include "core/platform.h"
#include "core/file.h"
#include "core/threading.h"

#include <stdio.h>
#include <stdlib.h>

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/hash.h"
#include "types.h"

#endif

#define perror(token, ...)\
amufile->logger.error(token, __VA_ARGS__)

#define perror_ret(token, ...)do{\
amufile->logger.error(token, __VA_ARGS__);\
amufile->syntax_analyzer.failed = 1;\
return 0;\
}while(0)

#define pwarn(token, ...)\
amufile->logger.warn(token, __VA_ARGS__);


#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_next(...) if(next_match(__VA_ARGS__))
#define expect_prev(...) if(prev_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))



template<typename... T>
amuNode* SyntaxAnalyzerThread::binop_parse(amuNode* node, amuNode* ret, Type next_stage, T... tokchecks){
    amuNode* out = ret;
    amuNode* lhs_node = ret;
    while(next_match(tokchecks...)){
        curt++;
        //make binary op expression
        Expression* op = arena.make_expression(curt->raw);
        op->type = binop_token_to_expression(curt->type);
        op->token_start = curt-1;

        //readjust parents to make the binary op the new child of the parent node
        //and the ret node a child of the new binary op
        change_parent(node, &op->node);
        change_parent(&op->node, out);

        //evaluate next expression
        curt++;
        Expression* rhs = (Expression*)(define(&op->node, next_stage));

        out = &op->node;
    }
    return out;
}



// amuNode* SyntaxAnalyzerThread::define(amuNode* node, Type stage){DPZoneScoped;
//     //ThreadSetName(amuStr8("parsing ", curt->raw, " in ", curt->file));
//     switch(stage){
//         case psFile:{ //-------------------------------------------------------------------------------------------------File
//             // i dont think this stage will ever be touched
//         }break;

//         case psDirective:{ //---------------------------------------------------------------------------------------Directive
//             // there is no reason to check for invalid directives here because that is handled by lexer
//             expect(Token_Directive_Import){
//                 b32 is_global = curt->is_global;
//                 Statement* s = arena.make_statement(curt->raw);
//                 s->type = Statement_Import;
//                 s->token_start = curt;
//                 curt++;
//                 expect(Token_OpenBrace){
//                     curt++;
//                     while(!curr_match(Token_CloseBrace)){
//                         amuNode* n = define(&s->node, psImport);
//                         if(!n) return 0;
//                         expect(Token_Comma) {
//                             curt++;
//                             expect(Token_CloseBrace){curt++;break;}
//                         }else expect(Token_CloseBrace) {curt++;break;}
//                         else perror_ret(curt, "expected a , or a } after import block.");

//                     }     
//                 }else{
//                     amuNode* n = define(&s->node, psImport);
//                     if(!n) return 0;
//                 }
//                 //NOTE(sushi) still not sure if we want to allow scoped importing
//                 //            this also doesnt work so im just commenting it out for now
//                 //if(is_global){
//                     //amufile->syntax_analyzer.import_directives.add(s);
//                 //}else{
//                 //    insert_last(node, &s->node);
//                 //}
//                 insert_first(node, (amuNode*)s);
//             }

//         }break;

//         case psImport:{ //---------------------------------------------------------------------------------------------Import
//             //#import 

//             expect(Token_LiteralString){

//             }

//             expect(Token_LiteralString){
//                 Expression* e = arena.make_expression(curt->raw);
//                 e->type = Expression_Literal;
//                 e->data.structure = compiler.builtin.types.str;
//                 e->token_start = curt;
//                 e->token_end = curt;
//                 insert_last(node, &e->node);
//                 curt++;
//                 expect(Token_OpenBrace){
//                     curt++;
//                     while(!curr_match(Token_CloseBrace)){
//                         expect(Token_Identifier){
//                             Expression* id = arena.make_expression(curt->raw);
//                             id->type = Expression_Identifier;
//                             id->token_start = curt;
//                             id->token_end = curt;
//                             curt++;
//                             expect(Token_As){
//                                 Expression* op = arena.make_expression(curt->raw);
//                                 op->type = Expression_BinaryOpAs;
//                                 op->token_start = id->token_start;
//                                 curt++;
//                                 expect(Token_Identifier){
//                                     op->token_end = curt;
//                                     Expression* idrhs = arena.make_expression(curt->raw);
//                                     idrhs->type = Expression_Identifier;
//                                     idrhs->token_start = curt;
//                                     idrhs->token_end = curt;
//                                     insert_last(&op->node, &id->node);
//                                     insert_last(&op->node, &idrhs->node);
//                                     insert_last(&e->node, &op->node);
//                                     curt++;
//                                 }else perror_ret(curt, "expected an identifier for subimport alias.");
//                             }else{
//                                 insert_last(&e->node, &id->node);
//                             }
//                             //this seems redundant
//                             //i dont think we need to even check for commas in this situation
//                             //but i prefer to enforce them for multiple items for consistency
//                             expect(Token_Comma){
//                                 curt++;
//                                 expect(Token_CloseBrace){curt++;break;}
//                             }else expect(Token_CloseBrace) {curt++;break;}
//                             else perror_ret(curt, "expected a , after subimport.");
//                         }else perror_ret(curt, "expected an identifier for subimport.");
//                     }
//                 }
//                 expect(Token_As){
//                     Expression* op = arena.make_expression(curt->raw);
//                     op->type = Expression_BinaryOpAs;
//                     op->token_start = e->token_start;
//                     change_parent((amuNode*)op, (amuNode*)e);
//                     insert_last(node, (amuNode*)op);
//                     curt++;
//                     expect(Token_Identifier){
//                         Expression* id = arena.make_expression(curt->raw);
//                         id->type = Expression_Identifier;
//                         id->token_start = curt;
//                         id->token_end = curt;
//                         insert_last((amuNode*)op, (amuNode*)id);
//                         curt++;
//                     }else perror_ret(curt, "expected an identifier after 'as' for import.");
//                 }
//                 return &e->node;
//             }else perror_ret(curt, "expected a string specifying an import.");

           
//         }break;

//         case psRun:{ //---------------------------------------------------------------------------------------------------Run
//             NotImplemented;
//         }break;

//         case psScope:{ //-----------------------------------------------------------------------------------------------Scope
//             Scope* s = arena.make_scope(curt->raw);
//             amuNode* me = &s->node;
//             insert_last(node, me);
//             while(!next_match(Token_CloseBrace)){
//                 curt++;
//                 expect(Token_Identifier){
//                     if(curt->is_declaration){
//                         amuNode* ret = define(me, psDeclaration);
//                         if(!ret) return 0;
//                         Declaration* d = (Declaration*)ret;
//                     }else{
//                         define(me, psStatement);
//                     }
//                 }else expect(Token_EOF){
//                     perror_ret(curt, "unexpected end of file.");
//                 } else {
//                     if(!define(me, psStatement)) return 0;
//                 }
//             }
//             curt++;
//             return &s->node;
//         }break;

//         case psDeclaration:{ //-----------------------------------------------------------------------------------Declaration
//             // TODO(sushi) rewrite this stuff, it's garbage
//             str8 name;
//             expect(Token_Identifier) {
//                 name = curt->raw;
//             } else expect(Token_Colon){ // anonymous struct declaration
//                 str8b gen;
//                 str8_builder_init(&gen, amufile->file->front);
//                 str8_builder_replace_codepoint(&gen, ' ', '_'); // TODO(sushi) add more needed replacements as they show up
//                 name = amuStr8("anonymous_struct_",gen.fin,"_",curt->l0,"_",curt->c0);
//                 str8_builder_deinit(&gen);
//             } else perror_ret(curt, "INTERNAL: syntax_analyzer stage psDeclaration started, but current token is not an identifier or colon.");
//             DPTracyDynMessage(toStr("declaring identifier ", curt->raw));
//             switch(curt->decl_type){
//                 case Declaration_Structure:{
//                     Struct* s = arena.make_struct(name);
//                     Declaration* decl = &s->decl;

//                     s->members.init();
//                     s->operators.init();
//                     s->conversions.init();
//                     s->decl.token_start = curt;
//                     s->decl.type = Declaration_Structure;
//                     s->decl.identifier = name;
//                     s->decl.declared_identifier = name;

//                     if(curt->is_global){
//                         if(is_internal) amufile->syntax_analyzer.internal_decl.add(&s->decl);
//                         else            amufile->syntax_analyzer.exported_decl.add(&s->decl);
//                     }

//                     //it's possible we have either an identifier, for named structs, or a colon, for anonymous structs
//                     //so we don't fail upon not finding an identifier. 
//                     expect(Token_Identifier){curt++;}
//                     expect(Token_Colon){curt++;}
//                     else perror_ret(curt, "INTERNAL: expected ':' for struct declaration. NOTE(sushi) tell me if this happens");
                    
//                     expect(Token_StructDecl){curt++;}
//                     else perror_ret(curt, "INTERNAL: expected 'struct' after ':' for struct definition. NOTE(sushi) tell me if this happens");

//                     expect(Token_OpenBrace){}
//                     else perror_ret(curt, "expected a '{' after 'struct' in definition of struct '", s->decl.identifier, "'.");

//                     while(!next_match(Token_CloseBrace)){
//                         curt++;
//                         expect(Token_Identifier){
//                             if(!curt->is_declaration){
//                                 perror(curt, "only declarations are allowed inside struct definitions. NOTE(sushi) if this is a declaration, then this error indicates something wrong internally, please let me know.");
//                                 return 0;
//                             }
//                             amuNode* fin = define(&s->decl.node, psDeclaration);
//                             if(!fin) return 0;
//                             if(((Declaration*)fin)->type == Declaration_Function){
//                                 perror_ret(curt, "only variables may be declared inside of structs.");
//                             }
//                             Declaration* d = (Declaration*)fin;
//                             s->members.add(d->identifier, &d->node);
//                         }
//                     }
//                     curt++;

//                     if(node) insert_last(node, (amuNode*)decl);

//                     for_node(s->decl.node.first_child){
//                         amufile->logger.log(Verbosity_Debug, it->debug);
//                     }

//                     condition_variable_notify_all(&this->cv);
//                     return (amuNode*)decl;
//                 }break;

//                 case Declaration_Function:{
//                     Function* f = arena.make_function(curt->raw);
//                     f->args.init();
//                     f->decl.token_start = curt;
//                     f->decl.type = Declaration_Function;
//                     f->decl.identifier = curt->raw;
//                     f->decl.declared_identifier = curt->raw;
//                     if(node) insert_last(node, &f->decl.node);
//                     b32 is_global = curt->is_global;
//                     str8 id = curt->raw;
//                     f->internal_label = id;
//                     f->internal_label = amuStr8(f->internal_label, "@");
//                     curt++;
//                     expect(Token_OpenParen){ // name(
//                         //disable checking for semicolon after variable declaration
//                         parsing_func_args = 1;
//                         //TODO(sushi) god please clean this up
//                         b32 found_defaulted = 0;
//                         while(1){
//                             curt++;
//                             expect(Token_CloseParen) { break; }
//                             else expect(Token_Identifier){
//                                 //its possible the user made an error that caused this token to not be marked as a declaration, but since 
//                                 //we know it should be here we manually mark it 
//                                 curt->decl_type = Declaration_Variable;
//                                 Variable* v = (Variable*)define(&f->decl.node, psDeclaration);
//                                 if(!v) return 0;
//                                 f->internal_label = amuStr8(f->internal_label, (v->decl.token_start + 2)->raw, ",");
//                                 forI(v->pointer_depth){
//                                     f->internal_label = amuStr8(f->internal_label, STR8("*"));
//                                 }
//                                 if(v->initialized) f->default_count++, found_defaulted = 1;
//                                 else if(found_defaulted) perror_ret(curt, "arguments following an argument with a default value must also have a default value");
//                                 expect(Token_Comma){
//                                     expect_next(Token_CloseParen) perror_ret(curt,"trailing comma in function declaration.");
//                                 }else expect(Token_CloseParen){break;}
//                                 else perror_ret(curt, "expected a , or ) after function variable declaration.");
//                             } else perror_ret(curt, "expected an identifier for function variable declaration.");
//                         }
//                         parsing_func_args = 0;
//                         curt++;
//                         expect(Token_Colon){ // name(...) :
//                             f->decl.colon_anchor = curt;
//                             curt++;
//                             //TODO(sushi) multiple return types
//                             expect_group(TokenGroup_Type){ // name(...) : <type>
//                                 f->data.structure = builtin_from_type(curt->type);
//                                 f->internal_label = amuStr8(f->internal_label, "@", curt->raw);
//                                 if(is_global){
//                                     if(is_internal) amufile->syntax_analyzer.internal_decl.add(&f->decl);
//                                     else            amufile->syntax_analyzer.exported_decl.add(&f->decl);
//                                 }
//                             }else expect(Token_Identifier){ // name(...) : <type>
//                                 //we are most likely referencing a struct type in this case
//                                 f->data.structure = 0;
//                                 f->internal_label = amuStr8(f->internal_label, "@", curt->raw);
//                                 if(curt->is_global){
//                                     if(is_internal) amufile->syntax_analyzer.internal_decl.add(&f->decl);
//                                     else            amufile->syntax_analyzer.exported_decl.add(&f->decl);
//                                 }
//                             } else perror(curt, "expected a type specifier after ':' in function declaration.");
//                             curt++;
//                             expect(Token_OpenBrace) {
//                                 return define(&f->decl.node, psScope);
//                             } else perror(curt, "expected '{' after function declaration.");
//                         } else perror(curt, "expected : after function definition.");
//                     } else perror(curt, "expected ( after identifier in function declaration.");

//                 }break;

//                 case Declaration_Variable:{ //name
//                     Variable* v = arena.make_variable(amuStr8(curt->raw, " - decl"));
//                     if(curt->is_global){
//                         if(is_internal) amufile->syntax_analyzer.internal_decl.add(&v->decl);
//                         else            amufile->syntax_analyzer.exported_decl.add(&v->decl);
//                     }
//                     v->decl.declared_identifier = curt->raw;
//                     v->decl.identifier = curt->raw;
//                     v->decl.token_start = curt;
//                     v->decl.type = Declaration_Variable;
//                     str8 id = curt->raw;
//                     curt++;
//                     expect(Token_Colon){ // name :
//                         v->decl.colon_anchor = curt;
//                         curt++;
//                         expect_group(TokenGroup_Type){ // name : <type>
//                             v->data.structure = builtin_from_type(curt->type);
//                         }else expect(Token_Identifier, Token_Colon){ // name : <type>
//                             // it's possible the user is defining a struct as the type specifier for this variable, so 
//                             // we must parse it.
//                             if(curt->is_declaration){
//                                 amuNode* n = define(node, psDeclaration);
//                                 if(!n) return 0;
//                                 // we can set the variable's type information early
//                                 // and we have to in the case of anonymous structs being used as the type,
//                                 // because otherwise we would have to have a complex procedure later for finding
//                                 // the anonymous struct belonging to a variable in validation.
//                                 v->data.structure = (Struct*)n;
//                             }else{
//                                 // it's also possible we are using a type in a module or something
//                                 amuNode* n = define(node, psAccess);
//                                 if(!n) return 0;
//                             }
//                         }else expect(Token_Assignment) v->data.implicit = 1;
//                         else perror(curt, "expected a typename or assignment for variable declaration, found ", token_type_str(curt->type), " instead.");
//                         if(!v->data.implicit) curt++;

//                         //parse pointer and array stuff 
//                         while(1){
//                             expect(Token_Multiplication){
//                                 v->data.structure = builtin_from_type(DataType_Ptr);
//                                 v->data.pointer_depth++;
//                                 curt++;
//                             }else expect(Token_OpenSquare){
//                                 NotImplemented;
//                             }else break;
//                         }
//                         expect(Token_Assignment){ // name : <type> = || name := 
//                             curt++;
//                             Expression* e = (Expression*)define(&v->decl.node, psExpression);
//                             if(!e) return 0;
//                             v->initialized = 1;
//                             curt++;
//                         }
//                         else if(v->data.implicit) perror(curt, "implicitly defined variable '", id, "' requires an assignment to deduce its type.");
//                         else expect(Token_Semicolon) {}
//                         else if(!parsing_func_args) perror(curt, "expected a ; or assignment after typename in declaration of variable '", id, "'.");

//                         //expect(Token_Semicolon){}
//                         //else if(!parsing_func_args) perror_ret(curt, "expected ; after variable declaration.");

//                         insert_last(node, &v->decl.node);
//                         return &v->decl.node;
//                     } else perror(curt, "Expected a ':' after identifier for variable decalration.");
//                 }break;

//                 case Declaration_Module:{

//                 }break;
//             }
//         }break;

//         case psStatement:{ //---------------------------------------------------------------------------------------Statement
//             switch(curt->type){
//                 case Token_OpenBrace:{
//                     define(node, psScope);
//                 }break;

//                 case Token_Using:{
//                     Statement* s = arena.make_statement();
//                     s->type = Statement_Using;
//                     s->token_start = curt;
//                     curt++;
//                     expect(Token_Identifier){
//                         Expression* e = arena.make_expression(curt->raw);
//                         e->type = Expression_Identifier;
//                         e->token_start = curt;
//                         e->token_end = curt;
//                         insert_last(&s->node, &e->node);
//                         curt++;
//                         expect(Token_Semicolon){}
//                         else perror_ret(curt, "expected a semicolon after using statement");
//                     } else perror_ret(curt, "expected a variable identifier after 'using'.");
//                 }break;

//                 case Token_Return:{
//                     Statement* s = arena.make_statement(curt->raw);
//                     s->type = Statement_Return;
//                     s->token_start = curt;
//                     insert_last(node, &s->node);
//                     curt++;
//                     expect(Token_Semicolon){
//                         curt++;
//                     }else{
//                         amuNode* n = define(&s->node, psExpression);
//                         if(!n) return 0;
//                         change_parent(&s->node, n);
//                         curt++;
//                         expect(Token_Semicolon){
//                             return &s->node;
//                         }else{
//                             perror_ret(s->token_start, "expected a ';' after return statement expression.");
//                         }
//                     }
//                     return &s->node;
//                 }break;

//                 case Token_If:{
//                     Statement* s = arena.make_statement(curt->raw);
//                     s->token_start = curt;
//                     s->type = Statement_Conditional;
//                     insert_last(node, &s->node);
//                     curt++;
//                     expect(Token_OpenParen){
//                         curt++;
//                         expect(Token_CloseParen) perror_ret(curt, "expected an expression for if statement.");
//                         amuNode* n = define(&s->node, psExpression);
//                         if(!n) return 0;
//                         curt++;
//                         expect(Token_CloseParen){
//                             curt++;
//                             expect(Token_OpenBrace){
//                                 n = define(&s->node, psScope);
//                                 if(!n) return 0;
//                                 s->token_end = curt;
//                             }else{  
//                                 //check that the user isnt trying to declare anything in an unbraced if statement
//                                 expect(Token_Identifier)
//                                     if(curt->is_declaration) perror_ret(curt, "can't declare something in an unscoped if statement.");
//                                 n = define(&s->node, psStatement);
//                                 if(!n) return 0;
//                                 curt++;
//                                 expect(Token_Semicolon){}
//                                 else perror_ret(curt, "expected a ;");
//                                 s->token_end = curt;
//                             }

//                         }else perror_ret(curt, "expected a ) after if statement's expression.");
//                     }else perror_ret(curt, "expected a ( for if statement.");
//                     return &s->node;
//                 }break;

//                 case Token_Else:{
//                     Statement* s = arena.make_statement(curt->raw);
//                     s->type = Statement_Else;
//                     s->token_start = curt;
//                     insert_last(node, &s->node);
//                     curt++;
//                     expect(Token_OpenBrace){
//                         amuNode* n = define(&s->node, psScope);
//                         if(!n) return 0;
//                         s->token_end = curt;
//                     }else{
//                         //check that the user isnt trying to declare anything in an unscoped else statement
//                         expect(Token_Identifier)
//                             if(curt->is_declaration) perror_ret(curt, "can't declare something in an unscoped else statement.");
//                         amuNode* n = define(&s->node, psStatement);
//                         if(!n) return 0;
//                         curt++;
//                         expect(Token_Semicolon){}
//                         else perror_ret(curt, "expected a ;");
//                         s->token_end = curt;
//                     }
//                     return &s->node;
//                 }break; 

//                 case Token_Pound:{
//                     TestMe;
//                     curt++;
//                     amuNode* n = define(node, psDirective);
//                     if(!n) return 0;
//                 }break;

//                 default:{
//                     //this is probably just some expression
//                     Statement* s = arena.make_statement();
//                     s->token_start = curt;
//                     s->type = Statement_Expression;
//                     s->node.debug = STR8("exp statement");
//                     insert_last(node, &s->node);
//                     amuNode* ret = define(&s->node, psExpression);
//                     if(!ret) return 0;
//                     curt++;
//                     s->token_end = curt;
//                     expect(Token_Semicolon){}
//                     else perror_ret(curt, "expected ; after statement.");

//                 }break;

//             }
//         }break;

//         case psExpression:{ //-------------------------------------------------------------------------------------Expression
//             expect(Token_Identifier){
//                 //go down expression chain first
//                 amuNode* n = define(node, psConditional);
//                 if(!n) return 0;
//                 Expression* e = (Expression*)n;

//                 expect_next(Token_Assignment){
//                     Token* before = curt++;
//                     Expression* op = arena.make_expression(curt->raw);
//                     op->token_start = before;
//                     op->type = Expression_BinaryOpAssignment;
//                     change_parent(&op->node, &e->node);
//                     curt++;
//                     op->token_end = curt;
//                     amuNode* ret = define(&op->node, psExpression);
//                     insert_last(node, &op->node);
//                     return &op->node;                                                
//                 }
//                 return n;
//             }else{
//                 Expression* e = (Expression*)(define(node, psConditional));
//                 return &e->node;
//             }

//         }break;

//         case psConditional:{ //-----------------------------------------------------------------------------------Conditional
//             expect(Token_If){
//                 NotImplemented;
//             }else{
//                 Expression* e = (Expression*)(define(node, psLogicalOR));
//                 return &e->node;
//             }
//         }break;

//         case psLogicalOR:{ //---------------------------------------------------------------------------------------LogicalOR
//             Expression* e = (Expression*)(define(node, psLogicalAND));
//             return binop_parse(node, &e->node, psLogicalAND, Token_OR);
//         }break;

//         case psLogicalAND:{ //-------------------------------------------------------------------------------------LogicalAND
//             Expression* e = (Expression*)(define(node, psBitwiseOR));
//             return binop_parse(node, &e->node, psBitwiseOR, Token_AND);
//         }break;

//         case psBitwiseOR:{ //---------------------------------------------------------------------------------------BitwiseOR
//             Expression* e = (Expression*)(define(node, psBitwiseXOR));
//             return binop_parse(node, &e->node, psBitwiseXOR, Token_BitOR);
//         }break;

//         case psBitwiseXOR:{ //-------------------------------------------------------------------------------------BitwiseXOR
//             Expression* e = (Expression*)(define(node, psBitwiseAND));
//             return binop_parse(node, &e->node, psBitwiseAND, Token_BitXOR);
//         }break;

//         case psBitwiseAND:{ //-------------------------------------------------------------------------------------BitwiseAND
//             Expression* e = (Expression*)(define(node, psEquality));
//             return binop_parse(node, &e->node, psEquality, Token_BitAND);
//         }break;

//         case psEquality:{ //-----------------------------------------------------------------------------------------Equality
//             Expression* e = (Expression*)(define(node, psRelational));
//             return binop_parse(node, &e->node, psRelational, Token_NotEqual, Token_Equal);
//         }break;

//         case psRelational:{ //-------------------------------------------------------------------------------------Relational
//             Expression* e = (Expression*)(define(node, psBitshift));
//             return binop_parse(node, &e->node, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
//         }break;

//         case psBitshift:{ //-----------------------------------------------------------------------------------------Bitshift
//             Expression* e = (Expression*)(define(node, psAdditive));
//             return binop_parse(node, &e->node, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
//         }break;

//         case psAdditive:{ //-----------------------------------------------------------------------------------------Additive
//             Expression* e = (Expression*)(define(node, psTerm));
//             return binop_parse(node, &e->node, psTerm, Token_Plus, Token_Negation);
//         }break;

//         case psTerm:{ //-------------------------------------------------------------------------------------------------Term
//             Expression* e = (Expression*)(define(node, psAccess));
//             return binop_parse(node, &e->node, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
//         }break;

//         case psAccess:{
//             Expression* e = (Expression*)(define(node, psFactor));
//             //dont use binop_parse because this stage is special
//             while(next_match(Token_Dot)){
//                 curt++;
//                 Expression* op = arena.make_expression(curt->raw);
//                 op->type = Expression_BinaryOpMemberAccess;
//                 op->token_start = curt-1;

//                 change_parent(node, &op->node);
//                 change_parent(&op->node, &e->node);

//                 curt++;
//                 expect(Token_Identifier){
//                     Expression* rhs = (Expression*)define((amuNode*)op, psFactor); // arena.make_expression(curt->raw);
//                     rhs->token_start = curt;
//                     rhs->token_end = curt;
//                     rhs->type = Expression_Identifier;
//                     insert_last(&op->node, &rhs->node);
//                     e = op;
//                 }else perror_ret(curt, "expected an identifier for member access.");
//             }
//             return &e->node;
//         }break;

//         case psFactor:{ //---------------------------------------------------------------------------------------------Factor
//             switch(curt->type){
//                 default:{
//                     perror_ret(curt, "unexpected token '", curt->raw, "' found in expression.");
//                 }break;

//                 case Token_Negation:{
//                     Expression* e = arena.make_expression(curt->raw);
//                     e->token_start = e->token_end = curt;
//                     e->type = Expression_UnaryOpNegate;
//                     curt++;
//                     amuNode* ret = define((amuNode*)e, psFactor);
//                     if(!ret) return 0;
//                     insert_last(node, (amuNode*)e);
//                     return &e->node;
//                 }break;

//                 case Token_LiteralFloat:{
//                     Expression* e = arena.make_expression(curt->raw);
//                     e->token_start = curt;
//                     e->type = Expression_Literal;
//                     e->data.structure = builtin_from_type(DataType_Float64);
//                     e->data.float64 = curt->f64_val;
//                     insert_last(node, &e->node);
//                     return &e->node;
//                 }break;

//                 case Token_LiteralInteger:{
//                     Expression* e = arena.make_expression(curt->raw);
//                     e->token_start = curt;
//                     e->type = Expression_Literal;
//                     e->data.structure = builtin_from_type(DataType_Signed64);
//                     e->data.int64 = curt->s64_val;
//                     insert_last(node, &e->node);
//                     return &e->node;
//                 }break;

//                 case Token_LiteralString:{
//                     Expression* e = arena.make_expression(curt->raw);
//                     e->token_start = curt;
//                     e->type = Expression_Literal;
//                     e->data.structure = compiler.builtin.types.str;
//                     e->data.string = e->token_start->raw;
//                     insert_last(node, &e->node);
//                     return &e->node;
//                 }break;

//                 case Token_OpenParen:{
//                     curt++;
//                     Token* start = curt;
//                     amuNode* ret = define(node, psExpression); if(!ret) return 0;
//                     curt++;
//                     expect(Token_CloseParen){
//                         return ret;
//                     }else perror(curt, "expected a ) to close ( in line ", start->l0, " on column ", start->c0);
//                 }break;

//                 case Token_LessThan:{
//                     Expression* e = arena.make_expression(STR8("cast"));
//                     e->token_start = curt;
//                     insert_last(node, &e->node);
//                     curt++;
//                     expect(Token_Identifier){
//                         e->type = Expression_Cast;
//                     }else expect_group(TokenGroup_Type){
//                         e->type = Expression_Cast;
//                     }else expect(Token_LogicalNOT){
//                         e->type = Expression_Reinterpret;
//                         e->node.debug = STR8("reinterpret");
//                         curt++;
//                         expect(Token_Identifier){}
//                         else expect_group(TokenGroup_Type){}
//                         else perror_ret(curt, "expected a typename for reinterpret cast");
//                     }else perror_ret(curt, "expected a typename or ! for cast");
                    
//                     Expression* id = arena.make_expression(curt->raw);
//                     id->type = Expression_Identifier;
//                     id->token_start = curt;
//                     id->token_end = curt;
//                     insert_last(&e->node, &id->node);
//                     curt++;
//                     expect(Token_GreaterThan){}
//                     else perror_ret(curt, "expected a > to close cast");

//                     curt++;

//                     amuNode* n = define(&id->node, psExpression);
//                     if(!n) return 0;
//                     return &e->node;
//                 }break;

//                 case Token_Identifier:{
//                     Expression* identifier_expression = arena.make_expression();
//                     identifier_expression->token_start = curt;
//                     expect_next(Token_OpenParen){
//                         //this must be a function call
//                         identifier_expression->node.debug = amuStr8("call ", curt->raw);
//                         identifier_expression->type = Expression_FunctionCall;
//                         curt++;
//                         while(1){
//                             curt++;
//                             expect(Token_CloseParen) {break;}
//                             Expression* arg = (Expression*)(define(&identifier_expression->node, psExpression));
//                             if(!arg) return 0;
//                             curt++;
//                             expect(Token_Comma){}
//                             else expect(Token_CloseParen) {break;}
//                             else perror_ret(curt, "expected a , or ) after function argument.");
//                         }
//                         insert_last(node, &identifier_expression->node);
//                         identifier_expression->token_end = curt;
//                     }else{
//                         //this is just some identifier
//                         identifier_expression->node.debug = curt->raw;
//                         identifier_expression->type = Expression_Identifier;
//                         insert_last(node, &identifier_expression->node);
//                         expect_next(Token_Increment, Token_Decrement){
//                             curt++;
//                             Expression* incdec = arena.make_expression((curt->type == Token_Increment ? STR8("++") : STR8("--")));
//                             incdec->type = (curt->type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix);
//                             insert_last(node, &incdec->node);
//                             change_parent(&incdec->node, &identifier_expression->node);
//                             return &incdec->node;
//                         } else expect_next(Token_OpenBrace) {
//                             curt++;
//                             amuNode* initializer = define(&identifier_expression->node, psInitializer);
//                             if(!initializer) return 0;
//                         }
//                     }
//                     return &identifier_expression->node;
//                 }break;

//                 case Token_OpenBrace:{
//                     amuNode* initializer = define(node, psInitializer);
//                     if(!initializer) return 0;
//                     return initializer;
//                 }break;
//             }

//         }break;

//         case psInitializer:{  //----------------------------------------------------------------------------------Initializer
//             Expression* initializer = arena.make_expression(STR8("initializer"));
//             initializer->type = Expression_InitializerList;
//             initializer->token_start = curt;
//             insert_last(node, &initializer->node);
//             expect_next(Token_CloseBrace){
//                 curt++;
//                 initializer->token_end = curt;
//                 return &initializer->node;
//             }else while(1){
//                 curt++;
//                 Expression* exp = (Expression*)define(&initializer->node, psExpression);
//                 if(!exp) return 0;
//                 curt++;
//                 expect(Token_Comma){
//                     // allow trailing comma
//                     expect_next(Token_CloseBrace){
//                         curt++;
//                         break;
//                     }
//                 }
//                 else expect(Token_CloseBrace) break;
//                 else perror_ret(curt, "expected a , or } after initializer list element.");
//             }
//             initializer->token_end = curt;
//             return &initializer->node;
//         }break;

//         case psType:{ //-------------------------------------------------------------------------------------------------Type
//             Expression* type = arena.make_expression(STR8("type"));
//             type->type = Expression_Type;
//             type->token_start = curt;
//             insert_last(node, (amuNode*)type);
            
//             expect(Token_Identifier){}
//             else perror_ret(curt, "expected an identifier for type specifier.");

//             while(next_match(Token_Dot)){
//                 curt++;
//                 Expression* op = arena.make_expression(curt->raw);
//                 op->type = Expression_Type;
//                 op->token_start = curt-1;
//                 insert_last(node, (amuNode*)op);
//                 change_parent((amuNode*)op, (amuNode*)type);

//                 curt++;
//                 expect(Token_Identifier){
//                     Expression* rhs = arena.make_expression(curt->raw);
//                     rhs->token_start = curt;
//                     rhs->token_end = curt;
//                     rhs->type = Expression_Type;
//                     insert_last((amuNode*)op, (amuNode*)rhs);
//                     type = op;
//                 }else perror_ret(curt, "expected identifier after '.' in type specifier.");
//             }
//         }break;
//     }

//     return 0;
// }

// void SyntaxAnalyzer::parse(){DPZoneScoped;
//     Stopwatch time = start_stopwatch();
//     amufile->logger.log(Verbosity_Stages, "Parsing...");
//     SetThreadName("Parsing ", amufile->file->name);

//     threads = array<SyntaxAnalyzerThread>(deshi_allocator);
//     threads.reserve(
//         amufile->preprocessor.internal_decl.count+
//         amufile->preprocessor.exported_decl.count+
//         amufile->lexer.imports.count
//     );

//     amufile->logger.log(Verbosity_StageParts, "Creating file module.");

//     str8 modname = amuStr8("file_module_", amufile->file->name);
//     amufile->module = arena.make_module(modname);
//     amufile->module->decl.type = Declaration_Module;
//     amufile->module->decl.declared_identifier = 
//     amufile->module->decl.identifier = modname;
//     amufile->module->decl.token_start = &amufile->lexer.tokens[0];
//     amufile->module->decl.token_end = &amufile->lexer.tokens[amufile->lexer.tokens.count-1];
//     amufile->module->functions.init();
    
//     amufile->logger.log(Verbosity_Detailed, "Spawning threads to parse import directives.");

//     for(u32 idx : amufile->lexer.imports){
//         threads.add(SyntaxAnalyzerThread());
//         SyntaxAnalyzerThread* pt = threads.last;
//         pt->cv = condition_variable_init();
//         pt->curt = amufile->lexer.tokens.readptr(idx);
//         pt->syntax_analyzer = this;
//         pt->node = &amufile->module->decl.node;
//         pt->stage = psDirective;
//         threader_add_job({&semantic_analyzer_threaded_stub, pt});
//     }
    
//     amufile->logger.log(Verbosity_Detailed, "Spawning threads to parse internal declarations.");

//     for(u32 idx : amufile->preprocessor.internal_decl){
//         threads.add(SyntaxAnalyzerThread());
//         SyntaxAnalyzerThread* pt = threads.last;
//         pt->cv = condition_variable_init();
//         pt->curt = amufile->lexer.tokens.readptr(idx);
//         pt->syntax_analyzer = this;
//         pt->node = &amufile->module->decl.node;
//         pt->stage = psDeclaration;
//         pt->is_internal = 1;
//         threader_add_job({&semantic_analyzer_threaded_stub, pt});
//     }

//     amufile->logger.log(Verbosity_Detailed, "Spawning threads to parse exported declarations.");
    
//     for(u32 idx : amufile->preprocessor.exported_decl){
//         threads.add(SyntaxAnalyzerThread());
//         SyntaxAnalyzerThread* pt = threads.last;
//         pt->cv = condition_variable_init();
//         pt->curt = amufile->lexer.tokens.readptr(idx);
//         pt->syntax_analyzer = this;
//         pt->node = &amufile->module->decl.node;
//         pt->stage = psDeclaration;
//         pt->is_internal = 0;
//         threader_add_job({&semantic_analyzer_threaded_stub, pt});
//     }

//     amufile->logger.log(Verbosity_StageParts, "Waking threads");

//     threader_wake_threads();

//     forI(threads.count){
//         while(!threads[i].finished){
//             condition_variable_wait(&threads[i].cv);
//         }
//     }

//     amufile->logger.log(Verbosity_Stages, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
// }

amuNode* SyntaxAnalyzerThread::define(amuNode* parent, Type stage){
    amuLogger& logger = amufile->logger;
    switch(stage){
        case psLabel:{ // -------------------------------------------------------------------------------Label
            Assert(curt->type == Token_Identifier, "label stage was called but the current token is not an identifier.");

            Label* label =  arena.make_label(curt->raw);
            insert_last(parent, (amuNode*)label);
            label->identifier = curt->raw;
            label->identifier_hash = curt->raw_hash;

            curt++;
            expect(Token_Comma){
                NotImplemented;
                // either declaring multiple variables of some type or capturing a multi return from a func
                // we need to make several labels
                curt++;
                while(1){
                    // we don't have to check if things are identifiers here, because that is
                    // already handled by lexer when it comes across a :
                    Label* label = arena.make_label(curt->raw);
                    curt++;
                    expect(Token_Colon) break;
                    curt++;
                }
                curt++;

                // the only case where this can happen is if the user is assigning all of these variables to 
                // the same type, or if its capturing a multi return from a function

                expect(Token_Identifier){
                    // this must be a multi decl for a single type
                    curt++;
                    expect(Token_Semicolon){}
                    else {
                        expect(Token_Assignment){
                            logger.error(curt, "a grouped declaration of variables cannot be assigned a value.");
                        }else{
                            logger.error(curt, "unexpected token after grouped declaration of variables.");
                        }
                        amufile->syntax_analyzer.failed = 1;
                        return 0;
                    }
                }else expect(Token_Assignment){
                    curt++;
                    expect(Token_Identifier){
                        
                    }
                }
            }else expect(Token_Colon){
                curt++;
                amuNode* n = define((amuNode*)label, psDeclaration);
                if(!n) return 0;
                label->entity = (Entity*)n;
            }

        }break;

        case psDeclaration:{
            switch(curt->type){ 
                case Token_Identifier: {
                    // this must be a variable declaration
                    
                }break;
                case Token_StructDecl:{
                    // the internal identifier for this struct
                    str8 structid = amuStr8("struct", curt->file, curt->l0, curt->c0);
                    Struct* structure = arena.make_struct((current_label ? amuStr8(current_label->identifier, ":", structid) : structid));
                    structure->entity.internal_label = structid;
                    structure->entity.token_start = curt;
                    structure->entity.type = Entity_Structure;
                    insert_last(parent, (amuNode*)structure);

                    curt++;
                    expect(Token_OpenParen){
                        NotImplemented;
                        // TODO(sushi) come back and handle this later once a better internal representation for generics is implemented
                        // a structure with parameters
                        curt++;
                        expect(Token_CloseParen){
                            // if no parameters are given, we just emit a note telling the user that this isn't necessary
                            logger.note(curt, "missing parameters for generic struct declaration. note that '()' is not required on a non-generic struct.");
                            curt++;
                        }else while(1){
                            expect(Token_CloseParen){ curt++; break; }
                            expect(Token_Identifier){ // the user must be giving a label    
                                expect_next(Token_Colon){
                                    Label* l = (Label*)define((amuNode*)structure, psLabel);
                                    if(!l) return 0;
                                    if(l->entity->type != Entity_Variable){
                                        amufile->syntax_analyzer.failed = 1;
                                        logger.error("only variable declarations or generic typenames are allowed for struct paramters.");
                                        return 0;
                                    }
                                }else expect(Token_Identifier){
                                    // it's possible that this is an identifier that was specified earlier using "?"identifier, so we leave it alone and let the semantic analyzer figure it out later 
                                    
                                }
                            }
                        }
                    }else expect(Token_OpenBrace){
                        curt++;
                        while(1){
                            expect(Token_CloseBrace){ curt++; break; }
                            expect(Token_Identifier){
                                expect_next(Token_Colon){
                                    Label* l = (Label*)define((amuNode*)structure, psLabel);
                                    if(!l) return 0;
                                    if(!match_any(l->entity->type, Entity_Variable, Entity_Structure)){
                                        amufile->syntax_analyzer.failed = 1;
                                        logger.error(l->entity->token_start, "struct members may only be variables or structures.");
                                        // no erroring out here because the separation is enough that we may continue
                                    }
                                }
                            }else{
                                // TODO(sushi) error recovery
                                amufile->syntax_analyzer.failed = 1;
                                logger.error(curt, "unexpected token in struct declaration.");
                                return 0;
                            }
                        }
                    }

                }break;
            }
        }break;
    }
    return 0;
}

void SyntaxAnalyzer::analyze() { DPZoneScoped;
    Stopwatch time = start_stopwatch();
    amuLogger& logger = amufile->logger;
    SetThreadName("Parsing ", amufile->file->name);

    logger.log(Verbosity_StageParts, "Reserving threads arena.");
    threads.init(
        amufile->preprocessor.internal_decl.count+
        amufile->preprocessor.exported_decl.count+
        amufile->lexical_analyzer.imports.count
    );

    logger.log(Verbosity_StageParts, "Creating file module.");

    str8 modname = amuStr8("file_module(", amufile->file->name, ")");
    amufile->module = arena.make_module(modname);
    amufile->module->entity.type = Entity_Module;
    amufile->module->entity.internal_label = modname;
    amufile->module->entity.token_start = &amufile->lexical_analyzer.tokens[0];
    amufile->module->entity.token_end = &amufile->lexical_analyzer.tokens[amufile->lexical_analyzer.tokens.count-1];
    amufile->module->functions.init();
    amufile->table_root = arena.make_label_table();

    logger.log(Verbosity_Detailed, "Spawning threads to parse labels.");

    for(u32 idx : amufile->lexical_analyzer.labels) {
        threads.add(SyntaxAnalyzerThread());
        SyntaxAnalyzerThread* pt = &threads.data[threads.count-1];
        pt->cv = condition_variable_init();
        pt->curt = amufile->lexical_analyzer.tokens.readptr(idx);
        pt->syntax_analyzer = this;
        pt->node = (amuNode*)amufile->module;
        pt->stage = psLabel;
        threader_add_job({&semantic_analyzer_threaded_stub, pt});
    }   

    

    threader_wake_threads();

    forI(threads.count){
        while(!threads[i].finished){
            condition_variable_wait(&threads[i].cv);
        }
    }

}