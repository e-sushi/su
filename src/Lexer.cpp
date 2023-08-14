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
		strcase("in"):        return token::in;
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
		strcase("print_type"):     return token::directive_print_type;
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

#undef strcase

} // namespace internal

Lexer
init() {
	Lexer out = {};
	return out;
}

void 
deinit(Lexer& lexer) {}

void
execute(Code* code) {
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

#define CASE2ALT(c1,t1, c2,t2, c3,t3) \
case c1:{                             \
token.kind = t1;                      \
stream_next;                          \
if      (*stream.str == c3){          \
token.kind = t3;                      \
stream_next;                          \
}else if(*stream.str == c2){          \
token.kind = t2;                      \
stream_next;                          \
}                                     \
}break;

#define CASE3(c1,t1,c2,t2,c3,t3) \
case c1:{                        \
token.kind = t1;                 \
stream_next;                     \
if(*stream.str == c2){           \
token.kind = t2;                 \
stream_next;                     \
if(*stream.str == c3){           \
token.kind = t3;                 \
stream_next;                     \
}                                \
}                                \
}break;                          \

    util::Stopwatch lexer_time = util::stopwatch::start();
    
    messenger::dispatch(message::attach_sender(code,
        message::make_debug(message::verbosity::stages, 
            String("beginning lexical analysis."))));

    String stream = code->raw;

    u32 line_num = 1, line_col = 1;
    u8* line_start = stream.str;

	u32 scope_level = 0;

	Token last_token = {};
	b32 label_latch = false; // true when a label has been found, set back to false when a label ending token is crossed (a semicolon or close brace)

	struct ModuleEntry {
		Module* module;
		u32 scope_level; // scope level that this module was started at 
		b32 label_latch;
	};

	ScopedArray<ModuleEntry> module_stack = array::init<ModuleEntry>();

	Module* file_module = code->source->module = module::create();
	ModuleEntry current_module = {file_module, 0, false};

	auto tokens = array::init<Token>();

	auto push_module = [&](Module* m) {
		array::push(module_stack, current_module);
		current_module = {m,scope_level+1,false};
	};

	auto pop_module = [&]() {
		current_module = array::pop(module_stack);
	};

    while(stream) {
        Token token = {};
        token.code = code;
        token.l0 = line_num;
        token.c0 = line_col;
        token.line_start = line_start;
        token.raw.str = stream.str;

        switch(string::codepoint(stream)) {
			case '\t': case '\n': case '\v': case '\f':  case '\r':
			case ' ': case 133: case 160: case 5760: case 8192:
			case 8193: case 8194: case 8195: case 8196: case 8197:
			case 8198: case 8199: case 8200: case 8201: case 8202:
			case 8232: case 8239: case 8287: case 12288:{
				while(isspace(string::codepoint(stream))){
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
	 			array::push(tokens, token);
				last_token = token;
			}continue;

			case '\'':{
				token.kind  = token::literal_character;
				token.group = token::group_literal;
				stream_next;
				
				while(*stream.str != '\'') {//skip until closing single quotes
					stream_next; 
					if(*stream.str == 0){
						diagnostic::lexer::
							unexpected_eof_single_quotes(&token);
                        return;
					}
				}
				
				token.l1 = line_num;
				token.c1 = line_col;
				token.raw.count = stream.str - (++token.raw.str); //dont include the single quotes
				array::push(tokens, token);
				last_token = token;
				stream_next;
			}continue; //skip token creation b/c we did it manually

			case '"':{
				token.kind  = token::literal_string;
				token.group = token::group_literal;
				stream_next;
				
				while(*stream.str != '"'){
					stream_next; //skip until closing double quotes
					if(*stream.str == 0){
						diagnostic::lexer::
							unexpected_eof_double_quotes(&token);
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
				if(!scope_level) current_module.label_latch = false;
				token.kind = token::semicolon;
				stream_next;
			} break;
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
				if(!current_module.label_latch && last_token.kind == token::identifier) {
					current_module.label_latch = true;
					// array::push(current_module.module->labels, tokens.count-1);
				}
				stream_next; 
			}break;
			
			CASE3('.', token::dot, '.', token::range, '.', token::ellipsis);
			CASE1('@', token::at);
			CASE1('#', token::pound);
			CASE1('`', token::backtick);
            CASE2('$', token::dollar, '$', token::double_dollar);

			case '{':{ //NOTE special for scope tracking and internals 
				token.kind = token::open_brace;
				scope_level++;
				stream_next;
			}break;
			
			case '}':{ //NOTE special for scope tracking and internals
				token.kind = token::close_brace;
				scope_level--;
				if(scope_level+1 == current_module.scope_level) pop_module();
				else if(scope_level == current_module.scope_level) current_module.label_latch = false;
				stream_next;
			}break;
			
			//// @operators ////
			CASE2('+', token::plus,             '=', token::plus_equal);
			CASE2('*', token::asterisk,         '=', token::asterisk_assignment);
			CASE2('%', token::percent,          '=', token::percent_equal);
			CASE2('~', token::tilde,            '=', token::tilde_assignment);
			CASE2ALT('&', token::ampersand,     '=', token::ampersand_assignment, '&', token::double_ampersand);
			CASE2ALT('|', token::vertical_line, '=', token::vertical_line_equals, '|', token::logi_or);
			CASE2('^', token::caret,            '=', token::caret_equal);

			case '=': {
				token.kind = token::equal;
				stream_next;
				if(*stream.str == '=') {
					token.kind = token::double_equal;
					stream_next;
				} else if(*stream.str == '>') {
					token.kind = token::match_arrow;
					stream_next;
				}
			} break;

			CASE2('!', token::explanation_mark, '=', token::explanation_mark_equal);

			case '-':{ // NOTE special because of ->
				token.kind = token::minus;
				stream_next;
				if(*stream.str == '>'){
					token.kind = token::function_arrow;
					// array::push(lexer.funcarrows, tokens.count);
					stream_next;
				}
			}break;

			case '/':{ //NOTE special because of comments
				token.kind = token::solidus;
				stream_next;
				if(*stream.str == '='){
					token.kind = token::solidus_assignment;
					stream_next;
				}else if(*stream.str == '/'){
					while(stream && *stream.str != '\n') stream_next; //skip single line comment
					continue; //skip token creation
				}else if(*stream.str == '*'){
					while((stream.count > 1) && !(stream.str[0] == '*' && stream.str[1] == '/')){ stream_next; } //skip multiline comment
					if(stream.count <= 1 && *(stream.str-1) != '/' && *(stream.str-2) != '*'){
                        diagnostic::lexer::
							multiline_comment_missing_end(&token);
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
					token.kind = token::less_than_equal;
					stream_next;
				}else if(*stream.str == '<'){
					token.kind = token::double_less_than;
					stream_next;
					if(*stream.str == '='){
						token.kind = token::double_less_than_equal;
						stream_next;
					}
				}
			}break;
			
			case '>':{ //NOTE special because of bitshift assignment
				token.kind = token::greater_than;
				stream_next;
				if      (*stream.str == '='){
					token.kind = token::greater_than_equal;
					stream_next;
				}else if(*stream.str == '>'){
					token.kind = token::double_greater_than;
					stream_next;
					if(*stream.str == '='){
						token.kind = token::double_greater_than_equal;
						stream_next;
					}
				}
			}break;
			
			default:{
				if(internal::is_identifier_char(string::codepoint(stream))){
				  	while(internal::is_identifier_char(string::codepoint(stream))) 
						stream_next; //skip until we find a non-identifier char

                	token.raw.count = stream.str - token.raw.str;
                	token.kind = internal::token_is_keyword_or_identifier(token.raw);
					token.hash = string::hash(token.raw);

					if(tokens.count && array::read(tokens, -1).kind == token::pound){
						token::kind kind = internal::token_is_directive_or_identifier(token.raw);
						if(kind == token::identifier){
                            diagnostic::lexer::
								unknown_directive(&token, token.raw);
						}
						token.kind = kind;
						array::pop(tokens);
					} else {
						switch(token.kind) {
							case token::structdecl: break; // array::push(lexer.structs, tokens.count); break;
							case token::moduledecl: {
								Module* m = module::create();
								// link the new module's symbol table to the current one 
								m->table.last = &current_module.module->table;
								push_module(m);
								token.module = m;
							} break;
						}
					}
				}else{
					diagnostic::lexer::invalid_token(&token);
					token.kind = token::error;
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
		}else if(token.kind >= token::plus && token.kind <= token::greater_than_equal){
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
            array::push(tokens, token);
        }

		last_token = token;
    }

	Token eof;
	eof.kind = token::end_of_file;
	eof.l0 = line_num;
	eof.c0 = line_col;
	array::push(tokens, eof);

	if(code::is_virtual(code)) {
		VirtualCode* code = code;
		code->vtokens = tokens;
	} else {
		code->source->tokens = tokens;
	}

    code->lexer->status.time = util::stopwatch::peek(lexer_time);

	// !Leak: a DString is generated by util::format_time, but messenger currently has no way to know 
	//        if a String needs to be cleaned up or not 
	messenger::dispatch(message::attach_sender(code,
		message::make_debug(message::verbosity::stages, String("finished lexical analysis in "), String(util::format_time(code->lexer->status.time)))));
} // lex::execute

void
output(Code* code, b32 human, String path) {
    FILE* out = fopen((char*)path.str, "w");
    if(!out) {
		diagnostic::internal::
			valid_path_but_internal_err(code, path, "TODO(sushi) get error info for failing to open lexer::output");
        return;
    }

	Array<Token>& tokens = code::get_token_array(code);

	if(human) {
		DString buffer = dstring::init();

		forI(tokens.count) {
			Token& t = array::readref(tokens, i);
			dstring::append(buffer, (u64)t.kind, "\"", t.raw, "\"", t.l0, ",", t.c0, "\n");
		}
		
		fwrite(buffer.str, buffer.count, 1, out);
		dstring::deinit(buffer);
	} else {
		Array<u32> data = array::init<u32>(4*tokens.count);

		Token* curt = array::readptr(tokens, 0);
		forI(tokens.count) {
			array::push(data, (u32)curt->kind);
			array::push(data, (u32)curt->l0);
			array::push(data, (u32)curt->c0);
			array::push(data, (u32)curt->raw.count);
		}

		fwrite(data.data, data.count*sizeof(u32), 1, out);
		array::deinit(data);
	}

    fclose(out);
}

} // namespace lex
} // namespace amu