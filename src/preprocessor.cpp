
b32 preprocess_file(LexedFile* file){
    for(u32 tidx : file->preprocessor_tokens){
        Token* curt = &file->tokens[tidx];
        switch(curt->type){
            case Token_Directive_Import:{
                    //importing modules from another file
                    Token* curt = &lfile.tokens[tidx];
                    curt++;
                    
                    if(curt->type == Token_OpenBrace){
                        //multiline directive 
                        while(curt->type != Token_CloseBrace){
                            if(curt->type == Token_Identifier || curt->type == Token_String){
                                Token* mod = curt;
                                
                                str8 check;

                                //attempt to find the module in import paths and current working directory
                                //first check cwd
                                if(file_exists((curt->type == Token_Identifier ? str8_concat(mod->raw, STR8(".su"), deshi_temp_allocator) : mod->raw){
                                    
                                }
                                curt++;
                                if(curt->type == Token_Dot){
                                    // importing specific modules from a module
                                    curt++;
                                    if(curt->type == Token_OpenBrace){
                                        //importing multiple modules from a module
                                        curt++;
                                        while(curt->type != Token_CloseBrace){
                                            if(curt->type == Token_Identifier){

                                            }
                                        }
                                    }

                                }
                            }
                            curt++;
                        }
                    }
                }break;
        } 
    }
}

b32 preprocess(){
    for(LexedFile& lfile : lexer.file_index){
        for(u32 tidx : lfile.preprocessor_tokens){
            Token directive = lfile.tokens[tidx];
            
            switch(directive.type){
                case Token_Directive_Import:{
                    //importing modules from another file
                    Token* curt = &lfile.tokens[tidx];
                    curt++;
                    
                    if(curt->type == Token_OpenBrace){
                        //multiline directive 
                        while(curt->type != Token_CloseBrace){
                            if(curt->type == Token_Identifier || curt->type == Token_String){
                                Token* mod = curt;
                                File
                                
                                //attempt to find the module in import paths and current working directory
                                //first check cwd
                                if(file_exists(mod->raw)){

                                }
                                curt++;
                                if(curt->type == Token_Dot){
                                    // importing specific modules from a module
                                    curt++;
                                    if(curt->type == Token_OpenBrace){
                                        //importing multiple modules from a module
                                        curt++;
                                        while(curt->type != Token_CloseBrace){
                                            if(curt->type == Token_Identifier){

                                            }
                                        }
                                    }

                                }
                            }
                            curt++;
                        }
                    }
                }break;
            }
        }
    }
}