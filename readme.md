# Recursive Descent Parser for Control Structures and Nested Expressions

## Student Information

- Name: Md. Alamin
- Student ID: 134
- Course: Programming Languages and Structures (CSC 461)
- Submission Date: May 28, 2025

## Project Description

This project implements a Recursive Descent Parser in C that parses and validates simple control structure blocks (if-else statements and while loops) and arithmetic expressions with nested brackets. The parser handles a special identifier, LTD, which represents the Last Three Digits of a student's ID, integrating with University automation systems.

## Grammar Implementation

The parser implements the following EBNF grammar:

```
<program> → <block>
<block> → "{" { <statement> } "}"
<statement> → <if-statement> | <while-statement> | <expression> ";"
<if-statement> → "if" "(" <condition> ")" <block> [ "else" <block> ]
<while-statement> → "while" "(" <condition> ")" <block>
<condition> → <expression> <relational-operator> <expression>
<relational-operator> → "==" | "!=" | "<" | ">" | "<=" | ">="
<expression> → <term> { ("+" | "-") <term> }
<term> → <factor> { ("*" | "/") <factor> }
<factor> → <number> | <identifier> | "LTD" | "(" <expression> ")"
<number> → <digit> { <digit> }
<identifier> → <letter> { <letter> | <digit> }
<digit> → "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
<letter> → "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
```

## Compilation Instructions

### Requirements

- C compiler (GCC recommended)
- Standard C libraries: stdio.h, stdlib.h, string.h, ctype.h, setjmp.h, unistd.h

### How to Compile

```bash
gcc -o parser main.c
```

## Runtime Instructions

### Basic Usage

```bash
./parser                # Run with default test case
./parser -interactive   # Show interactive menu
./parser -ltd 999       # Set custom LTD value to 134
./parser input.txt      # Parse code from input.txt
./parser -test          # Run all test cases
```

### Interactive Menu Options

1. Enter code via console input
2. Read code from a file
3. Run test suite
4. Use default test case
5. Change LTD value (default: 134)
6. Exit

### Command Line Arguments

- `-ltd NUM`: Set the Last Three Digits value
- `-test`: Run the test suite
- `-console`: Read input directly from console
- `-interactive`: Show interactive menu
- `filename`: Parse input from specified file

## Test Case Explanations

### Valid Test Cases

1. **Complex Nested Structure**:

   ```
   { if (a == LTD) { while (b < 100) { (a + b) * (b - LTD); } } else { (x + y) * (a - b); } }
   ```

   This test case demonstrates:

   - Nested if-else structure
   - While loop within if block
   - Complex expressions with parentheses
   - LTD token usage
   - Operator precedence
2. **Simple Expression**:

   ```
   { a + b; }
   ```

   Tests basic expression parsing and semicolon requirement.
3. **Simple If Statement**:

   ```
   { if (x > 5) { y + 10; } }
   ```

   Tests if statement with condition and block.
4. **While Loop**:

   ```
   { while (i <= 10) { sum + i; i + 1; } }
   ```

   Tests while loop with multiple statements in block.

### Invalid Test Cases

1. **Missing Semicolon**:

   ```
   { a + b }
   ```

   Error: Expression statements must end with semicolon.
2. **Mismatched Brackets**:

   ```
   { if (a == b) { a + b; }
   ```

   Error: Missing closing bracket for the program block.
3. **Invalid Identifier**:

   ```
   { 3a + 5; }
   ```

   Error: Identifiers cannot start with a digit.
4. **Nested If Without Proper Structure**:

   ```
   { if (a > b) if (c < d) { x; } }
   ```

   This code has potential ambiguity without proper braces.
5. **Else Without If**:

   ```
   { else { x; } }
   ```

   Error: 'else' keyword must follow an 'if' statement.

## Implementation Details

### Major Components

1. **Lexical Analysis (Lexer)**

   - Tokenizes input into keywords, identifiers, operators, etc.
   - Handles special LTD token recognition
   - Tracks line and column numbers for error reporting
2. **Recursive Descent Parser**

   - Implements one parsing function for each non-terminal in the grammar
   - Maintains recursive call integrity for nested structures
   - Validates syntactical correctness of the input
3. **Error Handling**

   - Detects mismatched brackets, unexpected tokens, and missing semicolons
   - Reports errors with line and column information
   - Provides context for the error with source code display
4. **Expression Evaluation**

   - Substitutes LTD with the student's ID digits
   - Uses symbolic representation for variables
   - Handles operator precedence through grammar structure
5. **Test Suite**

   - Includes both valid and invalid test cases
   - Tests nested structures and complex expressions
   - Verifies error detection capabilities

### Usage Example

```
$ ./parser -interactive

=== Recursive Descent Parser - Interactive Menu ===
1. Enter code via console input
2. Read code from a file
3. Run test suite
4. Use default test case
5. Change LTD value (currently: 134)
6. Exit
Enter your choice (1-6): 1

Enter your code (end with Ctrl+D on Unix/Linux or Ctrl+Z+Enter on Windows):
{ if (x > LTD) { x - 10; } else { x + 5; } }

Parsing the following input:
---
{ if (x > LTD) { x - 10; } else { x + 5; } }
---

Parsing <program>...
[Parsing details...]
Finished parsing <program>.

------------------------------------
Program parsed successfully!
------------------------------------

Press Enter to continue...
```
--------------------------
This implementation follows a modular design with clearly separated components for lexing, parsing, error handling, and evaluation. The parser strictly adheres to the grammar specification while providing robust error detection. The interactive interface makes testing and demonstration easier.
