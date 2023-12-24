#include "Lexer.h"
#include "representations/Token.h"
namespace amu {
namespace lexer::internal {

#define strcase(s) case string::static_hash(s)

local token::kind
token_is_keyword_or_identifier(String raw) {
	switch(raw.hash()) {
		strcase("return"):	  return token::return_;
		strcase("if"):		  return token::if_;
		strcase("else"):	  return token::else_;
		strcase("for"):		  return token::for_;
		strcase("while"):	  return token::while_;
		strcase("break"):	  return token::break_;
		strcase("continue"):  return token::continue_;
		strcase("defer"):	  return token::defer_;
		strcase("switch"):	  return token::switch_;
		strcase("loop"):	  return token::loop;
		strcase("struct"):	  return token::structdecl;
		strcase("module"):	  return token::moduledecl;
		strcase("import"):    return token::import;
		strcase("void"):	  return token::void_;
		strcase("s8"):		  return token::signed8;
		strcase("s16"):		  return token::signed16;
		strcase("s32"):		  return token::signed32;
		strcase("s64"):		  return token::signed64;
		strcase("u8"):		  return token::unsigned8;
		strcase("u16"):		  return token::unsigned16;
		strcase("u32"):		  return token::unsigned32;
		strcase("u64"):		  return token::unsigned64;
		strcase("f32"):		  return token::float32;
		strcase("f64"):		  return token::float64;
		strcase("str"):		  return token::string;
		strcase("any"):		  return token::any;
		strcase("using"):	  return token::using_;
		strcase("in"):		  return token::in;
		strcase("and"):		  return token::double_ampersand;
		strcase("or"):		  return token::logi_or;
		strcase("then"):	  return token::then;
	}
	return token::identifier;
}


local token::kind
token_is_directive_or_identifier(String raw) {
	switch(raw.hash()) {
		strcase("import"):				   return token::directive_import;
		strcase("internal"):			   return token::directive_internal;
		strcase("run"):					   return token::directive_run;
		strcase("compiler_break"):		   return token::directive_compiler_break;
		strcase("print_type"):			   return token::directive_print_type;
		strcase("print_meta_type"):		   return token::directive_print_meta_type;
		strcase("compiler_break_air_gen"): return token::directive_compiler_break_air_gen;
		strcase("vm_break"):			   return token::directive_vm_break;
		strcase("rand_int"):			   return token::directive_rand_int;
	}

	return token::identifier;
}

local token::kind
token_is_sid_or_identifier(String raw) {
	switch(raw.hash()) {
		strcase("print"): return token::sid_print;
	}

	return token::identifier;
}

FORCE_INLINE b32 
is_identifier_char(u32 codepoint) {
	if(isalnum(codepoint) || codepoint == '_' || codepoint > 127) 
		return true;
	if(string::isspace(codepoint)) 
		return false;
	return false;
}

} // namespace internal

Lexer* Lexer::
create() {
	Lexer* out = compiler::instance.storage.lexers.add();
	out->code = code;
	code->lexer = out;
	return out;
}

void Lexer::
destroy() {
}

void Lexer::
output(b32 human, String path) {
	FILE* out = fopen((char*)path.str, "w");
	if(!out) {
		diagnostic::internal::
			valid_path_but_internal_err(code, path, "TODO(sushi) get error info for failing to open lexer::output");
		return;
	}

	Array<Token>& tokens = code->get_token_array();

	if(human) {
		DString* buffer = DString::create();

		forI(tokens.count) {
			Token& t = tokens.readref(i);
			buffer->append((u64)t.kind, "\"", t.raw, "\"", t.l0, ",", t.c0, "\n");
		}
		
		fwrite(buffer->str, buffer->count, 1, out);
		buffer->deref();
	} else {
		Array<u32> data = Array<u32>::create(4*tokens.count);

		Token* curt = tokens.readptr(0);
		forI(tokens.count) {
			data.push((u32)curt->kind);
			data.push((u32)curt->l0);
			data.push((u32)curt->c0);
			data.push((u32)curt->raw.count);
		}

		fwrite(data.data, data.count*sizeof(u32), 1, out);
		data.destroy();
	}

	fclose(out);
}

Lexer* Lexer::
create(String buffer) {
	auto out = memory::allocate<Lexer>();
	out->tokens = Array<Token>::create(16);
	out->diag_stack = Array<Diag>::create();
	out->buffer = buffer;
	return out;
}

void Lexer::
destroy() {
	tokens.destroy();
	diag_stack.destroy();
}

b32 Lexer::
run() { 
	Processor::start("lexer");

	using enum Token::Kind;
	using enum Token::Group;

	messenger.dispatch(MessageBuilder::start(code, Message::Kind::Debug)
			.append("beginning lex")
			.message);

	auto start = util::stopwatch::start();

	String stream = code->raw;

	u64 line = 1,
		 col = 1;

	Token t = {};
	Token last_token = {};

	u32 peeked_codepoint = 0;

	auto next = [&]() -> u32 {
		if(peeked_codepoint) {
			auto out = peeked_codepoint;
			peeked_codepoint = 0;
			return out;
		}
		auto codepoint = stream.advance();
		if(!stream) return 0;
		if(codepoint == '\n') {
			line += 1;
			col = 1;
		} else {
			col += 1;
		}
		return codepoint;
	};

	auto peek = [&]() -> u32 {
		if(!peeked_codepoint) peeked_codepoint = next();
		return peeked_codepoint;
	};

	auto skip_whitespace = [&]() {
		while(string::isspace(peek())) next();
	};

	// writes final data to the current token
	// and appends it to the tokens array 
	auto finish_token = [&]() {
		t.l1 = line;
		t.c0 = col;
		t.hash = t.raw.hash();
		if(t.kind >= FloatLiteral && t.kind <= StringLiteral) {
			t.group = Literal;
		} else if(t.kind >= Semicolon && t.kind <= RightArrowThick) {
			t.group = Control;
		} else if(t.kind >= Plus && t.kind <= DollarDouble) {
			t.group = Operator;
		} else if(t.kind >= Return && t.kind <= Deref) {
			t.group = Keyword;
		} else if(t.kind >= Unsigned8 && t.kind <= Float64) {
			t.group = Type;
		} else if(t.kind >= Directive_Include && t.kind <= Directive_CompilerBreak) {
			t.group = Directive;
		} else {
			Assert(0);
		}
		tokens.push(t);
		last_token = t;
	};

	auto reset_token = [&]() {
		t = {
			.raw = {stream.str, 0},
			.l0 = line,
			.c0 = col,
		};
	};

	auto is_identifier_char = [&](u32 c) {
		if(util::any(String::is_alnum(codepoint), codepoint == '_', codepoint > 127))
			return true;
		return false;
	};

	auto keyword_or_identifier = [&](String s) {
		switch(s.hash()) {
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
		switch(raw.hash()) {
			strcase("include"):        return Directive_Include;
			strcase("run"):		       return Directive_Run;
			strcase("compiler_break"): return Directive_CompilerBreak;
		}
		return Identifier;
	};

	while(stream) { using namespace util;
		reset_token();
		skip_whitespace();
		auto c = next();
		// TODO(sushi) test perf difference when is_digit and is_alpha are moved to be 
		//             a bunch of cases
		if(String::is_digit(c)) {
			while(String::is_digit(c) || c == '_') c = next();

			if(all_match('.', stream.str[0], stream.str[1])) {
				// we treat this as a DotDouble
				t.raw.count = stream.str - t.raw.str;
				t.kind = IntegerLiteral;
				t.scalar_value = t.raw.to_s64();
			} else if(any_match(c, '.', 'e', 'E')) {
				c = next();
				while(String::is_digit(c)) c = next();
				t.raw.count = stream.str - t.raw.str;
				t.kind = FloatLiteral;
				t.scalar_value = t.raw.to_f64();
			} else if(any_match(c, 'x', 'X')) {
				c = next();
				while(String::is_xdigit(c)) c = next();
				t.raw.count = stream.str - t.raw.str;
				t.kind = IntegerLiteral;
				t.scalar_value = t.raw.to_s64();
			} else {
				t.raw.count = stream.str - t.raw.str;
				t.kind = IntegerLiteral;
				t.scalar_value = t.raw.to_s64();
			}
		} else if(String::is_alpha(c)) {
			while(is_identifier_char(c)) c = next();
			t.raw.count = stream.str - t.raw.str;
			
			if(last_token.kind == Pound) {
				auto kind = directive_or_identifier(t.raw);
				if(kind == Identifier) {
					diag_stack.push(Diag::unknown_directive(t, t.raw));
					return false; // TODO(sushi) continuation
				}
				t.kind = kind;
				tokens.pop();
			} else if(last_token.kind == Dollar) {
				auto kind = sid_or_identifier(t.raw);
				if(kind == Identifier) {
					diag_stack.push(Diag::unknown_sid(t, t.raw));
					return false; // TODO(sushi) continuation
				}
				t.kind = kind;
				tokens.pop();
			} else {
				t.kind = keyword_or_identifier(t.raw);
			}
		} else switch(c) {
#define one_glyph_map(c1, t1) \
			case c1: {        \
				t.kind = t1;  \
				c = next();   \
			} break;
#define two_glyph_map(c1, t1, c2, t2) \
			case c1: {                \
				t.kind = t1;          \
				c = next();           \
				if(c == c2) {         \
					t.kind = t2;      \
					c = next();       \
				}                     \ 
			} break; 
#define two_glyph_map_alt(c1, t1, c2, t2, c3, t3) \
			case c1: {                            \
				t.kind = t1;                      \
				c = next();                       \
				if(c == c2) {                     \
					t.kind = t2;                  \
					c = next();                   \
				} else if(c == c3) {              \
					t.kind = t3;                  \
					c = next();                   \
				}                                 \
			} break;
#define three_glyph_map(c1, t1, c2, t2, c3, t3) \
			case c1: {                          \
				t.kind = t1;                    \
				c = next();                     \
				if(c == c2) {                   \
					t.kind = t2;                \
					c = next();                 \
					if(c == c3) {               \
						t.kind = t3;            \
						c = next();             \
					}                           \
				}                               \
			} break;
#define two_or_three_glyph_map(c1, t1, c2, t2, c3, t3, c4, t4) \
			case c1: {                                         \
				t.kind = t1;                                   \
				c = next();                                    \
				if(c == c2) {                                  \
					t.kind = t2;                               \
					c = next();                                \
				} else if(c == c3) {                           \
					t.kind = t3;                               \
					c = next();                                \
					if(c == c4) {                              \
						t.kind = t4;                           \
						c = next();                            \
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
			two_or_three_glyph_map('<', GreaterThan, '=', GreaterThanEqual, '<', GreaterThanDouble, '=', GreaterThanDoubleEqual);

			default: return Diag::invalid_token(t);
		}
		finish_token();
	}
	
	// TODO(sushi) this always computes the message which is useless if we're
	//             not emitting debug 
	auto time = util::stopwatch::peek(start);
	messenger.dispatch(
			MessageBuilder::start(code, Message::Kind::Debug)
			.append("finished lex in ")
			.append(util::format_time(time)));

	return true;
}

} // namespace amu
