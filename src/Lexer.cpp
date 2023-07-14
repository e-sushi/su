namespace amu {
namespace lex {

namespace internal {

#define strcase(s) case string::static_hash(s)

local token::kind
token_is_keyword_or_identifier(String raw) {
    switch(string::hash(raw)) {
        strcase("return"):    return token::return_;
		strcase("if"):        return token::if_;
		strcase("else"):      return token::else_;
		strcase("for"):       return token::for_;
		strcase("while"):     return token::while_;
		strcase("break"):     return token::break_;
		strcase("continue"):  return token::continue_;
		strcase("defer"):     return token::defer_;
		strcase("switch"):    return token::switch_;
		strcase("loop"):      return token::loop;
		strcase("struct"):    return token::structdecl;
		strcase("module"):    return token::moduledecl;
		strcase("void"):      return token::void_;
		strcase("s8"):        return token::signed8;
		strcase("s16"):       return token::signed16;
		strcase("s32"):       return token::signed32;
		strcase("s64"):       return token::signed64;
		strcase("u8"):        return token::unsigned8;
		strcase("u16"):       return token::unsigned16;
		strcase("u32"):       return token::unsigned32;
		strcase("u64"):       return token::unsigned64;
		strcase("f32"):       return token::float32;
		strcase("f64"):       return token::float64;
		strcase("str"):       return token::string;
		strcase("any"):       return token::any;
		strcase("using"):     return token::using_;
    }
    return token::identifier;
}


local token::kind
token_is_directive_or_identifier(String raw) {
    switch(string::hash(raw)) {
        strcase("import"):         return token::directive_import;
		strcase("internal"):       return token::directive_internal;
		strcase("run"):            return token::directive_run;
		strcase("compiler_break"): return token::directive_compiler_break;
    }

    return token::identifier;
}

FORCE_INLINE b32 
is_identifier_char(u32 codepoint) {
    if(isalnum(codepoint) || codepoint == '_' || codepoint > 127) 
        return true;
    if(is_whitespace(codepoint)) 
        return false;
    return false;
}

#undef strcase

} // namespace internal

Lexer
init(Source* source) {
    Lexer out;
    out.source = source;
    out.tokens = array::init<Token>(source->file->bytes); // it will surely not take this many, but we want to try and minimize the amount of reallocations we need to do
	out.global_labels = array::init<spt>();
	out.structs = array::init<spt>();
	out.modules = array::init<spt>();
	out.funcarrows = array::init<spt>();
    out.status = {};
    return out;
}

void 
deinit(Lexer& lexer) {
    array::deinit(lexer.tokens);
    lexer.source = 0;
}

void
execute(Lexer& lexer) {
#define stream_next { string::advance(stream); line_col++; } 

//!ref: https://github.com/pervognsen/bitwise/blob/master/ion/lex.c
#define CASE1(c1,t1) \
case c1:{ 	         \
token.kind = t1;     \
stream_next;         \
}break;

#define CASE2(c1,t1, c2,t2) \
case c1:{                   \
token.kind = t1;            \
stream_next;                \
if(*stream.str == c2){      \
token.kind = t2;            \
stream_next;                \
}                           \
}break;

#define CASE3(c1,t1, c2,t2, c3,t3) \
case c1:{                          \
token.kind = t1;                   \
stream_next;                       \
if      (*stream.str == c3){       \
token.kind = t3;                   \
stream_next;                       \
}else if(*stream.str == c2){       \
token.kind = t2;                   \
stream_next;                       \
}                                  \
}break;

    Stopwatch lexer_time = start_stopwatch();
    
    messenger::dispatch(message::attach_sender(lexer.source,
        message::debug(message::verbosity::stages, 
            String("beginning lexical analysis."))));

    String stream = lexer.source->buffer;

    u32 line_num = 1, line_col = 1;
    u8* line_start = stream.str;

	u32 scope_level = 0;

	Token last_token = {};
	b32 label_latch = false; // true when a label has been found, set back to false when a label ending token is crossed (a semicolon or close brace)

    while(stream) {
        Token token = {};
        token.source = lexer.source;
        token.l0 = line_num;
        token.c0 = line_col;
        token.line_start = line_start;
        token.raw.str = stream.str;

        switch(utf8codepoint(stream.str)) {
			case '\t': case '\n': case '\v': case '\f':  case '\r':
			case ' ': case 133: case 160: case 5760: case 8192:
			case 8193: case 8194: case 8195: case 8196: case 8197:
			case 8198: case 8199: case 8200: case 8201: case 8202:
			case 8232: case 8239: case 8287: case 12288:{
				while(is_whitespace(utf8codepoint(stream.str))){
					if(*stream.str == '\n'){
						line_start = stream.str+1;
						line_num++;
						line_col = 0;
					};
					stream_next;
				}
			}continue;

			case '0': case '1': case '2': case '3': case '4': 
			case '5': case '6': case '7': case '8': case '9':{
				while(isdigit(*stream.str) || *stream.str == '_') stream_next;
				if(*stream.str == '.' || *stream.str == 'e' || *stream.str == 'E'){
					stream_next;
					while(isdigit(*stream.str)){ stream_next; } //skip to non-digit
					token.raw.count = stream.str - token.raw.str;
					token.kind      = token::literal_float;
					token.f64_val   = string::to_f64(token.raw); 
				}else if(*stream.str == 'x' || *stream.str == 'X'){
					stream_next;
					while(isxdigit(*stream.str)){ stream_next; } //skip to non-hexdigit
					token.raw.count = stream.str - token.raw.str;
					token.kind      = token::literal_integer;
					token.s64_val   = string::to_s64(token.raw);
				}else{
					token.raw.count = stream.str - token.raw.str;
					token.kind      = token::literal_integer;
					token.s64_val   = string::to_s64(token.raw); 
				}	

				token.l1 = line_num;
	 			token.c1 = line_col;
				token.group = token::group_literal;
	 			array::push(lexer.tokens, token);
				last_token = token;
			}continue;

			case '\'':{
				token.kind  = token::literal_character;
				token.group = token::group_literal;
				stream_next;
				
				while(*stream.str != '\'') {//skip until closing single quotes
					stream_next; 
					if(*stream.str == 0){
                        messenger::dispatch(message::attach_sender({lexer.source, token}, 
                            diagnostic::lexer::unexpected_eof_single_quotes()));
						lexer.status.failed = true;
                        return;
					}
				}
				
				token.l1 = line_num;
				token.c1 = line_col;
				token.raw.count = stream.str - (++token.raw.str); //dont include the single quotes
				array::push(lexer.tokens, token);
				last_token = token;
				stream_next;
			}continue; //skip token creation b/c we did it manually

			case '"':{
				token.kind  = token::literal_string;
				// token.group = TokenGroup_Literal;
				stream_next;
				
				
				while(*stream.str != '"'){
					stream_next; //skip until closing double quotes
					if(*stream.str == 0){
                        messenger::dispatch(message::attach_sender({lexer.source, token},
                            diagnostic::lexer::unexpected_eof_double_quotes()));
                        lexer.status.failed = true;
						return;
					}	
				} 
				
				token.l1 = line_num;
				token.c1 = line_col;
				token.raw.count = stream.str - (++token.raw.str); //dont include the double quotes
				last_token = token;
				stream_next;
			}continue; //skip token creation b/c we did it manually
			
			case ';': {
				if(!scope_level) label_latch = false;
				token.kind = token::semicolon;
				stream_next;
			} break;

			//CASE1(';', token::semicolon);
            // CASE1('(', Token::OpenParen);
            // CASE1(')', Token::CloseParen);
			case '(':{ 
				token.kind = token::open_paren;
				scope_level++;
				stream_next;
			}break;
			case ')':{ 
				token.kind = token::close_paren; 
				scope_level--;
				stream_next;
			}break;

			CASE1('[', token::open_square);
			CASE1(']', token::close_square);
			CASE1(',', token::comma);
			CASE1('?', token::question_mark);
			case ':':{ //NOTE special for declarations and compile time expressions
				token.kind = token::colon; 
				if(!label_latch && last_token.kind == token::identifier) {
					label_latch = true;
					array::push(lexer.global_labels, lexer.tokens.count-1);
				}
				stream_next; 
			}break;
			CASE3('.', token::dot, '.', token::range, '.', token::ellipsis);
			CASE1('@', token::at);
			CASE1('#', token::pound);
			CASE1('`', token::backtick);
            CASE1('$', token::dollar);

			case '{':{ //NOTE special for scope tracking and internals 
				token.kind = token::open_brace;
				scope_level++;
				stream_next;
			}break;
			
			case '}':{ //NOTE special for scope tracking and internals
				token.kind = token::close_brace;
				scope_level--;
				if(!scope_level) label_latch = false;

				stream_next;
			}break;
			
			//// @operators ////
			CASE2('+', token::plus,             '=', token::plus_assignment);
			CASE2('*', token::multiplication,   '=', token::multiplication_assignment);
			CASE2('%', token::modulo,           '=', token::modulo_assignment);
			CASE2('~', token::bit_not,          '=', token::bit_not_assignment);
			CASE3('&', token::bit_and,          '=', token::bit_and_assignment,    '&', token::logi_and);
			CASE3('|', token::bit_or,           '=', token::bit_or_assignment,     '|', token::logi_or);
			CASE2('^', token::bit_xor,          '=', token::bit_xor_assignment);

			case '=': {
				token.kind = token::assignment;
				stream_next;
				if(*stream.str == '=') {
					token.kind = token::equal;
					stream_next;
				} else if(*stream.str == '>') {
					token.kind = token::match_arrow;
					stream_next;
				}
			} break;

			CASE2('!', token::logical_not,      '=', token::not_equal);

			case '-':{ // NOTE special because of ->
				token.kind = token::negation;
				stream_next;
				if(*stream.str == '>'){
					token.kind = token::function_arrow;
					array::push(lexer.funcarrows, lexer.tokens.count);
					stream_next;
				}
			}break;

			case '/':{ //NOTE special because of comments
				token.kind = token::division;
				stream_next;
				if(*stream.str == '='){
					token.kind = token::division_assignment;
					stream_next;
				}else if(*stream.str == '/'){
					while(stream && *stream.str != '\n') stream_next; //skip single line comment
					continue; //skip token creation
				}else if(*stream.str == '*'){
					while((stream.count > 1) && !(stream.str[0] == '*' && stream.str[1] == '/')){ stream_next; } //skip multiline comment
					if(stream.count <= 1 && *(stream.str-1) != '/' && *(stream.str-2) != '*'){
						messenger::dispatch(message::attach_sender(lexer.source,
                            diagnostic::lexer::multiline_comment_missing_end()));
                        lexer.status.failed = 1;                        
						return;
					}
					stream_next; stream_next;
					continue; //skip token creation
				}
			}break;
		
			case '<':{ //NOTE special because of bitshift assignment
				token.kind = token::less_than;
				stream_next;
				if      (*stream.str == '='){
					token.kind = token::less_than_or_equal;
					stream_next;
				}else if(*stream.str == '<'){
					token.kind = token::bit_shift_left;
					stream_next;
					if(*stream.str == '='){
						token.kind = token::bit_shift_left_assignment;
						stream_next;
					}
				}
			}break;
			
			case '>':{ //NOTE special because of bitshift assignment
				token.kind = token::greater_than;
				stream_next;
				if      (*stream.str == '='){
					token.kind = token::greater_than_or_equal;
					stream_next;
				}else if(*stream.str == '>'){
					token.kind = token::bit_shift_right;
					stream_next;
					if(*stream.str == '='){
						token.kind = token::bit_shift_right_assignment;
						stream_next;
					}
				}
			}break;
			
			default:{
				if(internal::is_identifier_char(utf8codepoint(stream.str))){
				  	while(internal::is_identifier_char(utf8codepoint(stream.str))) 
						stream_next; //skip until we find a non-identifier char

                	token.raw.count = stream.str - token.raw.str;
                	token.kind = internal::token_is_keyword_or_identifier(token.raw);
					token.hash = string::hash(token.raw);

					if(lexer.tokens.count && array::read(lexer.tokens, -1).kind == token::pound){
						token::kind kind = internal::token_is_directive_or_identifier(token.raw);
						if(kind == token::identifier){
                            messenger::dispatch(message::attach_sender({lexer.source, token}, 
                                diagnostic::lexer::unknown_directive(token.raw)));
                            lexer.status.failed = true;
						}
						token.kind = kind;
						array::pop(lexer.tokens);
					} else {
						switch(token.kind) {
							case token::structdecl: array::push(lexer.structs, lexer.tokens.count); break;
							case token::moduledecl: array::push(lexer.modules, lexer.tokens.count); break;
						}
					}
				}else{
                    messenger::dispatch(message::attach_sender({lexer.source, token},
                        diagnostic::lexer::invalid_token()));
					token.kind = token::error;
                    lexer.status.failed = true;
					stream_next;
				}
			}break;
		}

		if(token.kind == token::identifier){
			token.group = token::identifier; // TODO(sushi) if this is never used just remove it 
		}else if(token.kind >= token::literal_float && token.kind <= token::literal_string){
			token.group = token::group_literal;
		}else if(token.kind >= token::semicolon && token.kind <= token::backtick){
			token.group = token::group_control;
		}else if(token.kind >= token::plus && token.kind <= token::greater_than_or_equal){
			token.group = token::group_operator;
		}else if(token.kind >= token::return_ && token.kind <= token::structdecl){
			token.group = token::group_keyword;
		}else if(token.kind >= token::void_ && token.kind <= token::struct_){
			token.group = token::group_type;
		}else if(token.kind >= token::directive_import && token.kind <= token::directive_run){
			token.group = token::group_directive;
		}

        if(token.kind != token::error) {
            token.l1 = line_num;
            token.c1 = line_col;
            token.raw.count = stream.str - token.raw.str;
            array::push(lexer.tokens, token);
        }

		last_token = token;
    }

	Token eof;
	eof.kind = token::end_of_file;
	eof.l0 = line_num;
	eof.c0 = line_col;
	eof.raw = String("");
	array::push(lexer.tokens, eof);

    lexer.status.time = peek_stopwatch(lexer_time);
} // lex::execute

void
output(Lexer& lexer, b32 human, String path) {
    FileResult result = {};
    File* out = file_init_result(path.s, FileAccess_WriteTruncateCreate, &result);
    if(!out) {
        messenger::dispatch(
            diagnostic::internal::valid_path_but_internal_err(path, String(result.message)));
        return;
    }

	if(human) {
		DString buffer = dstring::init();

		forI(lexer.tokens.count) {
			Token& t = array::readref(lexer.tokens, i);
			dstring::append(buffer, (u64)t.kind, "\"", t.raw, "\"", t.l0, ",", t.c0, "\n");
		}
		
		file_write(out, buffer.s.str, buffer.s.count);
		dstring::deinit(buffer);
	} else {
		Array<u32> data = array::init<u32>(4*lexer.tokens.count);

		Token* curt = array::readptr(lexer.tokens, 0);
		forI(lexer.tokens.count) {
			array::push(data, (u32)curt->kind);
			array::push(data, (u32)curt->l0);
			array::push(data, (u32)curt->c0);
			array::push(data, (u32)curt->raw.count);
		}

		file_write(out, data.data, data.count*sizeof(u32));
		array::deinit(data);
	}

    file_deinit(out);
}

} // namespace lex
} // namespace amu