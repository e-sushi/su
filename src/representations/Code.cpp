#include "representations/Code.h"
#include <condition_variable>
#include <mutex>
namespace amu {

Code* Code::
from(Code* code, Token* start) {
	Code* out;
	if(code->source) {
		SourceCode* sc = pool::add(compiler::instance.storage.source_code);
		sc->tokens.data = start;
		sc->tokens.count = code->get_tokens().data + code->get_tokens().count - start;
		out = sc->as<Code>();
	}
}

Code* Code::
from(Code* code, Token* start, Token* end) {
	Code* out;
	if(code->source) {
		SourceCode* sc = pool::add(compiler::instance.storage.source_code);
		sc->tokens.data = start;
		sc->tokens.count = end-start+1;
		out = sc->as<Code>();
	} else {
		auto c = (VirtualCode*)code;
		VirtualCode* vc = pool::add(compiler::instance.storage.virtual_code);
		vc->tokens = c->tokens.copy(start-c->tokens.data, end-start+1);
	}

	out->source = code->source;
	out->raw.str = start->raw.str;
	out->raw.count = end->raw.str - start->raw.str;

	if(end->kind == token::end_of_file) end--;

	out->identifier = DString::create(code->identifier, "/", start->raw);

	return out;
}

Code* Code::
from(Code* code, ASTNode* node) {
	auto out = Code::from(code, node->start, node->end);

	Parser::create(out);
	out->parser->root = node;

	switch(node->kind) {
		case ast::entity: {
			auto e = node->as<Entity>();
			switch(e->kind) {
				case entity::expr: {
					out->kind = code::expression;
				} break;
				case entity::var: {
					FixMe; // this is probably wrong or just shouldn't happen at all
					out->kind = code::label;
				} break;
			}
		} break;
		case ast::label: {
			out->kind = code::label;
		} break;
	}

	out->level = code::parse;

	// trying to create a Code object from an unhandled branch kind
	Assert(out->kind != code::unknown); 
	return out;
}

SourceCode* Code::
from(Source* source) {
	SourceCode* out = pool::add(compiler::instance.storage.source_code);
	out->raw = source->buffer;
	out->source = source;
	out->kind = code::source;
	out->ASTNode::kind = ast::code;
	out->identifier = source->name;
	return out;
}

//VirtualCode* Code::
//from(String s) {
//
//}

void Code::
destroy() {
	if(lexer) lexer->destroy();
	if(parser) parser->destroy();
	if(sema) sema->destroy();
	if(tac_gen) tac_gen->destroy();
	if(air_gen) air_gen->destroy();
	if(machine) machine->destroy();
}

//VirtualCode* Code::
//make_virtual() {
//
//}

b32 Code::
is_virtual() {
	return !this->source;
}

b32 Code::
is_processing() {
	return util::any(this->state, code::in_lex, code::in_parse, code::in_sema, code::in_tacgen, code::in_airgen, code::in_vm);
}

View<Token> Code::
get_tokens() {
	if(is_virtual()) {
		return ((VirtualCode*)this)->tokens.view();
	} else {
		return ((SourceCode*)this)->tokens;
	}
}

Array<Token>& Code::
get_token_array() {
	if(is_virtual()) {
		return ((VirtualCode*)this)->tokens;
	} else {
		return this->source->tokens;
	}
}

void Code::
add_diagnostic(Diagnostic d) {
	if(is_virtual()) {
		((VirtualCode*)this)->diagnostics.push(d);
	} else {
		this->source->diagnostics.push(d);
	}
}

b32 Code::
process_to(code::level l) {ZoneScoped;
	TracyCSetThreadName((char*)this->identifier.str);
	LockableName(mtx.mtx, (char*)this->identifier.str, this->identifier.count);
	while(1) {
		if(this->state >= (code::state)l) return true;
		switch(this->state) {
			case code::newborn: {
				if(!this->lexer) Lexer::create(this);
				change_state(code::in_lex);
				if(!this->lexer->start()) {
					change_state(code::failed);
					return false;
				}
				change_state(code::post_lex);
			} break;
			case code::post_lex: {
				if(!this->parser) Parser::create(this);
				change_state(code::in_parse);
				if(util::any(this->kind, code::source, code::module)) {
					// if this represents a source file or module we instead want to discretize
					// ourself and asyncronously process each child instead
					if(!this->parser->discretize_module()) return false;
					// spawn a new thread for each child using a promise to defer
					// when they begin
					std::promise<void> p;
					Future<void> fut{p.get_future().share()};
					auto futures = Array<Future<b32>>::create();
					for(auto c = first_child<Code>(); c; c = c->next<Code>()) {
						*futures.push() = c->process_to_async_deferred(fut, code::air);
					}
					// start all children then wait for each child to complete, 
					// if one of them fails then we early out and return fail ourself
					p.set_value();
					forI(futures.count) {
						if(!futures.read(i).get()){
							change_state(code::failed);
							return false;
						}
					}
					// there's no further processing needed for this code object (I believe), so
					// we can just return here
					change_state(code::post_airgen);
					return true;
					// otherwise, just parse this code object and move on like normal
				} else if(!this->parser->parse()) {
					 change_state(code::failed); 
					 return false;
				}
				change_state(code::post_parse);
			} break;
			case code::post_parse: {
				if(!this->sema) Sema::create(this);
				change_state(code::in_sema);
				if(!this->sema->start()) {
					change_state(code::failed);
					return false;
				}
				change_state(code::post_sema);
			} break;
			case code::post_sema: {
				if(!this->tac_gen) GenTAC::create(this);
				change_state(code::in_tacgen);
				this->tac_gen->generate();
				change_state(code::post_tacgen);
			} break;
			case code::post_tacgen: {
				if(!this->air_gen) GenAIR::create(this);
				change_state(code::in_airgen);
				this->air_gen->generate();
				change_state(code::post_airgen);
			} break;
			case code::post_airgen: {
				if(!this->machine) VM::create(this);
				change_state(code::in_vm);
				this->machine->run();
				change_state(code::post_vm);
			} break;
		}
	}
}

Future<b32> Code::
process_to_async(code::level level) {
	return threader.start(&Code::process_to, this, level);
}

Future<b32> Code::
process_to_async_deferred(Future<void> f, code::level level) {
	return threader.start_deferred(f, &Code::process_to, this, level);
}

b32 Code::
wait_until_level(code::level level, Code* dependent){ ZoneScoped;
	dependent->dependency = this;
	while(this->state < level) {
		mtx.lock();
		cv.wait(mtx);
		mtx.unlock();
		if(this->state == code::failed) return false;
	}
	return true;
}

void Code::
change_state(code::state s) { ZoneScoped;
	mtx.lock();
	defer { mtx.unlock(); };
	this->state = s;
	cv.notify_all();
}

DString* SourceCode::
display() {
	return DString::create(identifier);
}

DString* SourceCode::
dump() {
	return DString::create("SourceCode<", ScopedDStringRef(display()).x, ">");
}

DString* VirtualCode::
display() { return DString::create(Code::identifier); }

DString* VirtualCode::
dump() {
	return DString::create("VirtualCode<>");
}



namespace code {

TokenIterator::TokenIterator(Code* code) {
	this->code = code;
	this->curt = view::readptr(code->get_tokens(),	0);
	// this->stop = view::readptr(code::get_tokens(code), -1);
}

FORCE_INLINE Token*
TokenIterator::current() {
	return this->curt;
}

FORCE_INLINE token::kind
TokenIterator::current_kind() {
	return this->curt->kind;
}

FORCE_INLINE Token* 
TokenIterator::increment() {
	// if(this->curt == this->stop) return 0;
#if BUILD_SLOW
	this->curt++;
	if(this->curt->kind == token::directive_compiler_break) {
		this->curt++;
		DebugBreakpoint;
	}
	return this->curt;
#else
	return ++this->curt;
#endif
}

FORCE_INLINE Token* 
TokenIterator::next() { 
	return lookahead(1); 
}

FORCE_INLINE token::kind
TokenIterator::next_kind() { 
	Token* t = next(); 
	if(!t) return token::null; 
	return t->kind; 
}

FORCE_INLINE Token*
TokenIterator::prev() { 
	return lookback(1); 
}

FORCE_INLINE token::kind
TokenIterator::prev_kind() {
	Token* t = prev();
	if(!t) return token::null;
	return t->kind;
}


FORCE_INLINE Token*
TokenIterator::lookahead(u64 n) {
#if BUILD_SLOW
	Token* iter = this->curt;
	forI(n) {
		iter++;
		if(iter->kind == token::directive_compiler_break)
			iter++;
		// if(iter > this->stop) return 0;
	}
	return iter;
#else
	if(this->curt + n > this->stop) return 0;
	return this->curt + n;
#endif
}

FORCE_INLINE Token*
TokenIterator::lookback(u64 n) {
#if BUILD_SLOW
	Token* iter = this->curt;
	forI(n) {
		iter--;
		if(iter->kind == token::directive_compiler_break)
			iter--;
		if(iter < this->code->get_tokens().data) return 0;
	}
	return iter;
#else
	if(this->curt - n < this->code->get_tokens().data) return 0;
	return this->curt - n;
#endif
}

void TokenIterator:: 
skip_to_matching_pair() {
	u64 nesting = 0;
	switch(this->curt->kind) {
		case token::open_brace: { 
			while(increment()) {
				if(current_kind() == token::open_brace) nesting++;
				else if(current_kind() == token::close_brace) {
					if(nesting) nesting--; 
					else return;
				}
			}
		} break;
		case token::open_paren: {
			while(increment()) {
				if(current_kind() == token::open_paren) nesting++;
				else if(current_kind() == token::close_paren) {
					if(nesting) nesting--;
					else return;
				}
			}
		} break;
		case token::open_square: {
			while(increment()) {
				if(current_kind() == token::open_square) nesting++;
				else if(current_kind() == token::close_square) {
					if(nesting) nesting--;
					else return;
				}
			}
		} break;
	}
}

template<typename... T> FORCE_INLINE void TokenIterator::
skip_until(T... args) {
	while(!is_any(args...) && increment());
}

FORCE_INLINE b32 TokenIterator::
is(u32 kind) {
	return current_kind() == kind;
}

template<typename... T> FORCE_INLINE b32 TokenIterator::
is_any(T... args) {
	return (is(args) || ...);
}

FORCE_INLINE b32 TokenIterator::
next_is(u32 kind) {
	return next_kind() == kind;
}

FORCE_INLINE b32 TokenIterator::
prev_is(u32 kind) {
	return prev_kind() == kind;
}

DString* TokenIterator::
display_line() {
	u32 tab_offset = 0;

	u8* scan_left = curt->raw.str;

	while(scan_left != curt->code->raw.str && *scan_left != '\n'){
		scan_left--;
		if(*scan_left == '\t') tab_offset++;
	}

	u8* scan_right = scan_left + 1;

	while(scan_right != curt->code->raw.str + curt->code->raw.count && *scan_right != '\n')
		scan_right++;

	String line = {scan_left, s32(scan_right - scan_left)};
	
	s32 depth = curt->raw.str - scan_left + tab_offset * 4;

	DString* out = DString::create(line, "\n");

	forI(depth) {
		out->append(" ");
	}
	out->append("^");

	return out;
}


namespace format {

} // namespace format

namespace lines {

Lines 
get(Token* t, Options opt) {
	Lines out = {};
	out.lines = Array<String>::create();
	out.opt = opt;
	out.token = t;
	
	u8* scan_left = t->raw.str;

	// scan to beginning of current line
	while(scan_left != t->code->raw.str && *scan_left != '\n')
		scan_left--;

	ScopedArray<s32> newlines = Array<s32>::create();
	newlines.push(0);

	// we need to store newlines as offsets into a string
	// so we don't collect indexes going backwards cause we'd 
	// have to readjust them anyways 
	u32 before = opt.before;
	forI(before) { 
		while(scan_left != t->code->raw.str && *scan_left != '\n')
			scan_left--;
		if(scan_left == t->code->raw.str) break;
	}

	u8* scan_right = scan_left;
	u32 n_lines = amu::util::Min(opt.before, before) + opt.after + 1;
	forI(n_lines) {
		while(scan_right != t->code->raw.str + t->code->raw.count && *scan_right != '\n')
			scan_right++;
		newlines.push(s32(scan_right++-scan_left) + 1);
		if(scan_right == t->code->raw.str) break;
	}

	// turn the entire thing into a DString
	out.str = DString::create(String{
		scan_left, newlines.read(-1)
	});

	forI(newlines.count - 1) {
		out.lines.push({
			out.str->str + newlines.read(i),
			newlines.read(i+1) - newlines.read(i) - 1 
		});
	}

	if(opt.remove_leading_whitespace) {
		u32 min_leading = MAX_U32;
		forI(out.lines.count) {
			String line = out.lines.read(i);
			if(!line.count) continue;
			s32 whitespace_len = line.eat_whitespace().count;
			if(whitespace_len == line.count) continue;
			min_leading = amu::util::Min(min_leading, whitespace_len);
			if(!min_leading) break;
		}

		if(min_leading) {
			DString* nu = DString::create();
			forI(out.lines.count) {
				String line = out.lines.read(i);
				if(!line.count) nu->append("\n");
				else nu->append(String{line.str+min_leading, line.count-min_leading}, "\n");
			}
			DString* save = out.str;
			out.str = nu;
			save->deref();
		}

		out.lines = out.str->fin.find_lines();
	}

	if(opt.line_numbers) {
		
	}

	return out;
}

} // namespace lines

} // namespace code

void
to_string(DString* current, Code* c) {
	if(c->is_virtual()) { // TODO(sushi) more info for virtual code whenever its actually used
		auto vc = (VirtualCode*)c;
		current->append("VirtualCode<'", ScopedDeref(vc->display()).x, "'>");
	} else {
		auto sc = (SourceCode*)c;
		current->append("SourceCode<", code::strings[sc->kind], ">");
	}
}

DString*
to_string(Code* c) {
	DString* out = DString::create();
	to_string(out, c);
	return out;
}

} // namespace amu
