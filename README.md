# NuParsePro
This project implements a scanner for nuPython, a subset of the Python language that uses braces for grouping and introduces pointer-based operators. The scanner transforms nuPython source code into tokens for further processing by a compiler or interpreter.

# NuPython Scanner Project

## Overview
The nuPython Scanner is a specialized lexical analyzer for nuPython, a Python-inspired language with unique syntax features such as braces for grouping and pointer operators. This tool is designed to tokenize nuPython source code, preparing it for parsing and compilation.

## Features
- Lexical analysis for nuPython source code
- Token generation for language constructs
- Error handling for syntactical issues
- Identifies and categorizes nuPython specific tokens.
- Handles lexical constructs including identifiers, keywords, literals, and operators.
- Robust error handling for unrecognized tokens.

## Getting Started
### Prerequisites
- Ensure you have a C compiler installed on your system.

### Building the Scanner
To build the scanner, use the following command:
```bash
gcc -o nupython_scanner scanner.c

Usage
Run the scanner with a nuPython source file:

bash
Copy code
./nupython_scanner source_file.nup
Contributions
Contributions to the nuPython scanner are welcome. 
