# run_tests.py
# expected to be run from root folder su/ with the su.exe in the same folder

#NOT CURRENTLY FUNCTIONAL

import os,subprocess

valid_tests = [
    ["entry_point\\multi_digit.su", 100],
    ["entry_point\\newlines.su",     12],
    ["entry_point\\no_newlines.su",   6],
    ["entry_point\\return_0.su",      0],
    ["entry_point\\return_2.su",      2],
    ["entry_point\\spaces.su",        5],
    ["entry_point\\two_funcs.su",   100],
];

invalid_tests = [
    "entry_point\\missing_paren.su",
    "entry_point\\missing_retval.su",
    "entry_point\\no_brace.su",
    "entry_point\\no_semicolon.su",
    "entry_point\\no_space.su",
    "entry_point\\wrong_case.su",
];

def main():
    tests_total  = 0;
    tests_passed = 0;
    tests_failed = 0;

    #invalid tests should return errorlevel 0
    for name,expected in valid_tests:
        tests_total += 1;
        try:
            file = name[:-3];
            file_s = file+".s";
            file_su = file+".su";
            file_exe = file+".exe";
            subprocess.run("su.exe "+file_su, capture_output=True, check=True);
            subprocess.run("gcc "+"-m64 "+file_s+" -o "+file_exe);
            os.remove(file_s);
            try:
                subprocess.run(file_exe, capture_output=True, check=True);
                print("%-80s%s" % (file, "PASSED"));
            except subprocess.CalledProcessError as err:
                if(err.returncode == expected):
                    print("%-80s%s" % (file, "PASSED"));
                    tests_passed += 1;
                else:
                    print("%-80s%s" % (file, "FAILED"));
                    tests_failed += 1;
            os.remove(file_exe);
        except subprocess.CalledProcessError as err:
            print("%-80s%s" % (file, "FAILED"));
            print(err.stdout);
            
    #invalid tests should return errorlevel 3
    for name in invalid_tests:
        tests_total += 1;
        try:
            file = ("tests\\invalid\\"+name)[:-3];
            file_su = file+".su";
            subprocess.run("su.exe "+file_su, capture_output=True, check=True);
            os.remove(file+".s");
            print("%-80s%s" % (file, "FAILED"));
            tests_failed += 1;
        except subprocess.CalledProcessError as err:
            if(err.returncode == 3):
                print("%-80s%s" % (file, "PASSED"));
                tests_passed += 1;
            else:
                print(err.output.decode("utf-8"));
                print("%-80s%s" % (file, "FAILED"));
                tests_failed += 1;
            
    print("tests: %d; passed: %d; failed: %d" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
    main()