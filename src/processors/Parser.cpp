/*

	NOTES
	-----

	When an Entity is created, eg. a Function, Place, etc. It is pushed onto the stack 
	*after* the actual node that represents it is placed


*/

#include "representations/Code.h"
#include "representations/Expr.h"
#include "representations/Token.h"
#include <thread>
namespace amu {

Parser* Parser::
create(Code* code) {
	Parser* out = pool::add(compiler::instance.storage.parsers);
	out->table.stack = Array<LabelTable*>::create(128);
	out->table.current = 0;
	out->node.stack = Array<ASTNode*>::create(128);
	out->node.current = 0;
	out->code = code; 
	code->parser = out;
	out->token = code::TokenIterator(code);
	return out;
}

void Parser::
destroy() {
	table.stack.destroy();
	node.stack.destroy();
	pool::remove(compiler::instance.storage.parsers, this);
}

b32 Parser::
parse() {
	if(code->kind == code::module || code->kind == code::source) {
		return discretize_module();
	}
	if(!start()) return false;

	// util::println(ScopedDeref(code->parser->root->print_tree()).x);

	return true;
}

b32 Parser::
discretize_module() {
	messenger::qdebug(code, String("discretizing supposed module"));

	auto m = Module::create();
	table.push(m->table);
	defer { table.pop(); };

	code->parser->root = m->as<ASTNode>();
	
	auto save = token.curt;

	auto ls = token.current()->scope;
	// TODO(sushi) this doesn't work for subcode objects
	//			   or label groups 
	auto tokens = code->get_token_array();
	forI(ls->labels.count) {
		token.curt = tokens.readptr(ls->labels.read(i));
		if(table.search_local(token.current()->hash)) {
			diagnostic::parser::
				label_already_defined(token.current(), token.current());
			return false;
		}
		
		if(!label_get()) return false;

		auto l = node.pop()->as<Label>();
		table.add(l->start->raw, l);
		l->table = table.current;

		auto c = Code::from(code, l->start, tokens.readptr(-1));
		c->kind = code::label;
		c->state = code::post_lex;

		Parser::create(c);
		c->parser->root = l;

		l->code = c;
		node::insert_last(code, c);
	}

	messenger::qdebug(code, String("finished discretizing module"));
	return true;
}

b32 Parser::
start() {
	messenger::qdebug(code, String("starting parser"));
	Assert(code->parser, "a parser must have been made for a Code object created in the first stage.");
	switch(code->kind) {
		case code::label: {
			auto l = code->parser->root->as<Label>();
			table.push(l->table);
			if(!prescanned_label()) return false;
			table.pop();
			// adjust the Code object's kind to whatever the label ends up representing
			switch(l->entity->kind) {
				case entity::func: {
					code->kind = code::function;
					l->entity->as<Function>()->code = code;
				} break;
				case entity::var: {
					code->kind = code::var_decl;
				} break;
			}
		} break;
		case code::function: {
			if(!prescanned_function()) return false;
		} break;
		case code::typedef_: {
			if(!prescanned_type()) return false;
		} break;
		case code::var_decl: {
			if(!prescanned_var_decl()) return false;
		} break;
		case code::source: {
			Assert(0, "shouldn't reach this point but i need to see if there are any instances where we do");
			if(discretize_module()) return false;

			auto futures = Array<Future<b32>>::create();
			for(Code* c = code->first_child<Code>(); c; c = c->next<Code>()) {
				c->parser->table.push(code->parser->root->as<Module>()->table);
				futures.push(c->process_to_async(code::air));
				c->parser->table.pop();
				node::insert_last(code->parser->root, c->parser->root);
			}
			forI(futures.count) {
				if(!futures.read(i).get()) return false;
			}
		} break;
	}
	return true;
}

b32 Parser::
prescanned_label() {
	auto l = code->parser->root->as<Label>();
	token.increment();
	token.increment();
	if(!expression()) return false;
	node::insert_last(l, node.pop());
	switch(l->last_child<Expr>()->kind) {
		case expr::function: {
			auto e = l->last_child<Expr>();
			auto f = Function::create();
			f->type = e->first_child<Expr>()->type->as<FunctionType>();
			l->entity = f;
			f->label = l;
			f->def = e;
			f->code = code;
			code->kind = code::function;
		} break;
		case expr::typedef_: {
			auto e = l->last_child<Expr>();
			auto s = Structure::create();
			auto stype = Structured::create(s);
			stype->code = code;
			stype->def = e;

			u64 offset = 0;
			for(Member* m = e->first_child<Member>(); m; m = m->next<Member>()) {
				// TODO(sushi) this is probably very inefficient
				if(s->find_member(m->start->raw)) {
					diagnostic::parser::
						struct_duplicate_member_name(m->start, m->start);
					return false;
				}
				s->add_member(m->label->start->raw, m);
			}

			stype->def = e;

			e->type = stype;
			l->entity = stype;
			stype->label = l;
			code->kind = code::typedef_;
		} break;

		default: {
			// this must just be some variable
			auto v = Var::create();
			v->start = l->start;
			v->end = l->end;
			v->label = l;
			l->entity = v;
			v->code = code;
			code->kind = code::var_decl;
		} break;
	}
	return true;
}

b32 Parser::
prescanned_function() {
	Label* l = (Label*)code->parser->root;
	Function* f = (Function*)l->entity;

	token.skip_until(token::open_paren);

	if(!tuple()) return false;

	auto e = node.pop()->as<Expr>();

	while(e->first_child()) {
		node::change_parent(f, e->first_child());
	}

	node::insert_last(l, f);
 
	// e->destroy(); // this sucks 

	f->type = f->first_child<Expr>()->type->as<FunctionType>();

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

			u64 offset = 0;

			auto e = (Expr*)node.pop();
			for(Member* m = e->first_child<Member>(); m; m = m->next<Member>()) {
				// TODO(sushi) this is probably very inefficient
				if(s->find_member(m->start->raw)) {
					diagnostic::parser::
						struct_duplicate_member_name(m->start, m->start);
					return false;
				}
				s->add_member(m->label->display(), m);
			}

			stype->def = e;

			// reuse the typedef to place in the AST 
			e->type = type;
			node::insert_last(l, e);
		} break;
	}
	return true;
}

b32 Parser::
prescanned_var_decl() {
	auto l = code->parser->root->as<Label>();
	auto v = l->entity->as<Var>();

	token.increment();
	token.increment();

	if(!expression()) return false;

	auto e = node.pop()->as<Expr>();

	if(!token.is(token::semicolon)) {
		if(!token.is(token::close_brace)) {
			diagnostic::parser::
				missing_semicolon(token.current());
			return false;
		}
	}

	if(e->is<CompileTime>()) {
		v->is_compile_time = true;
		v->type = e->type;
		v->memory = (u8*)memory::allocate(v->type->size());
		memory::copy(v->memory, e->code->machine->stack, v->type->size());
		e->code->machine->destroy();
		node::change_parent(l, e->last_child());
	} else {
		// we still need to bring this variable into memory since it's global
		v->is_global = true;
		// and also evaluate it too, so that it is initialized 
		code->level = code::parse;
		node::change_parent(l, e);
		if(!code->process_to(code::sema)) return false;
		v->type = e->type;
		v->memory = (u8*)memory::allocate(v->type->size());
		map::add(compiler::instance.global_symbols, v->memory, v);
		if(!code->process_to(code::vm)) return false;
		code->machine->destroy();
	}
	return true;
}

b32 Parser::
struct_decl() {
	token.increment();

	if(!token.is(token::open_brace)) {
		diagnostic::parser::
			missing_open_brace_for_struct(token.current());
		return false;
	}

	token.increment();

	auto e = Expr::create(expr::typedef_);
	e->start = token.current();

	u64 count = 0;
	while(1) {
		if(token.is(token::close_brace)) break;
		if(token.is(token::identifier)) {
			auto mem = Member::create();
			mem->index = count++;
			mem->start = token.current();

			// just retrieve the label
			if(!label_get()) return false;

			auto l = node.pop()->as<Label>();
			mem->label = l;
			l->entity = mem;

			node::insert_last(mem, l);

			if(!expression()) return false;

			auto expr = node.pop()->as<Expr>();
			mem->type = expr->type;

			node::insert_last(mem, expr);

			if(!token.is(token::semicolon)) {
				diagnostic::parser::
					missing_semicolon(token.current());
				return false;
			}
			token.increment();

			if(expr->is(expr::func_def)) {
				diagnostic::parser::
					struct_member_functions_not_allowed(l->start);
				return false;
			}

			mem->end = token.current();

			node::insert_last(e, mem);
		} else {
			diagnostic::parser::
				struct_only_labels_allowed(token.current());
			return false;
		}
	}

	e->end = token.current();


	node.push(e);
	return true;
}

// A label does not create its entity
// because it's difficult for us to know 
// what context a label is being created in.
// It is up to whoever calls this to determine 
// what the label should be representing 
// and create it there.
b32 Parser::
label() {
	auto start = token.current();

	if(!label_get()) return false;
	
	auto l = node.pop()->as<Label>();
	
	if(table.search_local(l->start->hash)) {
		diagnostic::parser::
			label_already_defined(start, start);
		return false;
	}

	table.add(l->start->raw, l);
	
	if(!label_after_colon()) return false;

	// TODO(sushi) all of this stuff should be condensed somehow
	//			   all this does is create entities for certain
	//			   things that can appear after a label
	//			   this can probably be replaced by a ruleset/table
	ASTNode* cand = node.pop();
	if(cand->is_not<Entity>()) {
		TODO("a label returned to a node that is not an Entity"); 
	}

	node::insert_last(l, cand);

	l->start = l->first_child()->start;
	l->end = l->last_child()->end;

	node.push(l);
	return true;
}

// just parses <id> { "," <id> } : and pushes a label on the stack representing it 
// returns with the iterator at the token following the colon
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
			diagnostic::parser::
				expected_colon_for_label(token.current());
			return false;
		} break;
	}

	auto l = Label::create();
	l->start = expr->start;
	l->end = token.current();
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
		return false;
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
		case token::colon:
		case token::equal: {
			if(!expression()) return false;
		} break;

		case token::open_paren: {
			if(!expression()) return false;
			if(node.current->is<Tuple>()) {
				auto e = TupleLiteral::create();
				node::insert_last(e, node.pop());
				node.push(e);
				e->def = node.current;
			}
		} break;

		case token::structdecl: {
			diagnostic::parser::
				runtime_structures_not_allowed(token.current());
			return false;
		} break;

		default: {
			if(token.current()->group == token::group_type) {
				reduce_builtin_type_to_typeref_expression();
				token.increment();
				typeref();
			} else {
				// just expect an expression and hope for the best
				// if something's wrong here, it will probably be 
				// caught in Sema or in label() when we're checking
				// what we got after it 
				if(!expression()) return false;
				// diagnostic::parser::unexpected_token(token.current(), token.current());
				// return false;
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
	auto tuple = Tuple::create();
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
							tuple->table->last = table.current;
							table.push(tuple->table);
							found_label = 1;
						}
						if(!label()) return false;
						// what this label represents is decided later once we 
						// determine if this tuple is acting as the parameter 
						// list of a function
					} break;
					default: {
						goto expr;
					} break;
				}
			} break;
			default: {
expr:
				if(found_label) {
					diagnostic::parser::
						tuple_positional_arg_but_found_label(token.current());
					return false;
				}
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
		// all labels created inside of this tuple are assumed to point to expressions
		// so we just point them directly at what came after them
		forI(tuple->table->map.values.count) {
			auto l = tuple->table->map.values.read(i);
			l->entity = l->last_child<Entity>();
		}
		return true;
	} 

	// if this is a function parameter list, then any label must refer
	// to a variable
	forI(tuple->table->map.values.count) {
		auto l = tuple->table->map.values.read(i);
		auto v = Var::create();
		v->label = l;
		l->entity = v;
		v->start = l->start;
		v->end = l->end;
		v->def = l->last_child();
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
		diagnostic::parser::
			missing_function_return_type(token.current());
		return false;
	}

	auto e = Expr::create(expr::typeref);

	if(count > 1) {
		auto t = Tuple::create();
		t->kind = tuple::multireturn;

		ScopedArray<Type*> types = Array<Type*>::create();

		forI(count) {
			auto n = (Type*)node.pop();
			types.push(n);
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

	auto ft = FunctionType::create();
	ft->parameters = e->first_child();
	ft->returns = e->first_child()->next();
	ft->return_type = ft->returns->resolve_type(); // kind of redundant 
	ft->def = e;
	e->type = (Type*)ft;

	if(token.is(token::open_brace)) {
		// the symbol table of the block will go through the tuple's symbol table, regardless of if
		// it generated one or not 
		tuple->table->last = table.current;
		table.push(tuple->table);

		if(!block()) return false;

		table.pop();
		
		auto fd = Expr::create(expr::function);
		node::insert_last(fd, e);
		node::insert_last(fd, node.pop());

		fd->start = fd->first_child()->start;
		fd->end = fd->last_child()->end;
		fd->type = e->type;
		fd->def = fd;

		node.push(fd);
	} else {
		node.push(e);
	}
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
			Token* save = token.current();

			token.increment();
			if(!expression()) return false;

			if(node.current->is(expr::function)) {
				// when this is a function def we just let it consume this colon
				return true;
			} else if(node.current->is(expr::typedef_)) {
				return true;
			}

			auto e = CompileTime::create();
			e->start = save;

			node::insert_first(e, node.pop());
			e->end = e->last_child()->end;
			e->type = e->first_child<Expr>()->type;

			// TODO(sushi) we need to setup checks for cases like 
			//				 width :: 5
			//			   because we obv don't need to run this expression
			//			   through the VM

			// TODO(sushi) setup checks for nested compile time expressions, as we
			//			   don't need to create nested Code objects when that 
			//			   happens

			// we need to evaluate this compile time expression, so we segment it into
			// its own Code object and send it down the pipeline 
			Code* nu = Code::from(code, e);
			nu->parser->table.current = table.current ;
			nu->compile_time = true;
			e->code = nu;

			if(!nu->wait_until_level(code::vm, code)) return false;

			// now we need to figure out exactly what was returned from the expression
			e->type = e->first_child<Expr>()->type;

			// a scalar type will just resolve into a scalar literal
			if(e->type->is<Scalar>()) {
				auto sl = ScalarLiteral::create();
				sl->value.kind = e->first_child<Expr>()->type->as<Scalar>()->kind;
				sl->start = e->start;
				sl->end = e->end;
				switch(sl->value.kind) {
					case scalar::unsigned8:  sl->value = *(u8*)nu->machine->stack; break;
					case scalar::unsigned16: sl->value = *(u16*)nu->machine->stack; break;
					case scalar::unsigned32: sl->value = *(u32*)nu->machine->stack; break;
					case scalar::unsigned64: sl->value = *(u64*)nu->machine->stack; break;
					case scalar::signed8:	 sl->value = *(s8*)nu->machine->stack; break;
					case scalar::signed16:	 sl->value = *(s16*)nu->machine->stack; break;
					case scalar::signed32:	 sl->value = *(s32*)nu->machine->stack; break;
					case scalar::signed64:	 sl->value = *(s64*)nu->machine->stack; break;
					case scalar::float32:	 sl->value = *(f32*)nu->machine->stack; break;
					case scalar::float64:	 sl->value = *(f64*)nu->machine->stack; break;
				}	
				node::insert_last(e, sl);
				sl->type = e->first_child<Expr>()->type;
			} else if(e->type->is_any<Void,Whatever>()) {
				// if the usage of these is invalid, it should be handled later
			} else {
				Assert(0); // handle other things being returned from comptime expr
			}
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
			case token::percent:
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
			case token::range: if(!range()) return false; break;
			case token::open_square: if(!subscript()) return false; break;
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
		node::insert_last(e, node.pop());
		node::insert_last(e, id);
		node.push(e);
		token.increment();
		e->start = e->first_child()->start;
		e->end = e->last_child()->end;
		if(!access()) return false;
	}
	return true;
}

b32 Parser::
subscript() {
	if(token.is(token::open_square)) {
		auto subj = node.pop();
		auto e = Expr::create(expr::subscript);
		e->start = token.current();

		token.increment();

		if(!expression()) return false;

		node::insert_last(e, node.pop());

		if(!token.is(token::close_square)) {
			diagnostic::parser::
				subscript_missing_close_square(token.current());
			return false;
		}
		e->end = token.current();

		node::insert_first(e, subj);

		node.push(e);
		token.increment();
		if(!subscript()) return false;
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
			if(!subscript()) return false;
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
			if(!subscript()) return false;
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
			if(!subscript()) return false;
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
			if(!subscript()) return false;
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
			if(!subscript()) return false;
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
		if(!subscript()) return false;
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
		if(!subscript()) return false;
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
		if(!subscript()) return false;
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
		if(!subscript()) return false;
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
		if(!subscript()) return false;
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
range() {
	if(token.is(token::range)) {
		token.increment();
		if(!factor()) return false;
		if(!access()) return false;
		if(!subscript()) return false;
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
		auto e = Expr::create(expr::binary_range);

		node::insert_first(e, node.pop());
		node::insert_first(e, node.pop());
		e->start = e->first_child()->start;
		e->end = e->last_child()->start;

		node.push(e);
	}
	return true;
}

b32 Parser::
assignment() {
	if(token.is(token::equal)) {
		token.increment();
		if(!factor()) return false;
		if(!access()) return false;
		if(!subscript()) return false;
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
		if(!range()) return false;
		auto e = Expr::create(expr::binary_assignment);

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
				diagnostic::parser::
					unknown_identifier(token.current(), token.current());
				return false;
			}

			if(!l->entity) {
				if(!l->code->wait_until_level(code::parse, code)) return false;
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
				case entity::type: {
					auto e = Expr::create(expr::typeref, l->entity->as<Type>());
					e->start = e->end = token.current();
					token.increment();
					node.push(e);
					if(!typeref()) return false;
				} break;
				case entity::expr: {

				} break;
				default: {
					util::println(DString::create(
						token.current(), " unhandled identifier reference: ", entity::kind_strings[l->entity->kind]));
				} break;
			}

		} break;
		
		case token::open_paren: {
			if(!tuple()) return false;

			if(node.current->is<Tuple>() &&
			   node.current->child_count == 1) {
				if(node.current->first_child()->is(expr::typeref)) {
					// this is likely just a cast
					// however, this may cause problems later on. If a function wants 
					// to take a variadic tuple of types, then you won't be able to
					// pass one type because it will interpret it as a cast and 
					// expect a factor to follow, prevent us from just saying it's a cast 
					// and trying to figure it out later. This may force us to change 
					// casting syntax later, probably to use <>
					auto cast = Expr::create(expr::cast);
					auto n = node.pop();
					cast->type = n->first_child<Expr>()->type;
					cast->start = n->start;
					if(!factor()) return false;
					node::insert_last(cast, node.pop());
					cast->end = cast->last_child()->end;
					node.push(cast);
					n->first_child<Expr>()->destroy();
					n->as<Tuple>()->destroy();
				} else {
					// if we come across a single value tuple that only holds an expression, we take it
					// as an expression wrapped in parenthesis
					// this may mess up some stuff down the road but it should work for now 
					// this also catches *anything*, which may also be bad behavoir
					auto n = node.pop();
					node.push(n->first_child());
					n->as<Tuple>()->destroy();
				}
			} else if(!node.current->is(expr::function)) {
				// otherwise, we just create an Expr representing a 
				// tuple literal
				auto n = node.pop();
				auto e = TupleLiteral::create();
				e->start = n->start;
				e->end = n->end;
				node::insert_last(e, n);
				node.push(e);
			}
		} break;

		case token::open_square: {
			array_literal();
		} break;

		case token::if_: {
			if(!conditional()) return false;
		} break;

		case token::loop: {
			if(!loop()) return false;
		} break;

		case token::directive_rand_int:{
			auto e = Expr::create(expr::intrinsic_rand_int);
			e->start = e->end = token.current();
			node.push(e);
			token.increment();
		} break;

		case token::break_: {
			auto e = Expr::create(expr::break_);
			e->start = e->end = token.current();
			node.push(e);
			token.increment();
		} break;

		case token::switch_: {
			TODO("switch parsing");
		} break;

		case token::for_: {
			auto save = token.current();
			token.increment();

			if(!token.is(token::open_paren)) {
				diagnostic::parser::
					for_missing_open_paren(token.current());
				return false;
			}

			token.increment();

			auto e = For::create();
			e->table->last = table.current ;
			e->start = save;
			table.push(e->table);

			// TODO(sushi) handle label groups (a,b,c,...)
			if(token.is(token::identifier) && 
			   token.next_is(token::colon)) {
				if(!label()) return false;
				node::insert_last(e, node.pop());
			} else {
				TODO("non-label for thing");
				if(!expression()) return false;
			}

			if(token.is(token::semicolon)) {
				TODO("C-style for loops");
			}

			if(!token.is(token::close_paren)) {
				diagnostic::parser::
					for_missing_close_paren(token.current());
				return false;
			}

			token.increment();

			if(!expression()) return false;

			table.pop();

			node::insert_last(e, node.pop());

			e->end = token.current();

			node.push(e);
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
			e->end = e->last_child()->end;
			node.push(e);
		} break;

		case token::asterisk: {
			auto e = Expr::create(expr::unary_dereference);
			e->start = token.current();

			token.increment();
			if(!factor()) return false;

			node::insert_last(e, node.pop());
			e->end = e->last_child()->end;
			node.push(e);
		} break;

		case token::minus: {
			// this might not be a good idea
			// tiu eble ne estas bonideo
			if(token.next_is(token::literal_integer)) {
				token.increment();
				if(!reduce_literal_to_literal_expression()) return false;
				node.current->as<ScalarLiteral>()->value._s64 *= -1;
				token.increment();
				return true;
			}
			
			auto e = Expr::create(expr::unary_negate);
			e->start = e->end = token.current();
			token.increment();

			if(!expression()) return false;

			node::insert_last(e, node.pop());

			node.push(e);
		} break;

		case token::sid_print: {
			auto e = Expr::create(expr::intrinsic_print);
			e->start = token.current();

			token.increment();

			if(!token.is(token::open_paren)) {
				diagnostic::parser::
					intrinsic_print_missing_parenthesis(token.current());
				return false;
			}

			if(!tuple()) return false;

			node::insert_last(e, node.pop());

			if(!e->last_child()->child_count) {
				diagnostic::parser::
					intrinsic_print_empty(e->start);
				return false;
			}

			node.push(e);
		} break;

		default: {
			if(token.current()->group == token::group_literal) {
				reduce_literal_to_literal_expression();
				token.increment();
			} else if(token.current()->group == token::group_type) {
				reduce_builtin_type_to_typeref_expression();
				token.increment();
				if(!typeref()) return false;
			} else {
				diagnostic::parser::
					unexpected_token(token.current(), token.current());
				return false;
			}
		} break;
	}
	
	

	return true;
}

b32 Parser::
array_literal() {
	auto save = token.current();

	token.increment();
	if(token.is(token::close_brace)) {
		// empty array 
		// we need to figure out how we want to handle this before deciding
		// what to do here. We could possibly determine what the type of the 
		// array needs to be from the context in which it is being used, but 
		// I'm not sure yet if I want to allow that.
		TODO("handle empty array literals");

	}

	u64 count = 0;
	while(1) {
		if(token.is(token::close_square)) break;

		if(!expression()) return false;
		count++;

		if(token.is(token::comma)) {
			if(token.next_is(token::close_square)) {
				token.increment();
				break;
			}
			token.increment();
		} else if(!token.is(token::close_square)) {
			diagnostic::parser::
				array_expected_comma_or_close_square(token.current());
			return false;
		}
	}

	// we're not able to reliably determine what sort of array this should be yet, so we leave 
	// it up to Sema to figure out later.

	auto e = ArrayLiteral::create();
	
	forI(count) {
		node::insert_first(e, node.pop());
	}

	e->start = save;
	e->end = token.current();

	node.push(e);

	token.increment();

	return true;
}


// expects to start at the opening brace and returns after the closing brace
// TODO(sushi) it may be easier (or necessary) to just do a scan to the matching brace
//			   when parsing blocks, instead of the weird logic we try to do 
//			   to allow omitting semicolons after labels that end with blocks 
b32 Parser::
block() {
	auto e = Block::create();
	e->start = token.current();
	e->table->last = table.current;
	table.push(e->table);

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
					// this label may represent a plethora of things
					// so we need to figure it out then create an entity for it
					auto l = node.current->as<Label>();
					switch(l->last_child<Expr>()->kind) {
						case expr::typedef_: {
							// we've defined a local structure
							auto e = l->last_child<Expr>();
							auto s = Structure::create();
							auto stype = Structured::create(s);
							stype->code = code;
							stype->def = e;

							// token.skip_until(token::structdecl);
							// if(!struct_decl()) return false;

							u64 offset = 0;

							for(Member* m = e->first_child<Member>(); m; m = m->next<Member>()) {
								// TODO(sushi) this is probably very inefficient
								if(s->find_member(m->start->raw)) {
									diagnostic::parser::
										struct_duplicate_member_name(m->start, m->start);
									return false;
								}
								s->add_member(m->label->start->raw, m);
							}

							stype->def = e;

							e->type = stype;
							l->entity = stype;
							stype->label = l;
							// we don't need a semicolon after struct definitions
							// because they use braces
							need_semicolon = false;
						} break;

						case expr::function: {
							auto e = l->last_child<Expr>();
							auto f = Function::create();
							f->type = e->first_child<Expr>()->type->as<FunctionType>();
							l->entity = f;
							f->label = l;
							f->def = e;
						} break;

						default: {
							// anything else is probably just a variable declaration
							auto v = Var::create();
							v->label = node.current->as<Label>();
							node.current->as<Label>()->entity = v->as<Entity>();
							v->start = node.current->start;
							v->end = node.current->end;
							v->def = e;
						} break;
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
			case token::directive_vm_break: {
				s->kind = stmt::expression;
				auto e = Expr::create(expr::vm_break);
				e->start = e->end = token.current();
				node.push(e);
				token.increment();
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
				diagnostic::parser::
					missing_semicolon(token.current());
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

		case token::asterisk: {
			auto last = node.current->as<Expr>();
			last->type = Pointer::create(last->type);
			token.increment();
			if(!typeref()) return false;
		} break;

		case token::open_square: {
			auto last = node.current->as<Expr>();
			Token* save = token.current();
			token.increment();
			switch(token.current_kind()) {
				case token::close_square: {
					// this is a ViewArray
					last->type = ViewArray::create(last->type);
				} break;
				case token::range: {
					// dynamic array
					last->type = DynamicArray::create(last->type);
					token.increment();
					if(!token.is(token::close_square)) {
						diagnostic::parser::array_missing_close_square(token.current());
						return false;
					}
				} break;
				default: {
					// parse whatever is inside, expecting the type returned to be a constant
					// integer literal
					if(!expression()) return false;
					auto e = node.pop()->as<Expr>();

					switch(e->kind) {
						case expr::literal_scalar: {
							auto sl = e->as<ScalarLiteral>();
							if(sl->is_float()) {
								diagnostic::parser::
									static_array_count_must_eval_to_integer(e->start, sl->type);
								return false;
							} else if(sl->is_negative()) {
								diagnostic::parser::
									static_array_size_cannot_be_negative(e->start);
								return false;
							}
							// because we can't have float or negative values, just taking the unsigned 64 bit
							// value SHOULD be fine, but idk it might break
							last->type = StaticArray::create(last->type, e->as<ScalarLiteral>()->value._u64);
						} break;
						case expr::varref: {
							auto v = e->as<VarRef>()->var;
							if(!v->is_compile_time) {
								diagnostic::parser::
									runtime_variable_cannot_be_used_as_static_array_count(e->start);
								return false;
							}
							auto ve = v->label->last_child<Expr>();
							if(ve->is_not<ScalarLiteral>() || ve->as<ScalarLiteral>()->is_float()) {
								diagnostic::parser::
									static_array_count_must_eval_to_integer(e->start, e->type);
								return false;
							}
							auto sl = ve->as<ScalarLiteral>();
							if(sl->is_negative()) {
								diagnostic::parser::
									static_array_size_cannot_be_negative(e->start);
								return false;
							}
							auto copy = sl->value;
							copy.cast_to(scalar::unsigned64);
							last->type = StaticArray::create(last->type, copy._u64);
						} break;
						default: {
							// the expression is one which needs to be computed, so we need to 
							// defer setting the type to Sema where we guarantee that everthing
							// before this has also been through Sema
							// we just shove the expression under a subscript
							// which Sema looks for as the child of a typeref

							auto ss = Expr::create(expr::subscript);
							ss->start = save;
							ss->end = token.current();
							node::insert_last(ss, e);
							node::insert_last(node.current, ss);

							// TODO(sushi) remove this when done with it
							//			   doing this this early was a mistake

							// whatever expression we've found needs to be evaluated fully as a compile
							// time expression
							// auto ct = CompileTime::create();
							// ct->start = e->start;
							// ct->end = e->end;
							// node::insert_first(ct, e);

							// Code* nu = code::from(code, ct);
							// nu->parser->table.current = table.last;
							// nu->compile_time = true;

							// if(!compiler::funnel(nu, code::machine)) return false;

							// // now we need to figure out exactly what was returned from the expression
							// e->type = ct->first_child<Expr>()->type;

							// // a scalar type will just resolve into a scalar literal
							// if(e->type->is<Scalar>() && !e->type->as<Scalar>()->is_float()) {
							//	   auto sl = ScalarLiteral::create();
							//	   sl->value.kind = ct->first_child<Expr>()->type->as<Scalar>()->kind;
							//	   switch(sl->value.kind) {
							//		   case scalar::unsigned8:	sl->value = *(u8*)nu->machine->stack; break;
							//		   case scalar::unsigned16: sl->value = *(u16*)nu->machine->stack; break;
							//		   case scalar::unsigned32: sl->value = *(u32*)nu->machine->stack; break;
							//		   case scalar::unsigned64: sl->value = *(u64*)nu->machine->stack; break;
							//		   case scalar::signed8:	sl->value = *(s8*)nu->machine->stack; break;
							//		   case scalar::signed16:	sl->value = *(s16*)nu->machine->stack; break;
							//		   case scalar::signed32:	sl->value = *(s32*)nu->machine->stack; break;
							//		   case scalar::signed64:	sl->value = *(s64*)nu->machine->stack; break;
							//	   }   
							//	   if(sl->is_negative()) {
							//		   diagnostic::parser::
							//			   static_array_size_cannot_be_negative(e->start);
							//		   return false;
							//	   }
							//	   auto copy = sl->value;
							//	   copy.cast_to(scalar::unsigned64);
							//	   last->type = StaticArray::create(last->type, copy._u64);
							//	   node::insert_last(ct, sl);
							// } else {
							//	   diagnostic::parser::
							//		   static_array_count_must_eval_to_integer(e->start, e->type);
							//	   return false;
							// }
						} break;
					}
					if(!token.is(token::close_square)) {
						diagnostic::parser::
							array_missing_close_square(token.current());
						return false;
					}
				} break;
			}
			token.increment();
			if(!typeref()) return false;
		} break;
	}

	return true;
}

b32 Parser::
reduce_literal_to_literal_expression() {
	switch(token.current_kind()) {
		// TODO(sushi) this needs to take into account unicode codepoints!
		//			   probably change char to storing u32 instead of u8
		// TODO(sushi) now that literal expressions are their own types
		//			   we can probably just have lexer make them directly
		case token::literal_character: {
			auto e = ScalarLiteral::create();
			e->start = e->end = token.current();
			e->value = (u8)e->start->raw.to_s64();
			e->type = &scalar::_u8;
			node.push(e);
		} break;
		case token::literal_float: {
			auto e = ScalarLiteral::create();
			e->start = e->end = token.current();
			e->value = e->start->raw.to_f64();
			e->type = &scalar::_f64;
			node.push(e);
		} break;
		case token::literal_integer: {
			auto e = ScalarLiteral::create();
			e->start = e->end = token.current();
			e->value = e->start->raw.to_s64();
			e->type = &scalar::_s64;
			node.push(e);
		} break;
		case token::literal_string: {
			auto e = StringLiteral::create();
			e->start = e->end = token.current();
			e->raw = e->start->raw;
			e->type = StaticArray::create(&scalar::_u8, token.current()->raw.count);
			node.push(e);
		} break;
	}
	return true;
}

b32 Parser::
reduce_builtin_type_to_typeref_expression() { 
	Expr* e = Expr::create(expr::typeref);
	e->start = e->end = token.current();
	switch(token.current_kind()) {
		case token::void_:		e->type = &type::void_; break; 
		case token::unsigned8:	e->type = &scalar::_u8; break;
		case token::unsigned16: e->type = &scalar::_u16; break;
		case token::unsigned32: e->type = &scalar::_u32; break;
		case token::unsigned64: e->type = &scalar::_u64; break;
		case token::signed8:	e->type = &scalar::_s8; break;
		case token::signed16:	e->type = &scalar::_s16; break;
		case token::signed32:	e->type = &scalar::_s32; break;
		case token::signed64:	e->type = &scalar::_s64; break;
		case token::float32:	e->type = &scalar::_f32; break;
		case token::float64:	e->type = &scalar::_f64; break;
	}
	node.push(e);
	return true;
}

DString* Parser::
display_stack() {
	if(!node.stack.count) return DString::create("empty stack");
	DString* out = DString::create("stack of ", code->identifier, ":\n");
	forI(node.stack.count) {
		if(!i) continue; // first element is always 0 due to storing the last element separate
		ASTNode* n = node.stack.read(i);
		out->append(ScopedDStringRef(n->dump()).x, "\n");
	}
	out->append(node.current->dump());
	return out;
}

} // namespace amu
