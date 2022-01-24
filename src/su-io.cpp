//TODO make another log for messages
#define logf(tag,fmt,...) if(globals.verbose_print)printf(GLUE(GLUE("[",GLUE(GLUE(tag,"] "), fmt)), "\n"), __VA_ARGS__)
#define logfW(tag,level,fmt,...) if(!globals.supress_warnings && level <= globals.warning_level)printf(GLUE(GLUE("[",GLUE(GLUE(tag,"-warning] "), fmt)), "\n"), __VA_ARGS__)
#define logfWC(tag,level,code,fmt,...) if(enabledWC[code]) logfW(tag,level,fmt,__VA_ARGS__)
#define logfE(tag,fmt,...)  if(!globals.supress_errors) printf(GLUE(GLUE("[",GLUE(GLUE(tag,"-error] "), fmt)), "\n"), __VA_ARGS__)
#define log(tag,...)  if(globals.verbose_print){ PRINTLN(toStr("[", tag, "]", __VA_ARGS__)); }
#define logW(tag,level,...)  if(!globals.supress_warnings && level <= globals.warning_level){ PRINTLN(toStr("[", tag, "-warning-l",level, "] ",__VA_ARGS__));}
#define logWC(tag,level,code,...) if(enabledWC[code]) logW(tag,level,__VA_ARGS__)
#define logE(tag,...) if(!globals.supress_errors){PRINTLN(toStr("[", tag, "-error] ", __VA_ARGS__));}


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