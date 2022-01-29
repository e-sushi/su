#run_tests.py
#this file is expected to be in su/tests, but can be run from anywhere
#delle preset: python3 tests\run_tests.py --k --p -t 2 -f *
#_______________________
#Command Line Arguments:
#-e [str]
#  executable path
#  default: tests_dir + "..\\build\\debug\\su.exe"
#
#-f [str]
#  test filter
#  default: "*"
#
#-t [int]
#  timeout (in seconds)
#  default: 3
#
#--k
#  keep the .exe's
#
#--p
#  print su output on error (above the failed/passed message)
#_______________________
#TODOs:
#replace return codes with error codes

import sys,os,subprocess,ctypes

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
    ["variables/valid/multiple_vars.su",  3],
    ["variables/valid/no_initialize.su",  0],
    ["variables/valid/refer.su",          2],
    ["variables/valid/unused_exp.su",     0],
    ["variables/invalid/missing_return.su",       ReturnCode_Assembler_Failed],
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
    
    ["if_else/valid/declare_statement.su", 3],
    ["if_else/valid/else.su",              2],
    ["if_else/valid/if_nested.su",         1],
    ["if_else/valid/if_nested_2.su",       2],
    ["if_else/valid/if_nested_3.su",       3],
    ["if_else/valid/if_nested_4.su",       4],
    ["if_else/valid/if_nested_5.su",       1],
    ["if_else/valid/if_not_taken.su",      0],
    ["if_else/valid/if_taken.su",          1],
    ["if_else/valid/multiple_if.su",       8],
    ["if_else/invalid/mismatched_nesting.su", ReturnCode_Assembler_Failed],
    
    ["ternary/valid/assign_ternary.su",          2],
    ["ternary/valid/internal_assignment.su",     2],
    ["ternary/valid/multiple_ternary.su",       10],
    ["ternary/valid/nested_ternary.su",          7],
    ["ternary/valid/nested_ternary_2.su",       15],
    ["ternary/valid/ternary.su",                 4],
    ["ternary/valid/ternary_short_circuit.su",   7],
    ["ternary/valid/ternary_short_circuit_2.su", 3],
    ["ternary/invalid/c_ternary.su",           ReturnCode_Parser_Failed],
    ["ternary/invalid/incomplete_ternary.su",  ReturnCode_Parser_Failed],
    ["ternary/invalid/malformed_ternary.su",   ReturnCode_Parser_Failed],
    ["ternary/invalid/malformed_ternary_2.su", ReturnCode_Parser_Failed],
    ["ternary/invalid/rh_assignment.su",       ReturnCode_Parser_Failed],
    ["ternary/invalid/ternary_assign.su",      ReturnCode_Parser_Failed],
    
    #["scopes/valid/consecutive_blocks.su",       1],
    #["scopes/valid/consecutive_declarations.su", 3],
    #["scopes/valid/declare_after_block.su",      3],
    #["scopes/valid/declare_block.su",            1],
    #["scopes/valid/declare_late.su",             3],
    #["scopes/valid/multi_nesting.su",            3],
    #["scopes/valid/nested_if.su",                4],
    #["scopes/valid/nested_scope.su",             4],
    #["scopes/invalid/double_define.su",            ReturnCode_Parser_Failed],
    #["scopes/invalid/out_of_scope.su",             ReturnCode_Parser_Failed],
    #["scopes/invalid/syntax_err_extra_brace.su",   ReturnCode_Parser_Failed],
    #["scopes/invalid/syntax_err_missing_brace.su", ReturnCode_Parser_Failed],
    
    #["loops/valid/break.su",                 15],
    #["loops/valid/continue.su",               1],
    #["loops/valid/continue_empty_post.su",   30],
    #["loops/valid/empty_expression.su",      10],
    #["loops/valid/for.su",                    3],
    #["loops/valid/for_decl.su",               2],
    #["loops/valid/for_empty.su",              3],
    #["loops/valid/for_nested_scope.su",       3],
    #["loops/valid/for_variable_shadow.su",   65],
    #["loops/valid/nested_break.su",         250],
    #["loops/valid/nested_while.su",          65],
    #["loops/valid/return_in_while.su",        2],
    #["loops/valid/while_multi_statement.su",  6],
    #["loops/valid/while_single_statement.su", 6],
    #["loops/invalid/break_not_in_loop.su",                 ReturnCode_Parser_Failed],
    #["loops/invalid/continue_not_in_loop.su",              ReturnCode_Parser_Failed],
    #["loops/invalid/out_of_scope.su",                      ReturnCode_Parser_Failed],
    #["loops/invalid/syntax_err_empty_clause.su",           ReturnCode_Parser_Failed],
    #["loops/invalid/syntax_err_paren_mismatch.su",         ReturnCode_Parser_Failed],
    #["loops/invalid/syntax_err_statement_in_condition.su", ReturnCode_Parser_Failed],
    #["loops/invalid/syntax_err_too_few_for_clauses.su",    ReturnCode_Parser_Failed],
    #["loops/invalid/syntax_err_too_many_for_clauses.su",   ReturnCode_Parser_Failed],
];

def main():
    #gather command line args
    tests_dir = os.path.dirname(__file__)
    su_exe_path = os.path.join(tests_dir, "..\\build\\debug\\su.exe")
    test_filter = "*"
    print_errors = False
    keep_exes = False
    test_timeout = 3

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
        elif(sys.argv[arg_index] == "-t"):
            if(arg_index+1 >= len(sys.argv)):
                print("ERROR: no argument passed to -t");
                return;
            elif(sys.argv[arg_index+1].startswith("-")):
                print("ERROR: invalid argument for -t:", sys.argv[arg_index+1]);
                return;
            else:
                test_timeout = int(sys.argv[arg_index+1]);
                arg_index += 1;
        elif(sys.argv[arg_index] == "--p"):
            print_errors = True
        elif(sys.argv[arg_index] == "--k"):
            keep_exes = True
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
                subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
                
                try:
                    subprocess.run("gcc "+"-m64 "+file_s+" -o "+file_exe, timeout=test_timeout);
                except subprocess.CalledProcessError as err:
                    print("%s %s (link error)" % (path.ljust(60, '_'), "FAILED"));
                    tests_failed += 1;
                    continue;
                except subprocess.TimeoutExpired as err:
                    print("%s %s (linking took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
                    tests_failed += 1;
                    continue;
                
                try:
                    subprocess.run(file_exe, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
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
                except subprocess.TimeoutExpired as err:
                    print("%s %s (test took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
                    tests_failed += 1;
                if not(keep_exes): os.remove(file_exe);
            except subprocess.CalledProcessError as err:
                actual = ctypes.c_int32(err.returncode).value;
                if(print_errors): print(err.stdout);
                print("%s %s (compile error: %d)" % (path.ljust(60, '_'), "FAILED", actual));
                tests_failed += 1;
            except subprocess.TimeoutExpired as err:
                print("%s %s (compilation took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
                tests_failed += 1;
        elif(type == 'invalid'):
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
                
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
            except subprocess.TimeoutExpired as err:
                print("%s %s (compilation took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
        else:
            print("ERROR: test path did not match format 'group/validity/name.su': ", path);
    print("tests: %d; passed: %d; failed: %d;" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
    main();