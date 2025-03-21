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

void ir_dump(IRProgram* program)
{
    if (!program || !program->first)
    {
        printf("Empty program\n");
        return;
    }
    
    printf("IR Program (%zu operations):\n", program->count);
    printf("---------------------------------------\n");
    
    IROperation* op = program->first;
    int index = 0;
    
    while (op)
    {
        printf("[%d] ", index++);
        
        switch (op->type)
        {
            case IR_PTR_ADD:
                printf("PTR_ADD     value=%d\n", op->value);
                break;
            case IR_PTR_SUB:
                printf("PTR_SUB     value=%d\n", op->value);
                break;
            case IR_VAL_ADD:
                printf("VAL_ADD     value=%d\n", op->value);
                break;
            case IR_VAL_SUB:
                printf("VAL_SUB     value=%d\n", op->value);
                break;
            case IR_OUTPUT:
                printf("OUTPUT\n");
                break;
            case IR_INPUT:
                printf("INPUT\n");
                break;
            case IR_LOOP_START:
                printf("LOOP_START  id=%d\n", op->loop_id);
                break;
            case IR_LOOP_END:
                printf("LOOP_END    id=%d\n", op->loop_id);
                break;
            case IR_SET_ZERO:
                printf("SET_ZERO\n");
                break;
            case IR_SET_VAL:
                printf("SET_VAL     value=%d\n", op->value);
                break;
            case IR_ADD_MUL:
                printf("ADD_MUL     value=%d  offset=%d\n", op->value, op->offset);
                break;
            case IR_MOVE_VAL:
                printf("MOVE_VAL    value=%d  offset=%d\n", op->value, op->offset);
                break;
            case IR_SCAN_ZERO:
                printf("SCAN_ZERO   step=%d\n", op->value);
                break;
            case IR_SCAN_NONZERO:
                printf("SCAN_NONZERO step=%d\n", op->value);
                break;
            case IR_CONDITIONAL:
                printf("CONDITIONAL value=%d  offset=%d\n", op->value, op->offset);
                break;
            default:
                printf("UNKNOWN     type=%d\n", op->type);
                break;
        }
        
        op = op->next;
    }
    
    printf("---------------------------------------\n");
}