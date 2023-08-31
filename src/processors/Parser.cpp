/*

    NOTES
    -----

    When an Entity is created, eg. a Function, Place, etc. It is pushed onto the stack 
    *after* the actual node that represents it is placed


*/

namespace amu {

Parser* Parser::
create(Code* code) {
    Parser* out = pool::add(compiler::instance.storage.parsers);
    out->table.stack = array::init<LabelTable*>();
    out->table.last = 0;
    out->node.stack = array::init<ASTNode*>();
    out->node.current = 0;
    out->code = code;
    code->parser = out;
    out->token = code::TokenIterator(code);
    return out;
}

b32 Parser::
parse() {
    if(!prescan_start()) return false;
    if(!start()) return false;

    util::println(code->parser->root->print_tree());

    return true;
}

void Parser::
destroy() {
    array::deinit(table.stack);
    array::deinit(node.stack);
    pool::remove(compiler::instance.storage.parsers, this);
}

b32 Parser::
prescan_start() {
    switch(code->kind) {
        case code::source: {
            code::TokenIterator token(code);
            prescan_source();
        } break;
        case code::function: {
            // code::TokenIterator token = code::token_tokenator(code);
            // function(code, token);
        } break;
        case code::statement: {

        } break;
    }
    return true;
}

b32 Parser::
prescan_source() {
    // all source files are modules, so we create one for it here
    auto m = Module::create();
    table.push(&m->table);
    defer { table.pop(); };

    code->parser->root = m->as<ASTNode>();

    while(1) {
        switch(token.current_kind()) {
            case token::identifier: {
                prescan_label();
            } break;
        }
        if(token.increment()->kind == token::end_of_file) return true;
    }
}

b32 Parser::
prescan_module() {
    auto m = Module::create();
    m->table.last = table.last;
    table.push(&m->table);
    defer { table.pop(); };

    while(1) {
        switch(token.current_kind()) {
            case token::identifier: prescan_label(); break;
            default: return false;
        }
    }
}

b32 Parser::
prescan_label() {
    Token* id = token.current();
    switch(token.increment()->kind) {
        case token::colon: {
            switch(token.increment()->kind) {
                case token::colon: {
                    switch(token.increment()->kind) {
                        case token::open_paren: {
                            Token* open_paren = token.current(); 
                            token.skip_to_matching_pair();
                            switch(token.increment()->kind) {
                                case token::function_arrow: {
                                    token.skip_until(token::open_brace, token::semicolon);
                                    if(token.current_kind() == token::open_brace) {
                                        // this is a function definition, so we segment it into its own
                                        // Code object, create a Label and Function for it, construct the AST
                                        // for the Label and then push it onto whatever table we are 
                                        // currently working with
                                        token.skip_to_matching_pair();

                                        Code* nu = code::from(code, id, token.current());
                                        nu->kind = code::function;

                                        auto f = Function::create();
                                        f->start = open_paren;
                                        f->end = token.current();

                                        Label* l = Label::create();
                                        l->start = id;
                                        l->end = token.current();

                                        l->entity = (Entity*)f;
                                        f->label = l;

                                        auto e = Expr::create(expr::identifier);
                                        e->start = e->end = id;

                                        node::insert_first(l, e);
                                        
                                        nu->parser = Parser::create(nu);
                                        nu->parser->root = l->as<ASTNode>();

                                        table.add(id->raw, l);

                                        node::insert_last(code, nu);
                                    }
                                } break;
                                case token::semicolon: {
                                    // TODO(sushi) tuple constants
                                    NotImplemented;
                                } break;
                            }
                        } break;
                        case token::structdecl: {
                            if(token.increment()->kind != token::open_brace) {
                                diagnostic::parser::expected_open_brace(token.current());
                                return false;
                            }
                            Token* start = token.current();
                            token.skip_to_matching_pair();
                            
                            Code* nu = code::from(code, id, token.current());
                            nu->kind = code::typedef_;

                            Label* l = Label::create();
                            l->start = id;
                            l->end = token.current();

                            auto s = Structure::create();

                            auto stype = Structured::create(s);
                            stype->start = start;
                            stype->end =  token.current();

                            l->entity = (Entity*)stype;
                            stype->label = l;

                            auto e = Expr::create(expr::identifier);
                            e->start = e->end = id;

                            node::insert_first(l, e);

                            nu->parser = Parser::create(nu);
                            nu->parser->root = l->as<ASTNode>();

                            table.add(id->raw, l);

                            node::insert_last(code, nu);
                        } break;
                        case token::moduledecl: {
                            if(token.increment()->kind != token::open_brace) {
                                diagnostic::parser::expected_open_brace(token.current());
                                return false;
                            }
                            token.skip_to_matching_pair();
                            Code* nu = code::from(code, id, token.current());
                            nu->kind = code::module;
                            node::insert_last((TNode*)code, (TNode*)nu);
                            auto temp = code::TokenIterator(code);
                            prescan_module();
                        } break;
                        default: {
                            // this must be a compile time var declaration, so parse for an expression

                        } break;
                    }
                } break;
                case token::open_paren: {
                    // TODO(sushi) segment function variables
                    NotImplemented;
                } break;
            }
        } break;
        case token::comma: {
            // TODO(sushi) segmenting for multi labels
            NotImplemented; 
        } break;
    }
    return true;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
** ^^ ** ** ^^ ** ** ^^ ** ** ^^ ** ** ^^ ** ** ^^ ** ** ^^ ** ** ^^ ** ** ^^ 
 * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * 


                                Post-scan


 * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / * \ * / *  
 ** ,, ** ** ,, ** ** ,, ** ** ,, ** ** ,, ** ** ,, ** ** ,, ** ** ,, ** ** ,, 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

b32 Parser::
start() {
    Assert(code->parser, "a parser must have been made for a Code object created in the first stage.");
    switch(code->kind) {
        case code::function: {
            code::TokenIterator token(code);
            if(!prescanned_function()) return false;

            //util::println(code->parser->root->print_tree());
        } break;
        case code::typedef_: {
            code::TokenIterator token(code);
            if(!prescanned_type()) return false;
        } break;
        case code::source: {
            for(Code* c = code->first_child<Code>(); c; c = c->next<Code>()) {
                table.push(&code->parser->root->as<Module>()->table);
                if(!c->parser->parse()) return false;
                table.pop();
                node::insert_last(code->parser->root, c->parser->root);
            }
            // when we're done, we need to join all of the children's nodes into the source's
            // maybe 
        } break;
    }
    return true;
}


// parses a function definition, a function type followed by a block
// expects to be started at the opening parenthesis of the parameter tuple
// and returns with the token after the block's close brace
// leaves a typeref expression on the stack with the form:
// (expr:typeref
//      tuple         -- function parameters
//      tuple/typeref -- return information
//      block)        -- definition
// b32
// function(Code* code, code::TokenIterator& token) {
//     if(!tuple()) return false;
    
   

//     return true;
// }

// this function is called from ascent::start, so it only parses functions that 
// have been prescanned by the first stage of parsing. A Function entity and Label
// will have already been created
b32 Parser::
prescanned_function() {
    Label* l = (Label*)code->parser->root;
    Function* f = (Function*)l->entity;

    f->type = FunctionType::create();

    token.skip_until(token::open_paren);

    if(!tuple()) return false;

    auto e = node.pop()->as<Expr>();

    f->type = (FunctionType*)e->type;

    node::insert_last(l, e);

    return true;
}

b32 Parser::
prescanned_type() {
    Label* l = (Label*)code->parser->root;
    Type* type = (Type*)l->entity;

    switch(type->kind) {
        case type::kind::structured: {
            auto stype = (Structured*)type;
            Structure* s = stype->structure;
            
            token.skip_until(token::structdecl);
            if(!struct_decl()) return false;

            table.push(&s->table);

            auto e = (Expr*)node.pop();
            for(Label* n = e->first_child<Label>(); n; n = n->next<Label>()) {
                table.add(n->start->raw, n);
            }
            // reuse the typeref to place in the AST 
            e->type = type;
            node::insert_last(l, e);
        } break;
    }
    return true;
}


/*  parses a structure definition
    expected to be started at the 'struct' token
    leaves each declaration inside the struct on a typeref expression
    for example, the struct 
    
    struct {
        a: u32;
        b: u32;
    }

    will leave

    (expr:typeref
        (label<'a'> ...)
        (label<'b'> ...))

    on the stack
*/
b32 Parser::
struct_decl() {
    token.increment();

    if(!token.is(token::open_brace)) {
        diagnostic::parser::
            missing_open_brace_for_struct(token.current());
        return false;
    }

    token.increment();

    auto e = Expr::create(expr::typeref);
    
    while(1) {
        if(token.is(token::close_brace)) break;
        if(token.is(token::identifier)) {
            if(!label()) return false;
            if(!token.is(token::semicolon)) {
                diagnostic::parser::
                    missing_semicolon(token.current());
                return false;
            }
            token.increment();
            if(node.current->is<Function>()) {
                diagnostic::parser::
                    struct_member_functions_not_allowed(node.current->start);
                return false;
            }
        } else {
            diagnostic::parser::
                struct_only_labels_allowed(token.current());
            return false;
        }
        node::insert_last(e, node.pop());
    }

    node.push(e);
    return true;
}


// parses a label and whatever comes after it 
b32 Parser::
label() {
    if(!label_get()) return false;
    
    Label* l = (Label*)node.pop();
    
    table.add(l->start->raw, l);
    
    if(!label_after_colon()) return false;

    // TODO(sushi) all of this stuff should be condensed somehow 
    ASTNode* cand = node.pop();
    if(cand->is<Entity>()) switch(cand->as<Entity>()->kind) {
        case entity::expr: {
            auto expr = cand->as<Expr>();
            switch(expr->kind) {
                case expr::unary_assignment: {
                    auto v = Var::create();
                    v->label = l;
                    v->type = expr->type;
                    l->entity = v->as<Entity>();
                    node::insert_last(l, cand);
                } break;
                case expr::unary_comptime: {
                    // it's possible this a function def
                    switch(expr->type->kind) {
                        case type::kind::function: {
                            auto f = Function::create();
                            f->label = l;
                            f->type = expr->type->as<FunctionType>();
                            f->start = f->type->parameters->start;
                            l->entity = (Entity*)f;
                            node::insert_last(l, cand);
                        } break;
                    }
                } break;
                case expr::binary_assignment: {
                    auto v = Var::create();
                    v->label = l;
                    l->entity = v->as<Entity>();
                    node::insert_last(l, cand);
                } break;
                case expr::typeref: {
                    // this is probably when we have 
                    // <id> : <type> ;
                    Type* t = expr->type;
                    switch(t->kind) {
                        case type::kind::scalar:
                        case type::kind::structured: {
                            auto v = Var::create();
                            v->label = l;
                            v->type = t;
                            l->entity = v->as<Entity>();
                            node::insert_last(l, cand);
                        } break;

                        case type::kind::function: {
                            // if the last child of the typeref is not a block, then
                            // we do not create a function entity, this is simply a variable
                            // representing a function

                            if(!cand->last_child()->is<Block>()) {
                                auto v = Var::create();
                                v->label = l;
                                v->type = t;
                                l->entity = v->as<Entity>();
                                node::insert_last(l, cand);
                            } else {
                                auto f = Function::create();
                                f->label = l;
                                f->type = t->as<FunctionType>();
                                f->start = f->type->parameters->start;
                                l->entity = f->as<Entity>();
                                node::insert_last(l, cand);
                            }
                        } break;
                    }
                } break;
                default: {
                    util::println(
                        dstring::init("unhandled label case: ", expr::strings[expr->kind]));
                    return false;
                }
            }
        }
    } else {    
        Assert(0); // I don't think this should happen
    }

    l->start = l->first_child()->start;
    l->end = l->last_child()->end;

    node.push(l);
    return true;
}

// just parses <id> { "," <id> } : and pushes a label on the stack representing it 
// returns with the tokenator at the token following the colon
b32 Parser::
label_get() {
    auto expr = Expr::create(expr::identifier);
    expr->start = expr->end = token.current();
    node.push(expr);

    token.increment();
    switch(token.current_kind()) {
        case token::colon: break;
        case token::comma: {
            token.increment();
            if(!label_group_after_comma()) return false;
        } break;
        default: {
            diagnostic::parser::expected_colon_for_label(token.current());
            return false;
        } break;
    }

    auto l = Label::create();
    l->start = expr->start;
    node::insert_first(l, node.pop());
    node.push(l);
    token.increment();
    return true;
}


b32 Parser::
label_group_after_comma() {
     while(1) {
        if(!token.is(token::identifier)) break; 
        auto expr = Expr::create(expr::identifier);
        expr->start = expr->end = token.current();
        
        ASTNode* last = node.current;

        if(last->is<Tuple>()) {
            node::insert_last(last, expr);
            last->end = token.current();
        } else {
            // this is the second label, so the last must be another identifier
            // make the label group tuple
            auto group = Tuple::create();
            group->kind = tuple::label_group;
            node::insert_last(group, node.pop());
            node::insert_last(group, expr);
            group->start = group->first_child()->start;
            group->end = group->last_child()->end;
            node.push(group);
        }

        token.increment();
        
        if(token.is(token::comma)) token.increment(); 
        else break;
    }

    if(token.prev_is(token::comma)) {
        diagnostic::parser::
            label_group_missing_id(token.current());
    }

    if(token.is(token::colon)) {
        if(!label_after_colon()) return false;
    } else {
        diagnostic::parser::
            label_missing_colon(token.current());
        return false;
    }
    return true;
}


b32 Parser::
label_after_colon() {
    switch(token.current_kind()) {
        case token::identifier: {
            // this should be a typeref 
            Label* l = table.search(token.current()->hash);
            if(!l) {
                diagnostic::parser::
                    unknown_identifier(token.current());
                return false;
            }

            switch(l->entity->kind) {
                case entity::type: {
                    auto e = Expr::create(expr::typeref, l->entity->as<Type>());
                    e->start = e->end = token.current();

                    node.push(e);

                    token.increment();
                    if(!typeref()) return false;
                } break;
            }
        } break;

        case token::colon:
        case token::equal: {
            if(!expression()) return false;
        } break;

        case token::open_paren: {
            if(!tuple()) return false;
        } break;

        default: {
            if(token.current()->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                token.increment();
                typeref();
            } else {
                diagnostic::parser::unexpected_token(token.current(), token.current());
                return false;
            }
        } break;
    }
    return true;
}


// expects to be started at the opening parenthesis and returns 
// at the token following the closing parenthesis
// if the tuple is followed by '->', then this must be a function type
// that may be followed by a definition of a function. 
// In the case of a function definition the token is left at
// the closing brace
b32 Parser::
tuple() {
    Tuple* tuple = Tuple::create();
    tuple->start = token.current();

    token.increment();

    u32 count = 0;
    u32 found_label = 0;
    while(1) {
        if(token.is(token::close_paren)) break;
        switch(token.current_kind()) {
            case token::identifier: { // need to determine if this is a label or expression
                switch(token.next_kind()) {
                    case token::colon: {
                        if(!found_label) {
                            tuple->table.last = table.last;
                            table.push(&tuple->table);
                            found_label = 1;
                        }
                        if(!label()) return false;
                    } break;
                    default: {
                        if(found_label) {
                            diagnostic::parser::tuple_positional_arg_but_found_label(token.current());
                            return false;
                        }
                        if(!expression()) return false;
                    } break;
                }
            } break;
            default: {
                if(!expression()) return false;
            } break;
        }
        count += 1;
        if(token.is(token::comma)) token.increment();
        else if(!token.is(token::close_paren)) {
            diagnostic::parser::
                tuple_expected_comma_or_close_paren(token.current());
            return false;
        }
    }

    if(found_label)
        table.pop();
    

    forI(count) 
        node::insert_first(tuple, node.pop());

    tuple->end = token.current();

    token.increment();

    if(!token.is(token::function_arrow)){
        node.push(tuple);
        return true;
    } 

    // this must be a function type
    token.increment();

    count = 0;
    while(1) {
        if(!factor()) return false;
        count++;
        if(!token.is(token::comma)) break;
        token.increment();
    }

    if(!count) {
        diagnostic::parser::missing_function_return_type(token.current());
        return false;
    }

    Expr* e = Expr::create(expr::typeref);

    if(count > 1) {
        Tuple* t = Tuple::create();
        t->kind = tuple::multireturn;

        ScopedArray<Type*> types = array::init<Type*>();

        forI(count) {
            auto n = (Type*)node.pop();
            array::push(types, n);
            node::insert_first(t, n);
        }

        node::insert_last(e, t);

        t->start = t->first_child()->start;
        t->end = t->last_child()->start;
    } else {
        auto t = (Type*)node.pop();
        node::insert_first(e, t);
    }

    // push the argument tuple under the typeref we will be returning
    // and load its table onto the table stack for the block we're about to parse
    node::insert_first(e, tuple);

    if(token.is(token::open_brace)) {
        // the symbol table of the block will go through the tuple's symbol table, regardless of if
        // it generated one or not 
        tuple->table.last = table.last;
        table.push(&tuple->table);

        if(!block()) return false;

        table.pop();

        node::insert_last(e, node.pop());

        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
    }

    auto ft = FunctionType::create();
    ft->parameters = e->first_child();
    ft->returns = e->first_child()->next();
    ft->return_type = ft->returns->resolve_type(); // kind of redundant 
    e->type = (Type*)ft;

    node.push(e);

    return true;
}


b32 Parser::
expression() {
    switch(token.current_kind()) {
        case token::equal: {
            auto e = Expr::create(expr::unary_assignment);
            e->start = token.current();

            token.increment();
            if(!expression()) return false;

            node::insert_first(e, node.pop());
            e->end = e->last_child()->end;

            node.push(e);
        } break;

        case token::colon: {
            auto e = Expr::create(expr::unary_comptime);
            e->start = token.current();

            token.increment();

            if(!expression()) return false;

            node::insert_first(e, node.pop());
            e->end = e->last_child()->end;
            e->type = e->first_child<Expr>()->type;

            TODO("implement comptime code funneling");

            node.push(e);
        } break;

        case token::structdecl: {
            if(!struct_decl()) return false;
        } break;

        case token::moduledecl: {
            TODO("module decl parsing");
        } break;

        case token::return_: {
            auto e = Expr::create(expr::return_);
            e->start = token.current();

            token.increment();
            if(!expression()) return false;

            node::insert_first(e, node.pop());
            e->end = e->last_child()->end;

            node.push(e);
        } break;

        case token::using_: {
            TODO("using expression parsing");
        } break;

        default: if(!factor()) return false; 
    }

     // loop and see if any operators are being used, if so call their entry point
    b32 search = true;
    while(search) {
        switch(token.current_kind()) {
            case token::dot: if(!access()) return false; break;
            case token::asterisk:
            case token::solidus: if(!term()) return false; break;
            case token::plus:
            case token::minus: if(!additive()) return false; break;
            case token::double_less_than:
            case token::double_greater_than: if(!bit_shift()) return false; break;
            case token::double_equal: 
            case token::explanation_mark_equal: if(!equality()) return false; break;
            case token::less_than:
            case token::less_than_equal:
            case token::greater_than:
            case token::greater_than_equal: if(!relational()) return false; break;
            case token::ampersand: if(!bit_and()) return false; break;
            case token::caret: if(!bit_xor()) return false; break;
            case token::vertical_line: if(!bit_or()) return false; break;
            case token::double_ampersand: if(!logi_and()) return false; break;
            case token::logi_or: if(!logi_or()) return false; break;
            case token::equal: if(!assignment()) return false; break;
            //case token::open_brace: advance_curt(); if(!block()) return false; break;
            default: search = false;
        }
    }

    return true;
}


/*
    access: factor * { "." factor }
*/
b32 Parser::
access() { 
    if(token.current_kind() == token::dot) {
        token.increment();
        // when we parse accesses, we don't care about figuring out if the identifier is 
        // correct because we don't know what is being accessed and even if we did, it would
        // possibly not have been parsed yet 
        if(token.current_kind() != token::identifier) {
            diagnostic::parser::
                expected_identifier(token.current());
            return false;
        }

        auto id = Expr::create(expr::identifier);
        id->start = id->end = token.current();

        auto e = Expr::create(expr::binary_access);
        node::insert_first(e, node.pop());
        node::insert_first(e, id);
        node.push(e);
        token.increment();
        if(!access()) return false;
    }
    return true;
}


/*
    term: access * { ( "*" | "/" | "%" ) access }
*/
b32 Parser::
term() { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::percent: 
        case token::solidus: 
        case token::asterisk: {
            token.increment();
            if(!factor()) return false;
            if(!access()) return false;
            auto e = Expr::create(
                kind == token::percent ? expr::binary_modulo : 
                kind == token::solidus ? expr::binary_division : 
                    expr::binary_multiply);
            
            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            e->start = e->first_child()->start;
            e->end = e->last_child()->end;
            node.push(e);

            if(!term()) return false;
        } break;
    }
    return true;
}


/*
    additive: term * { ("+" | "-" ) term }
*/
b32 Parser::
additive() { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::plus:
        case token::minus: {
            token.increment();
            if(!factor()) return false;
            if(!access()) return false;
            if(!term()) return false;
            auto e = Expr::create(
                kind == token::plus 
                    ? expr::binary_plus 
                    : expr::binary_minus);

            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            e->start = e->first_child()->start;
            e->end = e->last_child()->end;
            node.push(e);

            if(!additive()) return false;
        } break;
    }
    return true;
}

/*
    bit-shift: additive { "<<" | ">>" additive }
*/
b32 Parser::
bit_shift() { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::double_less_than: 
        case token::double_greater_than: {
            token.increment(); 
            if(!factor()) return false;
            if(!access()) return false;
            if(!term()) return false;
            if(!additive()) return false;
            auto e = Expr::create(
                kind == token::double_less_than ? 
                      expr::binary_bit_shift_left :
                      expr::binary_bit_shift_right);
            
            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            e->start = e->first_child()->start;
            e->end = e->last_child()->end;
            node.push(e);

            if(!bit_shift()) return false;
        } break;
    }
    return true;
}

/*
    relational: bit-shift { ( ">" | "<" | "<=" | ">=" ) bit-shift }
*/
b32 Parser::
relational() { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::less_than:
        case token::less_than_equal:
        case token::greater_than:
        case token::greater_than_equal: {
            token.increment();
            if(!factor()) return false;
            if(!access()) return false;
            if(!term()) return false;
            if(!additive()) return false;
            if(!bit_shift()) return false;
            auto e = Expr::create(
                kind == token::less_than ?
                      expr::binary_less_than :
                      kind == token::less_than_equal ? 
                      expr::binary_less_than_or_equal :
                      kind == token::greater_than ? 
                      expr::binary_greater_than :
                      expr::binary_greater_than_or_equal);

            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            e->start = e->first_child()->start;
            e->end = e->last_child()->end;
            node.push(e);

            if(!relational()) return false;
        } break;
    }
    return true;
}

/*
    equality: relational { ( "!=" | "==" ) relational }
*/
b32 Parser::
equality() { 
    token::kind kind = token.current_kind();
    switch(kind) {
        case token::double_equal:
        case token::explanation_mark_equal: {
            token.increment();
            if(!factor()) return false;
            if(!access()) return false;
            if(!term()) return false;
            if(!additive()) return false;
            if(!bit_shift()) return false;
            if(!relational()) return false;
            auto e = Expr::create(
                kind == token::double_equal 
                    ? expr::binary_equal 
                    : expr::binary_not_equal);

            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            e->start = e->first_child()->start;
            e->end = e->last_child()->end;
            node.push(e);

            if(!equality()) return false;
        } break;
    }
    return true;
}

/*
    bit-and: equality { "&" equality } 
*/
b32 Parser::
bit_and() { 
    if(token.current_kind() == token::ampersand) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        auto e = Expr::create(expr::binary_bit_and);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
        node.push(e);

        if(!bit_and()) return false;
    }
    return true;
}

/*
    bit-xor: bit-and { "^" bit-and }
*/
b32 Parser::
bit_xor() { 
    if(token.current_kind() == token::caret) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        if(!bit_and()) return false;
        auto e = Expr::create(expr::binary_bit_xor);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
        node.push(e);

        if(!bit_xor()) return false;
    }
    return true;
}

/*
    bit-or: bit-xor { "|" bit-xor }
*/
b32 Parser::
bit_or() { 
    if(token.current_kind() == token::vertical_line) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        if(!bit_and()) return false;
        if(!bit_xor()) return false;
        auto e = Expr::create(expr::binary_bit_or);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
        node.push(e);

        if(!bit_or()) return false;
    }
    return true;
}

/*
    logi-and: bit-or { "&&" bit-or }
*/
b32 Parser::
logi_and() { 
    if(token.current_kind() == token::double_ampersand) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        if(!bit_and()) return false;
        if(!bit_xor()) return false;
        if(!bit_or()) return false;
        auto e = Expr::create(expr::binary_and);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
        node.push(e);

        if(!logi_and()) return false;
    }
    return true;
}

/*
    logi-or: logi-and { "||" logi-and }
*/
b32 Parser::
logi_or() { 
    if(token.current_kind() == token::logi_or) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        if(!bit_and()) return false;
        if(!bit_xor()) return false;
        if(!bit_or()) return false;
        if(!logi_and()) return false;
        auto e = Expr::create(expr::binary_or);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;
        node.push(e);

        if(!logi_or()) return false;
    }
    return true;
}

b32 Parser::
assignment() {
    if(token.is(token::equal)) {
        token.increment();
        if(!factor()) return false;
        if(!access()) return false;
        if(!term()) return false;
        if(!additive()) return false;
        if(!bit_shift()) return false;
        if(!relational()) return false;
        if(!equality()) return false;
        if(!bit_and()) return false;
        if(!bit_xor()) return false;
        if(!bit_or()) return false;
        if(!logi_and()) return false;
        if(!logi_or()) return false;
        Expr* e = Expr::create(expr::binary_assignment);

        node::insert_first(e, node.pop());
        node::insert_first(e, node.pop());
        e->start = e->first_child()->start;
        e->end = e->last_child()->end;

        node.push(e);

        if(!assignment()) return false;
    }
    return true;
}

b32 Parser::
conditional() {
    Expr* e = Expr::create(expr::conditional);
    e->start = token.current();

    token.increment();

    if(!token.is(token::open_paren)) {
        diagnostic::parser::
            if_missing_open_paren(token.current());
        return false;
    }

    token.increment();

    if(!expression()) return false;

    node::insert_last(e, node.pop());

    if(!token.is(token::close_paren)) {
        diagnostic::parser::
            if_missing_close_paren(token.current());
        return false;
    }

    token.increment();

    if(!expression()) return false;

    node::insert_last(e, node.pop());

    // check if the body of this if is returning something
    auto body = e->first_child<Expr>();
    if( !body->is<Block>() ||
        body->child_count && body->last_child()->is(stmt::block_final)) {
        e->flags.conditional.returning = true;
    }

    node.push(e);

    if(token.is(token::else_)) {
        token.increment();
        if(!expression()) return false;
        node::insert_last(e, node.pop());
    }

    e->end = token.current();

    return true;
}

b32 Parser::
loop() {
    auto e = Expr::create(expr::loop);
    e->start = token.current();

    token.increment();

    if(!expression()) return false;
    e->end = token.current();

    node::insert_last(e, node.pop());

    node.push(e);

    return true;
}

b32 Parser::
factor() {
    switch(token.current_kind()) {
        case token::identifier: {
            Label* l = table.search(token.current()->hash);
            if(!l) {
                diagnostic::parser::unknown_identifier(token.current());
                return false;
            }

            switch(l->entity->kind) {
                case entity::var: {
                    auto e = VarRef::create();
                    e->start = e->end = token.current();
                    e->var = l->entity->as<Var>();
                    token.increment();
                    node.push(e);
                } break;
                case entity::func: {
                    if(token.next_is(token::open_paren)) {
                        // must be a function call
                        auto id = Expr::create(expr::identifier);
                        id->start = id->end = token.current();
                        auto call = Call::create();
                        call->start = token.current();
                        call->callee = l->entity->as<Function>();
                        token.increment();
                        if(!tuple()) return false;
                        call->end = token.current();
                        call->arguments = node.pop()->as<Tuple>();
                        node::insert_last(call, id);
                        node::insert_last(call, call->arguments);
                        node.push(call);
                    } else {
                        TODO("plain function reference");
                        NotImplemented;
                    }
                } break;
                default: {
                    util::println(dstring::init(
                        token.current(), "unhandled identifier reference: ", ast::strings[l->entity->kind]));
                } break;
            }

        } break;
        
        case token::open_paren: {
            if(!tuple()) return false;

            if(node.current->is<Tuple>() && 
               node.current->child_count == 1 &&
               node.current->first_child()->is<Expr>()) {
                // if we come across a single value tuple that only holds an expression, we take it
                // as an expression wrapped in parenthesis
                // this may mess up some stuff down the road but it should work for now 

                auto n = node.pop();
                node.push(n->first_child());
                n->as<Tuple>()->destroy();
            }
        } break;

        case token::if_: {
            if(!conditional()) return false;
        } break;

        case token::loop: {
            if(!loop()) return false;
        } break;

        case token::switch_: {
            TODO("switch parsing");
        } break;

        case token::for_: {
            TODO("for parsing");
        } break;

        case token::open_brace: {
            if(!block()) return false;
        } break;

        case token::ampersand: {
            auto e = Expr::create(expr::unary_reference);
            e->start = token.current();

            token.increment();
            if(!factor()) return false;

            node::insert_last(e, node.pop());
            e->type = Pointer::create(e->last_child()->resolve_type());
            e->end = e->last_child()->end;
            node.push(e);
        } break;

        default: {
            if(token.current()->group == token::group_literal) {
                reduce_literal_to_literal_expression();
                token.increment();
            } else if(token.current()->group == token::group_type) {
                reduce_builtin_type_to_typeref_expression();
                token.increment();
            } else {
                diagnostic::parser::unexpected_token(token.current(), token.current());
                return false;
            }
        } break;
    }
    return true;
}

// expects to start at the opening brace and returns after the closing brace
// TODO(sushi) it may be easier (or necessary) to just do a scan to the matching brace
//             when parsing blocks, instead of the weird logic we try to do 
//             to allow omitting semicolons after labels that end with blocks 
b32 Parser::
block() {
    auto e = Block::create();
    e->start = token.current();
    e->table.last = table.last;
    table.push(&e->table);

    token.increment();

    b32 break_air_gen = false;

    u32 count = 0;
    while(1) {
        b32 need_semicolon = true;
        b32 last_expr = false;
        if(token.is(token::close_brace)) break;
        if(token.is(token::semicolon)){ // empty statements are fine, but ignored
            token.increment(); 
            continue; 
        }
        auto s = Stmt::create();
        s->kind = stmt::unknown;
        if(break_air_gen) {
            s->flags.break_air_gen = true;
            break_air_gen = false;
        }

        switch(token.current_kind()) {
            case token::identifier: {
                if(token.next_kind() == token::colon) {
                    if(!label()) return false;
                    s->kind = stmt::label;
                    if(token.prev_is(token::close_brace)) {
                        need_semicolon = false;
                    }
                } else {
                    if(!expression()) return false;
                    s->kind = stmt::expression;
                    // TODO(sushi) these rules are sketchy and i feel like they will break at some point 
                    if(token.prev_is(token::close_brace)) {
                        need_semicolon = false;
                        if(token.is(token::close_brace))
                            last_expr = true;
                    } else if(token.is(token::close_brace)) {
                        need_semicolon = false;
                        last_expr = true;
                    }
                }
            } break;
            case token::directive_compiler_break_air_gen: {
                break_air_gen = true;
                s->destroy();
                token.increment();
                if(!token.is(token::semicolon)) {
                    diagnostic::parser::missing_semicolon(token.current());
                    return false;
                }
                continue;
            } break;
            default: {
                if(!expression()) return false;
                s->kind = stmt::expression;
                // TODO(sushi) these rules are sketchy and i feel like they will break at some point 
                if(token.prev_is(token::close_brace)) {
                    need_semicolon = false;
                    if(token.is(token::close_brace))
                        last_expr = true;
                } else if(token.is(token::close_brace)) {
                    need_semicolon = false;
                    last_expr = true;
                }
            } break;
        }
        if(need_semicolon && !token.is(token::semicolon)) {
            if(!token.is(token::close_brace)) {
                diagnostic::parser::missing_semicolon(token.current());
                return false;
            }
            count++;
            node::insert_first(s, node.pop());
            s->start = s->first_child()->start;
            s->end = token.current();
            s->kind = stmt::block_final;
            node.push(s);
            break;
        } else {
            node::insert_first(s, node.pop());
            s->start = s->first_child()->start;
            s->end = token.current();

            count++;
            node.push(s);
            
            if(last_expr) {
                s->kind = stmt::block_final;
                break;
            } else if(need_semicolon) token.increment();
        }
    }

    forI(count) {
        node::insert_first(e, node.pop());
    }
    e->end = token.current();
    token.increment();
    node.push(e);
    table.pop();
    return true;
}

b32 Parser::
typeref() {
    Token* start = token.current();
    switch(token.current_kind()) {
        case token::equal: {
            token.increment();
            if(!expression()) return false;

            Expr* e = Expr::create(expr::binary_assignment);
            e->start = start;
            e->end = token.current();
            node::insert_first(e, node.pop());
            node::insert_first(e, node.pop());
            // always take the type of the lhs because it controls the type of the rhs expression
            e->type = e->first_child<Expr>()->type;
            node.push(e);
        } break;

        case token::open_brace: {
            TODO("type restricted blocks <typeref> \"{\" ... \"}\"");
        } break;

        case token::asterisk: {
            auto last = node.current->as<Expr>();
            last->type = Pointer::create(last->type);
            token.increment();
            if(!typeref()) return false;
        } break;
    }

    return true;
}

b32 Parser::
reduce_literal_to_literal_expression() {
    auto e = Expr::create(expr::literal);
    e->start = e->end = token.current();
    switch(token.current_kind()) {
        case token::literal_character: {
            e->type = &scalar::scalars[scalar::unsigned8];
        } break;
        case token::literal_float: {
            e->type = &scalar::scalars[scalar::float64];
        } break;
        case token::literal_integer: {
            e->type = &scalar::scalars[scalar::signed64];
        } break;
        case token::literal_string: {
            e->type = StaticArray::create(&scalar::scalars[scalar::unsigned8], token.current()->raw.count);
        } break;
    }
    node.push(e);
    return true;
}

b32 Parser::
reduce_builtin_type_to_typeref_expression() { 
    Expr* e = Expr::create(expr::typeref);
    e->start = e->end = token.current();
    switch(token.current_kind()) {
        case token::void_:      e->type = &type::void_; break; 
        case token::unsigned8:  e->type = &scalar::scalars[scalar::unsigned8]; break;
        case token::unsigned16: e->type = &scalar::scalars[scalar::unsigned16]; break;
        case token::unsigned32: e->type = &scalar::scalars[scalar::unsigned32]; break;
        case token::unsigned64: e->type = &scalar::scalars[scalar::unsigned64]; break;
        case token::signed8:    e->type = &scalar::scalars[scalar::signed8]; break;
        case token::signed16:   e->type = &scalar::scalars[scalar::signed16]; break;
        case token::signed32:   e->type = &scalar::scalars[scalar::signed32]; break;
        case token::signed64:   e->type = &scalar::scalars[scalar::signed64]; break;
        case token::float32:    e->type = &scalar::scalars[scalar::float32]; break;
        case token::float64:    e->type = &scalar::scalars[scalar::float64]; break;
    }
    node.push(e);
    return true;
}

FORCE_INLINE void Parser::NodeStack::
push(ASTNode* n) { 
    array::push(stack, current); 
    current = n; 
}

FORCE_INLINE ASTNode* Parser::NodeStack::
pop() { 
    ASTNode* save = current; 
    current = array::pop(stack); 
    return save; 
} 

void Parser::TableStack::
push(LabelTable* l) {
    array::push(stack, last); 
    last = l;

}
void Parser::TableStack::
pop() { 
    last = array::pop(stack); 
}

Label* Parser::TableStack::
search(u64 hashed_id) { 
    LabelTable* table = last;
    while(table) {
        auto [idx, found] = map::find(table->map, hashed_id);
        if(found) {
            return array::read(table->map.values, idx);
        }
        table = table->last;
    }
    return 0;
}

void Parser::TableStack::
add(String id, Label* l) {
    map::add(last->map, id, l);
}

DString Parser::
display_stack() {
    if(!node.stack.count) return dstring::init("empty stack");
    DString out = dstring::init("stack of ", code->identifier, ":\n");
    forI(node.stack.count) {
        if(!i) continue; // first element is always 0 due to storing the last element separate
        ASTNode* n = array::read(node.stack, i);
        dstring::append(out, n->debug_str(), "\n");
    }
    dstring::append(out, node.current->debug_str());
    return out;
}

} // namespace amu