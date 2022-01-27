

/*

Preprocessor BN
	
		<directive> :: = "#" <command>
		<command>   :: = ( <include> | <filter> )
		<include>   :: = """ <filename> """ { ( <include-rename> | <include-filters> ) }
		<filter>    :: = "filter" ( <string> | "internal" )

*/


void Preprocessor::preprocess_parse() {

}

//preprocesses lexed files to join their tokens together 
b32 Preprocessor::preprocess() {
	//not fully implemented
	LexedFile& lf = lexer.file_index[0];

	tokens = lf.tokens;
	var_decl = lf.var_decl;
	func_decl = lf.func_decl;
	struct_decl = lf.struct_decl;
	preprocessor_tokens = lf.preprocessor_tokens;


	return true;
}