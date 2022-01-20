//TODO remove returning arrays of expressions and pass them by reference and modify instead
//     or just use global arrays like we use a global string in assembling
//TODO optimize the tree by bypassing nodes for expressions
//	   like, if an expression goes straight to being an int, we don't need several layers of 
//     nodes determining that



// full backus naur stuff
// <program>       :: = <function>
// <function>      :: = "int" <id> "(" ")" "{" { <block item> } "}"
// <block item>    :: = <statement> | <declaration>
// <declaration>   :: = "int" <id> [ = <exp> ] ";"
// <statement>     :: = "return" <exp> ";" | <exp> ";" | "if" "(" <exp> ")" <statement> [ "else" <statement> ] 
// <exp>           :: = <id> "=" <exp> | <conditional>
// <conditional>   :: = <logical or> [ "?" <exp> ":" <conditional> ]
// <logical or>    :: = <logical and> { "||" <logical and> } 
// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
// <bitwise and>   :: = <equality> { "&" <equality> }
// <equality>      :: = <relational> { ("!=" | "==") <relational> }
// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
// <additive>      :: = <term> { ("+" | "-") <term> }
// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
// <unary>         :: = "!" | "~" | "-"

//master token
token curt;
array<token> tokens;

b32 parse_failed = false;

//These defines are mostly for conveinence and clarity as to what im doing
//#define token_next() curt = tokens.next()
#define token_last curt = tokens.prev()

void token_next(u32 count = 1) {
	curt = tokens.next(count);
}

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)

#define next_match(tok) (tokens.peek().type == tok)
#define next_match_any(tok_type) (tok_type.has(tokens.peek().type))
#define curr_atch(tok) (curt.type == tok)

#define ParseFail(error)\
std::cout << "\nError: " << error << "\n caused by token '" << curt.str << "' on line " << curt.line << std::endl;  parse_failed = true;

#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectOneOf(exp_type)\
if(exp_type.has(curt.type)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 

#define ExpectSignature(...) if(check_signature(__VA_ARGS__))
#define ElseExpectSignature(...)  else if(check_signature(__VA_ARGS__))

#define ExpectFail(error)\
 else { ParseFail(error); }

#define ExpectFailCode(failcode)\
 else { failcode }

local map<Token_Type, ExpressionType> binaryOps{
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
	{Token_BitXOR,		       Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
};

local map<Token_Type, ExpressionType> unaryOps{
	{Token_BitNOT,     Expression_UnaryOpBitComp},
	{Token_LogicalNOT, Expression_UnaryOpLogiNOT},
	{Token_Negation,   Expression_UnaryOpNegate},
};

local array<Token_Type> typeTokens{
	Token_Signed32,
	Token_Signed64,
	Token_Unsigned32,
	Token_Unsigned64,
	Token_Float32,
	Token_Float64,
};

template<class... T>
inline b32 check_signature(T... in) {
	int i = 0;
	return ((tokens.peek(i++).type == in) && ...);
}

Function*    function;
BlockItem*   blockitem;
Declaration* declaration;
Statement*   statement;
Expression*  expression;

Arena arena;

inline void new_function(string& identifier) {
	function = (Function*)arena.add(Function{ identifier });
	function->node.prev = function->node.next = &function->node;
}

inline void new_block_item() {
	blockitem = (BlockItem*)arena.add(BlockItem());
	blockitem->node.prev = blockitem->node.next = &blockitem->node;
}

inline void new_declaration() {
	declaration = (Declaration*)arena.add(Declaration());
	declaration->node.prev = declaration->node.next = &declaration->node;
}

inline void new_statement() {
	statement = (Statement*)arena.add(Statement());
	statement->node.prev = statement->node.next = &statement->node;
}

inline void new_expression(string& str, ExpressionType type) {
	expression = (Expression*)arena.add(Expression(str, type));
	expression->node.prev = expression->node.next = &expression->node;
}

enum ParseState {
	psGlobal,      	// <program>       :: = <function>
	psFunction,		// <function>      :: = "int" <id> "(" ")" "{" { <block item> } "}"
	psBlockItem,   	// <block item>    :: = <statement> | <declaration>
	psDeclaration,	// <declaration>   :: = "int" <id> [ = <exp> ] ";"
	psStatement,	// <statement>     :: = "return" <exp> ";" | <exp> ";" | "if" "(" <exp> ")" <statement> [ "else" <statement> ] | "{" { <block-item> } "}
	psExpression,	// <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional,	// <conditional>   :: = <logical or> [ "?" <exp> ":" <conditional> ]
	psLogicalOR,	// <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,	// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,	// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,	// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,	// <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,		// <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,	// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,		// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,		// <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,			// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,		// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
	psUnary,        // <unary>         :: = "!" | "~" | "-"
}; 

Node* debugprogramnode = 0;

#define EarlyOut goto emergency_exit
void parser(ParseState state, Node* node) {
	if (parse_failed) return;

	switch (state) {

		case psGlobal: { ////////////////////////////////////////////////////////////////////// @Global
			while (!(curt.type == Token_EOF || next_match(Token_EOF))) {
				ExpectOneOf(typeTokens) {
					token_next();
					Expect(Token_Identifier){
						if (next_match(Token_OpenParen)) {
							new_function(curt.str);
							NodeInsertChild(node, &function->node, "function");
							parser(psFunction, &function->node);
						}
					}
					Expect(Token_Identifier) {
						token_next();
						Expect(Token_OpenParen) {

						}
					}
				}ExpectFail("yeah i dont know right now");
				token_next();
			}
		}break;

		case psFunction: { //////////////////////////////////////////////////////////////////// @Function
			token_next();
			Expect(Token_OpenParen) { 
				token_next();
				//function parameter checking will go here
				Expect(Token_CloseParen) {
					token_next();
					Expect(Token_OpenBrace) {
						while (!next_match(Token_CloseBrace)) {
							token_next();
							//we are now within the scope of the function and expect only blockitems 
							new_block_item();
							NodeInsertChild(node, &blockitem->node, "blockitem");
							parser(psBlockItem, &blockitem->node);

							if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); EarlyOut; }
						}
					}ExpectFail("expected {");
				}ExpectFail("expected )");
			}ExpectFail("expected (");
		}break;

		case psBlockItem: {//////////////////////////////////////////////////////////////////// @BlockItem
			//here we look for whether we are declaring a variable or a statement
			
			ExpectOneOf(typeTokens) {
				new_declaration();
				NodeInsertChild(node, &declaration->node, "declaration");
				parser(psDeclaration, &declaration->node);
			}
			else {
				new_statement();
				NodeInsertChild(node, &statement->node, "statement");
				parser(psStatement, &statement->node);
			}
		}break;

		case psDeclaration: {////////////////////////////////////////////////////////////////// @Declaration
			Token_Type type = curt.type;
			token_next();
			Expect(Token_Identifier) {
				string id = curt.str; 
				token_next();
				Expect(Token_Assignment) {
					new_expression(curt.str, ExpressionGuard_Assignment);
					token_next();
					NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Assignment]);
					parser(psExpression, &expression->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("missing ; after variable assignment")
				}
				ElseExpect(Token_Semicolon) {}
				ExpectFail("missing semicolon or assignment after variable declaration");
			}
		}break;

		case psStatement: {//////////////////////////////////////////////////////////////////// @Statement
			switch (curt.type) {
				case Token_If: {
					statement->type = Statement_Conditional;
					statement->node.debug_str = "conditional statement";
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						parser(psExpression, &statement->node);
						if (!statement->node.first_child)
							ParseFail("missing expression for if statement");

						token_next();
						Expect(Token_CloseParen) {
							if (next_match_any(typeTokens)) {
								ParseFail("attempt to declare a variable in non-scoped if statement");
							}
							else {
								token_next();
								new_statement();
								statement->type = Statement_If;
								NodeInsertChild(node, &statement->node, "if statement");
								parser(psStatement, &statement->node);
								Expect(Token_Semicolon) {}
								ExpectFail("expected a ;");

								if (next_match(Token_Else)) {
									token_next();
									new_statement();
									statement->type = Statement_Else;
									NodeInsertChild(node, &statement->node, "else statement");
									token_next();
									parser(psStatement, &statement->node);
								}
							}
						}ExpectFail("expected a )");
					}ExpectFail("expected a (");
				}break;

				case Token_Literal:
				case Token_Identifier: {
					statement->type = Statement_Expression;
					parser(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {} 
					ExpectFail("Expected a ;");
				}break;

				case Token_Return: {
					statement->type = Statement_Return;
					token_next();
					parser(psExpression, node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ;");
					
				}break;
			}
		}break;

		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						new_expression(curt.str, Expression_IdentifierLHS);     
						NodeInsertChild(node, &expression->node, ExTypeStrings[Expression_IdentifierLHS]); token_next();
						new_expression(curt.str, Expression_BinaryOpAssignment); 
						NodeInsertChild(node, &expression->node, ExTypeStrings[Expression_BinaryOpAssignment]); token_next();
						new_expression(curt.str, ExpressionGuard_Assignment);    
						NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Assignment]);

						parser(psExpression, &expression->node);
					}
					else {
						//new_expression_on_expression(curt.str, ExpressionGuard_HEAD);
						new_expression(curt.str, ExpressionGuard_HEAD);
						NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
						parser(psConditional, &expression->node);
					}
				}break;
				default: {
					new_expression(curt.str, ExpressionGuard_HEAD);
					NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
					parser(psConditional, &expression->node);
				}break;

			}
			//pop_expression();
		}break;

		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			new_expression(curt.str, ExpressionGuard_Conditional);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Conditional]);
			parser(psLogicalOR, &expression->node);

			while (next_match(Token_QuestionMark)) {
				token_next();
				new_expression(curt.str, Expression_TernaryConditional);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[Expression_TernaryConditional]);
				parser(psExpression, &expression->node);
				token_next();
				Expect(Token_Colon) {
					new_expression(curt.str, ExpressionGuard_Conditional);
					token_next();
					NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Conditional]);
					parser(psConditional, &expression->node);
				}ExpectFail("Expected : for ternary conditional")
			}
		}break;

		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			new_expression(curt.str, ExpressionGuard_LogicalOR);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalOR]);
			parser(psLogicalAND, &expression->node);

			while (next_match(Token_OR)) {
				token_next();
				new_expression(curt.str, Expression_BinaryOpOR);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalOR]);
				parser(psLogicalAND, &expression->node);
			}
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			new_expression(curt.str, ExpressionGuard_LogicalAND);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalAND]);
			parser(psBitwiseOR, &expression->node);

			while (next_match(Token_AND)) {
				token_next();
				new_expression(curt.str, Expression_BinaryOpAND);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalAND]);
				parser(psBitwiseOR, &expression->node);
			}
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			new_expression(curt.str, ExpressionGuard_BitOR);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitOR]);
			parser(psBitwiseXOR, &expression->node);

			while (next_match(Token_BitOR)) {
				token_next();
				new_expression(curt.str, Expression_BinaryOpBitOR);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitOR]);
				parser(psBitwiseXOR, &expression->node);
			}
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			new_expression(curt.str, ExpressionGuard_BitXOR);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitXOR]);
			parser(psBitwiseAND, &expression->node);

			while (next_match(Token_BitXOR)) {
				token_next();
				new_expression(curt.str, Expression_BinaryOpBitXOR);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitXOR]);
				parser(psBitwiseAND, &expression->node);
			}
		}break;

		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			new_expression(curt.str, ExpressionGuard_BitAND);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitAND]);
			parser(psEquality, &expression->node);


			while (next_match(Token_BitAND)) {
				token_next();
				new_expression(curt.str, Expression_BinaryOpBitAND);
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitAND]);
				parser(psEquality, &expression->node);
			}
		}break;

		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			new_expression(curt.str, ExpressionGuard_Equality);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Equality]);
			parser(psRelational, &expression->node);

			while (next_match(Token_NotEqual) || next_match(Token_Equal)) {
				token_next();
				new_expression(curt.str, *binaryOps.at(curt.type));
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Equality]);
				parser(psRelational, &expression->node);
			}
		}break;

		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			new_expression(curt.str, ExpressionGuard_Relational);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Relational]);
			parser(psBitshift, &expression->node);


			while (next_match(Token_LessThan) || next_match(Token_GreaterThan) || next_match(Token_LessThanOrEqual) || next_match(Token_GreaterThanOrEqual)) {
				token_next();
				new_expression(curt.str, *binaryOps.at(curt.type));
				token_next();
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Relational]);
				parser(psBitshift, &expression->node);
			}
		}break;

		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			new_expression(curt.str, ExpressionGuard_BitShift);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitShift]);
			parser(psAdditive, &expression->node);


			while (next_match(Token_BitShiftLeft) || next_match(Token_BitShiftRight)) {
				token_next();
				new_expression(curt.str, *binaryOps.at(curt.type));
				NodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_BitShift);
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitShift]); 
				parser(psAdditive, &expression->node);
			}
		}break;

		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			new_expression(curt.str, ExpressionGuard_Additive);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Additive]);
			parser(psTerm, &expression->node);

			while (next_match(Token_Plus) || next_match(Token_Negation)) {
				token_next();
				new_expression(curt.str, *binaryOps.at(curt.type));
				NodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_Additive);
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Additive]);
				parser(psTerm, &expression->node);
			}
		}break;
			 
		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			new_expression(curt.str, ExpressionGuard_Term);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Term]);
			parser(psFactor, &expression->node);

			while (next_match(Token_Multiplication) || next_match(Token_Division) || next_match(Token_Modulo)) {
				token_next();
				new_expression(curt.str, *binaryOps.at(curt.type)); 
				NodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_Term);
				NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Term]);
				parser(psFactor, &expression->node);
			}
		}break;

		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			new_expression(curt.str, ExpressionGuard_Factor);
			NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Factor]);
			node = &expression->node;

			switch (curt.type) {
				case Token_Literal: {
					new_expression(curt.str, Expression_IntegerLiteral);
					NodeInsertChild(node, &expression->node, toStr(ExTypeStrings[Expression_IntegerLiteral], " ", curt.str));
				}break;

				case Token_OpenParen: {
					//new_expression(curt.str, ExpressionGuard_HEAD);
					//NodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
					token_next();
					parser(psExpression, &expression->node);
					token_next();
					Expect(Token_CloseParen) {}
					ExpectFail("expected a )");

				}break;

				case Token_Identifier: {
					new_expression(curt.str, Expression_IdentifierRHS);
					NodeInsertChild(node, &expression->node, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.str));
				}break;

				default: {
					ExpectOneOf(unaryOps) {
						new_expression(curt.str, *unaryOps.at(curt.type));
						NodeInsertChild(node, &expression->node, ExTypeStrings[*unaryOps.at(curt.type)]);
						token_next();
						parser(psFactor, &expression->node);
					}
					ExpectFail("unexpected token found in factor");
				}break;
			}


		}break;

		case psUnary: {//////////////////////////////////////////////////////////////////////// @Unary
		}break;
	}
			

emergency_exit: //TODO maybe not necessary
	int i;
}

// <program> ::= <function>
b32 suParser::parse(array<token>& tokens_in, Program& mother) {
	arena.init(Kilobytes(10));

	tokens = tokens_in;
	curt = tokens[0];

	mother.node.next = mother.node.prev = &mother.node;
	mother.node.debug_str = "program";

	debugprogramnode = &mother.node;
	
	parser(psGlobal, &mother.node);

	return parse_failed;
}