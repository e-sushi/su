////////////////// //NOTE custom logging doesnt append a \n at the end
//// @Logging ////
//////////////////
void log_code(cstring file, u32 char_offset, u32 l0, u32 c0, u32 l1, u32 c1){
	if(c1 < c0) Swap(c0, c1);
	if(l1 < l0) Swap(l0, l1);
	
	char* cursor = file.str + char_offset;
	char* end    = file.str + file.count;
	
	//find source start
	u32 new_line_counter = 0;
	while(cursor != file.str && new_line_counter < 3){
		if(*cursor-- == '\n') new_line_counter++;
	}
	l0 -= new_line_counter;
	
	//print source
	printf(" %5u | ", l0);
	while(cursor != end){
		putchar(*cursor);
		if(*cursor == '\n'){
			if(++l0 > l1) break;
			printf(" %5u | ", l0);
		}
		cursor++;
	}
	
	//print underline
	printf("        | %*s", c0, "");
	while(c0 < c1){
		putchar('~');
	}
	putchar('^'); putchar('\n');
}
void log_code(cstring file, u32 char_offset, u32 l0, u32 c0, u32 c1){ log_code(file, char_offset, l0, c0, l0, c1); }
void log_code(cstring file, u32 char_offset, u32 l0, u32 c0){ log_code(file, char_offset, l0, c0, l0, c0); }

//TODO replace usage of these with log_warning
#define logfWC(tag,level,code,fmt,...) if(!disabledWC[code]) logfW(tag,level,fmt,##__VA_ARGS__)
#define logWC(tag,level,code,...)      if(!disabledWC[code]) logW(tag,level,##__VA_ARGS__)

//// verbose ////
#define log(tag,...)             if(globals.verbose_print){ PRINTLN(toStr("[", tag, "]", ##__VA_ARGS__)); }
#define logW(tag,level,...)      if(!globals.supress_warnings && level <= globals.warning_level){ PRINTLN(toStr("[", tag, "-warning-l",level, "] ",##__VA_ARGS__));}
#define logE(tag,...)            if(globals.verbose_print) PRINTLN(toStr("[", tag, "-error] ", ##__VA_ARGS__))
#define logf(tag,fmt,...)        if(globals.verbose_print)printf("[" tag "]"fmt, "\n"), ##__VA_ARGS__)
#define logfW(tag,level,fmt,...) if(!globals.supress_warnings && level <= globals.warning_level)printf("[" tag "-warning] " fmt "\n"), ##__VA_ARGS__)
#define logfE(tag,fmt,...)       if(globals.verbose_print) printf("[" tag "-error] " fmt "\n", ##__VA_ARGS__)

//// warnings ////
#define log_warning_custom(code,filename_len,filename_str,line,col,custom,...) \
if((!globals.supress_warnings) && (code <= warning_level_mapping[globals.warning_level]) && (!disabledWC.test(code))) \
printf(WarningCodes_Messages[code], filename_len,filename_str, line, col, ((globals.warnings_as_errors) ? "WE" : "W"), code, ##__VA_ARGS__, custom)
#define log_warning(code,token,...) log_warning_custom(code, token->file.count,token->file.str, token->l0, token->c0, "\n", ##__VA_ARGS__)

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