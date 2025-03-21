#include <stdlib.h>
#include <stdio.h>
#include "bfc.h"

IRProgram* create_ir_program() 
{
    IRProgram* program = malloc(sizeof(IRProgram));
    if (!program) 
    {
        perror("Memory allocation error");
        return NULL;
    }
    
    program->first = NULL;
    program->last = NULL;
    program->count = 0;
    return program;
}

void free_ir_program(IRProgram* program)
{
    if (!program) 
        return;
    
    IROperation* op = program->first;
    while (op) /* traverse the list */
    {
        IROperation* next = op->next;
        free(op);
        op = next;
    }
    
    free(program);
}

IROperation* create_ir_op(IROptype type, int value, int offset, int loop_id) 
{
    IROperation* op = malloc(sizeof(IROperation));
    if (!op) 
    {
        perror("Memory allocation error");
        return NULL;
    }
    
    op->type = type;
    op->value = value;
    op->offset = offset;
    op->loop_id = loop_id;
    op->next = NULL;
    
    return op;
}

void add_ir_op(IRProgram* program, IROptype type, int value, int offset, int loop_id) 
{
    IROperation* op = create_ir_op(type, value, offset, loop_id);
    if (!op) 
    {
        fprintf(stderr, "Failed to create IR operation\n");
        return;
    }
    
    if (program->last) 
    {
        program->last->next = op;
        program->last = op;
    } 
    else
    {
        program->first = op;
        program->last = op;
    }
    
    program->count++;
}