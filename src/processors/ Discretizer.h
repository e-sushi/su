/*

    The Discretizer processes source/module Code objects into discrete Code objects representing independently process-able
    elements of the module/source.

    Originally I did this in Parser for SourceCode before starting any actual parsing, but it didn't make much sense to do it there
    after I stopped doing the prescan thing in parsing. 

    This might be more appropriate to put until systems/ but since it turns Code objects into other Code objects, I view it as a 
    stage of processing. 

    Later on we can experiment with discretizing smaller units of Code as well, such as function interiors or structures. Though, these 
    will probably be small enough to make discretizing (which is primarily done for global name resolution and parallelization) not worth it.

	On top of discretizing the Code objects this does the final dispatch of Code objects onto their own threads, all the way to AIR
	generation, though this should probably be customizable later.

	TODO(sushi) this reimplements label parsing, need to join them eventually so they don't get disconnected

*/


#include "representations/Code.h"
namespace amu {

b32
discretize(Code* code) {
	Assert(code->kind == code::module || code->kind == code::source);
	
	if(code->kind == code::module) {
		// LexicalScopes need to store a view over their tokens and their
		// list of labels need to be offset from their start
		TODO("handle module discretizations");
	}

	// we want to setup the symbol table for this code object so ensure
	// that it has a parser
	if(!code->parser) Parser::create(code);
	
	LexicalScope* scope = 0;

	auto token = code::TokenIterator(code);

	auto m = Module::create();
	code->parser->table.push(&m->table);
	defer { code->parser->table.pop(); };
	scope = code->get_tokens().data->scope;

	auto tokens = code->get_token_array();
	forI(scope->labels.count) {
		token.curt = tokens.readptr(scope->labels.read(i));
		auto lde = code->parser->table.search_local(token.current()->hash);
		if(lde) {
			diagnostic::parser::
				label_already_defined(token.current(), lde->start);
			return false;
		}
		
		// {{ label_get
		auto e = Expr::create(expr::identifier);
		e->start = e->end = token.current();

		token.increment();
		switch(token.current_kind()) {
			case token::colon: break;
			case token::comma: {
				TODO("join discretization with parsing so that we aren't reimplementing label parsing");
			} break;
			default: {
				diagnostic::parser::
					expected_colon_for_label(token.current());
				return false;
			} break;
		}

		auto l = Label::create();
		l->start = e->start;
		l->end = token.current();
		node::insert_first(l, e);
		// }}

		code->parser->table.add(l->start->raw, l);
		l->table = code->parser->table.current;
		
		auto c = Code::from(code, l->start);
		c->kind = code::label;
		c->level = code::lex;
		l->code = c;
		node::insert_last(code, c);
	}

	
	

}

} // namespace amu
