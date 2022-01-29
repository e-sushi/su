#pragma once
#ifndef SU_WARNINGS_H
#define SU_WARNINGS_H

//Don't add a warning to the list until you add checking and printing for it.
//Try not to rearrange the order of warnings; so if you want to remove one, put a dummy in its place.
//Feel free to edit the warning messages to be more clear.

//NOTE this is likely to vary for a bit since we just started working on su.
//NOTE c++ enum values are one greater than the previous enum's value if not set to a specific value

////////////////
//// @Codes ////
////////////////
enum WarningCodes{
	WarningCodes_NULL = 0,
	//// @level1 //// (warnings that are probably programmer errors, but are valid in rare cases)
	WarningCodes_Level1_Start = WarningCodes_NULL,
	
	WC_Overflow,
	//WC_Implicit_Narrowing_Conversion,
	
	WarningCodes_Level1_End,
	//// @level2 //// (all the other warnings)
	WarningCodes_Level2_Start = WarningCodes_Level1_End,
	
	
	
	WarningCodes_Level2_End,
	//// @level3 //// (warnings you might want to be aware of, but are valid in most cases)
	WarningCodes_Level3_Start = WarningCodes_Level2_End,
	
	WC_Unreachable_Code_After_Return,
	WC_Unreachable_Code_After_Break,
	WC_Unreachable_Code_After_Continue,
	//WC_Negative_Constant_Assigned_To_Unsigned_Variable,
	
	WarningCodes_Level3_End,
	WarningCodes_COUNT = WarningCodes_Level3_End
};



//////////////// //NOTE we might want to increment the pointer by 3 to skip the "WC_", but for now it includes it
//// @Names ////
////////////////
#define NAME(code) STRINGIZE(code)
const char* WarningCodes_Names[WarningCodes_COUNT] = {
	"null",
	
	//// @level1 //// (warnings that are probably programmer errors, but are valid in rare cases)
	NAME(WC_Overflow),
	//NAME(WC_Implicit_Narrowing_Conversion),
	
	//// @level2 //// (all the other warnings)
	
	
	//// @level3 //// (warnings you might want to be aware of, but are valid in most cases)
	NAME(WC_Unreachable_Code_After_Return),
	NAME(WC_Unreachable_Code_After_Break),
	NAME(WC_Unreachable_Code_After_Continue),
	//NAME(WC_Negative_Constant_Assigned_To_Unsigned_Variable),
};
#undef NAME



/////////////////// //NOTE we append "%s" to the end of strings for call-site formatting
//// @Messages ////
///////////////////
#define MSG(code,msg) "%.*s(%d,%d):[%s%d] " msg "%s"
const char* WarningCodes_Messages[WarningCodes_COUNT] = {
	"null",
	
	//// @level1 //// (warnings that are probably programmer errors, but are valid in rare cases)
	MSG(WC_Overflow, "Overflow in conversion from '%s' to '%s'."),
	//MSG(WC_Implicit_Narrowing_Conversion, ""),
	
	//// @level2 //// (all the other warnings)
	
	
	//// @level3 //// (warnings you might want to be aware of, but are valid in most cases)
	MSG(WC_Unreachable_Code_After_Return, "There is unreachable code after the return statement."),
	MSG(WC_Unreachable_Code_After_Break, "There is unreachable code after the break statement."),
	MSG(WC_Unreachable_Code_After_Continue, "There is unreachable code after the continue statement."),
	//MSG(WC_Negative_Constant_Assigned_To_Unsigned_Variable, ""),
};
#undef MSG

#include <bitset>
std::bitset<WarningCodes_COUNT> disabledWC; //NOTE disabled b/c bitset defaults to zero
int warning_level_mapping[] = { WarningCodes_Level1_Start-1, WarningCodes_Level2_Start-1, WarningCodes_Level3_Start-1, MAX_U16};

#endif //SU_WARNINGS_H