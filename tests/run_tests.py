#run_tests.py
#expected to be run from su/tests
#TODO add command-line args for su.exe path and tests directory

import os,subprocess,enum

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
    
    ["unary_ops/valid/bitwise.su",      0b11111111111111111111111111110011], #~12
    ["unary_ops/valid/bitwise_zero.su", 0b11111111111111111111111111111111], #~0
    ["unary_ops/valid/neg.su",          0b11111111111111111111111111111011], #-5
    ["unary_ops/valid/nested_ops.su",   0],
    ["unary_ops/valid/nested_ops_2.su", 1],
    ["unary_ops/valid/not_five.su",     0],
    ["unary_ops/valid/not_zero.su",     1],
    ["unary_ops/invalid/missing_const.su",        ReturnCode_Parser_Failed],
    ["unary_ops/invalid/missing_semicolon.su",    ReturnCode_Parser_Failed],
    ["unary_ops/invalid/nested_missing_const.su", ReturnCode_Parser_Failed],
    ["unary_ops/invalid/wrong_order.su",          ReturnCode_Parser_Failed],
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
                    if(err.returncode == expected):
                        print("%-60s%s" % (file, "PASSED"));
                        tests_passed += 1;
                    else:
                        print("%-60s%s (E: %d; A: %d)" % (file, "FAILED", expected, err.returncode));
                        tests_failed += 1;
                os.remove(file_exe);
            except subprocess.CalledProcessError as err:
                print("%-60s%s (compile error: %d)" % (file, "FAILED", err.returncode));
                tests_failed += 1;
        elif(type == 'invalid'):
            tests_total += 1;
            try:
                subprocess.run(compile_cmd, capture_output=True, check=True);
                
                os.remove(file_s);
                print("%-60s%s (no compile error)" % (file, "FAILED"));
                tests_failed += 1;
            except subprocess.CalledProcessError as err:
                if(err.returncode == expected):
                    print("%-60s%s" % (file, "PASSED"));
                    tests_passed += 1;
                else:
                    print("%-60s%s (E: %d; A: %d)" % (file, "FAILED", expected, err.returncode));
                    tests_failed += 1;
    print("tests: %d; passed: %d; failed: %d" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
    main()