#ifndef YADED_LEXER_H_
#define YADED_LEXER_H_

#include "la.h"

#include <stddef.h>

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s ", __FILE__, __LINE__, __func__);
#define debug_println debug_print puts("");
#endif

typedef enum {
    TOKEN_END = 0,
    TOKEN_COMMENT,
    TOKEN_INVALID,
    TOKEN_HASH,
    TOKEN_WHITESPACE,
    TOKEN_SYMBOL,
    TOKEN_BRACKET,
    TOKEN_STRLIT,
    TOKEN_CHRLIT,
    TOKEN_NUMLIT,
    TOKEN_SEMI,
} Token_Kind;

typedef struct {
    Token_Kind kind;
    size_t len;
    Vec2f pos;
} Token;

const char *token_kind_name(Token_Kind kind);

typedef struct {
    const char *s;
    size_t len;
    size_t cur;
} Lexer;

Lexer lexer_init(const char *s, size_t len);
Token lexer_next(Lexer *l);

#endif // YADED_LEXER_H_