//////////////////
//// @Logging ////
//////////////////
//TODO replace usage of these with log_warning
#define logfWC(tag,level,code,fmt,...) if(!disabledWC[code]) logfW(tag,level,fmt,##__VA_ARGS__)
#define logWC(tag,level,code,...) if(!disabledWC[code]) logW(tag,level,##__VA_ARGS__)

//NOTE custom logging doesnt append a \n at the end
//// verbose ////
#define log(tag,...)  if(globals.verbose_print){ PRINTLN(toStr("[", tag, "]", ##__VA_ARGS__)); }
#define logW(tag,level,...)  if(!globals.supress_warnings && level <= globals.warning_level){ PRINTLN(toStr("[", tag, "-warning-l",level, "] ",##__VA_ARGS__));}
#define logE(tag,...) PRINTLN(toStr("[", tag, "-error] ", ##__VA_ARGS__))
#define logf(tag,fmt,...) if(globals.verbose_print)printf("[" tag "]"fmt, "\n"), ##__VA_ARGS__)
#define logfW(tag,level,fmt,...) if(!globals.supress_warnings && level <= globals.warning_level)printf("[" tag "-warning] " fmt "\n"), ##__VA_ARGS__)
#define logfE(tag,fmt,...)  printf("[" tag "-error] " fmt "\n", ##__VA_ARGS__)

//// notes ////
//TODO gcc places notes after some errors. 
//     eg: undeclared identifier errors will only print the first time they are encountered in a scope
//#define log_note()

//// warnings ////
#include <bitset> //supposedly optimized compile-time bool array
std::bitset<WarningCodes_COUNT> disabledWC; //NOTE disabled b/c bitset defaults to zero

int warning_level_mapping[] = { WarningCodes_Level1_Start-1, WarningCodes_Level2_Start-1, WarningCodes_Level3_Start-1, MAX_U16};
#define log_warning_custom(code,filename_len,filename_str,line,col,custom,...) \
if((!globals.supress_warnings) && (code <= warning_level_mapping[globals.warning_level]) && (!disabledWC.test(code))) \
printf(WarningCodes_Messages[code], filename_len,filename_str, line, col, ((globals.warnings_as_errors) ? "WE" : "W"), code, ##__VA_ARGS__, custom)
#define log_warning(code,filename_len,filename_str,line,col,...) log_warning_custom(code, filename_len,filename_str, line, col, "\n", ##__VA_ARGS__)

//// errors ////
#define log_error_custom(code,filename_len,filename_str,line,col,custom,...) \
printf(ErrorCodes_Messages[code], filename_len,filename_str, line, col, "E", code, ##__VA_ARGS__, custom)
#define log_error(code,filename_len,filename_str,line,col,...) log_error_custom(code, filename_len,filename_str, line, col, "\n", ##__VA_ARGS__)

///////////////
//// @File ////
///////////////
string load_file(const char* filepath){
	string result;
	
	FILE* file = fopen(filepath, "rb");
    if(!file){ printf("Failed to open file: %s", filepath); return ""; }
    defer{ fclose(file); };
	
	fseek(file, 0, SEEK_END); //!NotPortable
    long file_size = ftell(file);
    rewind(file);
	
	result.reserve(file_size);
    fread(result.str, sizeof(char), file_size, file);
	result.count += file_size;
    return result;
}

b32 write_file(const char* filepath, const string& contents){
	FILE* file = fopen(filepath, "wb");
    if(!file){ printf("Failed to open file: %s", filepath); return false; }
    defer{ fclose(file); };
	
	fwrite(contents.str, sizeof(char), contents.count, file);
    return true;
}

struct FilePath{
	cstring full;      //"c:/apps/calc.exe"
	cstring directory; //"c:/apps/"
	cstring filename;  //"calc"
	cstring extension; //"exe"
	
	FilePath(cstring filepath){
		full      = filepath;
		directory = {};
		filename  = filepath;
		extension = {};
		
		u32 last_slash = find_last_char<'/', '\\'>(filepath);
		u32 last_dot   = find_last_char<'.'>(filepath);
		if(last_slash != npos && last_dot != npos){
			extension.str   = filepath.str   + (last_dot+1);
			extension.count = filepath.count - (last_dot+1);
			
			filename.str   = filepath.str   + (last_slash+1);
			filename.count = filepath.count - (last_slash+1) - (extension.count+1);
			
			directory.str   = filepath.str;
			directory.count = filename.str - filepath.str;
		}
	}
};