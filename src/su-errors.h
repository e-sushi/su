#pragma once
#ifndef SU_ERRORS_H
#define SU_ERRORS_H

////////////////
//// @Codes ////
////////////////
enum ErrorCodes{
	ErrorCodes_NULL = 0,
	EC_Success = 0,
	
	EC_Multiline_Comment_No_End,
	//EC_Reserved_Keyword_Used_As_Identifier,
	//EC_Redeclared_Struct,
	//EC_Undeclared_Identifier,
	//EC_Not_Enough_Semicolons_In_For_Header,
	//EC_Too_Many_Semicolons_In_For_Header,
	//EC_For_Header_Control_Not_Boolean_Expression,
	//EC_While_Header_Not_Boolean_Expression,
	//EC_Break_Outside_Loop_or_Switch,
	//EC_Continue_Outside_Loop,
	
	ErrorCodes_COUNT
};

//////////////// //NOTE we might want to increment the pointer by 3 to skip the "EC_", but for now it includes it
//// @Names ////
////////////////
#define NAME(code) STRINGIZE(code)
const char* ErrorCodes_Names[ErrorCodes_COUNT] = {
	"null",
	
	NAME(EC_Multiline_Comment_No_End),
	//NAME(EC_Reserved_Keyword_Used_As_Identifier),
	//NAME(EC_Redeclared_Identifier_In_Same_Scope),
	//NAME(EC_Undeclared_Identifier),
	//NAME(EC_Not_Enough_Semicolons_In_For_Header),
	//NAME(EC_Too_Many_Semicolons_In_For_Header),
	//NAME(EC_For_Header_Control_Not_Boolean_Expression),
	//NAME(EC_While_Header_Not_Boolean_Expression),
	//NAME(EC_Break_Outside_Loop_or_Switch),
	//NAME(EC_Continue_Outside_Loop),
};
#undef NAME

/////////////////// //NOTE we append "%s" to the end of strings for call-site formatting
//// @Messages ////
///////////////////
#define MSG(code,msg) "%.*s(%d:%d):[%s%d] " msg "%s"
const char* ErrorCodes_Messages[ErrorCodes_COUNT] = {
	"null",
	
	MSG(EC_Multiline_Comment_No_End, "Multi-line comment has no ending */ token."),
	//MSG(EC_Reserved_Keyword_Used_As_Identifier, "'%.*s' is a reserved keyword and can't be used as an identifier."),
	//MSG(EC_Redeclared_Struct, "A struct named '%.*s' already exists at '%.*s(%d,%d)'."),
	//MSG(EC_Undeclared_Identifier, "Undeclared identifier '%s'."),
	//MSG(EC_Not_Enough_Semicolons_In_For_Header, "Less than two semicolons in for loop header."),
	//MSG(EC_Too_Many_Semicolons_In_For_Header, "More than two semicolons in for loop header."),
	//MSG(EC_For_Header_Control_Not_Boolean_Expression, "No conversion from '%s' to a boolean expression."),
	//MSG(EC_While_Header_Not_Boolean_Expression, "No conversion from '%s' to a boolean expression."),
	//MSG(EC_Break_Outside_Loop_or_Switch, "Break statement outside of a loop."),
	//MSG(EC_Continue_Outside_Loop, "Continue statement outside of a loop."),
};
#undef MSG

#endif //SU_ERRORS_H
