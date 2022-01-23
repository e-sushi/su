#define logf(tag,fmt,...) printf(GLUE(GLUE("[",GLUE(GLUE(tag,"] "), fmt)), "\n"), __VA_ARGS__)
#define logfW(tag,fmt,...) printf(GLUE(GLUE("[",GLUE(GLUE(tag,"-warning] "), fmt)), "\n"), __VA_ARGS__)
#define logfE(tag,fmt,...) printf(GLUE(GLUE("[",GLUE(GLUE(tag,"-error] "), fmt)), "\n"), __VA_ARGS__)

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