#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    TOK_PTR_INC, /* > */
    TOK_PTR_DEC, /* > */
    TOK_VAL_INC, /* + */
    TOK_VAL_DEC, /* - */
    TOK_OUTPUT, /* . */
    TOK_INPUT, /* , */
    TOK_LOOP_START, /* [ */
    TOK_LOOP_END, /* ] */
    TOK_EOF  /* end of file */
} TokenType;

typedef struct
{
    TokenType type;
    size_t pos;
    int loop_id;
} Token;

/* what the lexer is supposed to throw out */
typedef struct
{
    Token* tokens; /* dynamic array of token */
    size_t count;
    size_t capacity;
} TokenArray;

/* in frontend.c */
char* preproc(const char* src);

TokenArray* tokenize(const char* proc_src);

void free_tkarr(TokenArray* arr);