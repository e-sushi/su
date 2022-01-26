token curt;
array<token>& tokens = lexer.tokens;

b32 parse_failed = false;

inline void token_next(u32 count = 1) {
	curt = tokens.next(count);
}

inline void token_prev(u32 count = 1) {
	curt = tokens.prev(count);
}
#define currtokidx u64(tokens.iter - tokens.data)

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)
#define curr_atch(tok) (curt.type == tok)
#define setTokenIdx(i) tokens.setiter(i), curt = *tokens.iter

#define ParseFail(...)\
{logE("parser", __VA_ARGS__, "\n caused by token '", curt.str, "' on line ", curt.line); parse_failed = true;}

#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectGroup(Token_Type)\
if(curt.group == Token_Type) 

#define ExpectOneOf(...)\
if(match_any(curt.type, __VA_ARGS__)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 

#define ElseExpectGroup(Token_Type)\
else if(curt.group == Token_Type) 

#define ExpectSignature(...) if(check_signature(__VA_ARGS__))
#define ElseExpectSignature(...)  else if(check_signature(__VA_ARGS__))

#define ExpectFail(...)\
else { ParseFail(__VA_ARGS__); } //TODO make it so this breakpoint only happens in debug mode or whatever

#define ExpectFailCode(failcode, error)\
else { ParseFail(error); failcode }

#define expstr(type) ExTypeStrings[type]

local map<Token_Type, ExpressionType> tokToExp{
	{Token_Multiplication,     Expression_BinaryOpMultiply},
	{Token_Division,           Expression_BinaryOpDivision},
	{Token_Negation,           Expression_BinaryOpMinus},
	{Token_Plus,               Expression_BinaryOpPlus},
	{Token_AND,                Expression_BinaryOpAND},
	{Token_OR,                 Expression_BinaryOpOR},
	{Token_LessThan,           Expression_BinaryOpLessThan},
	{Token_GreaterThan,        Expression_BinaryOpGreaterThan},
	{Token_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{Token_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{Token_Equal,              Expression_BinaryOpEqual},
	{Token_NotEqual,           Expression_BinaryOpNotEqual},
	{Token_BitAND,             Expression_BinaryOpBitAND},
	{Token_BitOR,              Expression_BinaryOpBitOR},
	{Token_BitXOR,             Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
	{Token_BitNOT,             Expression_UnaryOpBitComp},
	{Token_LogicalNOT,         Expression_UnaryOpLogiNOT},
	{Token_Negation,           Expression_UnaryOpNegate},
};

map<cstring, Node*> knownFuncs; 
map<cstring, Node*> knownVars;  
map<cstring, Node*> knownStructs;



inline DataType dataTypeFromToken(Token_Type type) {
	switch (type) {
		case Token_Void      : {return DataType_Void;}
		case Token_Signed8   : {return DataType_Signed8;}  
		case Token_Signed16  : {return DataType_Signed16; }
		case Token_Signed32  : {return DataType_Signed32;}     
		case Token_Signed64  : {return DataType_Signed64;}    
		case Token_Unsigned8 : {return DataType_Unsigned8;}  
		case Token_Unsigned16: {return DataType_Unsigned16;}  
		case Token_Unsigned32: {return DataType_Unsigned32;}
		case Token_Unsigned64: {return DataType_Unsigned64; }
		case Token_Float32   : {return DataType_Float32;}     
		case Token_Float64   : {return DataType_Float64;}   
		case Token_String    : {return DataType_String;}    
		case Token_Any       : {return DataType_Any;}
		case Token_Struct    : {return DataType_Structure;}    
		default: {PRINTLN("given token type is not a data type"); return DataType_Void;}
	}
}

inline upt dataTypeSizes(DataType type) {
	switch (type) {
		case DataType_Void       : {return    0;}
		case DataType_Signed8    : {return    1;}
		case DataType_Signed16   : {return    2;}
		case DataType_Signed32	 : {return    4;}
		case DataType_Signed64	 : {return    8;}
		case DataType_Unsigned8	 : {return    1;}
		case DataType_Unsigned16 : {return    2;}
		case DataType_Unsigned32 : {return    4;}
		case DataType_Unsigned64 : {return    8;}
		case DataType_Float32	 : {return    4;}
		case DataType_Float64	 : {return    8;}
		case DataType_String	 : {return    0;} //TODO size of string
		case DataType_Any		 : {return    0;} //most likely determined at compile or runtime
		case DataType_Structure	 : {return npos;} //should always be determined at compile time
	}
}

template<class... T>
inline b32 check_signature(u32 offset, T... in) {
	return ((tokens.peek(offset++).type == in) && ...);
}

template<typename... T> inline b32
next_match(T... in) {
	return ((tokens.peek(1).type == in) || ...);
}

template<typename... T> inline b32
next_match_group(T... in) {
	return ((tokens.peek(1).group == in) || ...);
}

struct Parser {
	Struct*      structure;
	Function*    function;
	Scope*       scope;
	Declaration* declaration;
	Statement*   statement;
	Expression*  expression;
	
	Arena arena;
	
} parser;

inline Node* new_structure(cstring& identifier, const string& node_str = "") {
	parser.structure = (Struct*)parser.arena.add(Struct());
	parser.structure->identifier   = identifier;
	parser.structure->node.type    = NodeType_Structure;
	parser.structure->node.comment = node_str;
	return &parser.structure->node;
}

inline Node* new_function(cstring& identifier, const string& node_str = "") {
	parser.function = (Function*)parser.arena.add(Function());
	parser.function->identifier   = identifier;
	parser.function->node.type    = NodeType_Function;
	parser.function->node.comment = node_str;
	return &parser.function->node;
}

inline Node* new_scope(const string& node_str = "") {
	parser.scope = (Scope*)parser.arena.add(Scope());
	parser.scope->node.type    = NodeType_Scope;
	parser.scope->node.comment = node_str;
	return &parser.scope->node;
}

inline Node* new_statement(StatementType type, const string& node_str = ""){
	parser.statement = (Statement*)parser.arena.add(Statement());
	parser.statement->type = type;
	parser.statement->node.type    = NodeType_Statement;
	parser.statement->node.comment = node_str;
	return &parser.statement->node;
}

inline Node* new_expression(cstring& str, ExpressionType type, const string& node_str = "") {
	parser.expression = (Expression*)parser.arena.add(Expression());
	parser.expression->expstr = str;
	parser.expression->type   = type;
	parser.expression->node.type = NodeType_Expression;
	if(!node_str.count) parser.expression->node.comment = ExTypeStrings[type];
	else                parser.expression->node.comment = node_str;
	return &parser.expression->node;
}

inline Node* new_declaration(cstring& identifier, DataType type, const string& node_str = "") {
	parser.declaration = (Declaration*)parser.arena.add(Declaration());
	parser.declaration->identifier = identifier;
	parser.declaration->node.type = NodeType_Declaration;
	parser.declaration->type = type;
	parser.declaration->node.comment = node_str;
	return &parser.declaration->node;
}

#define ConvertWarnIfOverflow(actualtype, type1, type2) actualtype og = sec->type1; sec->type2 = og; if (og != sec->type2) { log_warning(WC_Overflow, toStr("from " STRINGIZE(type1) " to " STRINGIZE(type2) " caused overflow. value went from ", og, " to ", sec->type2).str); } DebugPrintConversions(type1, type2);
#define DebugPrintConversions(type1, type2) log("DebugConversionPrint", STRINGIZE(type1), " was converted to ", STRINGIZE(type2));


#define ConvertGroup(a,b) \
switch (pri->datatype) {                                                                   \
	case DataType_Signed8:    { ConvertWarnIfOverflow(a, b,    int8); return true; } \
	case DataType_Signed16:   { ConvertWarnIfOverflow(a, b,   int16); return true; } \
	case DataType_Signed32:   { ConvertWarnIfOverflow(a, b,   int32); return true; } \
	case DataType_Signed64:   { ConvertWarnIfOverflow(a, b,   int32); return true; } \
	case DataType_Unsigned8:  { ConvertWarnIfOverflow(a, b,   uint8); return true; } \
	case DataType_Unsigned16: { ConvertWarnIfOverflow(a, b,  uint16); return true; } \
	case DataType_Unsigned32: { ConvertWarnIfOverflow(a, b,  uint32); return true; } \
	case DataType_Unsigned64: { ConvertWarnIfOverflow(a, b,  uint64); return true; } \
	case DataType_Float32:    { ConvertWarnIfOverflow(a, b, float32); return true; } \
	case DataType_Float64:    { ConvertWarnIfOverflow(a, b, float64); return true; } \
	case DataType_Structure:  { NotImplemented; }										   \
}

//secn is casted to match prin if possible
b32 type_check(Node* prin, Node* secn) {
	Expression* pri = ExpressionFromNode(prin);
	Expression* sec = ExpressionFromNode(secn);

	switch (sec->datatype) {
		case DataType_Signed8:    { ConvertGroup( s8,    int8); }break;
		case DataType_Signed16:   { ConvertGroup(s16,   int16); }break;
		case DataType_Signed32:   { ConvertGroup(s32,   int32); }break;
		case DataType_Signed64:   { ConvertGroup(s64,   int64); }break;
		case DataType_Unsigned8:  { ConvertGroup( u8,   uint8); }break;
		case DataType_Unsigned16: { ConvertGroup(u16,  uint16); }break;
		case DataType_Unsigned32: { ConvertGroup(u32,  uint32); }break;
		case DataType_Unsigned64: { ConvertGroup(u64,  uint64); }break;
		case DataType_Float32:    { ConvertGroup(f32, float32); }break;
		case DataType_Float64:    { ConvertGroup(f64, float64); }break;
		case DataType_Structure:  { NotImplemented; }break;
	}
	return false;
}

void set_expression_type_from_declaration(Node* exp, Node* decl) {
	Declaration* d = DeclarationFromNode(decl);
	Expression* e = ExpressionFromNode(exp);
	switch (d->type) {
		case DataType_Void       : {                                 e->datatype=DataType_Void;      }break;
		case DataType_Signed8    : {e->int8        = d->int8;        e->datatype=DataType_Signed8;   }break;
		case DataType_Signed16   : {e->int16       = d->int16;       e->datatype=DataType_Signed16;  }break;
		case DataType_Signed32	 : {e->int32       = d->int32;       e->datatype=DataType_Signed32;  }break;
		case DataType_Signed64	 : {e->int64       = d->int64;       e->datatype=DataType_Signed64;  }break;
		case DataType_Unsigned8	 : {e->uint8       = d->uint8;       e->datatype=DataType_Unsigned8; }break;
		case DataType_Unsigned16 : {e->uint16      = d->uint16;      e->datatype=DataType_Unsigned16;}break;
		case DataType_Unsigned32 : {e->uint32      = d->uint32;      e->datatype=DataType_Unsigned32;}break;
		case DataType_Unsigned64 : {e->uint64      = d->uint64;      e->datatype=DataType_Unsigned64;}break;
		case DataType_Float32	 : {e->float32     = d->float32;     e->datatype=DataType_Float32;   }break;
		case DataType_Float64	 : {e->float64     = d->float64;     e->datatype=DataType_Float64;   }break;
		case DataType_String	 : {e->str         = d->str;         e->datatype=DataType_String;    }break; 
		case DataType_Structure	 : {e->struct_type = d->struct_type; e->datatype=DataType_Structure; }break;  
		case DataType_Any		 : {/*TODO any type stuff*/         }break;
	}
}

enum ParseState_ {
	stNone        = 0,
	stInFunction  = 1 << 0,
	stInForLoop   = 1 << 1,
	stInWhileLoop = 1 << 2, 
}; typedef u32 ParseState;
ParseState pState = stNone;

#define StateSet(flag)    AddFlag(pState, flag)
#define StateUnset(flag)  RemoveFlag(pState, flag)
#define StateHas(flag)    HasFlag(pState, flag)
#define StateHasAll(flag) HasAllFlags(pState, flag)

enum ParseStage {
	psGlobal,      // <program>       :: = { ( <function> | <struct> ) }
	psStruct,      // <struct>        :: = "struct" <id> "{" { ( <declaration> ";" | <function> ) } "}" [<id>] ";"
	psFunction,    // <function>      :: = <type> <id> "(" [ <declaration> {"," <declaration> } ] ")" <scope>
	psScope,       // <scope>         :: = "{" { (<declaration> | <statement> | <scope>) } "}"
	psDeclaration, // <declaration>   :: = <type> <id> [ = <exp> ]
	psStatement,   // <statement>     :: = "return" <exp> ";" | <exp> ";" | <scope> 
	//                                   | "if" "(" <exp> ")" <statement> | <declaration> [ "else" <statement> | <declaration> ]
	//                                   | "for" "(" [<exp>] ";" [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "for" "(" <declaration> ";" [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "while" "(" <exp> ")" <statement>
	//                                   | "break" [<integer>] ";" 
	//                                   | "continue" ";"
	//                                   | <struct>
	psExpression,  // <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional, // <conditional>   :: = <logical or> | "if" "(" <exp> ")" <exp> "else" <exp> 
	psLogicalOR,   // <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,  // <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,   // <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,  // <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,  // <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,    // <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,  // <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,    // <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,    // <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,        // <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,      // <factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal> | <id> | <incdec> <id> | <id> <incdec> |  <funccall> | <memberaccess> | "if"
	//                <funccall>      :: = < id> "("[( <exp> | <id> = <exp> ) {"," ( <exp> | <id> = <exp> ) }] ")"
	//                <literal>       :: = <integer> | <float> | <string>
	//                <memberaccess>  :: = <id> "." <id> { "." <id> }
	//                <float>         :: = { <integer> } "." <integer> { <integer> }
	//                <string>        :: = """ { <char> } """
	//                <integer>       :: = (1|2|3|4|5|6|7|8|9|0) 
	//                <char>          :: = you know what chars are
	//                <type>          :: = (u8|u32|u64|s8|s32|s64|f32|f64|str|any)
	//                <incdec>        :: = "++" | "--"
	//                <unary>         :: = "!" | "~" | "-"
}; 

template<typename... T>
Node* binopParse(Node* node, Node* ret, ParseStage next_stage, T... tokcheck) {
	token_next();
	Node* me = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
	change_parent(me, ret);
	insert_last(node, me);
	token_next();
	ret = define(next_stage, me);
	
	while (next_match(tokcheck...)) {
		token_next();
		Node* me2 = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
		token_next();
		ret = define(next_stage, node);
		change_parent(me2, me);
		change_parent(me2, ret);
		insert_last(node, me2);
		me = me2;
	}
	return me;
}

//gathers all function, struct, and global var signatures in the program
Node* declare(Node* node, NodeType type) {
	//TODO overloaded parser.functions have different signatures
	switch (type) {
		case NodeType_Function: {
			//TODO check for redefinition
			string* funclabel = (string*)parser.arena.add(string());
			ExpectGroup(Token_Typename) {
				DataType dtype = dataTypeFromToken(curt.type);
				token_next();
				Expect(Token_Identifier) {
					Node* me = new_function(curt.str, toStr("func: ", dataTypeStrs[dtype], " ", curt.str));
					insert_last(node, me);
					parser.function->type = dtype;
					*funclabel = toStr(curt.str, "@", dataTypeStrs[dtype], "@");
					token_next();
					Expect(Token_OpenParen) {
						while (next_match_group(Token_Typename)) {
							token_next();
							Declaration* ret = DeclarationFromNode(declare(me, NodeType_Declaration));
							*funclabel += toStr(dataTypeStrs[ret->type], ",");
							parser.function->positional_args++;
							while (!next_match(Token_Comma, Token_CloseParen)) token_next();
							token_next();
						}
						//not really necessary, so remove if u want
						if (funclabel->endsWith("@")) *funclabel += "void";
						else if (funclabel->endsWith(",")) (*funclabel)--;
						//HACK
						if (next_match(Token_CloseParen)) token_next();
						Expect(Token_CloseParen) { 
							knownFuncs.add(parser.function->identifier, me);
							parser.function->token_idx = currtokidx + 1; 
							parser.function->internal_label = cstring{ funclabel->str, funclabel->count };
						}
						ExpectFail("expected a ) for func decl ", parser.function->identifier);
						return me;
					}ExpectFail("expected ( for function declaration of ", parser.function->identifier);
				}ExpectFail("expected identifier for function declaration");
			}ExpectFail("expected typename for function declaration");
		}break;
		
		case NodeType_Structure: {
			Expect(Token_StructDecl) {
				token_next();
				Expect(Token_Identifier) {
					Node* me = new_structure(curt.str, toStr("struct: ", curt.str));
					insert_last(node, me);
					token_next();
					parser.structure->token_idx = currtokidx;
					Expect(Token_OpenBrace) {
						while (match_any(tokens.peek().group, Token_Typename)) {
							token_next();
							DataType dtype = dataTypeFromToken(curt.type);
							token_next();
							Expect(Token_Identifier) {
								cstring id = curt.str;
								token_next();
								Expect(Token_OpenParen) {
									Node* f;
									if (knownFuncs.has(id)) {
										f = *knownFuncs.at(id);
										change_parent(me, f);
										knownFuncs.remove(id); //TODO do optimized swap remove instead
										while (!next_match(Token_CloseParen)) token_next();
										token_next(); token_next();
									}
									else {
										token_prev(2);
										f = declare(me, NodeType_Function);
										token_next();
									}
									u32 open_count = 1;
									Expect(Token_OpenBrace) {
										//TODO this can go wrong with certain kind of syntax errors 
										//TODO maybe theres a better way to do this?
										while (open_count) {
											if      (next_match(Token_OpenBrace))  open_count++;
											else if (next_match(Token_CloseBrace)) open_count--;
											else if (next_match(Token_EOF)) ParseFail("unexpected EOF while parsing function ", parser.structure->identifier, "::", FunctionFromNode(f)->identifier);
											token_next();
										}
										Expect(Token_CloseBrace) { parser.structure->member_funcs.add(id, FunctionFromNode(f)); }
										
									}
									
								}
								else ExpectOneOf(Token_Semicolon, Token_Assignment) {
									token_prev(2);
									declare(me, NodeType_Declaration);
									parser.structure->member_vars.add(curt.str, parser.declaration);
									//eat any possible default var stuff
									while (!next_match(Token_Semicolon)) token_next();
									token_next();
								}		
							}
						}
						token_next();
						Expect(Token_CloseBrace) {
							token_next();
							Expect(Token_Semicolon) { knownStructs.add(parser.structure->identifier, me); return me; }
							ExpectFail("expected ; for struct decl ", parser.structure->identifier);
						}ExpectFail("expected } for struct decl ", parser.structure->identifier);
					}ExpectFail("expected{after struct identifier ", parser.structure->identifier);
				}ExpectFail("expected an identifier for struct decl");
			}ExpectFail("expected 'struct' keyword for struck declaration (somehow 'declare' was called without this?)");
		}break;
		
		case NodeType_Declaration: {
			ExpectGroup(Token_Typename) {
				DataType dtype = dataTypeFromToken(curt.type);
				cstring type_id = curt.str;
				token_next();
				Expect(Token_Identifier) {
					string typestr;
					if (dtype == DataType_Structure) typestr = type_id;
					else typestr = dataTypeStrs[dtype];
					Node* var = new_declaration(curt.str, dtype, toStr("var: ", typestr, " ", curt.str));
					parser.declaration->token_idx = currtokidx;
					parser.declaration->type_id = type_id;
					change_parent(node, var);
					knownVars.add(curt.str, var);
					return var;
				}
			}
		}break;
	}
	return 0;
}

Node* define(ParseStage stage, Node* node) {
	if (parse_failed) return 0;
	
	switch (stage) {
		
		case psGlobal: { ////////////////////////////////////////////////////////////////////// @Global
			while (!(curt.type == Token_EOF || next_match(Token_EOF))) {
				if (parse_failed) return 0;
				ExpectGroup(Token_Typename) {
					ExpectSignature(1, Token_Identifier, Token_OpenParen) {
						define(psFunction, node);
					}
				}ExpectFail("yeah i dont know right now");
				token_next();
			}
		}break;
		
		case psStruct: {
			if (node->type == NodeType_Structure) 
				parser.structure = StructFromNode(node), setTokenIdx(parser.structure->token_idx);
			else 
				declare(node, NodeType_Structure);
			
			for (Declaration* v : parser.structure->member_vars) {
				define(psDeclaration, &v->node);
			}
			for (Function* f : parser.structure->member_funcs) {
				define(psFunction, &f->node);
			}
			Expect(Token_Semicolon) { return 0; /*empty struct*/ }
			while (!next_match(Token_CloseBrace)) token_next();
			token_next(2);
			Expect(Token_Semicolon) { token_next(); }
			ExpectFail("expected ; after struct definition");
			
		}break;
		
		case psFunction: { //////////////////////////////////////////////////////////////////// @Function
			StateSet(stInFunction);
			if (node->type == NodeType_Function) {
				Function* f = FunctionFromNode(node);
				
				for (Declaration* d : f->args) {
					if (define(psDeclaration, &d->node)) {
						f->positional_args--;
					}
				}
				if(curt.type != Token_OpenBrace)
					token_next(2);
				
				setTokenIdx(f->token_idx);
				Expect(Token_OpenBrace) {
					define(psScope, node);
				}ExpectFail("expected {");
			}
			else {
				declare(node, NodeType_Function);
				define(psFunction, node);
			}
			StateUnset(stInFunction);
			
		}break;
		
		case psScope: { /////////////////////////////////////////////////////////////////////// @Scope
			Node* me = new_scope("scope");
			insert_last(node, &parser.scope->node);
			while (!next_match(Token_CloseBrace)) {
				if(parser.scope->has_return_statement){
					log_warning(WC_Unreachable_Code_After_Return);
					if(globals.warnings_as_errors) break;
				}
				
				token_next();
				ExpectGroup(Token_Typename) {
					define(psDeclaration, me);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("missing ; after declaration assignment")
				}
				ElseExpect(Token_OpenBrace) {
					Scope* scope = parser.scope;
					define(psScope, me);
					parser.scope = scope;
				}
				else {
					define(psStatement, me);
				}
				if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); return 0; }
			}
			token_next();
		}break;
		
		case psDeclaration: {////////////////////////////////////////////////////////////////// @Declaration
			if (node->type == NodeType_Declaration)
				parser.declaration = DeclarationFromNode(node), setTokenIdx(parser.declaration->token_idx);
			else 
				declare(node, NodeType_Declaration);
			
			if (parser.declaration->type == DataType_Structure) {
				if (!knownStructs.has(parser.declaration->type_id)) { ParseFail("variable declared with unknown struct '", parser.declaration->type_id, "'"); return 0; }
				parser.declaration->struct_type = StructFromNode(*knownStructs.at(parser.declaration->type_id));
			}
			
			if (next_match(Token_Assignment)) {
				Node* ret = define(psExpression, &parser.declaration->node);
				return ret;
			}
			
		}break;
		
		case psStatement: {//////////////////////////////////////////////////////////////////// @Statement
			switch (curt.type) {
				case Token_If: {
					Node* ifno = new_statement(Statement_Conditional, "if statement");
					insert_last(node, ifno);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for if statement"); break; }
						define(psExpression, ifno);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, ifno);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a declaration in an unscoped if statement"); return 0; } //TODO there's no reason this cant be vali, just warn when it happens
								define(psStatement, ifno);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
							if (next_match(Token_Else)) {
								token_next();
								define(psStatement, ifno);
							}
						}ExpectFail("expected )");
					}ExpectFail("expected (");
				}break;
				
				case Token_Else: {
					new_statement(Statement_Else, "else statement");
					insert_last(node, &parser.statement->node);
					token_next();
					define(psStatement, &parser.statement->node);
				}break;
				
				case Token_For: {
					StateSet(stInForLoop);
					Node* me = new_statement(Statement_For, "for statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for for statement"); return 0; }
						ExpectGroup(Token_Typename) {
							//we are declaring a var for this for loop
							declare(me, NodeType_Declaration);
							define(psDeclaration, &parser.declaration->node);
						}
						else {
							define(psExpression, me);
							token_next();
						}
						Expect(Token_Semicolon) {
							token_next(); 
							define(psExpression, me);
							token_next();
							Expect(Token_Semicolon) {
								token_next();
								define(psExpression, me);
								token_next();
							}ExpectFail("missing second ; in for statement");
						}ExpectFail("missing first ; in for statement");
						
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a declaration in an unscoped for statement"); return 0; }
								define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for for loop");
					}ExpectFail("expected ( after for");
					StateUnset(stInForLoop);
				}break;
				
				case Token_While: {
					StateSet(stInWhileLoop);
					Node* me = new_statement(Statement_While, "while statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for while statement"); return 0; }
						ExpectGroup(Token_Typename) { ParseFail("declaration not allowed for while condition"); return 0; }
						define(psExpression, me);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a declaration in an unscoped for statement"); return 0; }
								define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for while");
					}ExpectFail("expected ( after while");
					StateUnset(stInWhileLoop);
				}break;
				
				case Token_Break: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("break not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Break, "break statement");
					insert_last(node, me);
					token_next();
				}break;
				
				case Token_Continue: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("continue not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Continue, "continue statement");
					insert_last(node, me);
					token_next();
				}break;
				
				case Token_Return: {
					parser.scope->has_return_statement = true;
					
					new_statement(Statement_Return, "return statement");
					insert_last(node, &parser.statement->node);
					token_next();
					define(psExpression, &parser.statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ;");
				}break;
				
				case Token_OpenBrace: {
					define(psScope, node);
				}break;
				
				case Token_StructDecl: {
					Node* me = new_statement(Statement_Struct, "struct statement");
					define(psStruct, me);
					insert_last(node, me);
				}break;
				
				case Token_Semicolon: {
					//eat multiple semicolons
				}break;
				
				default: {
					new_statement(Statement_Expression, "exp statement");
					insert_last(node, &parser.statement->node);
					define(psExpression, &parser.statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("Expected a ;");
				}break;
			}
		}break;
		
		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						if (Node** n = knownVars.at(curt.str); n) {
							Node* id = new_expression(curt.str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", curt.str));
							set_expression_type_from_declaration(id, *n);
							token_next();
							Node* me = new_expression(curt.str, Expression_BinaryOpAssignment, ExTypeStrings[Expression_BinaryOpAssignment]);
							token_next();
							change_parent(me, id);
							Node* ret = define(psExpression, me);
							type_check(id, ret);
							change_parent(me, ret);
							insert_last(node, me);
							return me;
						}else{ParseFail("unknown var '", curt.str, " referenced"); return 0; }
					}
					else {
						new_expression(curt.str, Expression_IdentifierLHS);
						Node* ret = define(psConditional, &parser.expression->node);
						if (!ret) return 0;
						insert_last(node, ret);
						return ret;
					}
				}break;
				
				default: {
					Node* ret = define(psConditional, node);
					return ret;
				}break;
			}
		}break;
		
		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			Expect(Token_If) {
				Node* me = new_expression(curt.str, Expression_TernaryConditional,  "if exp");
				insert_last(node, me);
				token_next();
				Expect(Token_OpenParen) {
					token_next();
					define(psExpression, me);
					token_next();
					Expect(Token_CloseParen) {
						token_next();
						define(psExpression, me);
						token_next();
						Expect(Token_Else) {
							token_next();
							define(psExpression, me);
							return me;
						}ExpectFail("conditional if's are required to have an else");
					}ExpectFail("expected ) for if expression")
				}ExpectFail("expected ( for if expression")
			}
			else {
				return define(psLogicalOR, node);
			}
		}break;
		
		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			Node* ret = define(psLogicalAND, node);
			if (!next_match(Token_OR))
				return ret;
			return binopParse(node, ret, psLogicalAND, Token_OR);
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			Node* ret = define(psBitwiseOR, node);
			if (!next_match(Token_AND))
				return ret;
			return binopParse(node, ret, psBitwiseOR);
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			Node* ret = define(psBitwiseXOR, node);
			if (!next_match(Token_BitOR))
				return ret;
			return binopParse(node, ret, psBitwiseXOR, Token_BitOR);
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			Node* ret = define(psBitwiseAND, node);
			if (!next_match(Token_BitXOR))
				return ret;
			return binopParse(node, ret, psBitwiseAND, Token_BitXOR);
		}break;
		
		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			Node* ret = define(psEquality, node);
			if (!next_match(Token_BitAND))
				return ret;
			return binopParse(node, ret, psEquality, Token_BitAND);
		}break;
		
		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			Node* ret = define(psRelational, node);
			if (!next_match(Token_NotEqual, Token_Equal))
				return ret;
			return binopParse(node, ret, psRelational, Token_NotEqual, Token_Equal);
		}break;
		
		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			Node* ret = define(psBitshift, node);
			if (!next_match(Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual))
				return ret;
			return binopParse(node, ret, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
		}break;
		
		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			Node* ret = define(psAdditive, node);
			if (!next_match(Token_BitShiftLeft, Token_BitShiftRight))
				return ret;
			return binopParse(node, ret, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
		}break;
		
		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			Node* ret = define(psTerm, node);
			if (!next_match(Token_Plus, Token_Negation))
				return ret;
			return binopParse(node, ret, psTerm, Token_Plus, Token_Negation);
		}break;
		
		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			Node* ret = define(psFactor, node);
			if (!next_match(Token_Multiplication, Token_Division, Token_Modulo))
				return ret;
			return binopParse(node, ret, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
		}break;
		
		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			switch (curt.type) {
				
				//TODO implicitly change types here when applicable, or do that where they're returned
				case Token_LiteralFloat: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					parser.expression->datatype = DataType_Float32;
					parser.expression->float32 = curt.float64; //TODO detect f64
					insert_last(node, &parser.expression->node);
					return var;
				}break;
				case Token_LiteralInteger: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					parser.expression->datatype = DataType_Signed64;
					parser.expression->int64 = curt.integer;
					insert_last(node, &parser.expression->node);
					return var;
				}break;
				
				case Token_LiteralString: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " \"", curt.str, "\""));
					parser.expression->datatype = DataType_String;
					parser.expression->str = curt.str;
					insert_last(node, &parser.expression->node);
					return var;
				}break;
				
				case Token_OpenParen: {
					token_next();
					Node* ret = define(psExpression, &parser.expression->node);
					change_parent(node, ret);
					token_next();
					Expect(Token_CloseParen) { return ret; }
					ExpectFail("expected a )");
				}break;
				
				case Token_Identifier: {
					if (next_match(Token_OpenParen)) {
						if (!knownFuncs.has(curt.str)) { ParseFail(toStr("unknown function ", curt.str, " referenced")); return 0; }
						Node* me = new_expression(curt.str, Expression_Function_Call, toStr(ExTypeStrings[Expression_Function_Call], " ", curt.str));
						insert_last(node, me);
						Function* f = FunctionFromNode(*knownFuncs.at(curt.str));
						parser.expression->datatype = f->type;
						token_next();
						
						//to order arguments correctly 
						array<Node*> expsend;
						array<cstring> named_args;
						
						expsend.resize(f->args.count);
						
						u32 positional_args_given = 0;
						while (!next_match(Token_CloseParen)) {
							token_next();
							b32 namedarg = 0;
							Expect(Token_Identifier) {
								cstring id = curt.str;
								token_next();
								Expect(Token_Assignment) {
									namedarg = 1;
									u32 argidx = f->args.findkey(id);
									if (argidx == npos)   { ParseFail("unidentified named argument, ", id, " (or you wanted to use ==?)"); return 0; }
									if (expsend[argidx]) { ParseFail("attempt to use named arg on argument who has already been set: ", id); return 0; }
									token_next();
									expsend[argidx] = define(psExpression, node);
									if (!expsend[argidx]) return 0;
									token_next();
								}
								else {
									token_prev();
								}
							}
							if (!namedarg) {
								//TODO handle too many args given
								//check if a positional arg was already filled by a named arg
								while (expsend[positional_args_given]) positional_args_given++;
								expsend[positional_args_given] = define(psExpression, node);
								if (!expsend[positional_args_given++]) return 0;
								token_next();
							}
							
							Expect(Token_Comma){}
							ElseExpect(Token_CloseParen) { break; }
							ExpectFail("no , separating function arguments");
						}
						
						if (positional_args_given < f->positional_args) {
							ParseFail("func ", f->identifier, " requires ", f->positional_args, " positional args but ", (positional_args_given ? "only " : ""), positional_args_given, (positional_args_given == 1 ? " was " : " were "), "given.");
							logE("parser", "missing args are: ");
							for (u32 i = 0; i < f->positional_args; i++) {
								if(!expsend[i])
									logE("parser", f->args[i]->identifier, " of type ", dataTypeStrs[f->args[i]->type]);
							}
							return 0;
						}
						
						for (u32 i = 0; i < expsend.count; i++) {
							if (!expsend[i]) {
								insert_last(me, f->args[i]->node.first_child->last_child);
							}
							else {
								change_parent(me, expsend[i]);
							}
						}
						
						Expect(Token_CloseParen) {}
						return me;
					}
					else if (next_match(Token_Dot)) {
						//member access
						if (Node** var = knownVars.at(curt.str); var) { 
							Node* me = new_expression(curt.str, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.str));
							ExpressionFromNode(me)->struct_type = DeclarationFromNode(*var)->struct_type;
							token_next();
							return define(psFactor, me);
						} else { ParseFail("attempt to access a member of an undeclared variable"); return 0; }
					}
					else {
						if (!knownVars.has(curt.str)) { ParseFail("unknown var '", curt.str, "' referenced"); return 0; }
						Node* var = new_expression(curt.str, Expression_IdentifierRHS, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.str));
						insert_last(node, var);
						set_expression_type_from_declaration(var, *knownVars.at(curt.str));
						if (next_match(Token_Increment, Token_Decrememnt)) {
							token_next();
							new_expression(curt.str, (curt.type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix));
							insert_last(node, &parser.expression->node);
							change_parent(&parser.expression->node, var);
							var = &parser.expression->node;
						}
						return var;
					}
					
				}break;
				
				case Token_If: {
					return define(psConditional, &parser.expression->node);
				}break;
				
				case Token_Increment: {
					new_expression(curt.str, Expression_IncrementPrefix);
					insert_last(node, &parser.expression->node);
					token_next();
					Node* ret = &parser.expression->node;
					Expect(Token_Identifier) {
						define(psFactor, &parser.expression->node);
					}ExpectFail("'++' needs l-value");
					return ret;
				}break;
				
				case Token_Decrememnt: {
					new_expression(curt.str, Expression_DecrementPrefix);
					insert_last(node, &parser.expression->node);
					token_next();
					Node* ret = &parser.expression->node;
					Expect(Token_Identifier) {
						define(psFactor, &parser.expression->node);
					}ExpectFail("'--' needs l-value");
					return ret;
				}break;
				
				case Token_Semicolon: {
					return node;
				}break;
				
				case Token_Negation: {
					new_expression(curt.str, Expression_UnaryOpNegate);
					insert_last(node, &parser.expression->node);
					token_next();
					Node* ret = &parser.expression->node;
					define(psFactor, &parser.expression->node);
					return ret;
				}break;
				
				case Token_LogicalNOT: {
					new_expression(curt.str, Expression_UnaryOpLogiNOT);
					insert_last(node, &parser.expression->node);
					token_next();
					Node* ret = &parser.expression->node;
					define(psFactor, &parser.expression->node);
					return ret;
				}break;
				
				case Token_BitNOT: {
					new_expression(curt.str, Expression_UnaryOpBitComp);
					insert_last(node, &parser.expression->node);
					token_next();
					Node* ret = &parser.expression->node;
					define(psFactor, &parser.expression->node);
					return ret;
				}break;
				
				case Token_Dot: {
					Expression* nodeexp = ExpressionFromNode(node);
					Struct* nodestruct = nodeexp->struct_type;
					Node* me = new_expression(curt.str, Expression_BinaryOpMemberAccess);
					token_next();
					Expect(Token_Identifier) {
						if (!nodeexp->struct_type->member_vars.has(curt.str)) { ParseFail("attempt to access undefined member of var ", tokens[currtokidx - 2].str); return 0; }
						Node* me2 = new_expression(curt.str, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.str));
						nodeexp = parser.expression;
						nodeexp->struct_type = (*nodestruct->member_vars.at(curt.str))->struct_type;
						nodestruct = nodeexp->struct_type;
						change_parent(me, node);
						change_parent(me, me2);
						while (next_match(Token_Dot)) {
							token_next();
							me2 = new_expression(curt.str, Expression_BinaryOpMemberAccess);
							token_next();
							Expect(Token_Identifier) {
								if (!nodestruct->member_vars.has(curt.str)) { ParseFail("attempt to access undefined member of var ", tokens[currtokidx - 2].str); return 0; }
								Node* next = new_expression(curt.str, Expression_IdentifierRHS, toStr(expstr(Expression_IdentifierRHS), curt.str));
								nodeexp = parser.expression;
								nodeexp->struct_type = (*nodestruct->member_vars.at(curt.str))->struct_type;
								nodestruct = nodeexp->struct_type;
								change_parent(me2, me);
								change_parent(me2, next);
								me = me2;
							}ExpectFail("expected identifier following member access dot");
						}
					}ExpectFail("expected identifier following member access dot");
					return me;
				}break;
				
				default: {
					ParseFail("unexpected token found in factor");
				}break;
			}
		}break;
	}
	return 0;
}

b32 parse_program(Program& mother) {
	parser.arena.init(Kilobytes(10));
	
	for (u32 i : lexer.func_decl) {
		tokens.setiter(i);
		curt = *tokens.iter;
		declare(&mother.node, NodeType_Function);
	}
	if (parse_failed) return 1;
	for (u32 i : lexer.struct_decl) {
		tokens.setiter(i);
		curt = *tokens.iter;
		declare(&mother.node, NodeType_Structure);
	}
	if (parse_failed) return 1;
	
	//TODO make a nice way to find all global parser.declarations
	
	for (Node* n : knownStructs) {
		define(psStruct, n);
	}
	
	for (Node* n : knownFuncs) {
		define(psFunction, n);
	}
	
	mother.node.comment = "program";
	
	tokens.setiter(0);
	curt = tokens[0];
	//parser(psGlobal, &mother.node);
	
	return parse_failed;
}