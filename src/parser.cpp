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

#define perror_ret(token, ...){\
amufile->logger.error(token, __VA_ARGS__);\
return 0;\
}

#define pwarn(token, ...)\
amufile->logger.warn(token, __VA_ARGS__);


#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_next(...) if(next_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))



template<typename... T>
amuNode* ParserThread::binop_parse(amuNode* node, amuNode* ret, Type next_stage, T... tokchecks){
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
        Expression* rhs = ExpressionFromNode(define(&op->node, next_stage));

        out = &op->node;
    }
    return out;
}



amuNode* ParserThread::define(amuNode* node, Type stage){DPZoneScoped;
    //ThreadSetName(amuStr8("parsing ", curt->raw, " in ", curt->file));
    switch(stage){
        case psFile:{ //-------------------------------------------------------------------------------------------------File
            // i dont think this stage will ever be touched
        }break;

        case psDirective:{ //---------------------------------------------------------------------------------------Directive
            // there is no reason to check for invalid directives here because that is handled by lexer
            expect(Token_Directive_Import){
                b32 is_global = curt->is_global;
                Statement* s = arena.make_statement(curt->raw);
                curt++;
                expect(Token_OpenBrace){
                    curt++;
                    while(!curr_match(Token_CloseBrace)){
                        amuNode* n = define(&s->node, psImport);
                        if(!n) return 0;
                        expect(Token_Comma) {
                            curt++;
                            expect(Token_CloseBrace){curt++;break;}
                        }else expect(Token_CloseBrace) {curt++;break;}
                        else perror_ret(curt, "expected a , or a } after import block.");

                    }     
                }else{
                    amuNode* n = define(&s->node, psImport);
                    if(!n) return 0;
                }
                //NOTE(sushi) still not sure if we want to allow scoped importing
                //            this also doesnt work so im just commenting it out for now
                //if(is_global){
                    amufile->parser.import_directives.add(s);
                //}else{
                //    insert_last(node, &s->node);
                //}
            }

        }break;

        case psImport:{ //---------------------------------------------------------------------------------------------Import
            //#import 
            //TODO(sushi) cleanup the massive code duplication here and consider moving import evaluation (not parsing) to the next stage
            array<Declaration*> imported_decls;

            expect(Token_LiteralString){
                Expression* e = arena.make_expression(curt->raw);
                e->type = Expression_Literal;
                e->token_start = curt;
                e->token_end = curt;
                insert_last(node, &e->node);
                curt++;
                expect(Token_OpenBrace){
                    curt++;
                    while(!curr_match(Token_CloseBrace)){
                        expect(Token_Identifier){
                            Expression* id = arena.make_expression(curt->raw);
                            id->type = Expression_Identifier;
                            id->token_start = curt;
                            id->token_end = curt;
                            curt++;
                            expect(Token_As){
                                Expression* op = arena.make_expression(curt->raw);
                                op->type = Expression_BinaryOpAs;
                                op->token_start = id->token_start;
                                curt++;
                                expect(Token_Identifier){
                                    op->token_end = curt;
                                    Expression* idrhs = arena.make_expression(curt->raw);
                                    idrhs->type = Expression_Identifier;
                                    idrhs->token_start = curt;
                                    idrhs->token_end = curt;
                                    insert_last(&op->node, &id->node);
                                    insert_last(&op->node, &idrhs->node);
                                    insert_last(&e->node, &op->node);
                                    curt++;
                                }else perror_ret(curt, "expected an identifier for subimport alias.");
                            }else{
                                insert_last(&e->node, &id->node);
                            }
                            //this seems redundant
                            //i dont think we need to even check for commas in this situation
                            //but i prefer to enforce them for multiple items for consistency
                            expect(Token_Comma){
                                curt++;
                                expect(Token_CloseBrace){curt++;break;}
                            }else expect(Token_CloseBrace) {curt++;break;}
                            else perror_ret(curt, "expected a , after subimport.");
                        }else perror_ret(curt, "expected an identifier for subimport.");
                    }

                }
                return &e->node;
            }

           
        }break;

        case psRun:{ //---------------------------------------------------------------------------------------------------Run
            NotImplemented;
        }break;

        case psScope:{ //-----------------------------------------------------------------------------------------------Scope
            Scope* s = arena.make_scope(curt->raw);
            amuNode* me = &s->node;
            insert_last(node, me);
            while(!next_match(Token_CloseBrace)){
                curt++;
                expect(Token_Identifier){
                    if(curt->is_declaration){
                        amuNode* ret = define(me, psDeclaration);
                        if(!ret) return 0;
                        Declaration* d = DeclarationFromNode(ret);
                    }else{
                        define(me, psStatement);
                    }
                }else expect(Token_EOF){
                    perror_ret(curt, "unexpected end of file.");
                } else {
                    define(me, psStatement);
                }
            }
            curt++;
            return &s->node;
        }break;

        case psDeclaration:{ //-----------------------------------------------------------------------------------Declaration
            expect(Token_Identifier) {} else perror_ret(curt, "INTERNAL: parser stage psDeclaration started, but current token is not an identifier.");
            DPTracyDynMessage(toStr("declaring identifier ", curt->raw));
            switch(curt->decl_type){
                case Declaration_Structure:{
                    Struct* s = arena.make_struct(curt->raw);
                    Declaration* decl = &s->decl;

                    s->members.init();
                    s->operators.init();
                    s->conversions.init();
                    s->decl.token_start = curt;
                    s->decl.type = Declaration_Structure;
                    s->decl.identifier = curt->raw;
                    s->decl.declared_identifier = curt->raw;

                    if(curt->is_global){
                        if(is_internal) amufile->parser.internal_decl.add(&s->decl);
                        else            amufile->parser.exported_decl.add(&s->decl);
                    }
                    //TODO(sushi) since these expects arent linked to any alternatives they can just early out to reduce nesting
                    curt++;
                    expect(Token_Colon){
                        s->decl.colon_anchor = curt;
                        curt++;
                        expect(Token_StructDecl){
                            curt++;
                            expect(Token_OpenBrace){
                                while(!next_match(Token_CloseBrace)){
                                    curt++;
                                    expect(Token_Identifier){
                                        if(!curt->is_declaration){
                                            perror(curt, "only declarations are allowed inside struct definitions. NOTE(sushi) if this is a declaration, then this error indicates something wrong internally, please let me know.");
                                            return 0;
                                        }
                                        amuNode* fin = define(&s->decl.node, psDeclaration);
                                        if(!fin){
                                            return 0;
                                        } 
                                        Declaration* d = DeclarationFromNode(fin);
                                        s->members.add(d->identifier, &d->node);
                                    }
                                }
                            } else perror(curt, "expected a '{' after 'struct' in definition of struct '", s->decl.identifier, "'.");
                        } else perror(curt, "INTERNAL: expected 'struct' after ':' for struct definition. NOTE(sushi) tell me if this happens");
                    } else perror(curt, "INTERNAL: expected ':' for struct declaration. NOTE(sushi) tell me if this happens");

                    this->cv.notify_all();
                }break;

                case Declaration_Function:{
                    Function* f = arena.make_function(curt->raw);
                    f->args.init();
                    f->decl.token_start = curt;
                    f->decl.type = Declaration_Function;
                    f->decl.identifier = curt->raw;
                    f->decl.declared_identifier = curt->raw;
                    if(node) insert_last(node, &f->decl.node);
                    b32 is_global = curt->is_global;
                    str8 id = curt->raw;
                    f->internal_label = id;
                    f->internal_label = amuStr8(f->internal_label, "@");
                    curt++;
                    expect(Token_OpenParen){ // name(
                        //disable checking for semicolon after variable declaration
                        check_var_decl_semicolon = 0;
                        //TODO(sushi) god please clean this up
                        while(1){
                            curt++;
                            expect(Token_CloseParen) { break; }
                            else expect(Token_Identifier){
                                //its possible the user made an error that caused this token to not be marked as a declaration, but since 
                                //we know it should be here we manually mark it 
                                curt->decl_type = Declaration_Variable;
                                Variable* v = VariableFromNode(define(&f->decl.node, psDeclaration));
                                if(!v) return 0;
                                f->internal_label = amuStr8(f->internal_label, (v->decl.token_start + 2)->raw, ",");
                                forI(v->pointer_depth){
                                    f->internal_label = amuStr8(f->internal_label, STR8("*"));
                                }
                                expect(Token_Comma){
                                    expect_next(Token_CloseParen) perror_ret(curt,"trailing comma in function declaration.");
                                }else expect(Token_CloseParen){break;}
                                else perror_ret(curt, "expected a , or ) after function variable declaration.");
                            } else perror_ret(curt, "expected an identifier for function variable declaration.");
                        }
                        //reenable
                        check_var_decl_semicolon = 1;
                        curt++;
                        expect(Token_Colon){ // name(...) :
                            f->decl.colon_anchor = curt;
                            curt++;
                            //TODO(sushi) multiple return types
                            expect_group(TokenGroup_Type){ // name(...) : <type>
                                f->data.structure = builtin_from_type(curt->type);
                                f->internal_label = amuStr8(f->internal_label, "@", curt->raw);
                                if(is_global){
                                    if(is_internal) amufile->parser.internal_decl.add(&f->decl);
                                    else            amufile->parser.exported_decl.add(&f->decl);
                                }
                            }else expect(Token_Identifier){ // name(...) : <type>
                                //we are most likely referencing a struct type in this case
                                f->data.structure = 0;
                                f->internal_label = amuStr8(f->internal_label, "@", curt->raw);
                                if(curt->is_global){
                                    if(is_internal) amufile->parser.internal_decl.add(&f->decl);
                                    else            amufile->parser.exported_decl.add(&f->decl);
                                }
                            } else perror(curt, "expected a type specifier after ':' in function declaration.");
                            curt++;
                            expect(Token_OpenBrace) {
                                return define(&f->decl.node, psScope);
                            } else perror(curt, "expected '{' after function declaration.");
                        } else perror(curt, "expected : after function definition.");
                    } else perror(curt, "expected ( after identifier in function declaration.");

                }break;

                case Declaration_Variable:{ //name
                    Variable* v = arena.make_variable(amuStr8(curt->raw, " -decl"));
                    if(curt->is_global){
                        if(is_internal) amufile->parser.internal_decl.add(&v->decl);
                        else            amufile->parser.exported_decl.add(&v->decl);
                    }
                    v->decl.declared_identifier = curt->raw;
                    v->decl.identifier = curt->raw;
                    v->decl.token_start = curt;
                    v->decl.type = Declaration_Variable;
                    str8 id = curt->raw;
                    curt++;
                    expect(Token_Colon){ // name :
                        v->decl.colon_anchor = curt;
                        curt++;
                        expect_group(TokenGroup_Type){ // name : <type>
                            v->data.structure = builtin_from_type(curt->type);
                        }else expect(Token_Identifier){ // name : <type>
                            //do nothing, because the variable's structure will be filled out later
                        }else expect(Token_Assignment) v->data.implicit = 1;
                        else perror(curt, "expected a typename or assignment for variable declaration, found ", token_type_str(curt->type), " instead.");
                        curt++;
                        //parse pointer and array stuff 
                        while(1){
                            expect(Token_Multiplication){
                                v->data.structure = builtin_from_type(DataType_Ptr);
                                v->data.pointer_depth++;
                                curt++;
                            }else expect(Token_OpenSquare){
                                NotImplemented;
                            }else{
                                break;
                            }
                        }
                        expect(Token_Assignment){ // name : <type> = || name := 
                            Token* before = curt++;
                            Expression* e = ExpressionFromNode(define(&v->decl.node, psExpression));
                            if(!e) return 0;
                            curt++;
                        } else if(v->data.implicit) perror(curt, "Expected a type specifier or assignment after ':' in declaration of variable '", id, "'");
                        //NOTE(sushi) we do not do this check when we are parsing a function declarations arguments
                        if(check_var_decl_semicolon){
                            expect(Token_Semicolon){}
                            else perror_ret(curt, "expected ; after variable declaration.");
                        }
                        insert_last(node, &v->decl.node);
                        return &v->decl.node;
                    } else perror(curt, "Expected a ':' after identifier for variable decalration.");
                }break;
            }
        }break;

        case psStatement:{ //---------------------------------------------------------------------------------------Statement
            switch(curt->type){
                case Token_OpenBrace:{
                    define(node, psScope);
                }break;

                case Token_Using:{
                    Statement* s = arena.make_statement();
                    s->type = Statement_Using;
                    s->token_start = curt;
                    curt++;
                    expect(Token_Identifier){
                        Expression* e = arena.make_expression(curt->raw);
                        e->type = Expression_Identifier;
                        e->token_start = curt;
                        e->token_end = curt;
                        insert_last(&s->node, &e->node);
                        curt++;
                        expect(Token_Semicolon){}
                        else perror_ret(curt, "expected a semicolon after using statement");
                    } else perror_ret(curt, "expected a variable identifier after 'using'.");
                }break;

                case Token_Return:{
                    Statement* s = arena.make_statement(curt->raw);
                    s->type = Statement_Return;
                    s->token_start = curt;
                    insert_last(node, &s->node);
                    curt++;
                    expect(Token_Semicolon){
                        curt++;
                    }else{
                        amuNode* n = define(&s->node, psExpression);
                        if(!n) return 0;
                        change_parent(&s->node, n);
                    }
                    return &s->node;
                }break;

                case Token_If:{
                    Statement* s = arena.make_statement(curt->raw);
                    s->token_start = curt;
                    s->type = Statement_Conditional;
                    insert_last(node, &s->node);
                    curt++;
                    expect(Token_OpenParen){
                        curt++;
                        expect(Token_CloseParen) perror_ret(curt, "expected an expression for if statement.");
                        amuNode* n = define(&s->node, psExpression);
                        if(!n) return 0;
                        curt++;
                        expect(Token_CloseParen){
                            curt++;
                            expect(Token_OpenBrace){
                                n = define(&s->node, psScope);
                                if(!n) return 0;
                                s->token_end = curt;
                            }else{  
                                //check that the user isnt trying to declare anything in an unbraced if statement
                                expect(Token_Identifier)
                                    if(curt->is_declaration) perror_ret(curt, "can't declare something in an unscoped if statement.");
                                n = define(&s->node, psStatement);
                                if(!n) return 0;
                                curt++;
                                expect(Token_Semicolon){}
                                else perror_ret(curt, "expected a ;");
                                s->token_end = curt;
                            }

                        }else perror_ret(curt, "expected a ) after if statement's expression.");
                    }else perror_ret(curt, "expected a ( for if statement.");
                    return &s->node;
                }break;

                case Token_Else:{
                    Statement* s = arena.make_statement(curt->raw);
                    s->type = Statement_Else;
                    s->token_start = curt;
                    insert_last(node, &s->node);
                    curt++;
                    expect(Token_OpenBrace){
                        amuNode* n = define(&s->node, psScope);
                        if(!n) return 0;
                        s->token_end = curt;
                    }else{
                        //check that the user isnt trying to declare anything in an unscoped else statement
                        expect(Token_Identifier)
                            if(curt->is_declaration) perror_ret(curt, "can't declare something in an unscoped else statement.");
                        amuNode* n = define(&s->node, psStatement);
                        if(!n) return 0;
                        curt++;
                        expect(Token_Semicolon){}
                        else perror_ret(curt, "expected a ;");
                        s->token_end = curt;
                    }
                    return &s->node;
                }break; 

                case Token_Pound:{
                    TestMe;
                    curt++;
                    amuNode* n = define(node, psDirective);
                    if(!n) return 0;
                }break;

                default:{
                    //this is probably just some expression
                    Statement* s = arena.make_statement();
                    s->token_start = curt;
                    s->type = Statement_Expression;
                    insert_last(node, &s->node);
                    amuNode* ret = define(&s->node, psExpression);
                    if(!ret) return 0;
                    curt++;
                    s->token_end = curt;
                    expect(Token_Semicolon){}
                    else perror_ret(curt, "expected ; after statement.");

                }break;

            }
        }break;

        case psExpression:{ //-------------------------------------------------------------------------------------Expression
            expect(Token_Identifier){
                //go down expression chain first
                amuNode* n = define(node, psConditional);
                if(!n) return 0;
                Expression* e = ExpressionFromNode(n);

                expect_next(Token_Assignment){
                    curt++;
                    Expression* op = arena.make_expression(curt->raw);
                    op->type = Expression_BinaryOpAssignment;
                    change_parent(&op->node, &e->node);
                    curt++;
                    amuNode* ret = define(&op->node, psExpression);
                    insert_last(node, &op->node);
                    return &op->node;                                                
                }
                return n;
            }else{
                Expression* e = ExpressionFromNode(define(node, psConditional));
                return &e->node;
            }

        }break;

        case psConditional:{ //-----------------------------------------------------------------------------------Conditional
            expect(Token_If){
                NotImplemented;
            }else{
                Expression* e = ExpressionFromNode(define(node, psLogicalOR));
                return &e->node;
            }
        }break;

        case psLogicalOR:{ //---------------------------------------------------------------------------------------LogicalOR
            Expression* e = ExpressionFromNode(define(node, psLogicalAND));
            return binop_parse(node, &e->node, psLogicalAND, Token_OR);
        }break;

        case psLogicalAND:{ //-------------------------------------------------------------------------------------LogicalAND
            Expression* e = ExpressionFromNode(define(node, psBitwiseOR));
            return binop_parse(node, &e->node, psBitwiseOR, Token_AND);
        }break;

        case psBitwiseOR:{ //---------------------------------------------------------------------------------------BitwiseOR
            Expression* e = ExpressionFromNode(define(node, psBitwiseXOR));
            return binop_parse(node, &e->node, psBitwiseXOR, Token_BitOR);
        }break;

        case psBitwiseXOR:{ //-------------------------------------------------------------------------------------BitwiseXOR
            Expression* e = ExpressionFromNode(define(node, psBitwiseAND));
            return binop_parse(node, &e->node, psBitwiseAND, Token_BitXOR);
        }break;

        case psBitwiseAND:{ //-------------------------------------------------------------------------------------BitwiseAND
            Expression* e = ExpressionFromNode(define(node, psEquality));
            return binop_parse(node, &e->node, psEquality, Token_BitAND);
        }break;

        case psEquality:{ //-----------------------------------------------------------------------------------------Equality
            Expression* e = ExpressionFromNode(define(node, psRelational));
            return binop_parse(node, &e->node, psRelational, Token_NotEqual, Token_Equal);
        }break;

        case psRelational:{ //-------------------------------------------------------------------------------------Relational
            Expression* e = ExpressionFromNode(define(node, psBitshift));
            return binop_parse(node, &e->node, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
        }break;

        case psBitshift:{ //-----------------------------------------------------------------------------------------Bitshift
            Expression* e = ExpressionFromNode(define(node, psAdditive));
            return binop_parse(node, &e->node, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
        }break;

        case psAdditive:{ //-----------------------------------------------------------------------------------------Additive
            Expression* e = ExpressionFromNode(define(node, psTerm));
            return binop_parse(node, &e->node, psTerm, Token_Plus, Token_Negation);
        }break;

        case psTerm:{ //-------------------------------------------------------------------------------------------------Term
            Expression* e = ExpressionFromNode(define(node, psAccess));
            return binop_parse(node, &e->node, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
        }break;

        case psAccess:{
            Expression* e = ExpressionFromNode(define(node, psFactor));
            //dont use binop_parse because this stage is special
            while(next_match(Token_Dot)){
                curt++;
                Expression* op = arena.make_expression(curt->raw);
                op->type = Expression_BinaryOpMemberAccess;
                op->token_start = curt-1;

                change_parent(node, &op->node);
                change_parent(&op->node, &e->node);

                curt++;
                expect(Token_Identifier){
                    Expression* rhs = arena.make_expression(curt->raw);
                    rhs->token_start = curt;
                    rhs->token_end = curt;
                    rhs->type = Expression_Identifier;
                    insert_last(&op->node, &rhs->node);
                    e = op;
                }else perror_ret(curt, "expected an identifier for member access.");
            }
            return &e->node;
        }break;

        case psFactor:{ //---------------------------------------------------------------------------------------------Factor
            switch(curt->type){
                default:{
                    perror_ret(curt, "unexpected token '", curt->raw, "' found in expression.");
                }break;
                case Token_LiteralFloat:{
                    Expression* e = arena.make_expression(curt->raw);
                    e->token_start = curt;
                    e->type = Expression_Literal;
                    e->data.structure = builtin_from_type(DataType_Float64);
                    e->data.float64 = curt->f64_val;
                    insert_last(node, &e->node);
                    return &e->node;
                }break;

                case Token_LiteralInteger:{
                    Expression* e = arena.make_expression(curt->raw);
                    e->token_start = curt;
                    e->type = Expression_Literal;
                    e->data.structure = builtin_from_type(DataType_Signed64);
                    e->data.int64 = curt->s64_val;
                    insert_last(node, &e->node);
                    return &e->node;
                }break;

                case Token_OpenParen:{
                    curt++;
                    Token* start = curt;
                    amuNode* ret = define(node, psExpression);
                    curt++;
                    expect(Token_CloseParen){
                        return ret;
                    }else perror(curt, "expected a ) to close ( in line ", start->l0, " on column ", start->c0);
                }break;

                case Token_LessThan:{
                    Expression* e = arena.make_expression(STR8("cast"));
                    e->token_start = curt;
                    insert_last(node, &e->node);
                    curt++;
                    expect(Token_Identifier){
                        e->type = Expression_Cast;
                    }else expect_group(TokenGroup_Type){
                        e->type = Expression_Cast;
                    }else expect(Token_LogicalNOT){
                        e->type = Expression_Reinterpret;
                        e->node.debug = STR8("reinterpret");
                        curt++;
                        expect(Token_Identifier){}
                        else expect_group(TokenGroup_Type){}
                        else perror_ret(curt, "expected a typename for reinterpret cast");
                    }else perror_ret(curt, "expected a typename or ! for cast");
                    
                    Expression* id = arena.make_expression(curt->raw);
                    id->type = Expression_Identifier;
                    id->token_start = curt;
                    id->token_end = curt;
                    insert_last(&e->node, &id->node);
                    curt++;
                    expect(Token_GreaterThan){}
                    else perror_ret(curt, "expected a > to close cast");

                    curt++;

                    amuNode* n = define(&id->node, psExpression);
                    if(!n) return 0;
                    return &e->node;
                }break;

                case Token_Identifier:{
                    Expression* e = arena.make_expression();
                    e->token_start = curt;
                    expect_next(Token_OpenParen){
                        //this must be a function call
                        e->node.debug = amuStr8("call ", curt->raw);
                        e->type = Expression_FunctionCall;
                        curt++;
                        while(1){
                            curt++;
                            expect(Token_CloseParen) {break;}
                            Expression* arg = ExpressionFromNode(define(&e->node, psExpression));
                            if(!arg) return 0;
                            curt++;
                            expect(Token_Comma){}
                            else expect(Token_CloseParen) {break;}
                            else perror_ret(curt, "expected a , or ) after function argument.");
                        }
                        insert_last(node, &e->node);
                        e->token_end = curt;
                    }else{
                        //this is just some identifier
                        e->node.debug = curt->raw;
                        e->type = Expression_Identifier;
                        insert_last(node, &e->node);
                        expect_next(Token_Increment, Token_Decrement){
                            curt++;
                            Expression* incdec = arena.make_expression((curt->type == Token_Increment ? STR8("++") : STR8("--")));
                            incdec->type = (curt->type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix);
                            insert_last(node, &incdec->node);
                            change_parent(&incdec->node, &e->node);
                            return &incdec->node;
                        }
                    }
                    return &e->node;
                }break;

                
            }
        }break;
    }


    return 0;
}

void Parser::parse(){DPZoneScoped;
    Stopwatch time = start_stopwatch();
    amufile->logger.log(Verbosity_Stages, "Parsing...");
    SetThreadName("Parsing ", amufile->file->name);

    threads = array<ParserThread>(deshi_allocator);
    pending_globals.init();

   
    amufile->logger.log(Verbosity_StageParts, "Checking that imported files are parsed");

    threads.reserve(
        amufile->preprocessor.internal_decl.count+
        amufile->preprocessor.exported_decl.count+
        amufile->lexer.imports.count
    );
    
    
    for(u32 idx : amufile->lexer.imports){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->cv.init();
        pt->curt = amufile->lexer.tokens.readptr(idx);
        pt->parser = this;
        pt->node = &amufile->parser.base;
        pt->stage = psDirective;
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }
        
    for(u32 idx : amufile->preprocessor.internal_decl){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->cv.init();
        pt->curt = amufile->lexer.tokens.readptr(idx);
        pt->parser = this;
        pt->node = &amufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 1;
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }

    for(u32 idx : amufile->preprocessor.exported_decl){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->cv.init();
        pt->curt = amufile->lexer.tokens.readptr(idx);
        pt->parser = this;
        pt->node = &amufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 0;
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }

    amufile->logger.log(Verbosity_StageParts, "Waking threads");

    DeshThreadManager->wake_threads();

    forI(threads.count){
        while(!threads[i].finished){
            threads[i].cv.wait();
        }
    }

    amufile->logger.log(Verbosity_StageParts, "Joining nodes to base.");
    //join everything under one node. 
    //import nodes are not joined here because leaving them out keeps the tree clean
    //and we can just manually access them in validator
    forI(amufile->parser.internal_decl.count){
        change_parent(&amufile->parser.base, &amufile->parser.internal_decl.read(i)->node);
    }
    forI(amufile->parser.exported_decl.count){
        change_parent(&amufile->parser.base, &amufile->parser.exported_decl.read(i)->node);
    }
    
        
    amufile->logger.log(Verbosity_Stages, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
}