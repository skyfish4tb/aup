#include <stdio.h>
#include <string.h>
#include "lexer.hpp"

using namespace aup;

Lexer::Lexer(const char *source)
{
    start = source;
    current = source;
    currentLine = source;

    line = 1;
    position = 1;
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c == '_')
        || (c == '$');
}

static bool isDigit(char c)
{
    return (c >= '0')
        && (c <= '9');
}

static bool isAtEnd(Lexer& lexer)
{
    return *lexer.current == '\0';
}

static char advance(Lexer& lexer)
{
    lexer.current++;
    lexer.position++;
    return lexer.current[-1];
}

static char peek(Lexer& lexer)
{
    return *lexer.current;
}

static char peekNext(Lexer& lexer)
{
    if (isAtEnd(lexer)) return '\0';
    return lexer.current[1];
}

static void newLine(Lexer& lexer)
{
    lexer.line++;
    lexer.position = 0;
    lexer.currentLine = lexer.current + 1;
}

static bool match(Lexer& lexer, char expected)
{
    if (isAtEnd(lexer)) return false;
    if (*lexer.current != expected) return false;

    lexer.current++;
    lexer.position++;
    return true;
}

static Token makeToken(Lexer& lexer, TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    token.column = lexer.position - token.length;
    token.currentLine = lexer.currentLine;

    return token;
}

static Token errorToken(Lexer& lexer, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer.line;
    token.column = lexer.position - 1;

    return token;
}

static void skipWhitespace(Lexer& lexer)
{
    for (;;) {
        char c = peek(lexer);
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance(lexer);
            break;

        case '\n':
            newLine(lexer);
            advance(lexer);
            break;

        case '/':
            if (peekNext(lexer) == '/') {
                // A comment goes until the end of the line.   
                while (peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
            }
            else {
                return;
            }
            break;

        default:
            return;
        }
    }
}

static TokenType checkKeyword(Lexer& lexer, int start, int length, const char *rest, TokenType type)
{
    if (lexer.current - lexer.start == start + length &&
        memcmp(lexer.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Lexer& lexer)
{
    switch (lexer.start[0]) {
        case 'a': return checkKeyword(lexer, 1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(lexer, 1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(lexer, 1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'a': return checkKeyword(lexer, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(lexer, 2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(lexer, 2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(lexer, 1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(lexer, 1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(lexer, 1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(lexer, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(lexer, 1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'h': return checkKeyword(lexer, 2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(lexer, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(lexer, 1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer& lexer)
{
    while (isAlpha(peek(lexer)) || isDigit(peek(lexer))) advance(lexer);

    return makeToken(lexer, identifierType(lexer));
}

static Token number(Lexer& lexer)
{
    while (isDigit(peek(lexer))) advance(lexer);

    // Look for a fractional part.             
    if (peek(lexer) == '.' && isDigit(peekNext(lexer))) {
        // Consume the ".".                      
        advance(lexer);

        while (isDigit(peek(lexer))) advance(lexer);
    }

    return makeToken(lexer, TOKEN_NUMBER);
}

static Token string(Lexer& lexer, char start)
{
    while (peek(lexer) != start && !isAtEnd(lexer)) {
        if (peek(lexer) == '\n') newLine(lexer);
        advance(lexer);
    }

    if (isAtEnd(lexer)) return errorToken(lexer, "Unterminated string.");

    // The closing quote.                                    
    advance(lexer);
    return makeToken(lexer, TOKEN_STRING);
}

Token Lexer::scan()
{
    Lexer& lexer = *this;

    skipWhitespace(lexer);

    lexer.start = lexer.current;

    if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

    char c = advance(lexer);
    if (isAlpha(c)) return identifier(lexer);
    if (isDigit(c)) return number(lexer);

    switch (c) {
        case '(': return makeToken(lexer, TOKEN_LEFT_PAREN);
        case ')': return makeToken(lexer, TOKEN_RIGHT_PAREN);
        case '[': return makeToken(lexer, TOKEN_LEFT_BRACKET);
        case ']': return makeToken(lexer, TOKEN_RIGHT_BRACKET);
        case '{': return makeToken(lexer, TOKEN_LEFT_BRACE);
        case '}': return makeToken(lexer, TOKEN_RIGHT_BRACE);
        case ';': return makeToken(lexer, TOKEN_SEMICOLON);
        case ',': return makeToken(lexer, TOKEN_COMMA);
        case '.': return makeToken(lexer, TOKEN_DOT);
        case '-': return makeToken(lexer, TOKEN_MINUS);
        case '+': return makeToken(lexer, TOKEN_PLUS);
        case '/': return makeToken(lexer, TOKEN_SLASH);
        case '*': return makeToken(lexer, TOKEN_STAR);

        case '!':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

        case '\'':
        case '\"': return string(lexer, c);
    }

    return errorToken(lexer, "Unexpected character.");
}
