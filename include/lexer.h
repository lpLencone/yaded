#ifndef MEDO_LEXER_H_
#define MEDO_LEXER_H_

#include "la.h"

#include <stddef.h>



typedef enum {
    TOKEN_END = 0,
    TOKEN_INLINE_COMMENT,
    TOKEN_BLOCK_COMMENT,
    TOKEN_INVALID,
    TOKEN_HASH,
    TOKEN_WHITESPACE,
    TOKEN_SYMBOL,
    TOKEN_BRACKET,
    TOKEN_STRLIT,
    TOKEN_CHRLIT,
    TOKEN_NUMLIT,
    TOKEN_KEYWORD,
    TOKEN_SEMI,
} Token_Kind;

typedef struct {
    Token_Kind kind;
    size_t len;
    Vec2f pos;
} Token;

typedef struct {
    const char **keywords; // NULL terminated keywords array {"a", "b", "c", NULL}
    const char *s;
    size_t len;
    size_t cur;
} Lexer;

Lexer lexer_init(const char *s, size_t len, const char **keywords);
Token lexer_next(Lexer *l);

#endif // MEDO_LEXER_H_
