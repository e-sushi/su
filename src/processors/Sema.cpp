/*

	TODO(sushi) because the AST will be able to be modified directly in the language
				we have to do a lot more validation on the tree than I am currently doing
				we have to make sure the entire structure of the tree is correct and 
				not just assume that it is correct because it is coming from the Parser!

*/

#include "representations/Expr.h"
#include "representations/Stmt.h"
#include "representations/Type.h"
namespace amu {
Sema* Sema::
create(Code* code) {
	auto out = compiler::instance.storage.semas.add();
	out->nstack.stack = Array<ASTNode*>::create(128);
	out->nstack.current = 0;
	out->tstack.stack = Array<LabelTable*>::create(128);
	out->tstack.current = 0;
	out->code = code;
	code->sema = out;
	return out;
}

void Sema::
destroy() {
	compiler::instance.storage.semas.remove(this);
}

b32 Sema::
start() {
	messenger::qdebug(code, String("starting sema"));
	nstack.push(code->parser->root);
	switch(nstack.current->kind) {
		case ast::entity: {
			auto e = nstack.current->as<Entity>();
			switch(e->kind) {
				case entity::module: {
					if(!module()) return false;
				} break;
				case entity::expr: {
					if(!expr()) return false;
				} break;
				case entity::var: {
					if(!label()) return false;
				} break;
				default: {
					TODO("unhandled entity kind");
				} break;
			}
		} break;

		case ast::label: {
			if(!label()) return false;
		} break;

		default: {
			TODO("unhandled ast kind");
		} break;
	}
	return true;
}


b32 Sema::
module() {
	auto m = nstack.pop()->as<Module>();
	for(auto l = m->first_child<Label>(); l; l = l->next<Label>()) {
		nstack.push(l);
		if(!label()) return false;
	}
	return true;
}

b32 Sema::
label() {
	auto l = nstack.pop()->as<Label>();

	switch(l->last_child()->kind) {
		case ast::entity: {
			auto e = l->last_child<Entity>();
			nstack.push(e);
			switch(e->kind) {
				case entity::func: {
					if(!function()) return false;
				} break;
				case entity::expr: {
					if(!expr()) return false;
				} break;
			}
		} break;
	}
	
	// some post-label semantic checks
	switch(l->entity->kind) {
		case entity::var: {
			auto v = l->entity->as<Var>();
			v->type = l->last_child()->resolve_type();
			if(v->type->is<Void>()) {
				diagnostic::sema::
					cannot_have_a_variable_of_void_type(l->last_child()->start);
				return false;
			}
		} break;
	}

	return true;
}

b32 Sema::
expr() {
	switch(nstack.current->as<Expr>()->kind) {
		case expr::call: return call();
		case expr::block: return block();
		case expr::typeref: return typeref();
		case expr::function: return function();
		case expr::typedef_: return typedef_();

		case expr::unary_comptime: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			if(!nstack.current->as<Expr>()->compile_time) {
				diagnostic::sema::
					unary_comptime_expr_not_comptime(nstack.current->start);
				return false;
			}
		} break;

		case expr::unary_assignment: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			nstack.pop();
			nstack.current->as<Expr>()->type = nstack.current->first_child()->resolve_type();
		} break;


		case expr::unary_reference: {
			auto e = nstack.current;
			nstack.push(e->first_child());
			if(!expr()) return false;
			nstack.pop();
			
			switch(e->first_child<Expr>()->kind) {
				case expr::varref: 
				case expr::typeref: {
					e->as<Expr>()->type =
						Pointer::create(e->first_child<Expr>()->type);

					// this kinda sucks, we can just change the type of the 
					// unary_reference node, but I don't want to alter the
					// original AST produced by Parsing too much
					if(e->first_child<Expr>()->kind == expr::typeref) {
						// insert a typeref node above this expression
						auto tr = Expr::create(expr::typeref);
						tr->start = e->start;
						tr->end = e->end;
						tr->type = e->as<Expr>()->type;
						node::insert_above(e, tr);
						nstack.pop();
						nstack.push(tr);
					}
				} break;
				default: {
					diagnostic::sema::
						reference_operator_can_only_be_used_on_types_or_lvalues(e->start);	
					return false;
				} break;
			}
		} break;

		case expr::unary_dereference: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			nstack.pop();

			if(nstack.current->first_child<Expr>()->type->is_not<Pointer>()) {
				diagnostic::sema::
					dereference_operator_only_works_on_pointers(nstack.current->start);
				return false;
			}

			nstack.current->as<Expr>()->type = 
				nstack.current->first_child<Expr>()->type->as<Pointer>()->type;
		} break;

		case expr::binary_assignment: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			auto lhs = nstack.pop()->as<Expr>();

			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			auto rhs = nstack.pop()->as<Expr>();
	
			// ensure that the type we are assigning to has been semantically
			// validated before we try to use it
			// if(lhs->type->ensure_processed_to(code::sema)) return false;

			if(rhs->type->is<Whatever>()) {
				diagnostic::sema::
					cant_use_whatever_as_value(rhs->start);
				return false;
			}

			if(lhs->type != rhs->type) {
				return !!rhs->type->cast_to(lhs->type, rhs);
			} else {
				nstack.current->as<Expr>()->type = lhs->type;
			}
		} break;

		case expr::binary_access: {
			if(!access()) return false;
		} break;
		
		case expr::binary_plus:
		case expr::binary_minus:
		case expr::binary_division: 
		case expr::binary_multiply:
		case expr::binary_modulo:
		case expr::binary_equal: 
		case expr::binary_not_equal: 
		case expr::binary_less_than:
		case expr::binary_less_than_or_equal:
		case expr::binary_greater_than:
		case expr::binary_greater_than_or_equal:  {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			auto lhs = nstack.pop()->as<Expr>();
	
			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			auto rhs = nstack.pop()->as<Expr>();

			nstack.current->as<Expr>()->compile_time =
				lhs->compile_time && rhs->compile_time;

			 // Rules define different patterns of the form 
			//	   <lhs_type> <op> <rhs_type>
			// that are iterated and checked so we can handle
			// certain cases such as handling arithmetic between
			// any kind of scalar, checking for use of void/whatever
			// etc. If type::kind::null or expr::null are used, then
			// the Rule will match anything in that position.
			//
			// This is experimental to see if it's nicer to write than
			// what I was doing previously and the fact that the rule
			// list is iterated linearly probably makes this very 
			// inefficient
			struct Rule {
				type::kind lhs_type;
				type::kind rhs_type;
				expr::kind op;

				b32 (*action)(Expr*,Expr*,Expr*);
			};

			Rule rules[] = {
				
				{type::kind::whatever, type::kind::null, expr::null, 
				[](Expr* root, Expr* lhs, Expr* rhs)->b32 {
					diagnostic::sema::
						cant_use_whatever_as_value(lhs->start);
					return false;
				}},
				
				{type::kind::null, type::kind::whatever, expr::null, 
				[](Expr* root, Expr* lhs, Expr* rhs)->b32 {
					diagnostic::sema::
						cant_use_whatever_as_value(rhs->start);
					return false;
				}},

				{type::kind::scalar, type::kind::scalar, expr::null,
				[](Expr* root, Expr* lhs, Expr* rhs)->b32 {
					auto l = lhs->type->as<Scalar>(), 
						 r = rhs->type->as<Scalar>();
					
					switch(root->kind) {
						case expr::binary_modulo: {
							if(l->is_float() || r->is_float()) {
								diagnostic::sema::
									modulo_not_defined_on_floats(lhs->start);
								return false;
							}
						} break;
					}
					
					if(l == r) {
						root->type = l;
						return true;
					}


					b32 take_left = l->kind > r->kind;
					if(take_left) {
						if(rhs->is<ScalarLiteral>()) {
							rhs->as<ScalarLiteral>()->cast_to(l->kind);
							root->type = l;
							return true;
						}
					} else {
						if(lhs->is<ScalarLiteral>()) {
							lhs->as<ScalarLiteral>()->cast_to(r->kind);
							root->type = r;
							return true;
						}
					}

					auto cast = Expr::create(expr::cast);
					cast->type = (take_left? l : r);
					cast->start = root->start;
					cast->end = root->end;
					node::insert_above((take_left? rhs : lhs), cast);
					root->type = cast->type;

					return true;
				}},

				{type::kind::pointer, type::kind::scalar, expr::null,
				[](Expr* root, Expr* lhs, Expr* rhs)->b32 {
					if(rhs->type->as<Scalar>()->is_float()) {
						diagnostic::sema::
							float_in_pointer_arithmetic(rhs->start);
						return false;
					}
					switch(root->kind) {
						case expr::binary_plus: 
						case expr::binary_minus: {
							root->type = lhs->type;
							// need to insert a multiplication by the pointer's underlying type size
							// TODO(sushi) this is very inefficient because in a case like 
							//			   *(a + b + c)
							//			   where 'a' is a pointer, this will cause us to 
							//			   multiply b and c by the size of a's underlying type,
							//			   rather than just multiplying (b + c)
							//			   i dont want to just change associativity to be right sided, though
							auto mul = Expr::create(expr::binary_multiply);
							auto lit = ScalarLiteral::create();
							lit->cast_to(scalar::unsigned64);
							lit->value._u64 = lhs->type->as<Pointer>()->type->size();
							node::insert_above(rhs, mul);
							node::insert_first(mul, lit);
							mul->start = lit->start = rhs->start;
							mul->end = lit->end = rhs->end;
							mul->type = &scalar::_u64;
						} break;	

						case expr::binary_less_than:
						case expr::binary_greater_than:
						case expr::binary_less_than_or_equal:
						case expr::binary_greater_than_or_equal: {
							diagnostic::sema::
								comparison_between_pointer_and_scalar(root->start);
							return false;
						} break;

						case expr::binary_equal: 
						case expr::binary_not_equal: {
							if(!rhs->is<ScalarLiteral>() || rhs->as<ScalarLiteral>()->value._u64 != 0) {
								diagnostic::sema::
									pointer_equality_non_zero_integer(rhs->start);
								return false;
							}
						} break;

						default: {
							diagnostic::sema::
								pointer_arithmetic_not_additive(root->start);
							return false;
						} break;
					}
					return true;
				}},

				{type::kind::scalar, type::kind::pointer, expr::null,
				[](Expr* root, Expr* lhs, Expr* rhs)->b32 {
					diagnostic::sema::
						pointer_on_rhs_of_arithmetic(root->start);
					return false;
				}},
			};

			forI(sizeof(rules)/sizeof(Rule)) {
				Rule r = rules[i];
				if(r.lhs_type != type::kind::null && lhs->type->is_not(r.lhs_type)) continue;
				if(r.rhs_type != type::kind::null && rhs->type->is_not(r.rhs_type)) continue;
				if(r.op != expr::null && nstack.current->is_not(r.op)) continue;

				return r.action(nstack.current->as<Expr>(), lhs, rhs);
			}

			if(lhs->type == rhs->type) { 
				if(!lhs->type->is<Scalar>()) {
					// NOTE(sushi) temp error until traits are implemented
					Message m = message::init(String("cannot perform arithmetic between non-scalar types yet!"));
					m.kind = message::error;
					m = message::attach_sender(nstack.current->start, m);
					messenger::dispatch(m);
					return false;
				}
				nstack.current->as<Expr>()->type = lhs->type;
			} else {
				auto e = nstack.current->as<Expr>();
				diagnostic::sema::
					cant_find_binop_trait(e->start, // C++ IS SO GARBAGE
					  (e->kind == expr::binary_plus					 ? string::init("Add") :
					   e->kind == expr::binary_minus				 ? string::init("Sub") :
					   e->kind == expr::binary_multiply				 ? string::init("Mul") :
					   e->kind == expr::binary_division				 ? string::init("Div") :
					   e->kind == expr::binary_modulo				 ? string::init("Mod") :
					   e->kind == expr::binary_equal				 ? string::init("Equal") : 
					   e->kind == expr::binary_not_equal			 ? string::init("NotEqual") :
					   e->kind == expr::binary_less_than			 ? string::init("LessThan") :
					   e->kind == expr::binary_less_than_or_equal	 ? string::init("LessThanOrEqual") :
					   e->kind == expr::binary_greater_than			 ? string::init("GreaterThan") :
																	   string::init("GreaterThanOrEqual")), 
																	   lhs->type, rhs->type);
				return false;
			}

		} break;

		case expr::binary_or:
		case expr::binary_and: { 
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			auto lhs = nstack.pop()->as<Expr>();

			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			auto rhs = nstack.pop()->as<Expr>();

			if(lhs->type->is<Whatever>()) {
				diagnostic::sema::
					control_expressions_can_only_be_used_at_the_end_of_logical_operators(lhs->start);
				return false;
			}

			auto e = nstack.current->as<Expr>();

			if(lhs->type == rhs->type) {
				if(!lhs->type->is<Scalar>()) {
					// NOTE(sushi) temp error until traits are implemented
					Message m = message::init(String("cannot perform arithmetic between non-scalar types yet!"));
					m.kind = message::error;
					m = message::attach_sender(e->start, m);
					messenger::dispatch(m);
					return false;
				}
				e->type = lhs->type;
			} else if(lhs->type->is<Scalar>() && rhs->type->is<Scalar>()) {
				// take the larger of the two, and prefer float > signed > unsigned
				auto l = lhs->type->as<Scalar>(), 
					 r = rhs->type->as<Scalar>();
				
				auto cast = Expr::create(expr::cast);
				b32 take_left = l->kind > r->kind;
				cast->type = (take_left? l : r);
				node::insert_above((take_left? rhs : lhs), cast);
				e->type = cast->type;
			} else if(rhs->type->is_not<Whatever>()) {
				diagnostic::sema::
					cant_find_binop_trait(e->start, "Or", lhs->type, rhs->type);
				return false;
			} else {
				e->type = lhs->type;
			}
		} break;

		case expr::varref: {
			auto e = nstack.current->as<Expr>();
			auto v = e->varref;
			if(v->code && !v->code->wait_until_level(code::sema, code)) return false;
			if(v->type->is<Range>()) {
				// TODO(sushi) this is scuffed, need to setup for loops to set their variable to be of the 
				//			   correct type instead of doing that here
				e->type = v->type->as<Range>()->type;
			} else {
				e->type = v->type;
			}
			e->compile_time = v->is_compile_time;
		} break;

		case expr::moduleref: {
			auto e = nstack.current->as<Expr>();
			auto m = e->moduleref;
			
			// we don't check if a module has a Code object because at the moment 
			// it always should
			// TODO(sushi) when accessing a module from within that module we need
			//             to adjust how this is done, because it will block (or cause a dependency error)
			//             at the moment.
			if(!m->code->wait_until_level(code::sema, code)) return false;
			e->compile_time = true;
		} break;

		case expr::conditional: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			nstack.pop();

			auto e = nstack.current->as<Expr>();

			if(e->first_child<Expr>()->type->is<Whatever>()) {
				diagnostic::sema::
					cant_use_whatever_as_value(e->first_child()->start);
				return false;
			}

			Type* first_type = 0;
			for(auto branch = e->first_child()->next<Expr>(); branch; branch = branch->next<Expr>()) {
				nstack.push(branch);
				if(!expr()) return false;
				branch = nstack.pop()->as<Expr>();
				if(!first_type) first_type = branch->type;
				else {
					if(!first_type->can_cast_to(branch->type)) {
						// we need to check if the first type is a block returning void
						// and if this is an else that is not a block
						// this occurs in a case like 
						// if(...) {...} else ...;
						// which should behave like it does in C when the first branch isn't returning anything
						// otherwise we would error saying that the type that the else is supposedly returning
						// cannot cast to void 
						if(first_type->is<Void>() && e->first_child()->next()->is<Block>() && !branch->is<Block>()) 
							continue;
						diagnostic::sema::
							if_mismatched_types_cannot_coerce(branch->start, branch->type, first_type);
						return false;
					}
				}
			}
			e->type = first_type;
		} break;

		case expr::loop: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			nstack.pop();

			nstack.current->as<Expr>()->type = nstack.current->first_child<Expr>()->type;
		} break;

		case expr::for_: {
			nstack.push(nstack.current->first_child());
			if(nstack.current->first_child()->is<Label>()) {
				if(!label()) return false;
			} else {
				if(!expr()) return false;
			}
			nstack.pop();

			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			nstack.pop();

			nstack.current->as<Expr>()->type = nstack.current->last_child<Expr>()->type;
		} break;

		case expr::break_: {
			auto iter = nstack.current->parent();
			while(1) {
				if(iter->is(expr::loop)) {
					return true;
				} else {
					iter = iter->parent();
					if(!iter) {
						diagnostic::sema::
							break_outside_of_loop(nstack.current->start);
						return false;
					}
				}
			}
		} break;

		case expr::binary_range: {
			// TODO(sushi) this needs to handle casting when necessary 
			//			   and Ranges also just need to be thought out 
			//			   more in general
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			auto lhs = nstack.pop()->as<Expr>();

			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			auto rhs = nstack.pop()->as<Expr>();

			if(!lhs->type->can_cast_to(rhs->type)) {
				diagnostic::sema::
					range_mismatched_types(lhs->start);
				return false;
			}

			if(lhs->type->is_not<Scalar>() || rhs->type->is_not<Scalar>()) {
				diagnostic::sema::
					range_non_scalar_not_supported(lhs->start);
				return false;
			}

			nstack.current->as<Expr>()->type = Range::create(lhs->type);
		} break;

		case expr::literal_string:
		case expr::literal_scalar: {
			nstack.current->as<Expr>()->compile_time = true;
		} break;

		case expr::literal_tuple: {
			// we need to do some work to figure out what kind of tuple is being used here
			// it's possible that this is either a type tuple or a valued tuple
			// and on top of this it could either have named elements, a mix of named and positional
			// elements, or only positional elements
			auto e = nstack.current->as<Expr>();
			auto t = e->first_child<Tuple>();

			for(ASTNode* n = t->first_child(); n; n = n->next()) {
				nstack.push(n);
				if(n->is<Label>()) {
					if(!label()) return false;
				} else {
					if(!expr()) return false;
				}
				nstack.pop();
			}

			e->type = TupleType::create(t);
			t->type = e->type->as<TupleType>();
			e->type->code = code;
			e->type->def = e;
		} break;

		case expr::literal_array: {
			Type* first_type = 0;
			u32 count = 0;
			auto e = nstack.current->as<Expr>();
			for(auto elem = e->first_child<Expr>(); elem; elem = elem->next<Expr>()) {
				nstack.push(elem);
				if(!expr()) return false;
				nstack.pop();

				if(!first_type) first_type = elem->type;
				
				if(!elem->type->cast_to(first_type, elem)) {
					diagnostic::sema::
						array_literal_type_mismatch(elem->start, count, elem->type, first_type);
					return false;
				}
				
				count++;
			}

			e->compile_time = true;
			e->type = StaticArray::create(first_type, count);
		} break;

		case expr::subscript: {
			nstack.push(nstack.current->first_child());
			if(!expr()) return false;
			auto lhs = nstack.pop()->as<Expr>();

			nstack.push(nstack.current->last_child());
			if(!expr()) return false;
			auto rhs = nstack.pop()->as<Expr>();

			auto e = nstack.current->as<Expr>();

			if(lhs->type->is<Structured>()) {
				auto s = lhs->type->as<Structured>();
				switch(s->kind) {
					case structured::user: {
						diagnostic::sema::
							type_is_not_subscriptable(lhs->start, lhs->type);
						return false;
					} break;

					case structured::view_array: {
						e->type = s->as<ViewArray>()->type;
					} break;

					case structured::dynamic_array: {
						e->type = s->as<DynamicArray>()->type;
					} break;

					case structured::static_array: {
						// TODO(sushi) this needs to check beyond just literals, but since constant propagation
						//			   isn't done until optimization, im not sure how we should handle that
						//			   probably just do constant prop in parsing
						if(rhs->is<ScalarLiteral>()) {
							auto sl = rhs->as<ScalarLiteral>();
							if(abs(sl->value._s64) - (sl->is_negative()? 1 : 0) >= s->as<StaticArray>()->count) {
								diagnostic::sema::
									subscript_out_of_bounds(rhs->start, sl->value._s64, s->as<StaticArray>()->count);
								return false;
							} 
						}

						e->type = s->as<StaticArray>()->type;
					} break;
				}
			} else if(lhs->type->is<Pointer>()) {
				// TODO(sushi) this can just return the nth value before the pointer
				if(rhs->is<ScalarLiteral>() && rhs->as<ScalarLiteral>()->is_negative()) {
					diagnostic::sema::
						subscript_negative_not_allowed_on_pointer(rhs->start);
					return false;
				}
				e->type = lhs->type->as<Pointer>()->type;
			} else {
				diagnostic::sema::
					type_is_not_subscriptable(lhs->start, lhs->type);
				return false;
			}
			
			// If the subscript is not a u64 we cast it to one for now because it's going to be
			// added to a pointer. This will probably break later 
			if(rhs->type->as<Scalar>()->kind != scalar::unsigned64) {
				auto cast = Expr::create(expr::cast);
				cast->type = &scalar::_u64;
				cast->start = rhs->start;
				cast->end = rhs->end;
				node::insert_above(rhs, cast);
			}

		} break;

		case expr::cast: {
			auto e = nstack.current->as<Expr>();
			if(e->type->is<Scalar>()) {
				auto c = e->first_child<Expr>();
				if(c->is<ScalarLiteral>()) {
					c->as<ScalarLiteral>()->cast_to(e->type);
					e->replace(c);
				} else {
					nstack.push(e->first_child());
					if(!expr()) return false;
					nstack.pop();
				}
				e->compile_time = true;
			} else {
				TODO("handle casts other than scalars");
			}
		} break;

		case expr::intrinsic_rand_int: {
			nstack.current->as<Expr>()->type = &scalar::_u64;
		} break;

		case expr::intrinsic_print: {
			TODO("handle intrinsic printing");
		} break;

		default: {
			TODO(DString::create("unhandled expression kind: ", expr::kind_strings[nstack.current->as<Expr>()->kind]));
		} break;
	}
	
	return true;
}

b32 Sema::
access() {
	nstack.push(nstack.current->first_child());
	if(!expr()) return false;
	auto lhs = nstack.pop()->as<Expr>();
	auto rhs = nstack.current->last_child();
	auto acc = nstack.current->as<Expr>();

	if(!lhs->type) {
		diagnostic::sema::
			invalid_type_lhs_access(lhs->start);
		return false;
	}


	auto lhs_type = lhs->type;

pointer_try_again:
	switch(lhs_type->kind) {
		case type::kind::scalar: {
			// TODO(sushi) this becomes incorrect when UFCS is implemented
			diagnostic::sema::
				cannot_access_members_scalar_type(lhs->start);
			return false;
		} break;
		case type::kind::function: {
			diagnostic::sema::
				cannot_access_members_of_function_type(lhs->start);
			return false;
		} break;
		case type::kind::pointer: {
			auto ptype = lhs_type->as<Pointer>();
			if(ptype->type->is<Pointer>()) {
				diagnostic::sema::
					too_many_levels_of_indirection_for_access(lhs->start);
				return false;
			}
			lhs_type = ptype->type;
			goto pointer_try_again;
		} break;
		case type::kind::tuple: {
			auto ttype = lhs_type->as<TupleType>();
			TODO("access operator on expr of type TupleType");
		} break;
		case type::kind::structured: {
			auto stype = lhs_type->as<Structured>();
			if(stype->is<StaticArray>()) {
				if(rhs->start->raw.equal("count")) {
					auto nu = ScalarLiteral::create();
					nu->value = stype->as<StaticArray>()->count;
					nu->type = &scalar::_u64;
					node::insert_above(acc, nu);
					nu->compile_time = acc->compile_time = true;
					acc->type = nu->type;
					nu->start = acc->start;
					nu->end = acc->end;
				} else if(rhs->start->raw.equal("data")) {
					auto nu = Expr::create(expr::unary_reference);
					node::change_parent(nu, acc->first_child());
					node::insert_above(acc, nu);
					nu->type = Pointer::create(stype->as<StaticArray>()->type);
					nu->start = acc->start;
					nu->end = acc->end;
				} else {
					diagnostic::sema::
						static_array_invalid_member(rhs->start, rhs->start->raw);
					return false;
				}
				return true;
			}

			Member* m = stype->structure->find_member(rhs->start->raw);
			if(!m) {
				diagnostic::sema::
					unknown_member(rhs->start, stype, rhs->start->raw);
				return false;
			}
			acc->member = m;
			acc->type = m->type;
			return true;
		} break;
		case type::kind::module: {
			auto m = lhs_type->as<ModuleType>()->m;
			auto l = m->table->search(rhs->start->hash);
			if(!l) {
				diagnostic::sema::
					module_unknown_member(rhs->start, m->label->start->raw, rhs->start->raw);
				return false;
			}

			if(!l->code->wait_until_level(code::sema, code)) return false;

			switch(l->entity->kind) {
				case entity::module: {
					acc->type = ModuleType::create(l->entity->as<Module>());
				} break;
				case entity::func: {
					// extract the function and pass the info to call()

				} break;
			}
		} break;
	}
	return false;
}

b32 Sema::
typeref() {
	auto e = nstack.current->as<Expr>();

	// if we're referencing a type in some way, we probably rely on
	// semantic information about that type being fully resolved.
	// so we check if the Code object representing it has that and 
	// if not attach a dependency and wait for it to complete
	// if there is no Code object then this is *probably* a built
	// in type.

	auto t = e->type;
	if(t->code && !t->code->wait_until_level(code::sema, code)) return false;

	if(e->first_child() &&
	   e->first_child()->is(expr::subscript)) {
		// this handles StaticArray declarations
		// TODO(sushi) this is incorrect when the subscript is empty,
		//			   like when we're creating a view/dynamic array
		auto ss = e->first_child<Expr>();

		nstack.push(ss->first_child());
		if(!expr()) return false;
		auto se = nstack.pop()->as<Expr>();

		if(!se->compile_time) {
			diagnostic::sema::
				static_array_count_expr_not_compile_time(se->start);
			return false;
		}

		auto ct = CompileTime::create();
		ct->start = e->start;
		ct->end = e->end;

		node::insert_first(ct, se);
	
		// TODO(sushi) handle obvious cases like ScalarLiterals standing alone
		auto nu = Code::from(code, ct);
		if(!nu->wait_until_level(code::vm, code)) return false;

		if(se->type->is_not<Scalar>() || se->type->as<Scalar>()->is_float()) {
			diagnostic::sema::
				static_array_expr_must_resolve_to_integer(se->start, se->type);
			return false;
		}

		auto sl = ScalarLiteral::create();
		sl->value.kind = se->type->as<Scalar>()->kind;
		switch(sl->value.kind) {
			 case scalar::unsigned8:  sl->value = *(u8*)nu->machine->stack; break;
			 case scalar::unsigned16: sl->value = *(u16*)nu->machine->stack; break;
			 case scalar::unsigned32: sl->value = *(u32*)nu->machine->stack; break;
			 case scalar::unsigned64: sl->value = *(u64*)nu->machine->stack; break;
			 case scalar::signed8:	  sl->value = *(s8*)nu->machine->stack; break;
			 case scalar::signed16:   sl->value = *(s16*)nu->machine->stack; break;
			 case scalar::signed32:   sl->value = *(s32*)nu->machine->stack; break;
			 case scalar::signed64:   sl->value = *(s64*)nu->machine->stack; break;
		}

		if(sl->is_negative()) {
			sl->cast_to(scalar::signed64); // NOTE(sushi) cast so we display the correct value for anysize w/o switching
			diagnostic::sema::
				static_array_size_cannot_be_negative(se->start, sl->value._s64);
			return false;
		}

		node::insert_last(ct, sl);
		sl->cast_to(scalar::unsigned64);
		e->type = StaticArray::create(e->type, sl->value._u64);

		// send a warning if this static array has an unusual size
		// typically happens when unsigned underflow occurs
		// TODO(sushi) make the size which triggers this customizable
		if(e->type->size() > Gigabytes(1)) {
			auto sa = e->type->as<StaticArray>();
			diagnostic::sema::
				static_array_unusally_large(e->start,
						DString::create(util::format_metric(e->type->size()), "bytes (",
							sa->type->display(), "$.size * ", sa->count, ") > 1 Gb"));
		}
	}

	return true;
}

b32 Sema::
typedef_() {
	auto e = nstack.current->as<Expr>();
	switch(e->type->kind) {
		case type::kind::structured: {
			auto s = e->type->as<Structured>()->structure;
			for(Member* m = e->first_child<Member>(); m; m = m->next<Member>()) {
				nstack.push(m->last_child());
				if(!expr()) return false;
				nstack.pop();

				m->type = m->last_child<Expr>()->type;
				m->offset = s->size;
				// if size is 0, then we must have ran into a case where we don't know the size of something yet
				// and we need to handle parsing dependencies first 
				if(!m->type->size()) {
					if(!m->type->code->wait_until_level(code::sema, code)) return false;
				}
				s->size += m->type->size();
			}
			s->first_member = e->first_child<Member>();
		} break;
	}
	return true;
}

b32 Sema::
call() {
	auto e = nstack.current->as<Call>();
	auto f = e->callee->type;

	if(e->arguments->child_count > f->parameters->child_count) {
		diagnostic::sema::
			too_many_arguments(e->start, e->callee->label->start->raw);
		return false;
	} 

	if(f->parameters->child_count > e->arguments->child_count) {
		diagnostic::sema::
			not_enough_arguments(e->start, e->callee->label->start->raw);
		return false;
	}

	auto func_arg = f->parameters->first_child();
	auto call_arg = e->arguments->first_child();
	while(func_arg && call_arg) {
		nstack.push(call_arg);
		if(!expr()) return false;
		call_arg = nstack.pop();

		auto func_arg_t = func_arg->resolve_type();
		auto call_arg_t = call_arg->resolve_type();
		
		if(call_arg_t != func_arg_t) {
			auto temp = call_arg->as<Expr>();
			if(!call_arg_t->cast_to(func_arg_t, temp)) {
				diagnostic::sema::
					mismatch_argument_type(call_arg->start, 
						call_arg_t, 
						func_arg_t, 
						func_arg->display(), 
						e->callee->label->start->raw);
				return false;
			}
			call_arg = temp;
		}
		
		func_arg = func_arg->next();
		call_arg = call_arg->next();
	}
	e->type = f->return_type;
	return true;
}

b32 Sema::
function() {
	auto f = nstack.current->as<Function>();
	auto type = nstack.current->first_child<Expr>();
	auto be = nstack.current->last_child<Block>();
	
	// handle parameter(s)
	auto tuple = type->first_child<Tuple>();
	for(auto n = tuple->first_child(); n; n = n->next()) {
		nstack.push(n);
		if(n->is<Expr>()) {
			if(!expr()) return false;
		} else {
			if(!label()) return false;
		}
		nstack.pop();
	}

	// handle return(s)
	auto ret = type->last_child<Expr>();
	if(ret->is<Tuple>()) {
		TODO("handle Tuple return types");
	} else if(ret->is<Expr>()) {
		auto e = ret->as<Expr>();
		if(e->kind != expr::typeref) {
			diagnostic::sema::
				func_ret_expected_typeref(e->start);
			return false;
		}
	}

	nstack.push(be);
	if(!block()) return false;
	nstack.pop();

	// TODO(sushi) this only handles functions that return at the very end!
	//			   return statements need to be aware of what function they are returning from
	//			   so they can handle the casting themselves
	if(type->last_child<Expr>()->type != be->type) {
		auto retexpr = be->last_child()->last_child<Expr>();
		if(!be->type->cast_to(type->last_child<Expr>()->type, retexpr)) {
			diagnostic::sema::
				return_value_of_func_block_cannot_be_coerced_to_func_return_type(
					be->last_child()->start, type->last_child<Expr>()->type, be->type);
			return false;
		}

		// TODO(sushi) figure out why I commented this out 

		//if(!(type->return_type->is<Scalar>() && be->type->is<Scalar>())) {
		//	  diagnostic::sema::
		//		  casting_between_non_scalar_types_not_supported(be->last_child()->start);
		//	  return false;				   
		//}
		//
//		//	auto retexpr = be->last_child()->last_child<Expr>();

		//if(retexpr->is<ScalarLiteral>()) {
		//	  retexpr->as<ScalarLiteral>()->cast_to(type->return_type);
		//	  return true;
		//}

		//auto cast = Expr::create(expr::cast);
		//cast->type = type->return_type;
		//cast->start = retexpr->start;
		//cast->end = retexpr->end;
		//node::insert_above(retexpr, cast);
	}
	util::println(nstack.current->print_tree());
	return true;
}

b32 Sema::
block() {
	auto e = nstack.current->as<Block>();

	// TODO(sushi) for cases like this we can probably get away with repeatedly pushing
	//			   w/o pops then just pop all at the end, but pop doesn't take a count
	//			   rn so yeah do that later! :)
	for(auto s = e->first_child<Stmt>(); s; s = s->next<Stmt>()) {
		nstack.push(s->first_child());
		switch(s->kind) {
			case stmt::label: {
				if(!label()) return false;
			} break;

			case stmt::block_final:
			case stmt::expression: {
				if(!expr()) return false;
			} break;
		}
		nstack.pop();
	}

	auto last = e->last_child<Stmt>();
	if(!last || last->kind != stmt::block_final) {
		e->type = &type::void_;
	} else {
		e->type = last->first_child<Expr>()->type;
	}

	return true;
}
} // namespace amu
