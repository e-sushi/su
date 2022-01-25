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
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level1 (warnings that are probably programmer errors, but are valid in rare cases)
	WarningCodes_Level1_Start = WarningCodes_NULL,
	
	
	WC_Not_All_Paths_Return,
	//WC_Implicit_Narrowing_Conversion,
	
	
	WarningCodes_Level1_End,
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level2 (all the other warnings)
	WarningCodes_Level2_Start = WarningCodes_Level1_End-1,
	
	
	
	
	
	WarningCodes_Level2_End,
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level3 (warnings you might want to be aware of, but are valid in most cases)
	WarningCodes_Level3_Start = WarningCodes_Level2_End-1,
	
	
	WC_Unreachable_Code_After_Return,
	//WC_Negative_Constant_Assigned_To_Unsigned_Variable,
	
	
	WarningCodes_Level3_End,
	WarningCodes_COUNT = WarningCodes_Level3_End
};



//////////////// //NOTE we might want to increment the pointer by 3 to skip the "WC_", but for now it includes it
//// @Names ////
////////////////
#define NAME(warning) STRINGIZE(warning)
const char* WarningCodes_Names[WarningCodes_COUNT] = {
	"NULL",
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level1 (warnings that are probably programmer errors, but are valid in rare cases)
	
	NAME(WC_Not_All_Paths_Return),
	//NAME(WC_Implicit_Narrowing_Conversion),
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level2 (all the other warnings)
	
	
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level3 (warnings you might want to be aware of, but are valid in most cases)
	
	NAME(WC_Unreachable_Code_After_Return),
	//NAME(WC_Negative_Constant_Assigned_To_Unsigned_Variable),
};
#undef NAME



/////////////////// //NOTE we append "%s" to the end of strings for call-site formatting
//// @Messages ////
///////////////////
#define MSG(warning,msg) "[%s%d] "msg"\n%s"
const char* WarningCodes_Messages[WarningCodes_COUNT] = {
	"null", //WarningCodes_Null
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level1 (warnings that are probably programmer errors, but are valid in rare cases)
	
	MSG(WC_Not_All_Paths_Return, "Not all code paths in the non-void function '%s' return a value. Auto-inserting a return 0 at the end of the function."),
	//MSG(WC_Implicit_Narrowing_Conversion, ""),
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level2 (all the other warnings)
	
	
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @Level3 (warnings you might want to be aware of, but are valid in most cases)
	
	MSG(WC_Unreachable_Code_After_Return, "There is unreachable code after the return statement."),
	//MSG(WC_Negative_Constant_Assigned_To_Unsigned_Variable, ""),
};
#undef MSG



////////////////////////
//// @Functionality ////
////////////////////////
//TODO maybe move errors here as well and have an error tracker so we stop compiling a file after a certain amount of errors

#include <bitset> //supposedly optimized compile-time bool array
std::bitset<WarningCodes_COUNT> disabledWC; //NOTE disabled b/c bitset defaults to zero

//TODO add info for file, line, column, related code, call-site formatting
int warning_level_mapping[] = { WarningCodes_Level1_Start-1, WarningCodes_Level2_Start-1, WarningCodes_Level3_Start-1, MAX_U16};
#define log_warning(warning,...) \
if(   (!globals.supress_warnings) \
&& (warning <= warning_level_mapping[globals.warning_level]) \
&& (!disabledWC.test(warning))){ \
printf(WarningCodes_Messages[warning], ((globals.warnings_as_errors) ? "WX" : "W"), warning, __VA_ARGS__, ""); \
}



