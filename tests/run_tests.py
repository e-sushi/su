#run_tests.py
#expected to be run from su/tests
#TODO add command-line args for su.exe path and tests directory

import os,subprocess,enum,ctypes

su_exe_path = "..\\build\\debug\\su.exe"

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
    
    ["comparison_ops/valid/and_false.su",                              0],
    ["comparison_ops/valid/and_true.su",                            True],
    ["comparison_ops/valid/eq_false.su",                               0],
    ["comparison_ops/valid/eq_true.su",                             True],
    ["comparison_ops/valid/ge_false.su",                               0],
    ["comparison_ops/valid/ge_true.su",                             True],
    ["comparison_ops/valid/gt_false.su",                               0],
    ["comparison_ops/valid/gt_true.su",                             True],
    ["comparison_ops/valid/le_false.su",                               0],
    ["comparison_ops/valid/le_true.su",                             True],
    ["comparison_ops/valid/lt_false.su",                               0],
    ["comparison_ops/valid/lt_true.su",                             True],
    ["comparison_ops/valid/ne_false.su",                               0],
    ["comparison_ops/valid/ne_true.su",                             True],
    ["comparison_ops/valid/or_false.su",                               0],
    ["comparison_ops/valid/or_true.su",                             True],
    ["comparison_ops/valid/precedence.su",                          True],
    ["comparison_ops/valid/precedence_2.su",                           0],
    ["comparison_ops/valid/precedence_3.su",                        True],
    ["comparison_ops/valid/skip_on_failure_multi_short_circuit.su", True],
    ["comparison_ops/valid/skip_on_failure_short_circuit_and.su",      0],
    ["comparison_ops/valid/skip_on_failure_short_circuit_or.su",       0],
    ["comparison_ops/invalid/missing_first_op.su",  ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_mid_op.su",    ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_second_op.su", ReturnCode_Parser_Failed],
    ["comparison_ops/invalid/missing_semicolon.su", ReturnCode_Parser_Failed],
];

def main():
    tests_total  = 0;
    tests_passed = 0;
    tests_failed = 0;

    for path,expected in tests:
        path_arr = path.split('/');
        group = path_arr[0];
        type  = path_arr[1];
        name  = path_arr[2];
        file     = path[:-3];
        file_s   = file+".s";
        file_su  = file+".su";
        file_exe = file+".exe";
        compile_cmd = su_exe_path+" -i "+file_su+" -o "+"/".join([group,type,""]);
        #print(compile_cmd)
        
        if(type == 'valid'): #valid tests should return errorlevel 0 from su.exe
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True);
                subprocess.run("gcc "+"-m64 "+file_s+" -o "+file_exe);
                if not(os.path.exists(file_exe)): continue;
                
                try:
                    subprocess.run(file_exe, capture_output=True, check=True);
                    print("%-60s%s" % (file, "PASSED"));
                except subprocess.CalledProcessError as err:
                    actual = ctypes.c_int32(err.returncode).value;
                    if((actual == expected) or (expected == True and expected != 0)):
                        print("%-60s%s" % (file, "PASSED"));
                        tests_passed += 1;
                    else:
                        print("%-60s%s (E: %d; A: %d)" % (file, "FAILED", expected, actual));
                        tests_failed += 1;
                os.remove(file_exe);
            except subprocess.CalledProcessError as err:
                actual = ctypes.c_int32(err.returncode).value;
                print("%-60s%s (compile error: %d)" % (file, "FAILED", actual));
                tests_failed += 1;
        elif(type == 'invalid'):
            tests_total += 1;
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True);
                
                os.remove(file_s);
                print("%-60s%s (no compile error)" % (file, "FAILED"));
                tests_failed += 1;
            except subprocess.CalledProcessError as err:
                actual = ctypes.c_int32(err.returncode).value;
                if((actual == expected) or (expected == True and expected != 0)):
                    print("%-60s%s" % (file, "PASSED"));
                    tests_passed += 1;
                else:
                    print("%-60s%s (E: %d; A: %d)" % (file, "FAILED", expected, actual));
                    tests_failed += 1;
    print("tests: %d; passed: %d; failed: %d" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
    main()