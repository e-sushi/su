#run_tests.py
#this file is expected to be in su/tests, but can be run from anywhere
#_______________________
#Command Line Arguments:
#-e [str]
#  executable path
#  default: tests_dir + "..\\build\\debug\\su.exe"
#
#-f [str]
#  test filter
#  default: "*"
#_______________________
#TODOs:

import sys,os,subprocess,enum,ctypes

ReturnCode_Success                = 0
ReturnCode_No_File_Passed         = 1
ReturnCode_File_Not_Found         = 2
ReturnCode_File_Locked            = 3
ReturnCode_File_Invalid_Extension = 4
ReturnCode_Invalid_Argument       = 5
ReturnCode_Lexer_Failed           = 6
ReturnCode_Preprocessor_Failed    = 7
ReturnCode_Parser_Failed          = 8
ReturnCode_Assembler_Failed       = 9

tests = [
    ["entry_point/valid/multi_digit.su", 100],
    ["entry_point/valid/newlines.su",     12],
    ["entry_point/valid/no_newlines.su",   6],
    ["entry_point/valid/return_0.su",      0],
    ["entry_point/valid/return_2.su",      2],
    ["entry_point/valid/spaces.su",        5],
    ["entry_point/valid/two_funcs.su",   100],
    ["entry_point/invalid/missing_paren.su",  ReturnCode_Parser_Failed],
    ["entry_point/invalid/missing_retval.su", ReturnCode_Parser_Failed],
    ["entry_point/invalid/no_brace.su",       ReturnCode_Parser_Failed],
    ["entry_point/invalid/no_semicolon.su",   ReturnCode_Parser_Failed],
    ["entry_point/invalid/no_space.su",       ReturnCode_Parser_Failed],
    ["entry_point/invalid/wrong_case.su",     ReturnCode_Parser_Failed],
    
    ["unary_ops/valid/bitwise.su",      -13],
    ["unary_ops/valid/bitwise_zero.su",  -1],
    ["unary_ops/valid/neg.su",           -5],
    ["unary_ops/valid/nested_ops.su",     0],
    ["unary_ops/valid/nested_ops_2.su",   1],
    ["unary_ops/valid/not_five.su",       0],
    ["unary_ops/valid/not_zero.su",       1],
    ["unary_ops/invalid/missing_const.su",        ReturnCode_Parser_Failed],
    ["unary_ops/invalid/missing_semicolon.su",    ReturnCode_Parser_Failed],
    ["unary_ops/invalid/nested_missing_const.su", ReturnCode_Parser_Failed],
    ["unary_ops/invalid/wrong_order.su",          ReturnCode_Parser_Failed],
    
    ["binary_ops/valid/add.su",             3],
    ["binary_ops/valid/associativity.su",  -4],
    ["binary_ops/valid/associativity_2.su", 1],
    ["binary_ops/valid/div.su",             2],
    ["binary_ops/valid/div_neg.su",        -2],
    ["binary_ops/valid/mult.su",            6],
    ["binary_ops/valid/parens.su",         14],
    ["binary_ops/valid/precedence.su",     14],
    ["binary_ops/valid/sub.su",            -1],
    ["binary_ops/valid/sub_neg.su",         3],
    ["binary_ops/valid/unop_add.su",        0],
    ["binary_ops/valid/unop_parens.su",    -3],
    ["binary_ops/invalid/malformed_paren.su",   ReturnCode_Parser_Failed],
    ["binary_ops/invalid/missing_first_op.su",  ReturnCode_Parser_Failed],
    ["binary_ops/invalid/missing_second_op.su", ReturnCode_Parser_Failed],
    ["binary_ops/invalid/no_semicolon.su",      ReturnCode_Parser_Failed],
    
    ["comparison_ops/valid/and_false.su",                                0],
    ["comparison_ops/valid/and_true.su",                            "true"],
    ["comparison_ops/valid/eq_false.su",                                 0],
    ["comparison_ops/valid/eq_true.su",                             "true"],
    ["comparison_ops/valid/ge_false.su",                                 0],
    ["comparison_ops/valid/ge_true.su",                             "true"],
    ["comparison_ops/valid/gt_false.su",                                 0],
    ["comparison_ops/valid/gt_true.su",                             "true"],
    ["comparison_ops/valid/le_false.su",                                 0],
    ["comparison_ops/valid/le_true.su",                             "true"],
    ["comparison_ops/valid/lt_false.su",                                 0],
    ["comparison_ops/valid/lt_true.su",                             "true"],
    ["comparison_ops/valid/ne_false.su",                                 0],
    ["comparison_ops/valid/ne_true.su",                             "true"],
    ["comparison_ops/valid/or_false.su",                                 0],
    ["comparison_ops/valid/or_true.su",                             "true"],
    ["comparison_ops/valid/precedence.su",                          "true"],
    ["comparison_ops/valid/precedence_2.su",                             0],
    ["comparison_ops/valid/precedence_3.su",                        "true"],
    ["comparison_ops/valid/precedence_4.su",                             0],
    ["comparison_ops/valid/precedence_5.su",                        "true"],
    ["comparison_ops/valid/skip_on_failure_multi_short_circuit.su", "true"],
    ["comparison_ops/valid/skip_on_failure_short_circuit_and.su",        0],
    ["comparison_ops/valid/skip_on_failure_short_circuit_or.su",         0],
    ["comparison_ops/invalid/missing_first_op.su",  ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_mid_op.su",    ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_second_op.su", ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_semicolon.su", ReturnCode_Parser_Failed],
    
    ["variables/valid/assign.su",         2],
    ["variables/valid/assign_val.su",     4],
    ["variables/valid/exp_return_val.su", 1],
    ["variables/valid/initialize.su",     0],
    ["variables/valid/missing_return.su", 0],
    ["variables/valid/multiple_vars.su",  3],
    ["variables/valid/no_initialize.su",  0],
    ["variables/valid/refer.su",          2],
    ["variables/valid/unused_exp.su",     0],
    ["variables/invalid/redefine.su",                ReturnCode_Parser_Failed],
    ["variables/invalid/syntax_err_bad_decl.su",     ReturnCode_Parser_Failed],
    ["variables/invalid/syntax_err_bad_decl_2.su",   ReturnCode_Parser_Failed],
    ["variables/invalid/syntax_err_bad_lvalue.su",   ReturnCode_Parser_Failed],
    ["variables/invalid/syntax_err_bad_lvalue_2.su", ReturnCode_Parser_Failed],
    ["variables/invalid/syntax_err_no_semicolon.su", ReturnCode_Parser_Failed],
    ["variables/invalid/undeclared_var.su",          ReturnCode_Parser_Failed],
    ["variables/invalid/var_declared_late.su",       ReturnCode_Parser_Failed],
    
    ["inc_dec/valid/decrement_post.su",        9],
    ["inc_dec/valid/decrement_pre.su",         8],
    ["inc_dec/valid/increment_post.su",       11],
    ["inc_dec/valid/increment_post_return.su", 5],
    ["inc_dec/valid/increment_pre.su",        12],
    ["inc_dec/valid/increment_pre_return.su",  6],
    ["inc_dec/invalid/literal_dec_post.su", ReturnCode_Parser_Failed],
    ["inc_dec/invalid/literal_dec_pre.su",  ReturnCode_Parser_Failed],
    ["inc_dec/invalid/literal_inc_post.su", ReturnCode_Parser_Failed],
    ["inc_dec/invalid/literal_inc_pre.su",  ReturnCode_Parser_Failed],
    ["inc_dec/invalid/unknown_dec_post.su", ReturnCode_Parser_Failed],
    ["inc_dec/invalid/unknown_dec_pre.su",  ReturnCode_Parser_Failed],
    ["inc_dec/invalid/unknown_inc_post.su", ReturnCode_Parser_Failed],
    ["inc_dec/invalid/unknown_inc_pre.su",  ReturnCode_Parser_Failed],
    
    
];

def main():
    #gather command line args
    tests_dir = os.path.dirname(__file__)
    su_exe_path = os.path.join(tests_dir, "..\\build\\debug\\su.exe")
    test_filter = "*"
    print_errors = False

    arg_index = 1;
    for _ in range(len(sys.argv)):
        if(_ != arg_index): continue;
        #print(arg_index)
        #print(sys.argv[arg_index])
        
        if(sys.argv[arg_index] == "-e"):
            if(arg_index+1 >= len(sys.argv)):
                print("ERROR: no argument passed to -e");
                return;
            elif(sys.argv[arg_index+1].startswith("-")):
                print("ERROR: invalid argument for -e:", sys.argv[arg_index+1]);
                return;
            else:
                su_exe_path = sys.argv[arg_index+1];
                arg_index += 1;
        elif(sys.argv[arg_index] == "-f"):
            if(arg_index+1 >= len(sys.argv)):
                print("ERROR: no argument passed to -f");
                return;
            elif(sys.argv[arg_index+1].startswith("-")):
                print("ERROR: invalid argument for -f:", sys.argv[arg_index+1]);
                return;
            else:
                test_filter = sys.argv[arg_index+1];
                arg_index += 1;
        elif(sys.argv[arg_index] == "--p"):
            print_errors = True
        else:
            print("ERROR: unknown flag: ", sys.argv[arg_index]);
            return;
        arg_index += 1;
    #print(su_exe_path);
    #print(test_filter);
    
    filter_wildcard = test_filter.endswith("*")

    #iterate thru tests
    tests_total  = 0;
    tests_passed = 0;
    tests_failed = 0;
    for path,expected in tests:
        #print(path);
        #print(path.startswith(test_filter[0:-1]))
        
        if(filter_wildcard):
            if not(path.startswith(test_filter[0:-1])):
                #print(1);
                continue;
        elif(path != test_filter):
                #print(2);
                continue;
    
        tests_total += 1;
        full_path = tests_dir + "/" + path;
        path_arr = full_path.split('/');
        group = path_arr[-3];
        type  = path_arr[-2];
        name  = path_arr[-1];
        file     = full_path[:-3];
        file_s   = file+".s";
        file_su  = file+".su";
        file_exe = file+".exe";
        compile_cmd = su_exe_path+" -i "+file_su+" -o "+"/".join([tests_dir,group,type,""]);
        #print(compile_cmd)
        
        if(type == 'valid'): #valid tests should return errorlevel 0 from su.exe
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8");
                subprocess.run("gcc "+"-m64 "+file_s+" -o "+file_exe);
                if not(os.path.exists(file_exe)):
                    print("%s %s (link error)" % (path.ljust(60, '_'), "FAILED"));
                    tests_failed += 1;
                    continue;
                
                try:
                    subprocess.run(file_exe, capture_output=True, check=True, encoding="utf-8");
                    #print("%-60s %s (E: %d; A: %d)" % (path, "PASSED", expected, 0));
                    print("%-60s %s" % (path, "PASSED"));
                    tests_passed += 1;
                except subprocess.CalledProcessError as err:
                    actual = ctypes.c_int32(err.returncode).value;
                    if((actual == expected) or (expected == "true" and actual != 0)):
                        #print("%-60s%s (E: %d; A: %d)" % (path, "PASSED", expected, actual));
                        print("%-60s %s" % (path, "PASSED"));
                        tests_passed += 1;
                    else:
                        if(print_errors): print(err.stdout);
                        print("%s %s (E: %d; A: %d)" % (path.ljust(60, '_'), "FAILED", expected, actual));
                        tests_failed += 1;
                os.remove(file_exe);
            except subprocess.CalledProcessError as err:
                actual = ctypes.c_int32(err.returncode).value;
                if(print_errors): print(err.stdout);
                print("%s %s (compile error: %d)" % (path.ljust(60, '_'), "FAILED", actual));
                tests_failed += 1;
        elif(type == 'invalid'):
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8");
                
                os.remove(file_s);
                print("%s %s (no compile error)" % (path.ljust(60, '_'), "FAILED"));
                tests_failed += 1;
            except subprocess.CalledProcessError as err:
                actual = ctypes.c_int32(err.returncode).value;
                if((actual == expected) or (expected == "true" and actual != 0)):
                    #print("%-60s%s (E: %d; A: %d)" % (path, "PASSED", expected, actual));
                    print("%-60s %s" % (path, "PASSED"));
                    tests_passed += 1;
                else:
                    if(print_errors): print(err.stdout);
                    print("%s %s (E: %d; A: %d)" % (path.ljust(60, '_'), "FAILED", expected, actual));
                    tests_failed += 1;
        else:
            print("ERROR: test path did not match format 'group/validity/name.su': ", path);
    print("tests: %d; passed: %d; failed: %d;" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
    main();