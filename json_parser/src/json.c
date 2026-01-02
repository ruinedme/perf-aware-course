#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "table.h"

typedef enum
{
    // Single character tokens
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE, // Start/End of a object
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET, // Start/End of an array
    TOKEN_COMMA,
    TOKEN_COLON, // object/array value separator, object key separator
                 // Literals
    TOKEN_STRING,
    TOKEN_NUMBER,  // floating point number
    TOKEN_INTEGER, // signed integer values
                   // Keywords
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    // Other
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;
const char *token_table = "{}[],:SNITF0E$";
typedef struct
{
    TokenType type;
    const char *start;
    size_t length;
    size_t line;
} Token;

typedef struct
{
    const char *start;
    const char *current;
    size_t line;
} Scanner;

Scanner scanner;
void initScanner(const char *buff)
{
    scanner.start = buff;
    scanner.current = buff;
    scanner.line = 1;
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static bool match(char expected)
{
    if (isAtEnd())
        return false;
    if (*scanner.current != expected)
        return false;
    scanner.current++;
    return true;
}

static void skipWhiteSpace()
{
    for (;;)
    {
        char c = peek();
        if (c == '\n')
            scanner.line++;
        if (isspace(c))
        {
            advance();
        }
        else
        {
            return;
        }
    }
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = scanner.current - scanner.start;
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner.line;
    return token;
}

static Token string()
{

    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
    {
        fprintf(stderr, "Unterminated string on line %d\n", scanner.line);
        exit(2);
    }

    advance(); // The closing quote
    return makeToken(TOKEN_STRING);
}

static Token number()
{
    while (isdigit(peek()))
        advance();

    // Have floating point number
    if (peek() == '.' && isdigit(peekNext()))
    {
        advance(); // consume the '.'
        while (isdigit(peek()))
            advance();

        // got exponent
        if (peek() == 'e' || peek() == 'E')
        {
            advance();
            if (match('+') || match('-'))
            {
                advance();
                while (isdigit(peek()))
                    advance();
            }
            else
            {
                return errorToken("Expected + or - following exponent.");
            }

        }
        return makeToken(TOKEN_NUMBER);
    }

    // Have an integer
    return makeToken(TOKEN_INTEGER);
}

static TokenType checkKeyword(int start, int length,
                              const char *rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_ERROR;
}

static TokenType identifierType()
{
    switch (scanner.start[0])
    {
    case 't':
        return checkKeyword(1, 3, "rue", TOKEN_TRUE);
    case 'f':
        return checkKeyword(1, 4, "alse", TOKEN_FALSE);
    case 'n':
        return checkKeyword(1, 3, "ull", TOKEN_NULL);
    }

    return TOKEN_ERROR;
}

static Token identifier()
{
    while (isalpha(peek()))
        advance();
    TokenType type = identifierType();
    if (type == TOKEN_ERROR)
    {
        return errorToken("Unexpected value");
    }

    return makeToken(type);
}

Token scanToken()
{
    skipWhiteSpace();
    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();
    if (isdigit(c) || c == '-')
        return number();

    if (isalpha(c))
        return identifier();

    switch (c)
    {
    case '{':
        return makeToken(TOKEN_LEFT_BRACE); // start object
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE); // end object
    case '[':
        return makeToken(TOKEN_LEFT_BRACKET); // start array
    case ']':
        return makeToken(TOKEN_RIGHT_BRACKET); // end array
    case ':':
        return makeToken(TOKEN_COLON); // object member separator
    case ',':
        return makeToken(TOKEN_COMMA); // value separator
    case '"':
        return string();
    }

    return errorToken("Unexpected character.");
}

static Table *scanObject();
static Array *scanArray();

static Value scanValue()
{
    Token token = scanToken();
    // printf("Token: %c\n", token_table[token.type]);
    Value value;

    switch (token.type)
    {
    // a nested table
    case TOKEN_LEFT_BRACE:
    {
        Table *t = scanObject();
        value = TABLE_VAL(t);
        break;
    }
    // an array
    case TOKEN_LEFT_BRACKET:
    {
        Array *array = scanArray();
        value = ARRAY_VAL(array);
        break;
    }
    case TOKEN_STRING:
    {
        // TODO: create copy string function
        size_t length = token.length - 2;
        char str[token.length];
        strncpy(str, token.start + 1, length);
        str[length] = '\0';
        value = STRING_VAL(str);
        break;
    }
    case TOKEN_NUMBER:
    {
        value = NUMBER_VAL(atof(token.start));
        break;
    }
    case TOKEN_INTEGER:
    {
        value = INT_VAL(atoll(token.start));
        break;
    }
    case TOKEN_TRUE:
    {
        value = BOOL_VAL(true);
        break;
    }
    case TOKEN_FALSE:
    {
        value = BOOL_VAL(false);
        break;
    }
    case TOKEN_NULL:
    {
        value = NULL_VAL;
        break;
    }
    default:
        fprintf(stderr, "Unexpected member value.\n");
        exit(1);
    }

    return value;
}

static Array *scanArray()
{
    Array *array = initArray();
    // Token token = scanToken();

    // Value value;
    // while
    Token token = makeToken(TOKEN_LEFT_BRACKET);

    while (token.type != TOKEN_RIGHT_BRACKET)
    {
        if (isAtEnd())
        {
            fprintf(stderr, "Missing closing ']'.\n");
            exit(1);
        }

        Value value = scanValue();
        pushArray(array, value);
        token = scanToken();
    }

    return array;
}


static Table *scanObject()
{
    Table *table = initTable();
    Token token = scanToken(); // next token shuold either be a } or a string

    while (token.type != TOKEN_RIGHT_BRACE)
    {

        if (token.type == TOKEN_EOF)
        {
            fprintf(stderr, "Missing closing '}'.\n");
            return NULL;
        }

        if (token.type != TOKEN_STRING)
        {
            fprintf(stderr, "Expected string.\n");
            return NULL;
        }

        size_t length = token.length - 1; // token length includes surrounding quotes
        char key[length];
        // memcpy(key, token.start+1, length);
        strncpy(key, token.start + 1, length);
        key[length - 1] = '\0';

        // next token is a :
        token = scanToken();
        if (token.type != TOKEN_COLON)
        {
            fprintf(stderr, "Expected ':'.\n");
            return NULL;
        }

        // next token is a valid value token type
        // printf("Key: %s\n", key);
        Value value = scanValue();
        tableSet(table, key, value);

        // next token should be a comma or right brace
        token = scanToken();
        if (token.type == TOKEN_COMMA)
            token = scanToken();
    }

    return table;
}

Table *parseJSON(char *buff)
{
    printf("Calling initScanner\n");
    initScanner(buff);
    Token token = scanToken();

    switch (token.type)
    {
    case TOKEN_LEFT_BRACE:
        printf("Found Object\n");
        return scanObject();
    case TOKEN_LEFT_BRACKET:
    {
        printf("callin scanArray()\n");
        Array *array = scanArray();
        // probably a better way to handle this
        Table *table = initTable();
        tableSet(table, "0", ARRAY_VAL(array));
        return table;
        break;
    }
    default:
        fprintf(stderr, "Unexpected Token on line %d.\n", token.line);
        return NULL;
    }
}
