namespace amu {
namespace lex {

namespace internal {

#define strcase(s) case string::static_hash(s)

local Token::Type
token_is_keyword_or_identifier(String raw) {
    switch(string::hash(raw)) {
        strcase("return"):    return Token::Return;
		strcase("if"):        return Token::If;
		strcase("else"):      return Token::Else;
		strcase("for"):       return Token::For;
		strcase("while"):     return Token::While;
		strcase("break"):     return Token::Break;
		strcase("continue"):  return Token::Continue;
		strcase("defer"):     return Token::Defer;
		strcase("struct"):    return Token::StructDecl;
		strcase("module"):    return Token::ModuleDecl;
		strcase("this"):      return Token::This;
		strcase("void"):      return Token::Void;
		strcase("s8"):        return Token::Signed8;
		strcase("s16"):       return Token::Signed16;
		strcase("s32"):       return Token::Signed32;
		strcase("s64"):       return Token::Signed64;
		strcase("u8"):        return Token::Unsigned8;
		strcase("u16"):       return Token::Unsigned16;
		strcase("u32"):       return Token::Unsigned32;
		strcase("u64"):       return Token::Unsigned64;
		strcase("f32"):       return Token::Float32;
		strcase("f64"):       return Token::Float64;
		strcase("str"):       return Token::String;
		strcase("any"):       return Token::Any;
		strcase("as"):        return Token::As;
		strcase("using"):     return Token::Using;
		strcase("operator"):  return Token::Operator;
    }
    return Token::Identifier;
}


local Token::Type
token_is_directive_or_identifier(String raw) {
    switch(string::hash(raw)) {
        strcase("import"):   return Token::Directive_Import;
		strcase("internal"): return Token::Directive_Internal;
		strcase("run"):      return Token::Directive_Run;
    }

    return Token::Identifier;
}

FORCE_INLINE b32 
is_identifier_char(u32 codepoint) {
    if(isalnum(codepoint) || codepoint == '_' || codepoint > 127) 
        return true;
    if(is_whitespace(codepoint)) 
        return false;
    return false;
}

FORCE_INLINE void
advance(LexicalAnalyzer& lexer, String& stream) {

}

#undef strcase

} // namespace internal

LexicalAnalyzer
init(Source* source) {
    LexicalAnalyzer out;
    out.source = source;
    out.tokens = array::init<Token>();
    out.colons = array::init<spt>();
    out.status = {};
    return out;
}

void 
deinit(LexicalAnalyzer& lexer) {
    array::deinit(lexer.tokens);
    lexer.source = 0;
}

void
analyze(LexicalAnalyzer& lexer) {
#define stream_next { string::advance(stream); line_col++; } 

#define CASE1(c1,t1) \
case c1:{ 	         \
token.type = t1;     \
stream_next;         \
}break;

#define CASE2(c1,t1, c2,t2) \
case c1:{                   \
token.type = t1;            \
stream_next;                \
if(*stream.str == c2){      \
token.type = t2;            \
stream_next;                \
}                           \
}break;

#define CASE3(c1,t1, c2,t2, c3,t3) \
case c1:{                          \
token.type = t1;                   \
stream_next;                       \
if      (*stream.str == c3){       \
token.type = t3;                   \
stream_next;                       \
}else if(*stream.str == c2){       \
token.type = t2;                   \
stream_next;                       \
}                                  \
}break;

    Stopwatch lexer_time = start_stopwatch();
    
    messenger::dispatch(message::attach_sender(lexer.source,
        message::debug(message::verbosity::stages, 
            String("Beginning lexical analysis."))));

    String stream = lexer.source->buffer;

    u32 line_num = 1, line_col = 1;
    u8* line_start = stream.str;

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
						line_start = stream.str;
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
					token.type      = Token::LiteralFloat;
					token.f64_val   = string::to_f64(token.raw); 
				}else if(*stream.str == 'x' || *stream.str == 'X'){
					stream_next;
					while(isxdigit(*stream.str)){ stream_next; } //skip to non-hexdigit
					token.raw.count = stream.str - token.raw.str;
					token.type      = Token::LiteralInteger;
					token.s64_val   = string::to_s64(token.raw);
				}else{
					token.raw.count = stream.str - token.raw.str;
					token.type      = Token::LiteralInteger;
					token.s64_val   = string::to_s64(token.raw); 
				}

				token.l1 = line_num;
	 			token.c1 = line_col;
	 			array::push(lexer.tokens, token);
			}continue;

			case '\'':{
				token.type  = Token::LiteralCharacter;
				//token.group = TokenGroup_Literal;
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
				stream_next;
			}continue; //skip token creation b/c we did it manually

			case '"':{
				token.type  = Token::LiteralString;
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
				array::push(lexer.tokens, token);
				stream_next;
			}continue; //skip token creation b/c we did it manually
			
			CASE1(';', Token::Semicolon);
            CASE1('(', Token::OpenParen);
            CASE1(')', Token::CloseParen);
			// case '(':{ 
			// 	token.type = Token::OpenParen;
			// 	paren_depth++;
			// 	last_open_paren = amufile->lexical_analyzer.tokens.count;
			// 	stream_next;
			// }break;
			// case ')':{ 
			// 	token.type = Token::CloseParen; 
			// 	if(!paren_depth){
			// 		logger.error(&token, "closing parenthesis ')' with no opening parenthesis '('.");
			// 		amufile->lexical_analyzer.failed = 1;
			// 		return;
			// 	}
			// 	paren_depth--;
			// 	stream_next;
			// }break;

			CASE1('[', Token::OpenSquare);
			CASE1(']', Token::CloseSquare);
			CASE1(',', Token::Comma);
			CASE1('?', Token::QuestionMark);
			case ':':{ //NOTE special for declarations and compile time expressions
				token.type = Token::Colon; 
                array::push(lexer.colons, lexer.tokens.count);
				stream_next; 
			}break;
			CASE1('.', Token::Dot);
			CASE1('@', Token::At);
			CASE1('#', Token::Pound);
			CASE1('`', Token::Backtick);
            CASE1('{', Token::OpenBrace);
            CASE1('}', Token::CloseBrace);
            CASE1('$', Token::Dollar);

			// case '{':{ //NOTE special for scope tracking and internals 
			// 	token.type = Token::OpenBrace;
			// 	//NOTE(sushi) internal directive scopes do affect scope level
			// 	if(amufile->lexical_analyzer.tokens.count && amufile->lexical_analyzer.tokens[amufile->lexical_analyzer.tokens.count-1].type == Token::Directive_Internal){
			// 		internal_scope_depth = scope_depth;
			// 	}else{
			// 		scope_depth++;
			// 	}
			// 	last_open_brace = amufile->lexical_analyzer.tokens.count;
			// 	stream_next;
			// }break;
			
			// case '}':{ //NOTE special for scope tracking and internals
			// 	token.type = Token::CloseBrace;
			// 	if(!scope_depth){
			// 		logger.error(&token, "closing brace '}' with no opening brace '{'.");
			// 		amufile->lexical_analyzer.failed = 1;
			// 	}
			// 	if(scope_depth == internal_scope_depth){
			// 		internal_scope_depth = -1;
			// 	}else{
			// 		scope_depth--;
			// 	}
			// 	stream_next;
			// }break;
			
			//// @operators ////
			CASE3('+', Token::Plus,            '=', Token::PlusAssignment,      '+', Token::Increment);
			CASE2('*', Token::Multiplication,  '=', Token::MultiplicationAssignment);
			CASE2('%', Token::Modulo,          '=', Token::ModuloAssignment);
			CASE2('~', Token::BitNOT,          '=', Token::BitNOTAssignment);
			CASE3('&', Token::BitAND,          '=', Token::BitANDAssignment,    '&', Token::AND);
			CASE3('|', Token::BitOR,           '=', Token::BitORAssignment,     '|', Token::OR);
			CASE2('^', Token::BitXOR,          '=', Token::BitXORAssignment);
			CASE2('=', Token::Assignment,      '=', Token::Equal);
			CASE2('!', Token::LogicalNOT,      '=', Token::NotEqual);
			
			case '-':{ // NOTE special because of ->
				token.type = Token::Negation;
				stream_next;
				if(*stream.str == '>'){
					token.type = Token::FunctionArrow;
					stream_next;
				}
			}break;

			case '/':{ //NOTE special because of comments
				token.type = Token::Division;
				stream_next;
				if(*stream.str == '='){
					token.type = Token::DivisionAssignment;
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
				token.type = Token::LessThan;
				stream_next;
				if      (*stream.str == '='){
					token.type = Token::LessThanOrEqual;
					stream_next;
				}else if(*stream.str == '<'){
					token.type = Token::BitShiftLeft;
					stream_next;
					if(*stream.str == '='){
						token.type = Token::BitShiftLeftAssignment;
						stream_next;
					}
				}
			}break;
			
			case '>':{ //NOTE special because of bitshift assignment
				token.type = Token::GreaterThan;
				stream_next;
				if      (*stream.str == '='){
					token.type = Token::GreaterThanOrEqual;
					stream_next;
				}else if(*stream.str == '>'){
					token.type = Token::BitShiftRight;
					stream_next;
					if(*stream.str == '='){
						token.type = Token::BitShiftRightAssignment;
						stream_next;
					}
				}
			}break;
			
			default:{
				if(internal::is_identifier_char(utf8codepoint(stream.str))){
				  	while(internal::is_identifier_char(utf8codepoint(stream.str))) 
						stream_next; //skip until we find a non-identifier char

                	token.raw.count = stream.str - token.raw.str;
                	token.type = internal::token_is_keyword_or_identifier(token.raw);
					token.hash = string::hash(token.raw);

					if(lexer.tokens.count && array::read(lexer.tokens, -1).type == Token::Pound){
						Type type = internal::token_is_directive_or_identifier(token.raw);
						if(type == Token::Identifier){
                            messenger::dispatch(message::attach_sender({lexer.source, token}, 
                                diagnostic::lexer::unknown_directive(token.raw)));
                            lexer.status.failed = true;
						}
					}
				}else{
                    messenger::dispatch(message::attach_sender({lexer.source, token},
                        diagnostic::lexer::invalid_token()));
					token.type = Token::ERROR;
                    lexer.status.failed = true;
					stream_next;
				}
			}break;
		}

        if(token.type != Token::ERROR) {
            token.l1 = line_num;
            token.c1 = line_col;
            token.raw.count = stream.str - token.raw.str;
            array::push(lexer.tokens, token);
        }
    }

    lexer.status.time = peek_stopwatch(lexer_time);
} // lex::analyze

// the lexer is serialized by outputting 
// token-type "raw" line,column /
void
output(LexicalAnalyzer& lexer, String path) {
    FileResult result = {};
    File* out = file_init_result(path.s, FileAccess_WriteTruncateCreate, &result);
    if(!out) {
        messenger::dispatch(
            diagnostic::internal::valid_path_but_internal_err(path, String(result.message)));
        return;
    }

    DString buffer = dstring::init();


    forI(lexer.tokens.count) {
        Token& t = array::readref(lexer.tokens, i);
        dstring::append(buffer, (u64)t.type, "\"", t.raw, "\"", t.l0, ",", t.c0, "\n");
    }

    file_write(out, buffer.s.str, buffer.s.count);
    file_deinit(out);
    dstring::deinit(buffer);
}

} // namespace lex
} // namespace amu