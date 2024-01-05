#include "Lexer.h"
#include "systems/Compiler.h"
#include "representations/Token.h"
#include "representations/Code.h"

namespace amu {

Lexer* Lexer::
create(Allocator* allocator, Code* code) {
	auto out = new (allocator->allocate(sizeof(Lexer))) Lexer;
	code->lexer = out;
	out->code = code;
	out->Processor::init("Lexer", code);
	out->tokens = Array<Token>::create(16);
	return out;
}

void Lexer::
destroy() {
	tokens.destroy();
	Processor::deinit();
}

b32 Lexer::
run() { 
	Processor::start();
	defer { 
		Processor::end(); 
		emit_diagnostics();
	};

	using enum Token::Kind;

	String stream = code->raw;

	u64 line = 1,
		 col = 1;

	Token t = {};
	Token last_token = {};

	u32 current_codepoint = *stream.str;

	// TODO(sushi) there's tons of places where we calc the codepoint several times 
	//             primarily in the disconnection between peek() and next()

	u32 peeked_codepoint = 0;
	u32 peeked_advance = 0;

	auto next = [&]() -> u32 {
		if(peeked_codepoint) {
			auto save = peeked_codepoint;
			peeked_codepoint = 0;
			return save;
		}
		current_codepoint = stream.advance().codepoint;
		if(!stream) return 0;
		if(current_codepoint == '\n') {
			line += 1;
			col = 1;
		} else {
			col += 1;
		}
		return current_codepoint;
	};

	auto current = [&]() -> u32 {
		return current_codepoint;
	};

	auto skip_whitespace = [&]() {
		while(1) {
			if(String::is_space(current())) {
				next();
			}
		}
	};

	// writes final data to the current token
	// and appends it to the tokens array 
	auto finish_token = [&]() {
		t.l1 = line;
		t.c0 = col;
		t.hash = t.raw.hash();
		if(t.kind >= FloatLiteral && t.kind <= StringLiteral) {
			t.group = Token::Group::Literal;
		} else if(t.kind >= Semicolon && t.kind <= RightArrowThick) {
			t.group = Token::Group::Control;
		} else if(t.kind >= Plus && t.kind <= DollarDouble) {
			t.group = Token::Group::Operator;
		} else if(t.kind >= Return && t.kind <= Deref) {
			t.group = Token::Group::Keyword;
		} else if(t.kind >= Unsigned8 && t.kind <= Float64) {
			t.group = Token::Group::Type;
		} else if(t.kind >= Directive_Include && t.kind <= Directive_CompilerBreak) {
			t.group = Token::Group::Directive;
		} else {
			Assert(0);
		}
		tokens.push(t);
		TRACE(code, "Lexed token ", t);
		last_token = t;
	};

	auto reset_token = [&]() {
		t = {
			.raw = {stream.str, 0},
			.code = code,
			.l0 = line,
			.c0 = col,
		};
	};

	auto is_identifier_char = [&](u32 c) {
		if(util::any(
				String::is_alnum(c), 
				c == '_', 
				c > 127))
			return true;
		return false;
	};

	auto is_integer_char = [&](u32 c) {
		if(String::is_digit(c) || c == '_')
			return true;
		return false;
	};

	auto keyword_or_identifier = [&](String s) {
		switch(s.hash()) {
#define strcase(s) case util::static_string_hash(s)
			strcase("return"):	  return Return;
			strcase("if"):		  return If;
			strcase("else"):	  return Else;
			strcase("for"):		  return For;
			strcase("while"):	  return While;
			strcase("break"):	  return Break;
			strcase("continue"):  return Continue;
			strcase("defer"):	  return Defer;
			strcase("switch"):	  return Switch;
			strcase("loop"):	  return Loop;
			strcase("struct"):	  return Struct;
			strcase("module"):	  return Module;
			strcase("import"):    return Import;
			strcase("void"):	  return Void;
			strcase("s8"):		  return Signed8;
			strcase("s16"):		  return Signed16;
			strcase("s32"):		  return Signed32;
			strcase("s64"):		  return Signed64;
			strcase("u8"):		  return Unsigned8;
			strcase("u16"):		  return Unsigned16;
			strcase("u32"):		  return Unsigned32;
			strcase("u64"):		  return Unsigned64;
			strcase("f32"):		  return Float32;
			strcase("f64"):		  return Float64;
			strcase("using"):	  return Using;
			strcase("and"):		  return And;
			strcase("or"):		  return Or;
			strcase("then"):	  return Then;
		}
		return Identifier;
	};

	auto directive_or_identifier = [&](String s) {
		switch(s.hash()) {
			strcase("include"):        return Directive_Include;
			strcase("run"):		       return Directive_Run;
			strcase("compiler_break"): return Directive_CompilerBreak;
		}
		return Identifier;
	};

	auto sid_or_identifier = [&](String s) {
		return Identifier;
	};

	while(stream) { using namespace util;
		skip_whitespace();
		reset_token();
		switch(current()) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9': {
				while(is_integer_char(next()));

				if(all_match('.', stream.str[0], stream.str[1])) {
					// we treat this as a DotDouble
					t.raw.count = stream.str - t.raw.str;
					t.kind = IntegerLiteral;
					t.scalar_value = t.raw.to_s64();
				} else if(any_match(current(), '.', 'e', 'E')) {
					next();
					while(is_integer_char(current()) && next());
					t.raw.count = stream.str - t.raw.str;
					t.kind = FloatLiteral;
					t.scalar_value = t.raw.to_f64();
				} else if(any_match(current(), 'x', 'X')) {
					next();
					while(is_integer_char(current()) && next());
					t.raw.count = stream.str - t.raw.str;
					t.kind = IntegerLiteral;
					t.scalar_value = t.raw.to_s64();
				} else {
					t.raw.count = stream.str - t.raw.str;
					t.kind = IntegerLiteral;
					t.scalar_value = t.raw.to_s64();
				}
			} break;

#define one_glyph_map(c1, t1) \
			case c1: {        \
				t.kind = t1;  \
				next();       \
			} break;
#define two_glyph_map(c1, t1, c2, t2) \
			case c1: {                \
				t.kind = t1;          \
				next();               \
				if(current() == c2) { \
					t.kind = t2;      \
					next();           \
				}                     \
			} break; 
#define two_glyph_map_alt(c1, t1, c2, t2, c3, t3) \
			case c1: {                            \
				t.kind = t1;                      \
				next();                           \
				if(current() == c2) {             \
					t.kind = t2;                  \
					next();                       \
				} else if(current() == c3) {      \
					t.kind = t3;                  \
					next();                       \
				}                                 \
			} break;
#define three_glyph_map(c1, t1, c2, t2, c3, t3) \
			case c1: {                          \
				t.kind = t1;                    \
				next();                         \
				if(current() == c2) {           \
					t.kind = t2;                \
					next();                     \
					if(current() == c3) {       \
						t.kind = t3;            \
						next();                 \
					}                           \
				}                               \
			} break;
#define two_or_three_glyph_map(c1, t1, c2, t2, c3, t3, c4, t4) \
			case c1: {                                         \
				t.kind = t1;                                   \
				next();                                        \
				if(current() == c2) {                          \
					t.kind = t2;                               \
					next();                                    \
				} else if(current() == c3) {                   \
					t.kind = t3;                               \
					next();                                    \
					if(current() == c4) {                      \
						t.kind = t4;                           \
						next();                                \
					}                                          \
				}                                              \
			} break;

			one_glyph_map('[', LSquare);
			one_glyph_map(']', RSquare);
			one_glyph_map(',', Comma);
			one_glyph_map('?', QuestionMark);
			one_glyph_map('@', At);
			one_glyph_map('#', Pound);
			one_glyph_map('`', Backtick);
			one_glyph_map(':', Colon);

			two_glyph_map('$', Dollar,          '$', DollarDouble);
			two_glyph_map('+', Plus,            '=', PlusEqual);
			two_glyph_map('*', Asterisk,        '=', AsteriskEqual);
			two_glyph_map('%', Percent,         '=', PercentEqual);
			two_glyph_map('~', Tilde,           '=', TildeEqual);
			two_glyph_map('^', Caret,           '=', CaretEqual);
			two_glyph_map('!', ExplanationMark, '=', ExplanationMarkEqual);
			two_glyph_map('-', Minus,           '>', RightArrow);

			two_glyph_map_alt('=', Equal, '=', EqualDouble, '>', RightArrowThick);
			two_glyph_map_alt('&', Ampersand, '=', AmpersandEqual, '&', AmpersandDouble);

			three_glyph_map('.', Dot, '.', DotDouble, '.', DotTriple);

			two_or_three_glyph_map('<', LessThan, '=', LessThanEqual, '<', LessThanDouble, '=', LessThanDoubleEqual);
			two_or_three_glyph_map('>', GreaterThan, '=', GreaterThanEqual, '>', GreaterThanDouble, '=', GreaterThanDoubleEqual);

			default: {
				while(is_identifier_char(next()));
				t.raw.count = stream.str - t.raw.str;
				
				if(last_token.kind == Pound) {
					auto kind = directive_or_identifier(t.raw);
					if(kind == Identifier) {
						Diag::unknown_directive(diag_stack, &t, t.raw);
						return false; // TODO(sushi) continuation
					}
					t.kind = kind;
					tokens.pop();
				} else if(last_token.kind == Dollar) {
					auto kind = sid_or_identifier(t.raw);
					if(kind == Identifier) {
						Diag::unknown_sid(diag_stack, &t, t.raw);
						return false; // TODO(sushi) continuation
					}
					t.kind = kind;
					tokens.pop();
				} else {
					t.kind = keyword_or_identifier(t.raw);
				}


				finish_token();
				Diag::invalid_token(diag_stack, &tokens[-1], &tokens[-1]);
				return false;
			}
		}
		finish_token();
	}
	
	return true;
}

void Lexer::
output(b32 human, String path) {
	FixMe;
//	FILE* out = fopen((char*)path.str, "w");
//	Assert(out);
//
//	Array<Token>& tokens = code->get_token_array();
//
//	if(human) {
//		DString* buffer = DString::create();
//
//		forI(tokens.count) {
//			Token& t = tokens.readref(i);
//			buffer->append((u64)t.kind, "\"", t.raw, "\"", t.l0, ",", t.c0, "\n");
//		}
//		
//		fwrite(buffer->str, buffer->count, 1, out);
//		buffer->deref();
//	} else {
//		Array<u32> data = Array<u32>::create(4*tokens.count);
//
//		Token* curt = tokens.readptr(0);
//		forI(tokens.count) {
//			data.push((u32)curt->kind);
//			data.push((u32)curt->l0);
//			data.push((u32)curt->c0);
//			data.push((u32)curt->raw.count);
//		}
//
//		fwrite(data.data, data.count*sizeof(u32), 1, out);
//		data.destroy();
//	}
//
//	fclose(out);
}

} // namespace amu
