amu's test suite involves a collection of amu source files and a single binary results file. When a test is written, and the compiler is in a state in which it is expected to output correct data, the testing script invokes the compiler on that test, the compiler dumps the appropriate stage data, and the script saves that data in the binary results file.

I'm gonna leave some ideas for the layout of this file here. When referring to the binary results file, I will use the acronym BRF.

The full path to each test file will be hashed and stored in a sorted map associating the hash to a location in the BRF. 