#define _DEFAULT_SOURCE

#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define consume(n) \
    do { \
        token.len += n; \
        l->cur += n; \
    } while (0) 

#define lchar l->s[l->cur]

static bool ishex(char c)
{
    return (c >= '0' && c <= '9') || (tolower(c) >= 'a' && tolower(c) <= 'f');
}

static bool isbin(char c)
{
    return (c == '0' || c == '1');
}

typedef struct {
    const char *s[10];
    Token_Kind k;
} Literal_Token;

static Literal_Token lit_tokens[] = {
    {.s = {"(", ")", "{", "}", "[", "]",    NULL}, .k = TOKEN_BRACKET},
    {.s = {";",                             NULL}, .k = TOKEN_SEMI},
    {.s = {                                 NULL}, .k = 0},
};

Lexer lexer_init(const char *s, size_t len, const char **keywords)
{
    Lexer l = {0};
    l.s = s;
    l.len = len;
    l.keywords = keywords;
    return l;
}

static bool lexer_strneq(Lexer *l, const char *s, size_t n)
{
    return (l->cur + n <= l->len && strncmp(&lchar, s, n) == 0);
}
#define lexer_streq(l, s) lexer_strneq(l, s, strlen(s))

static bool lexer_char_in_str(Lexer *l, const char *s)
{
    return (l->cur < l->len && *strchrnul(s, lchar) != '\0');
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

    if (lchar == '#') {
        token.kind = TOKEN_HASH;
        while (l->cur < l->len) {
            consume(1);

            if (lchar == '\n' && 
                l->s[l->cur - 1] != '\\')
            {
                consume(1);
                break;
            }
        }

        return token;
    }

    if (lexer_streq(l, "//")) {
        token.kind = TOKEN_INLINE_COMMENT;
        while (l->cur < l->len) {
            consume(1);
            if (lchar == '\n' && 
                l->s[l->cur - 1] != '\\') break;
        }
        return token;
    }

    if (lexer_streq(l, "/*")) {
        token.kind = TOKEN_BLOCK_COMMENT;
        while (l->cur < l->len) {
            consume(1);

            if (lexer_streq(l, "*/")) {
                consume(2);
                break;
            }
        }
        return token;
    }

    if (isspace(lchar)) {
        token.kind = TOKEN_WHITESPACE;
        while (isspace(lchar)) {
            consume(1);
        }
        return token;
    }

    if (lchar == '\"') {
        token.kind = TOKEN_STRLIT;

        while (l->cur < l->len && lchar != '\n') {
            consume(1);
            if (lchar == '\"' && l->s[l->cur - 1] != '\\') {
                consume(1);
                break;
            }
        }
        return token;
    }

    if (lchar == '\'') {
        token.kind = TOKEN_CHRLIT;

        while (l->cur < l->len && lchar != '\n') {
            consume(1);
            if (lchar == '\'' && l->s[l->cur - 1] != '\\') {
                consume(1);
                break;
            }
        }
        return token;
    }

    if (is_symbol_start(lchar)) {
        if (l->keywords != NULL) {
            for (size_t i = 0; l->keywords[i] != NULL; i++) {
                if (lexer_streq(l, l->keywords[i])) {
                    size_t klen = strlen(l->keywords[i]);
                    if (l->cur + klen > l->len || is_symbol(l->s[l->cur + klen])) break;
                    token.kind = TOKEN_KEYWORD;
                    token.len = klen;
                    l->cur += klen;
                    return token;
                }
            }
        }
        token.kind = TOKEN_SYMBOL;
        while (l->cur < l->len && is_symbol(lchar)) {
            consume(1);
        }
        return token;
    }

    if (isdigit(lchar) || lchar == '.') {
        token.kind = TOKEN_NUMLIT;
        bool has_dot = lchar == '.';

        if (lchar == '0') {
            if (l->cur + 1 < l->len && tolower(l->s[l->cur + 1]) == 'x') {
                consume(2);
                
                if (l->cur >= l->len || !ishex(lchar)) {
                    goto invalid_consume;
                }

                while (l->cur < l->len && ishex(lchar)) {
                    consume(1);
                }
                if (is_symbol(lchar) || lchar == '.') {
                    goto invalid_consume;
                }
                return token;

            } else if (l->cur + 1 < l->len && tolower(l->s[l->cur + 1]) == 'b') {
                consume(2);

                if (l->cur >= l->len || !isbin(lchar)) {
                    goto invalid_consume;
                }

                while (l->cur < l->len && isbin(lchar)) {
                    consume(1);
                }
                if (l->cur < l->len && is_symbol(lchar) || lchar == '.') {
                    goto invalid_consume;
                }
                return token;
            }
        }

        while (l->cur < l->len && (isdigit(lchar) || lexer_char_in_str(l, "."))) {
            if (lexer_char_in_str(l, ".")) {
                if (has_dot) {
                    goto invalid_consume;
                } else {
                    has_dot = true;
                }
            }
            consume(1);
        }
        
        if (!has_dot) {
            if (lexer_char_in_str(l, "LUlu")) {
                consume(1);
                if (lexer_char_in_str(l, "LUlu")) {
                    consume(1);
                }
            }
            if (l->cur < l->len && is_symbol(lchar)) {
                goto invalid_consume;
            }
        } else {
            if (lexer_char_in_str(l, "LFlf")) {
                consume(1);
            }
            if (l->cur < l->len && is_symbol(lchar)) {
                goto invalid_consume;
            }
        }

        return token;
    }

    for (size_t i = 0; lit_tokens[i].k != 0; i++) {
        for (size_t j = 0; lit_tokens[i].s[j] != NULL; j++) {
            // NOTE: There cannot be newlines inside of literal tokens
            if (lexer_streq(l, lit_tokens[i].s[j])) {
                size_t len = strlen(lit_tokens[i].s[j]);
                token.kind = lit_tokens[i].k;
                consume(len);
                return token;
            }
        }
    }

invalid_consume:
    consume(1);
    token.kind = TOKEN_INVALID;

    return token;
}