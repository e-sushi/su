/*
    A Token represents the smallest piece of information we may have in a given file
*/

#ifndef AMU_TOKEN_H
#define AMU_TOKEN_H

#include "storage/String.h"
#include "storage/DString.h"
#include "representations/ScalarValue.h"

namespace amu{

struct Code;
struct LexicalScope;

struct Token {
	enum class Group {
		Null,
		Literal,
		Control,
		Operator,
		Keyword,
		Type,
		Directive,
	};

	enum class Kind {
		Null,

		Comment,

		Identifier,

		FloatLiteral,
		IntegerLiteral,
		CharacterLiteral,
		StringLiteral,

		Semicolon,
		LBrace,
		RBrace,
		LParen,
		RParen,
		LSquare,
		RSquare,
		Comma,
		QuestionMark,
		Colon,
		Dot,
		DotDouble,
		DotTriple,
		At,
		Pound,
		Backtick,
		RightArrow,
		RightArrowThick,

		Plus,
		PlusEqual,
		Minus,
		MinusEqual,
		Asterisk,
		AsteriskEqual,
		Solidus,
		SoldusEqual,
		Tilde,
		TildeEqual,
		Ampersand,
		AmpersandEqual,
		AmpersandDouble,
		VerticalLine,
		VerticalLineEqual,
		VerticalLineDouble,
		Caret,
		CaretEqual,
		LessThan,
		LessThanEqual,
		LessThanDouble,
		LessThanDoubleEqual,
		GreaterThan,
		GreaterThanEqual,
		GreaterThanDouble,
		GreaterThanDoubleEqual,
		Percent,
		PercentEqual,
		Equal,
		EqualDouble,
		ExplanationMark,
		ExplanationMarkEqual,
		Dollar,
		DollarDouble,

		Return,
		If,
		Else,
		And,
		Or,
		Then,
		Loop,
		While,
		For,
		Switch,
		Break,
		Continue,
		Defer,
		Struct,
		Module,
		Using,
		Import,
		Ptr,
		Ref,
		Deref,
		
		Void,
		Unsigned8,
		Unsigned16,
		Unsigned32,
		Unsigned64,
		Signed8,
		Signed16,
		Signed32,
		Signed64, 
		Float32,
		Float64,

		Directive_Include,
		Directive_Run,
		Directive_CompilerBreak,
	};

    String raw;
    u64 hash;

    Kind kind; 
    Group group;

	LexicalScope* scope;

    Code* code; 
    u64 l0, l1;
    u64 c0, c1;
    
	ScalarValue scalar_value;

	b32 is(Group x) { return group == x; }
	b32 is(Kind x)  { return kind == x; }
};

void to_string(DString* start, Token* t);

} // namespace amu

#endif // AMU_TOKEN_H
