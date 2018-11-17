# search-engine
Small C++ search engine with basic indexer and query capabilities

TO BUILD:
* On Linux: 
  1. Install gcc/build tools. For example, on Ubuntu this can be done by running "sudo apt-get install build-essential"
  1. Unzip hw2_src.zip package.
  2. Navigate to unzipped folder in the terminal and run "make all" command. 

* On Windows: 
  1. Install GCC compiler for windows from http://mingw.org/
  2. Unzip hw2_src.zip package.
  3. Navigate to unzipped folder in the DOS command window and run "mingw32-make.exe all" command. 

TO RUN:
  The program can be run in different modes:
  1. ./search-engine          // interactive mode (allows user to execute from a set of predefined queries or custom query)
  2. ./search-engine -index  // will print the positional index to the screen
  3. ./search-engine -squad-train-data [train file] -squad-dev-data [dev file] // will use Squad data files (see https://rajpurkar.github.io/SQuAD-explorer/) for building the index