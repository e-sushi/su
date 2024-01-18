#include "representations/Code.h"
#include "representations/Source.h"
#include "storage/String.h"
#include "systems/Messenger.h"
#include "systems/Compiler.h"
#include "processors/Lexer.h"
#include <condition_variable>
#include <mutex>

#include "Code.h"

namespace amu {

Code* Code::
from(Source* source) {
	auto out = new (compiler.bump.allocate(sizeof(Code))) Code;
	out->kind = Code::Kind::Source;
	out->stage = Code::Stage::Newborn;
	out->raw = source->buffer;
	out->identifier = source->name;
	out->source = source;
	out->allocator.init();
	return out;
}

DString Code::
display() {
	NotImplemented;
	return {};
}

DString Code::
dump() {
	NotImplemented;
	return {};
}

//Code* Code::
//from(Code* code, Token* start) {
//	Code* out;
//	if(code->source) {
//		auto scode = code->as<SourceCode>();
//		SourceCode* sc = compiler::instance.storage.source_code.add();
//		sc->tokens.data = start;
//		sc->tokens.count = scode->tokens.data + scode->tokens.count - start;
//		sc->source = code->source;
//		out = sc->as<Code>();
//	} else {
//		TODO("VirtualCode object from single start token");
//	}
//
//	out->identifier = DString::create(code->identifier, "/", start->raw);
//
//	return out;
//}
//
//Code* Code::
//from(Code* code, Token* start, Token* end) {
//	Code* out;
//	if(code->source) {
//		SourceCode* sc = compiler::instance.storage.source_code.add();
//		sc->tokens.data = start;
//		sc->tokens.count = end-start+1;
//		out = sc->as<Code>();
//	} else {
//		auto c = (VirtualCode*)code;
//		VirtualCode* vc = compiler::instance.storage.virtual_code.add();
//		vc->tokens = c->tokens.copy(start-c->tokens.data, end-start+1);
//	}
//
//	out->source = code->source;1
//	out->raw.str = start->raw.str;
//	out->raw.count = end->raw.str - start->raw.str;
//
//	if(end->kind == token::end_of_file) end--;
//
//	out->identifier = DString::create(code->identifier, "/", start->raw);
//
//	messenger::qdebug(out, String("created Code object from "), ScopedDeref(code->display()).x->fin);
//
//	return out;
//}
//
//Code* Code::
//from(Code* code, ASTNode* node) {
//	auto out = Code::from(code, node->start, node->end);
//
//	Parser::create(out);
//	out->parser->root = node;
//
//	switch(node->kind) {
//		case ast::entity: {
//			auto e = node->as<Entity>();
//			switch(e->kind) {
//				case entity::expr: {
//					out->kind = code::expression;
//				} break;
//				case entity::var: {
//					FixMe; // this is probably wrong or just shouldn't happen at all
//					out->kind = code::label;
//				} break;
//			}
//		} break;
//		case ast::label: {
//			out->kind = code::label;
//		} break;
//	}
//
//	out->level = code::parse;
//
//	messenger::qdebug(out, String("created Code object from "), ScopedDeref(code->display()).x->fin);
//
//	// trying to create a Code object from an unhandled branch kind
//	Assert(out->kind != code::unknown); 
//	return out;
//}
//
//SourceCode* Code::
//from(Source* source) {
//	SourceCode* out = compiler::instance.storage.source_code.add();
//	out->raw = source->buffer;
//	out->source = source;
//	out->kind = code::source;
//	out->ASTNode::kind = ast::code;
//	out->identifier = source->name;
//	
//	// for now the name of the Source Module will just be the name of the file
//	// which means we'll currently enforce the same restrictions on filenames 
//	// that we enforce on labels, though I'm not sure if this is how I'd want 
//	// it to work forever.
//	
//	// we create a Module and VirtualLabel for that module here
//	auto m = Module::create();
//
//	auto filename = source->front;
//	while(filename) {
//		auto cp = string::codepoint(filename);
//		if(string::isspace(cp) || !(isalnum(cp) || cp == '_' || cp > 127)) {
//			diagnostic::lexer::
//				filename_cant_be_identifier(out);
//			return 0;
//		}
//		filename.advance();
//	}
//
//	auto l = VirtualLabel::create(DString::create(source->front));
//	l->start->code = out;
//
//	l->entity = m;
//	m->label = l;
//	
//	// add the module to its own table
//	// this allows a source file to refer to itself which handles global
//	// namespace resolution similar to C++'s ::Thing. It also prevents 
//	// things with the same name as the file being defined in its global
//	// scope. Later on I plan to support a formal definition of the file's
//	// module using a syntax like
//	// 		filename :: module;
//	// whose purpose would be primarily to setup parameters for modules that 
//	// are represented by files:
//	// 		filename :: module(...);
//	m->table->add(filename, l);
//	l->table = m->table;
//	
//	source->module = m;
//
//	messenger::qdebug(out, String("created SourceCode"));
//	return out;
//}
//
////VirtualCode* Code::
////from(String s) {
////
////}
//
//void Code::
//destroy() {
//	if(lexer) lexer->destroy();
//	if(parser) parser->destroy();
//	if(sema) sema->destroy();
//	if(tac_gen) tac_gen->destroy();
//	if(air_gen) air_gen->destroy();
//	if(machine) machine->destroy();
//}
//
////VirtualCode* Code::
////make_virtual() {
////
////}
//
//b32 Code::
//is_virtual() {
//	return !this->source;
//}
//
//b32 Code::
//is_processing() {
//	return util::any(this->state, code::in_lex, code::in_parse, code::in_sema, code::in_tacgen, code::in_airgen, code::in_vm);
//}
//
//View<Token> Code::
//get_tokens() {
//	if(is_virtual()) {
//		return ((VirtualCode*)this)->tokens.view();
//	} else {
//		return ((SourceCode*)this)->tokens;
//	}
//}
//
//Array<Token>& Code::
//get_token_array() {
//	if(is_virtual()) {
//		return ((VirtualCode*)this)->tokens;
//	} else {
//		return this->source->tokens;
//	}
//}
//
//void Code::
//add_diagnostic(Diagnostic d) {
//	if(is_virtual()) {
//		((VirtualCode*)this)->diagnostics.push(d);
//	} else {
//		this->source->diagnostics.push(d);
//	}
//}
//
//b32 Code::
//process_to(code::level l) {
//	while(1) {
//		if(this->state >= (code::state)l) return true;
//		switch(this->state) {
//			
//			case code::newborn: {
//				messenger::qdebug(this, String("processing to lex"));
//
//				if(!this->lexer) Lexer::create(this);
//				change_state(code::in_lex);
//
//				if(!this->lexer->start()) {
//					messenger::qdebug(this, String("failed lexing"));
//					change_state(code::failed);
//					return false;
//				}
//
//				change_state(code::post_lex);
//
//				messenger::qdebug(this, String("finished lex processing"));
//			} break;
//
//			case code::post_lex: {
//				messenger::qdebug(this, String("performing parsing"));
//
//				if(!this->parser) Parser::create(this);
//				if(this->kind == code::source) { // TODO(sushi) put this somewhere better
//					// we need to assign the label created for this Source code as the 
//					// root of the Parser
//					this->parser->root = this->source->module->label;
//				}
//				change_state(code::in_parse);
//			
//				if(util::any(this->kind, code::source, code::module)) {
//					// if this represents a source file or module we want to discretize
//					// ourself and asyncronously process each child instead
//					if(!this->parser->discretize_module()) return false;
//
//					// spawn a new thread for each child using a promise to defer
//					// when they begin
//					std::promise<void> p;
//					Future<void> fut{p.get_future().share()};
//
//					auto futures = Array<Future<b32>>::create();
//					for(auto c = first_child<Code>(); c; c = c->next<Code>()) {
//						*futures.push() = c->process_to_async_deferred(fut, code::air);
//					}
//
//					// start all children then wait for each child to complete, 
//					// if one of them fails then we early out and return fail ourself
//					p.set_value();
//					forI(futures.count) {
//						if(!futures.read(i).get()) {
//							change_state(code::failed);
//							return false;
//						}
//					}
//
//					// there's no further processing needed for this code object (I believe), so
//					// we can just return here and say that we've made it to AIR
//					messenger::qdebug(this, String("finished parsing"));
//					change_state(code::post_airgen);
//					return true;
//					// otherwise, just parse this code object and move on like normal
//				} else if(!this->parser->parse()) {
//					 messenger::qdebug(this, String("failed parsing"));
//					 change_state(code::failed); 
//					 return false;
//				}
//				
//				// if we come across a module declaration in parsing, we parse 
//				// the module's header then immediately return to process_to
//				// so that we may do discretization of the module in the same 
//				// way we do it for source files.
//				// we do not notify waiters since the module isn't actually done
//				// being processed yet
//				if(this->kind == code::module) {
//					change_state(code::post_lex, false);
//					this->parser->root->as<Label>()->entity->as<Module>()->code = this;
//					continue;
//				}
//
//				messenger::qdebug(this, String("finished parsing"));
//				change_state(code::post_parse);
//			} break;
//			case code::post_parse: {
//				messenger::qdebug(this, String("performing semantic analysis"));
//
//				if(!this->sema) Sema::create(this);
//				change_state(code::in_sema);
//				if(!this->sema->start()) {
//					messenger::qdebug(this, String("failed semantic analysis"));
//					change_state(code::failed);
//					return false;
//				}
//				change_state(code::post_sema);
//
//				messenger::qdebug(this, String("finished semantic analysis"));
//			} break;
//			case code::post_sema: {
//				messenger::qdebug(this, String("generating TAC"));
//
//				if(!this->tac_gen) GenTAC::create(this);
//				change_state(code::in_tacgen);
//				this->tac_gen->generate();
//				change_state(code::post_tacgen);
//
//				messenger::qdebug(this, String("finished TAC generation"));
//			} break;
//			case code::post_tacgen: {
//				messenger::qdebug(this, String("generaing AIR"));
//
//				if(!this->air_gen) GenAIR::create(this);
//				change_state(code::in_airgen);
//				this->air_gen->generate();
//				change_state(code::post_airgen);
//
//				messenger::qdebug(this, String("finished generating AIR"));
//			} break;
//			case code::post_airgen: {
//				messenger::qdebug(this, String("running through VM"));
//
//				if(!this->machine) VM::create(this);
//				change_state(code::in_vm);
//				this->machine->run();
//				change_state(code::post_vm);
//
//				messenger::qdebug(this, String("finished running through VM"));
//			} break;
//		}
//	}
//}
//
//Future<b32> Code::
//process_to_async(code::level level) {
//	return threader.start(&Code::process_to, this, level);
//}
//
//Future<b32> Code::
//process_to_async_deferred(Future<void> f, code::level level) {
//	return threader.start_deferred(f, &Code::process_to, this, level);
//}
//
//b32 Code::
//wait_until_level(code::level level, Code* dependent){ 
//	messenger::qdebug(dependent, String("waiting until "), ScopedDeref(this->display()).x->fin, String(" reaches stage "), code::state_strings[level]);
//	dependent->dependency = this;
//	mtx.lock();
//	defer { mtx.unlock(); };
//	while(this->state < (code::state)level) {
//		messenger::qdebug(dependent, String("sleeping until stage is reached"));
//		cv.wait(mtx);
//		messenger::qdebug(dependent, String("notified, checking stage of dependency (which is "), code::state_strings[this->state], String(")"));
//	}
//	if(this->state == code::failed) return false;
//	messenger::qdebug(dependent, String("dependency on "), ScopedDeref(this->display()).x->fin, String(" fulfilled"));
//	return true;
//}
//
//void Code::
//change_state(code::state s, b32 notify) { 
//	mtx.lock();
//	defer { mtx.unlock(); };
//	messenger::qdebug(this, String("changing state to "), code::state_strings[s]);
//	this->state = s;
//	if(notify) {
//		messenger::qdebug(this, String("changed state and notifying waiters. new state: "), code::state_strings[s]);
//		if(compiler::instance.options.break_on_error && s == code::failed) {
//			DebugBreakpoint;
//		}
//		cv.notify_all();
//	}
//}
//
//DString* SourceCode::
//display() {
//	return DString::create(identifier);
//}
//
//DString* SourceCode::
//dump() {
//	return DString::create("SourceCode<", ScopedDStringRef(display()).x, ">");
//}
//
//DString* VirtualCode::
//display() { return DString::create(Code::identifier); }
//
//DString* VirtualCode::
//dump() {
//	return DString::create("VirtualCode<>");
//}
//
//


b32 Code::
process_to(Stage target) {
	DEBUG(this, "requested to process to stage ", target);
	using enum Stage;
	while(1) {
		if(stage >= target) return true;
		switch(stage) {
			case Newborn: {
				TRACE(this, "Newborn processing to stage Lex");
				Assert(!lexer);
				lexer = Lexer::create(&allocator, this);
				if(!lexer->run()) return false;
				stage = Lex;
			} break;
		}
	}
}

void
to_string(DString& current, Code::Stage stage) {
	using enum Code::Stage;
	switch(stage) {
		case Newborn: current.append("Newborn"); return;
		case Lex: current.append("Lex"); return;
		case Parse: current.append("Parse"); return;
		case Sema: current.append("Sema"); return;
		case TAC: current.append("TAC"); return;
		case AIR: current.append("AIR"); return;
		case VM: current.append("VM"); return;
	}
	FATAL(MessageSender(), "unknown code stage passed to 'to_string': ", (u32)stage);
}

void
to_string(DString& current, Code* c) {
	current.append("Code<>");
}

} // namespace amu
