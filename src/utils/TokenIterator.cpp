#include "TokenIterator.h"
#include "representations/Code.h"

namespace amu {

TokenIterator::
TokenIterator(Code* code) {
	this->code = code;
	this->curt = code->tokens.start;
}

FORCE_INLINE Token* TokenIterator::
current() {
	return this->curt;
}

FORCE_INLINE Token::Kind TokenIterator::
current_kind() {
	return this->curt->kind;
}

FORCE_INLINE Token* TokenIterator::
increment() {
#if AMU_DEBUG
	this->curt++;
	if(this->curt->kind == Token::Kind::Directive_CompilerBreak) {
		this->curt++;
		DebugBreakpoint;
	}
	return this->curt;
#else
	return ++this->curt;
#endif
}

FORCE_INLINE Token* TokenIterator::
next() { 
	return lookahead(1); 
}

FORCE_INLINE Token::Kind TokenIterator::
next_kind() { 
	Token* t = next(); 
	if(!t) return Token::Kind::Null; 
	return t->kind; 
}

FORCE_INLINE Token* TokenIterator::
prev() { 
	return lookback(1); 
}

FORCE_INLINE Token::Kind TokenIterator::
prev_kind() {
	Token* t = prev();
	if(!t) return Token::Kind::Null;
	return t->kind;
}


FORCE_INLINE Token* TokenIterator::
lookahead(u64 n) {
#if AMU_DEBUG
	Token* iter = this->curt;
	forI(n) {
		iter++;
		if(iter->kind == Token::Kind::Directive_CompilerBreak)
			iter++;
	}
	return iter;
#else
	FixMe;
	if(this->curt + n > this->stop) return 0;
	return this->curt + n;
#endif
}

FORCE_INLINE Token* TokenIterator::
lookback(u64 n) {
#if AMU_DEBUG
	Token* iter = this->curt;
	forI(n) {
		iter--;
		if(iter->kind == Token::Kind::Directive_CompilerBreak)
			iter--;
		if(iter < this->code->tokens.start) return 0;
	}
	return iter;
#else
	FixMe;
	if(this->curt - n < this->code->get_tokens().data) return 0;
	return this->curt - n;
#endif
}

void TokenIterator:: 
skip_to_matching_pair() {
	FixMe;
//	u64 nesting = 0;
//	switch(this->curt->kind) {
//		case token::open_brace: { 
//			while(increment()) {
//				if(current_kind() == token::open_brace) nesting++;
//				else if(current_kind() == token::close_brace) {
//					if(nesting) nesting--; 
//					else return;
//				}
//			}
//		} break;
//		case token::open_paren: {
//			while(increment()) {
//				if(current_kind() == token::open_paren) nesting++;
//				else if(current_kind() == token::close_paren) {
//					if(nesting) nesting--;
//					else return;
//				}
//			}
//		} break;
//		case token::open_square: {
//			while(increment()) {
//				if(current_kind() == token::open_square) nesting++;
//				else if(current_kind() == token::close_square) {
//					if(nesting) nesting--;
//					else return;
//				}
//			}
//		} break;
//	}
}

template<typename... T> FORCE_INLINE void TokenIterator::
skip_until(T... args) {
	while(!is_any(args...) && increment());
}

FORCE_INLINE b32 TokenIterator::
is(Token::Kind kind) {
	return current_kind() == kind;
}

template<typename... T> FORCE_INLINE b32 TokenIterator::
is_any(T... args) {
	return (is(args) || ...);
}

FORCE_INLINE b32 TokenIterator::
next_is(Token::Kind kind) {
	return next_kind() == kind;
}

FORCE_INLINE b32 TokenIterator::
prev_is(Token::Kind kind) {
	return prev_kind() == kind;
}

DString TokenIterator::
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

	DString out = DString(line, "\n");

	forI(depth) {
		out.append(" ");
	}
	out.append("^");

	return out;
}


} // namespace amu
