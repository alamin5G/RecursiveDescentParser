// parser.c
// Recursive Descent Parser for Control Structures and Nested Expressions
// Course: Programming Languages and Structures
// Code by: Md. Alamin
// Student ID: 21303134

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>  // jmp_buf, setjmp, and longjmp
#include <unistd.h>  // dup, dup2, and close functions

// --- Configuration ---

#define MAX_SYMBOLS 100
int g_student_ltd_value = 134; // Default value for LTD (Last Three Digits of Student ID)
// --- Global Variables for Lexer and Parser ---
static const char *g_source_code; // Start of the source code
static const char *g_source_ptr;  // Pointer to the current character

// =============1. Lexer (Scanner) Implementation============== start
// --- Token Definitions ---
typedef enum {
    TOKEN_EOF,
    TOKEN_ERROR,

    // Keywords
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,

    // Symbols
    TOKEN_LBRACE,    // {
    TOKEN_RBRACE,    // }
    TOKEN_LPAREN,    // (
    TOKEN_RPAREN,    // )
    TOKEN_SEMICOLON, // ;

    // Operators
    TOKEN_PLUS,      // +
    TOKEN_MINUS,     // -
    TOKEN_MULTIPLY,  // *
    TOKEN_DIVIDE,    // /

    // Relational Operators
    TOKEN_EQ,        // ==
    TOKEN_NEQ,       // !=
    TOKEN_LT,        // <
    TOKEN_GT,        // >
    TOKEN_LTE,       // <=
    TOKEN_GTE,       // >=

    // Literals and Identifiers
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_LTD        // Special identifier "LTD"
} TokenType;

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_LBRACE: return "LBRACE ('{')";
        case TOKEN_RBRACE: return "RBRACE ('}')";
        case TOKEN_LPAREN: return "LPAREN ('(')";
        case TOKEN_RPAREN: return "RPAREN (')')";
        case TOKEN_SEMICOLON: return "SEMICOLON (';')";
        case TOKEN_PLUS: return "PLUS ('+')";
        case TOKEN_MINUS: return "MINUS ('-')";
        case TOKEN_MULTIPLY: return "MULTIPLY ('*')";
        case TOKEN_DIVIDE: return "DIVIDE ('/')";
        case TOKEN_EQ: return "EQ ('==')";
        case TOKEN_NEQ: return "NEQ ('!=')";
        case TOKEN_LT: return "LT ('<')";
        case TOKEN_GT: return "GT ('>')";
        case TOKEN_LTE: return "LTE ('<=')";
        case TOKEN_GTE: return "GTE ('>=')";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_LTD: return "LTD";
        default: return "UNKNOWN_TOKEN";
    }
}

typedef struct {
    TokenType type;
    char value[100]; // Buffer for number value or identifier name
    int line;        // Line number where the token starts
    int col;         // Column number where the token starts
} Token;

typedef struct {
    char name[100];
    int value;
} Symbol;

Symbol g_symbol_table[MAX_SYMBOLS];
int g_symbol_count = 0;

// --- Global Variables for Lexer and Parser ---
static Token g_current_token;      // The current token being processed by the parser
static int g_current_line = 1;     // Current line number in the source
static int g_current_col = 1;      // Current column number in the source
static int g_start_col_for_token = 1; // Column where the current token began

// --- Forward Declarations for Parser Functions ---
static void program();
static void block();
static void statement();
static void if_statement();
static void while_statement();
static void condition();
static void relational_operator();
static void expression();
static void term();
static void factor();

// Forward declarations for evaluator functions
static int eval_expression();
static int eval_term();
static int eval_factor();
// Error handling forward declaration
static void error_at_current_token(const char* message);

// Pre-defined test cases
const char* test_cases[] = {
    // Valid test cases
    "{ if (a == LTD) { while (b < 100) { (a + b) * (b - LTD); } } else { (x + y) * (a - b); } }",
    "{ a + b; }",
    "{ if (x > 5) { y + 10; } }",
    "{ while (i <= 10) { sum + i; i + 1; } }",
    
    // Invalid test cases
    "{ a + b }", // Missing semicolon
    "{ if (a == b) { a + b; }", // Mismatched brackets
    "{ 3a + 5; }", // Invalid identifier
    "{ if (a > b) if (c < d) { x; } }", // Nested if without braces for outer if
    "{ else { x; } }" // else without if
};

// read input from console
char* read_from_console() {
    printf("Enter program (end with Ctrl+D on Unix/Linux or Ctrl+Z on Windows):\n");
    
    // Initial buffer size
    size_t bufsize = 1024;
    char* buffer = (char*)malloc(bufsize);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    size_t position = 0;
    int c;
    
    while ((c = getchar()) != EOF) {
        // Resize buffer if needed
        if (position >= bufsize - 1) {
            bufsize *= 2;
            char* new_buffer = (char*)realloc(buffer, bufsize);
            if (!new_buffer) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        
        buffer[position++] = (char)c;
    }
    
    // Null-terminate the string
    buffer[position] = '\0';
    return buffer;
}

// Function to look up or add a symbol
static int get_symbol_value(const char* name) {
    // Look for existing symbol
    for (int i = 0; i < g_symbol_count; i++) {
        if (strcmp(g_symbol_table[i].name, name) == 0) {
            return g_symbol_table[i].value;
        }
    }
    
    // Add new symbol with dummy value (0)
    if (g_symbol_count < MAX_SYMBOLS) {
        strcpy(g_symbol_table[g_symbol_count].name, name);
        g_symbol_table[g_symbol_count].value = 0; // Default value
        return g_symbol_table[g_symbol_count++].value;
    } else {
        error_at_current_token("Symbol table overflow");
        return 0;
    }
}


// =============3. Error Handling============== start

// --- Error Handling ---
static void error_at_current_token(const char* message) {
    fprintf(stderr, "Syntax Error on line %d, col %d: %s\n", g_current_token.line, g_current_token.col, message);
    fprintf(stderr, "Near token: '%s' (Type: %s)\n", g_current_token.value, token_type_to_string(g_current_token.type));
    
    // Find the beginning of the line
    const char* line_start = g_source_ptr;
    while (line_start > g_source_code && *(line_start-1) != '\n') {
        line_start--;
    }
    
    // Find the end of the line
    const char* line_end = g_source_ptr;
    while (*line_end != '\0' && *line_end != '\n') {
        line_end++;
    }
    
    // Print the line
    fprintf(stderr, "Line %d: ", g_current_token.line);
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);
    
    // Print a caret pointing to the error position
    fprintf(stderr, "%*s^\n", g_current_token.col - 1, "");
    
    exit(EXIT_FAILURE);
}

// --- Lexer Implementation ---
static Token make_token(TokenType type, const char* value_str, int line, int col) {
    Token token;
    token.type = type;
    if (value_str != NULL) {
        strncpy(token.value, value_str, sizeof(token.value) - 1);
        token.value[sizeof(token.value) - 1] = '\0';
    } else {
        token.value[0] = '\0';
    }
    token.line = line;
    token.col = col;
    return token;
}

static void skip_whitespace_and_comments() {
    while (*g_source_ptr != '\0') {
        if (isspace((unsigned char)*g_source_ptr)) {
            if (*g_source_ptr == '\n') {
                g_current_line++;
                g_current_col = 1;
            } else {
                g_current_col++;
            }
            g_source_ptr++;
        } 
        // Handle C-style comments
        else if (*g_source_ptr == '/' && *(g_source_ptr + 1) == '*') {
            g_source_ptr += 2; // Skip /*
            g_current_col += 2;
            
            while (!(*g_source_ptr == '*' && *(g_source_ptr + 1) == '/') && *g_source_ptr != '\0') {
                if (*g_source_ptr == '\n') {
                    g_current_line++;
                    g_current_col = 1;
                } else {
                    g_current_col++;
                }
                g_source_ptr++;
            }
            
            if (*g_source_ptr == '\0') {
                // Unclosed comment
                error_at_current_token("Unclosed comment detected");
            } else {
                g_source_ptr += 2; // Skip */
                g_current_col += 2;
            }
        }
        // Handle C++-style comments
        else if (*g_source_ptr == '/' && *(g_source_ptr + 1) == '/') {
            g_source_ptr += 2; // Skip //
            g_current_col += 2;
            
            while (*g_source_ptr != '\n' && *g_source_ptr != '\0') {
                g_source_ptr++;
                g_current_col++;
            }
        }
        else {
            break; // Not whitespace or comment
        }
    }
}

static Token lexer_get_next_token_internal() {
    skip_whitespace_and_comments();
    g_start_col_for_token = g_current_col; // Record column at start of token

    if (*g_source_ptr == '\0') {
        return make_token(TOKEN_EOF, "EOF", g_current_line, g_start_col_for_token);
    }

    char current_char = *g_source_ptr;
    char next_char = *(g_source_ptr + 1);

    // Multi-character operators (==, !=, <=, >=)
    if (current_char == '=' && next_char == '=') { g_source_ptr += 2; g_current_col += 2; return make_token(TOKEN_EQ, "==", g_current_line, g_start_col_for_token); }
    if (current_char == '!' && next_char == '=') { g_source_ptr += 2; g_current_col += 2; return make_token(TOKEN_NEQ, "!=", g_current_line, g_start_col_for_token); }
    if (current_char == '<' && next_char == '=') { g_source_ptr += 2; g_current_col += 2; return make_token(TOKEN_LTE, "<=", g_current_line, g_start_col_for_token); }
    if (current_char == '>' && next_char == '=') { g_source_ptr += 2; g_current_col += 2; return make_token(TOKEN_GTE, ">=", g_current_line, g_start_col_for_token); }

    // Single-character symbols and operators
    char single_char_val[2] = {current_char, '\0'};
    g_source_ptr++; g_current_col++;
    switch (current_char) {
        case '{': return make_token(TOKEN_LBRACE, single_char_val, g_current_line, g_start_col_for_token);
        case '}': return make_token(TOKEN_RBRACE, single_char_val, g_current_line, g_start_col_for_token);
        case '(': return make_token(TOKEN_LPAREN, single_char_val, g_current_line, g_start_col_for_token);
        case ')': return make_token(TOKEN_RPAREN, single_char_val, g_current_line, g_start_col_for_token);
        case ';': return make_token(TOKEN_SEMICOLON, single_char_val, g_current_line, g_start_col_for_token);
        case '+': return make_token(TOKEN_PLUS, single_char_val, g_current_line, g_start_col_for_token);
        case '-': return make_token(TOKEN_MINUS, single_char_val, g_current_line, g_start_col_for_token);
        case '*': return make_token(TOKEN_MULTIPLY, single_char_val, g_current_line, g_start_col_for_token);
        case '/': return make_token(TOKEN_DIVIDE, single_char_val, g_current_line, g_start_col_for_token);
        case '<': return make_token(TOKEN_LT, single_char_val, g_current_line, g_start_col_for_token);
        case '>': return make_token(TOKEN_GT, single_char_val, g_current_line, g_start_col_for_token);
    }
    // Backtrack if not a recognized single character
    g_source_ptr--; g_current_col--;

    // Numbers: <digit> { <digit> }
    if (isdigit((unsigned char)current_char)) {
        char num_buffer[100];
        int i = 0;
        num_buffer[i++] = current_char;
        g_source_ptr++; g_current_col++;
        while (*g_source_ptr != '\0' && isdigit((unsigned char)*g_source_ptr)) {
            if (i < sizeof(num_buffer) - 1) num_buffer[i++] = *g_source_ptr;
            g_source_ptr++; g_current_col++;
        }
        num_buffer[i] = '\0';
        return make_token(TOKEN_NUMBER, num_buffer, g_current_line, g_start_col_for_token);
    }

    // Identifiers and Keywords: <letter> { <letter> | <digit> } (allow underscore)
    if (isalpha((unsigned char)current_char) || current_char == '_') {
        char id_buffer[100];
        int i = 0;
        id_buffer[i++] = current_char;
        g_source_ptr++; g_current_col++;
        while (*g_source_ptr != '\0' && (isalnum((unsigned char)*g_source_ptr) || *g_source_ptr == '_')) {
            if (i < sizeof(id_buffer) - 1) id_buffer[i++] = *g_source_ptr;
            g_source_ptr++; g_current_col++;
        }
        id_buffer[i] = '\0';

        // Check for keywords
        if (strcmp(id_buffer, "if") == 0) return make_token(TOKEN_IF, id_buffer, g_current_line, g_start_col_for_token);
        if (strcmp(id_buffer, "else") == 0) return make_token(TOKEN_ELSE, id_buffer, g_current_line, g_start_col_for_token);
        if (strcmp(id_buffer, "while") == 0) return make_token(TOKEN_WHILE, id_buffer, g_current_line, g_start_col_for_token);
        if (strcmp(id_buffer, "LTD") == 0) return make_token(TOKEN_LTD, id_buffer, g_current_line, g_start_col_for_token);

        return make_token(TOKEN_IDENTIFIER, id_buffer, g_current_line, g_start_col_for_token);
    }

    // If no rule matches, it's an unrecognized character
    char error_char_val[2] = {current_char, '\0'};
    g_source_ptr++; g_current_col++; // Consume the erroneous character to avoid infinite loop
    Token err_token = make_token(TOKEN_ERROR, error_char_val, g_current_line, g_start_col_for_token);
    // Error will be reported by advance()
    return err_token;
}

// =============1. Lexer (Scanner) Implementation============== end

// --- Parser Helper Functions ---
// Consumes the current token and gets the next one from the lexer.
static void advance() {
    g_current_token = lexer_get_next_token_internal();
    if (g_current_token.type == TOKEN_ERROR) {
        char error_msg[150];
        sprintf(error_msg, "Lexical error: Unrecognized character '%s'", g_current_token.value);
        error_at_current_token(error_msg); // This will exit
    }
}

// Checks if the current token matches the expected type.
// If yes, consumes it (advances). If no, reports an error.
static void eat(TokenType expected_type, const char* error_message) {
    if (g_current_token.type == expected_type) {
        advance();
    } else {
        char full_error_message[256];
        sprintf(full_error_message, "%s. Expected %s, but got %s ('%s').",
                error_message,
                token_type_to_string(expected_type),
                token_type_to_string(g_current_token.type),
                g_current_token.value);
        error_at_current_token(full_error_message);
    }
}
// =============3. Error Handling============== end

// =============2. Recursive Descent Parser============== start
// --- Recursive Descent Parser Functions ---
// <program> -> <block>
static void program() {
    printf("Parsing <program>...\n");
    block();
    if (g_current_token.type != TOKEN_EOF) {
        error_at_current_token("Expected end of input (EOF) after program block, but found more tokens.");
    }
    printf("Finished parsing <program>.\n");
}

// <block> -> "{" { <statement> } "}"
static void block() {
    printf("Parsing <block>...\n");
    eat(TOKEN_LBRACE, "Expected '{' to start a block");
    while (g_current_token.type != TOKEN_RBRACE && g_current_token.type != TOKEN_EOF) {
        // Check if the current token can start a statement
        TokenType tt = g_current_token.type;
        if (tt == TOKEN_IF || tt == TOKEN_WHILE || // Keywords for statements
            tt == TOKEN_LPAREN ||                   // Start of ( <expression> ) ;
            tt == TOKEN_IDENTIFIER || tt == TOKEN_NUMBER || tt == TOKEN_LTD) { // Start of <expression> ;
            statement();
        } else {
            error_at_current_token("Invalid token inside block. Expected a statement or '}'.");
            break;
        }
    }
    eat(TOKEN_RBRACE, "Expected '}' to end a block");
    printf("Finished parsing <block>.\n");
}

// <statement> -> <if-statement> | <while-statement> | <expression> ";"
static void statement() {
    printf("Parsing <statement> (current token: %s)...\n", token_type_to_string(g_current_token.type));
    if (g_current_token.type == TOKEN_IF) {
        if_statement();
    } else if (g_current_token.type == TOKEN_WHILE) {
        while_statement();
    } else if (g_current_token.type == TOKEN_LPAREN ||
               g_current_token.type == TOKEN_IDENTIFIER ||
               g_current_token.type == TOKEN_NUMBER ||
               g_current_token.type == TOKEN_LTD) {
        expression();
        eat(TOKEN_SEMICOLON, "Expected ';' after expression statement");
    } else {
        error_at_current_token("Invalid start of a statement. Expected 'if', 'while', or an expression.");
    }
    printf("Finished parsing <statement>.\n");
}

// <if-statement> -> "if" "(" <condition> ")" <block> [ "else" <block> ]
static void if_statement() {
    printf("Parsing <if-statement>...\n");
    eat(TOKEN_IF, "Expected 'if' keyword");
    eat(TOKEN_LPAREN, "Expected '(' after 'if'");
    condition();
    eat(TOKEN_RPAREN, "Expected ')' after if-condition");
    block();
    if (g_current_token.type == TOKEN_ELSE) {
        eat(TOKEN_ELSE, "Expected 'else' keyword");
        block();
    }
    printf("Finished parsing <if-statement>.\n");
}

// <while-statement> -> "while" "(" <condition> ")" <block>
static void while_statement() {
    printf("Parsing <while-statement>...\n");
    eat(TOKEN_WHILE, "Expected 'while' keyword");
    eat(TOKEN_LPAREN, "Expected '(' after 'while'");
    condition();
    eat(TOKEN_RPAREN, "Expected ')' after while-condition");
    block();
    printf("Finished parsing <while-statement>.\n");
}

// <condition> -> <expression> <relational-operator> <expression>
static void condition() {
    printf("Parsing <condition>...\n");
    expression();
    relational_operator();
    expression();
    printf("Finished parsing <condition>.\n");
}

// <relational-operator> -> "==" | "!=" | "<" | ">" | "<=" | ">="
static void relational_operator() {
    printf("Parsing <relational-operator> (current token: %s)...\n", g_current_token.value);
    switch (g_current_token.type) {
        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LTE:
        case TOKEN_GTE:
            printf("Recognized relational operator: %s\n", g_current_token.value);
            advance(); // Consume the operator
            break;
        default:
            error_at_current_token("Expected a relational operator (e.g., ==, <, >=)");
    }
    printf("Finished parsing <relational-operator>.\n");
}

// <expression> -> <term> { ("+" | "-") <term> }
static void expression() {
    printf("Parsing <expression>...\n");
    term();
    while (g_current_token.type == TOKEN_PLUS || g_current_token.type == TOKEN_MINUS) {
        printf("Recognized operator in expression: %s\n", g_current_token.value);
        advance(); // Consume '+' or '-'
        term();
    }
    printf("Finished parsing <expression>.\n");
}

// <term> -> <factor> { ("*" | "/") <factor> }
static void term() {
    printf("Parsing <term>...\n");
    factor();
    while (g_current_token.type == TOKEN_MULTIPLY || g_current_token.type == TOKEN_DIVIDE) {
        printf("Recognized operator in term: %s\n", g_current_token.value);
        advance(); // Consume '*' or '/'
        factor();
    }
    printf("Finished parsing <term>.\n");
}

// <factor> -> <number> | <identifier> | "LTD" | "(" <expression> ")"
static void factor() {
    printf("Parsing <factor> (current token type: %s, value: '%s')...\n", token_type_to_string(g_current_token.type), g_current_token.value);
    if (g_current_token.type == TOKEN_NUMBER) {
        printf("Recognized number: %s\n", g_current_token.value);
        eat(TOKEN_NUMBER, "Error processing number in factor."); // eat already advances
    } else if (g_current_token.type == TOKEN_IDENTIFIER) {
        printf("Recognized identifier: %s\n", g_current_token.value);
        eat(TOKEN_IDENTIFIER, "Error processing identifier in factor.");
    } else if (g_current_token.type == TOKEN_LTD) {
        printf("Recognized LTD, substituting with value: %d\n", g_student_ltd_value);
        eat(TOKEN_LTD, "Error processing LTD in factor.");
    } else if (g_current_token.type == TOKEN_LPAREN) {
        eat(TOKEN_LPAREN, "Expected '(' for sub-expression in factor");
        expression();
        eat(TOKEN_RPAREN, "Expected ')' after sub-expression in factor");
    } else {
        char error_msg[200];
        sprintf(error_msg, "Invalid factor. Expected number, identifier, LTD, or '('. Got token type %s ('%s')",
                token_type_to_string(g_current_token.type), g_current_token.value);
        error_at_current_token(error_msg);
    }
    printf("Finished parsing <factor>.\n");
}

// =============2. Recursive Descent Parser============== end


// =============4. Expression Evaluation============== start

// Example implementation for expression evaluation
static int eval_term() {
    int result = eval_factor();
    
    while (g_current_token.type == TOKEN_MULTIPLY || g_current_token.type == TOKEN_DIVIDE) {
        TokenType op = g_current_token.type;
        advance(); // Consume '*' or '/'
        int factor_value = eval_factor();
        
        if (op == TOKEN_MULTIPLY) {
            result *= factor_value;
        } else { // DIVIDE
            if (factor_value == 0) {
                error_at_current_token("Division by zero");
            }
            result /= factor_value;
        }
    }
    
    return result;
}

static int eval_expression() {
    int result = eval_term();
    
    while (g_current_token.type == TOKEN_PLUS || g_current_token.type == TOKEN_MINUS) {
        TokenType op = g_current_token.type;
        advance(); // Consume '+' or '-'
        int term_value = eval_term();
        
        if (op == TOKEN_PLUS) {
            result += term_value;
        } else { // MINUS
            result -= term_value;
        }
    }
    
    return result;
}

static int eval_factor() {
    int result = 0;
    
    if (g_current_token.type == TOKEN_NUMBER) {
        result = atoi(g_current_token.value);
        eat(TOKEN_NUMBER, "Error processing number in factor evaluation");
    } else if (g_current_token.type == TOKEN_IDENTIFIER) {
        result = get_symbol_value(g_current_token.value);
        eat(TOKEN_IDENTIFIER, "Error processing identifier in factor evaluation");
    } else if (g_current_token.type == TOKEN_LTD) {
        result = g_student_ltd_value;
        eat(TOKEN_LTD, "Error processing LTD in factor evaluation");
    } else if (g_current_token.type == TOKEN_LPAREN) {
        eat(TOKEN_LPAREN, "Expected '(' for sub-expression in factor evaluation");
        result = eval_expression();
        eat(TOKEN_RPAREN, "Expected ')' after sub-expression in factor evaluation");
    } else {
        error_at_current_token("Invalid factor in evaluation");
    }
    
    return result;
}

// =============4. Expression Evaluation============== end

// --- Initialization and Main Driver ---
static void initialize_parser(const char* source_code) {
    g_source_code = source_code;
    g_source_ptr = source_code;
    g_current_line = 1;
    g_current_col = 1;
    g_start_col_for_token = 1;
    g_symbol_count = 0;
    
    // Load the first token to prime the parser
    advance();
}

// Function to read entire file into a string
char* read_file_to_string(const char* filename) {
    FILE *file = fopen(filename, "rb"); // Open in binary mode to correctly get length
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for file buffer\n");
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, length, file) != (size_t)length) {
        fprintf(stderr, "Error reading file\n");
        fclose(file);
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

// =============5. Test Case Suite============== start

// Process a single test case
static void process_test_case(const char* test_input, int test_number, int is_valid_expected) {
    printf("\n\n------------------------------------------\n");
    printf("TEST CASE %d: %s\n", test_number, is_valid_expected ? "VALID" : "INVALID");
    printf("------------------------------------------\n");
    printf("Input: %s\n\n", test_input);
    
    // Reinitialize parser state
    initialize_parser(test_input);
    
    int success = 1;
    
    // Skip the regular program call if we expect the test to fail
    if (is_valid_expected) {
        // Try to parse, but catch errors
        int old_stderr = dup(STDERR_FILENO);
        freopen("/dev/null", "w", stderr); // Redirect stderr to prevent error messages printing
        
        // Use a setjmp/longjmp to simulate try/catch
        jmp_buf env;
        int error_code = setjmp(env);
        
        if (!error_code) {
            program(); // Start parsing
            printf("✓ Program parsed successfully!\n");
        } else {
            success = 0;
            printf("✗ Parsing failed unexpectedly!\n");
        }
        
        // Restore stderr
        fflush(stderr);
        dup2(old_stderr, STDERR_FILENO);
        close(old_stderr);
    } else {
        // For invalid cases, we expect an error
        int old_stderr = dup(STDERR_FILENO);
        freopen("/dev/null", "w", stderr); // Redirect stderr
        
        // Use a setjmp/longjmp to simulate try/catch
        jmp_buf env;
        if (setjmp(env) == 0) {
            program(); // Start parsing
            printf("✗ Expected parsing to fail, but it succeeded!\n");
            success = 0;
        } else {
            printf("✓ Parsing failed as expected for invalid input.\n");
        }
        
        // Restore stderr
        fflush(stderr);
        dup2(old_stderr, STDERR_FILENO);
        close(old_stderr);
    }
    
    printf("\nTest result: %s\n", success ? "PASS" : "FAIL");
}

// =============5. Test Case Suite============== start


// display menu for choosing test method
void display_interactive_menu() {
    printf("\n=== Recursive Descent Parser - Interactive Menu ===\n");
    printf("1. Enter code via console input\n");
    printf("2. Read code from a file\n");
    printf("3. Run test suite\n");
    printf("4. Use default test case\n");
    printf("5. Change LTD value (currently: %d)\n", g_student_ltd_value);
    printf("6. Exit\n");
    printf("Enter your choice (1-6): ");
}

// Main function
int main(int argc, char *argv[]) {
    const char* input_source = NULL;
    char* file_content = NULL;
    int run_test_suite = 0;
    int use_console_input = 0;
    int interactive_mode = argc == 1; // If no arguments are provided, go to interactive mode
    char filename[256];

    printf("Recursive Descent Parser\n");
    printf("Default LTD value: %d\n", g_student_ltd_value);
    
    if (!interactive_mode) {
        printf("Usage: %s [-ltd NUM] [-test] [-console] [-interactive] [filename]\n", argv[0]);
        printf("  -ltd NUM     : Set custom Last Three Digits value\n");
        printf("  -test        : Run the test suite\n");
        printf("  -console     : Read input from console\n");
        printf("  -interactive : Show interactive menu\n");
        printf("  filename     : Read input from specified file\n\n");
    }

    // Parse command line arguments if not in interactive mode
    int arg_offset = 1;
    while (!interactive_mode && arg_offset < argc) {
        if (strcmp(argv[arg_offset], "-ltd") == 0 && arg_offset + 1 < argc) {
            g_student_ltd_value = atoi(argv[arg_offset + 1]);
            printf("Using custom LTD value from command line: %d\n", g_student_ltd_value);
            arg_offset += 2;
        } else if (strcmp(argv[arg_offset], "-test") == 0) {
            run_test_suite = 1;
            arg_offset++;
        } else if (strcmp(argv[arg_offset], "-console") == 0) {
            use_console_input = 1;
            arg_offset++;
        } else if (strcmp(argv[arg_offset], "-interactive") == 0) {
            interactive_mode = 1;
            arg_offset++;
        } else {
            break;
        }
    }

    // Interactive menu handling
    if (interactive_mode) {
        int choice;
        do {
            display_interactive_menu();
            if (scanf("%d", &choice) != 1) {
                // Clear input buffer if scanf fails
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                choice = 0;  // Invalid choice
            }
            
            // Clear any remaining characters in input buffer
            while (getchar() != '\n');

            switch (choice) {
                case 1: // Console input
                    printf("Enter your code (end with Ctrl+D on Unix/Linux or Ctrl+Z+Enter on Windows):\n");
                    file_content = read_from_console();
                    if (file_content) {
                        input_source = file_content;
                        
                        printf("\nParsing the following input:\n---\n%s\n---\n\n", input_source);
                        initialize_parser(input_source);
                        
                        // Try to parse with error handling
                        jmp_buf env;
                        if (setjmp(env) == 0) {
                            program();
                            printf("\n------------------------------------\n");
                            printf("Program parsed successfully!\n");
                            printf("------------------------------------\n");
                        } else {
                            printf("\n------------------------------------\n");
                            printf("Parsing failed!\n");
                            printf("------------------------------------\n");
                        }
                        
                        free(file_content);
                        file_content = NULL;
                    }
                    break;
                    
                case 2: // File input
                    printf("Enter filename: ");
                    if (scanf("%255s", filename) == 1) {
                        file_content = read_file_to_string(filename);
                        if (file_content) {
                            input_source = file_content;
                            
                            printf("\nParsing file: %s\n", filename);
                            printf("---\n%s\n---\n\n", input_source);
                            initialize_parser(input_source);
                            
                            // Try to parse with error handling
                            jmp_buf env;
                            if (setjmp(env) == 0) {
                                program();
                                printf("\n------------------------------------\n");
                                printf("Program parsed successfully!\n");
                                printf("------------------------------------\n");
                            } else {
                                printf("\n------------------------------------\n");
                                printf("Parsing failed!\n");
                                printf("------------------------------------\n");
                            }
                            
                            free(file_content);
                            file_content = NULL;
                        } else {
                            printf("Error: Could not read file '%s'\n", filename);
                        }
                    }
                    break;
                    
                case 3: // Test suite
                    {
                        printf("Running test suite...\n");
                        
                        const int valid_test_count = 4;  // First 4 test cases are valid
                        const int total_test_count = sizeof(test_cases) / sizeof(test_cases[0]);
                        
                        for (int i = 0; i < total_test_count; i++) {
                            process_test_case(test_cases[i], i + 1, i < valid_test_count);
                        }
                        
                        printf("\nTest suite completed.\n");
                    }
                    break;
                    
                case 4: // Default test case
                    input_source = test_cases[0];
                    
                    printf("\nParsing default test case:\n---\n%s\n---\n\n", input_source);
                    initialize_parser(input_source);
                    
                    // Try to parse with error handling
                    {
                        jmp_buf env;
                        if (setjmp(env) == 0) {
                            program();
                            printf("\n------------------------------------\n");
                            printf("Program parsed successfully!\n");
                            printf("------------------------------------\n");
                        } else {
                            printf("\n------------------------------------\n");
                            printf("Parsing failed!\n");
                            printf("------------------------------------\n");
                        }
                    }
                    break;
                    
                case 5: // Change LTD value
                    printf("Current LTD value: %d\n", g_student_ltd_value);
                    printf("Enter new LTD value: ");
                    int new_ltd;
                    if (scanf("%d", &new_ltd) == 1) {
                        g_student_ltd_value = new_ltd;
                        printf("LTD value updated to: %d\n", g_student_ltd_value);
                    } else {
                        printf("Invalid input. LTD value unchanged.\n");
                        // Clear input buffer
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                    }
                    break;
                    
                case 6: // Exit
                    printf("Exiting program. Goodbye!\n");
                    break;
                    
                default:
                    printf("Invalid choice. Please enter a number between 1 and 6.\n");
            }
            
            if (choice != 6) {
                printf("\nPress Enter to continue...");
                getchar();  // Wait for user to press enter
            }
            
        } while (choice != 6);
        
        return 0;
    }

    
    // Process based on provided flags
    if (run_test_suite) {
        printf("Running test suite...\n");
        
        // Count of valid test cases (the first 4)
        const int valid_test_count = 4;
        const int total_test_count = sizeof(test_cases) / sizeof(test_cases[0]);
        
        for (int i = 0; i < total_test_count; i++) {
            process_test_case(test_cases[i], i + 1, i < valid_test_count);
        }
        
        printf("\nTest suite completed.\n");
        return 0;
    }

    // Get input source (priority: console > file > default test case)
    if (use_console_input) {
        printf("Reading from console input...\n");
        file_content = read_from_console();
        if (!file_content) {
            return 1; // Error reading from console
        }
        input_source = file_content;
    }
    else if (argc > arg_offset) { // A filename is provided
        printf("Attempting to read input from file: %s\n", argv[arg_offset]);
        file_content = read_file_to_string(argv[arg_offset]);
        if (!file_content) {
            return 1; // Error reading file
        }
        input_source = file_content;
    } 
    else {
        // Default test case if no file is provided
        printf("No input file provided. Using a default valid test case.\n");
        input_source = test_cases[0]; // Use first test case as default
    }

    printf("\nParsing the following input:\n---\n%s\n---\n\n", input_source);

    initialize_parser(input_source);
    program(); // Start parsing

    printf("\n------------------------------------\n");
    printf("Program parsed successfully!\n");
    printf("------------------------------------\n");

    if (file_content) {
        free(file_content); // Clean up if content was read from file
    }

    return 0; // Success
}