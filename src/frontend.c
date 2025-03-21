#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bfc.h"

char* preproc(const char* src)
{
    size_t len = strlen(src);
    char* proc = malloc(len + 1);
    if (!proc)
    {
        perror("memory allocation error");
        return NULL;
    }

    size_t k = 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = src[i];
        if (c == '>' || c == '<' || c == '+' || c == '-' || 
            c == '.' || c == ',' || c == '[' || c == ']') 
        {
            proc[k++] = c;
        }
    }

    proc[k] = '\0';
    return proc;
}

TokenArray* create_tokens(size_t init)
{
    TokenArray* arr = malloc(sizeof(TokenArray));
    if (!arr)
        return NULL;

    arr->tokens = (Token*)malloc(init * sizeof(Token));
    if (!arr->tokens)
    {
        free(arr);
        return NULL;
    }

    arr->count = 0;
    arr->capacity = init;
    return arr;
}

void add_tok(TokenArray* arr, TokenType type, size_t pos)
{
    if (arr->count >= arr->capacity) 
    {
        size_t new_capacity = arr->capacity * 2;
        Token* new_tokens = (Token*)realloc(arr->tokens, new_capacity * sizeof(Token));
        if (!new_tokens)
        {
            fprintf(stderr, "Failed to expand token array\n");
            return;
        }
        
        arr->tokens = new_tokens;
        arr->capacity = new_capacity;
    }

    arr->tokens[arr->count].type = type;
    arr->tokens[arr->count].pos = pos;
    arr->count++;
}

void free_tkarr(TokenArray* arr)
{
    if (arr && arr->tokens)
        free(arr->tokens);
    free(arr);
}

TokenArray* tokenize(const char* proc_src)
{
    size_t code = strlen(proc_src);
    TokenArray* tokens = create_tokens(code);

    if (!tokens)
    {
        fprintf(stderr, "failed to create token array");
        return NULL;
    }

    int loopid = 0;
    int loop_stack[256];
    int loop_depth = 0;

    for (size_t i = 0; i < code; ++i)
    {
        char c = proc_src[i];

        switch (c) 
        {
            case '>':
                add_tok(tokens, TOK_PTR_INC, i);
                break;
                
            case '<':
                add_tok(tokens, TOK_PTR_DEC, i);
                break;
                
            case '+':
                add_tok(tokens, TOK_VAL_INC, i);
                break;
                
            case '-':
                add_tok(tokens, TOK_VAL_DEC, i);
                break;
                
            case '.':
                add_tok(tokens, TOK_OUTPUT, i);
                break;
                
            case ',':
                add_tok(tokens, TOK_INPUT, i);
                break;
                
            case '[':
                if (loop_depth >= 256) 
                {
                    fprintf(stderr, "Error: Too many nested loops\n");
                    free_tkarr(tokens);
                    return NULL;
                }
                
                loop_stack[loop_depth++] = loopid;
                add_tok(tokens, TOK_LOOP_START, i);
                tokens->tokens[tokens->count-1].loop_id = loopid;
                loopid++;
                break;
                
            case ']':
                if (loop_depth <= 0) 
                {
                    fprintf(stderr, "Error: Unmatched closing bracket\n");
                    free_tkarr(tokens);
                    return NULL;
                }
                
                loop_depth--;
                add_tok(tokens, TOK_LOOP_END, i);
                tokens->tokens[tokens->count-1].loop_id = loop_stack[loop_depth];
                break;
                
            default: /* shouldn't happen because we preprocessed the input */
                fprintf(stderr, "Unexpected character: %c\n", c);
                break;
        }
    }

    if (loop_depth > 0) 
    {
        fprintf(stderr, "Error: Unmatched opening bracket(s)\n");
        free_tkarr(tokens);
        return NULL;
    }

    return tokens;
}
