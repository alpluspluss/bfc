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

IRProgram* parse(TokenArray* tokens)
{
    IRProgram* program = create_ir_program();
    if (!program) 
    {
        fprintf(stderr, "Failed to create IR program\n");
        return NULL;
    }
    
    for (size_t i = 0; i < tokens->count; i++) 
    {
        Token* token = &tokens->tokens[i];
        
        switch (token->type)
        {
            case TOK_PTR_INC:
                add_ir_op(program, IR_PTR_ADD, 1, 0, -1);
                break;
                
            case TOK_PTR_DEC:
                add_ir_op(program, IR_PTR_SUB, 1, 0, -1);
                break;
                
            case TOK_VAL_INC:
                add_ir_op(program, IR_VAL_ADD, 1, 0, -1);
                break;
                
            case TOK_VAL_DEC:
                add_ir_op(program, IR_VAL_SUB, 1, 0, -1);
                break;
                
            case TOK_OUTPUT:
                add_ir_op(program, IR_OUTPUT, 0, 0, -1);
                break;
                
            case TOK_INPUT:
                add_ir_op(program, IR_INPUT, 0, 0, -1);
                break;
                
            case TOK_LOOP_START:
                add_ir_op(program, IR_LOOP_START, 0, 0, token->loop_id);
                break;
                
            case TOK_LOOP_END:
                add_ir_op(program, IR_LOOP_END, 0, 0, token->loop_id);
                break;
                
            case TOK_EOF: /* end of file*/
                break;
        }
    }
    
    return program;
}