#define _DEFAULT_SOURCE

#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    const char *s[10];
    Token_Kind k;
} Literal_Token;

static Literal_Token lit_tokens[] = {
    {.s = {"(", ")", "{", "}", "[", "]",    NULL}, .k = TOKEN_BRACKET},
    {.s = {";",                             NULL}, .k = TOKEN_SEMI},
    {.s = {                                 NULL}, .k = 0},
};

Lexer lexer_init(const char *s, size_t len)
{
    Lexer l = {0};
    l.s = s;
    l.len = len;
    return l;
}

bool lexer_is_equal_n(Lexer *l, const char *s, size_t n)
{
    return (l->cur + n <= l->len && strncmp(&l->s[l->cur], s, n) == 0);
}
#define lexer_is_equal(l, s) lexer_is_equal_n(l, s, strlen(s))

bool lexer_current_char_in(Lexer *l, const char *s)
{
    return (l->cur < l->len && *strchrnul(s, l->s[l->cur]) != '\0');
}

bool lexer_starts_with(Lexer *l, const char *prefix)
{
    assert(l->cur < l->len);

    return lexer_is_equal(l, prefix);
}

static bool is_symbol_start(char c)
{
    return isalpha(c) || c == '_';
}

static bool is_symbol(char c)
{
    return isalnum(c) || c == '_';
}

Token lexer_next(Lexer *l)
{
    Token token = {0};

    if (l->cur >= l->len) return token;

    if (l->s[l->cur] == '#') {
        token.kind = TOKEN_HASH;
        while (l->cur < l->len) {
            token.len++;
            l->cur++;

            if (l->s[l->cur] == '\n' && 
                l->s[l->cur - 1] != '\\') break;
        }
        if (l->cur < l->len) {
            l->cur++;
        }
        return token;
    }

    if (lexer_is_equal(l, "//")) {
        token.kind = TOKEN_COMMENT;
        while (l->cur < l->len) {
            token.len++;
            l->cur++;

            if (l->s[l->cur] == '\n' && 
                l->s[l->cur - 1] != '\\') break;
        }
        if (l->cur < l->len) {
            l->cur++;
        }
        return token;
    }

    if (isspace(l->s[l->cur])) {
        token.kind = TOKEN_WHITESPACE;
        while (isspace(l->s[l->cur])) {
            l->cur++;
            token.len++;
        }
        return token;
    }

    if (l->s[l->cur] == '\"') {
        token.kind = TOKEN_STRLIT;
        l->cur++;
        token.len++;
        while (l->cur < l->len && l->s[l->cur] != '\"') {
            l->cur++;
            token.len++;
            if (l->s[l->cur] == '\"' && l->s[l->cur - 1] == '\\') {
                l->cur++;
                token.len++;
            }
        }
        l->cur++;
        token.len++;
        if (l->cur < l->len) {
            l->cur++;
        }
        return token;
    }

    if (is_symbol_start(l->s[l->cur])) {
        token.kind = TOKEN_SYMBOL;
        while (l->cur < l->len && is_symbol(l->s[l->cur])) {
            l->cur++;
            token.len++;
        }
        return token;
    }

    if (isdigit(l->s[l->cur]) || l->s[l->cur] == '.') {
        token.kind = TOKEN_NUMLIT;
        bool has_dot = l->s[l->cur] == '.';

        while (l->cur < l->len && (isdigit(l->s[l->cur]) || lexer_current_char_in(l, "."))) {
            if (lexer_current_char_in(l, ".")) {
                if (has_dot) {
                    goto invalid;
                }
                else {
                    has_dot = true;
                }
            }
            token.len++;
            l->cur++;
        }
        
        if (!has_dot) {
            if (lexer_current_char_in(l, "LUlu")) {
                token.len++;
                l->cur++;
                if (lexer_current_char_in(l, "LUlu")) {
                    token.len++;
                    l->cur++;
                }
            } 
            if (l->cur < l->len && is_symbol(l->s[l->cur])) {
                goto invalid;
            }
        } else {
            if (lexer_current_char_in(l, "LFlf")) {
                token.len++;
                l->cur++;
            } 
            if (l->cur < l->len && is_symbol(l->s[l->cur])) {
                goto invalid;
            }
        }

        return token;
    }

    for (size_t i = 0; lit_tokens[i].k != 0; i++) {
        for (size_t j = 0; lit_tokens[i].s[j] != NULL; j++) {
            // NOTE: There cannot be newlines inside of tokens
            if (lexer_starts_with(l, lit_tokens[i].s[j])) {
                size_t len = strlen(lit_tokens[i].s[j]);
                token.kind = lit_tokens[i].k;
                token.len = len;
                l->cur += len;
                return token;
            }
        }
    }

invalid:
l->cur++;
    token.kind = TOKEN_INVALID;
    token.len++;

    return token;
}