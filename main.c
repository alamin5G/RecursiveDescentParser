// parser.c
// Recursive Descent Parser for Control Structures and Nested Expressions
// Course: Programming Languages and Structures

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- Configuration ---
// Default Last Three Digits of Student ID. Can be changed.
int g_student_ltd_value = 123;

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

// --- Global Variables for Lexer and Parser ---
static const char *g_source_ptr;   // Pointer to the current character in the source code
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

// --- Error Handling ---
static void error_at_current_token(const char* message) {
    fprintf(stderr, "Syntax Error on line %d, col %d: %s\n", g_current_token.line, g_current_token.col, message);
    fprintf(stderr, "Near token: '%s' (Type: %s)\n", g_current_token.value, token_type_to_string(g_current_token.type));
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
        } else {
            break; // Not whitespace
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

// --- Initialization and Main Driver ---
static void initialize_parser(const char* source_code) {
    g_source_ptr = source_code;
    g_current_line = 1;
    g_current_col = 1;
    g_start_col_for_token = 1;
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


int main(int argc, char *argv[]) {
    const char* input_source = NULL;
    char* file_content = NULL; // For content read from file

    printf("Recursive Descent Parser\n");
    printf("Default LTD value: %d\n", g_student_ltd_value);

    // Optional: Allow setting LTD from command line, e.g., ./parser -ltd 999 source.txt
    // Or ./parser source.txt (uses default LTD)
    // Or ./parser -ltd 999 (uses hardcoded test string with new LTD)
    // Or ./parser (uses hardcoded test string with default LTD)

    int arg_offset = 1;
    if (argc > 2 && strcmp(argv[1], "-ltd") == 0) {
        g_student_ltd_value = atoi(argv[2]);
        printf("Using custom LTD value from command line: %d\n", g_student_ltd_value);
        arg_offset = 3; // Next argument is potentially the filename
    }


    if (argc > arg_offset) { // A filename is provided
        printf("Attempting to read input from file: %s\n", argv[arg_offset]);
        file_content = read_file_to_string(argv[arg_offset]);
        if (!file_content) {
            return 1; // Error reading file
        }
        input_source = file_content;
    } else {
        // Default test case if no file is provided
        printf("No input file provided. Using a default valid test case.\n");
        input_source = "{ if (a == LTD) { while (b < 100) { (a + b) * (b - LTD); } } else { (x + y) * (a - b); } }";
        // Example of an invalid case to test:
        // input_source = "{ a + b }"; // Missing semicolon
        // input_source = "{ if (a == b) { a + b; }"; // Mismatched brackets (missing final '}')
        // input_source = "{ 3a + 5; }"; // Invalid identifier start -> Lexer will make '3' then 'a', parser should catch 'a' as unexpected
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
