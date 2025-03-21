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

typedef enum
{
    IR_PTR_ADD, /* add to pointer */
    IR_PTR_SUB, /* substract from pointer */
    IR_VAL_ADD, /* add to current cell */
    IR_VAL_SUB, /* substract from current cell */
    IR_OUTPUT, /* output current cell */
    IR_INPUT, /* input to current cell */
    IR_LOOP_START, /* start of loop */
    IR_LOOP_END, /* end of loop */

    /* higher level of operations from analysis; lower form of IR 
     * we will get these from doing the first pass */
    IR_SET_ZERO, /* set current cell to zero*/
    IR_SET_VAL, /* set current cell to constant value */
    IR_ADD_MUL, /* add multiple cell[pt r +offset] += cell[ptr] * factor */
    IR_MOVE_VAL, /* move value; cell[ptr + offset] += cell[ptr], cell[ptr] = 0 */
    IR_SCAN_ZERO, /* scan for zero; ptr += offset until cell[ptr] == 0 */
    IR_SCAN_NONZERO, /* scan for non-zero; ptr += offset until cell[ptr] != 0 */
    IR_CONDITIONAL /* conditional operation based on current cell */
} IROptype;

typedef struct IROperation
{
    IROptype type;
    int value; /* value for operations; +/- amount, etc. */
    int offset; /* memory offset for operations that reference other cells */
    int loop_id; /* for loop start/end matching */
    struct IROperation* next; /* linked list; not the most efficient solution but this should do */
} IROperation;

typedef struct
{
    IROperation* first;
    IROperation* last;
    size_t count;
} IRProgram;

typedef struct
{
    uint32_t* code;
    size_t capacity;
    size_t size;
} CodeBuffer;

/* in frontend.c */
char* preproc(const char* src);

TokenArray* tokenize(const char* proc_src);

void free_tkarr(TokenArray* arr);

IRProgram* parse(TokenArray* tokens);

/* optimize1.c; this function performs IR level optimizations */
IRProgram* optimize1(IRProgram* program);

/* codegen.c */
CodeBuffer* codegen(IRProgram* program);

void free_code_buffer(CodeBuffer* buf);

/* ir.c; this is IR utils */
IRProgram* create_ir_program();

void free_ir_program(IRProgram* program);

IROperation* create_ir_op(IROptype type, int value, int offset, int loop_id);

void add_ir_op(IRProgram* program, IROptype type, int value, int offset, int loop_id);