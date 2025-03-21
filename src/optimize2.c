#include <stdlib.h>
#include "bfc.h"

void optimize_move_loops(IRProgram* program) 
{
    IROperation* op = program->first;
    
    while (op && op->next && op->next->next && op->next->next->next && op->next->next->next->next) 
    {
        /* check for [->+<] pattern */
        if (op->type == IR_LOOP_START &&
            op->next->type == IR_VAL_SUB && op->next->value == 1 &&
            op->next->next->type == IR_PTR_ADD && 
            op->next->next->next->type == IR_VAL_ADD && 
            op->next->next->next->next->type == IR_PTR_SUB &&
            op->next->next->next->next->next->type == IR_LOOP_END &&
            op->next->next->next->next->next->loop_id == op->loop_id &&
            op->next->next->next->next->value == op->next->next->value) 
        {
            /* replace with a MOVE_VAL operation */
            IROperation* loop_end = op->next->next->next->next->next;
            IROperation* after_loop = loop_end->next;
            
            op->type = IR_MOVE_VAL;
            op->value = op->next->next->next->value; /* save the add amount */
            op->offset = op->next->next->value; /* save the offset */
            op->next = after_loop;
            
            if (loop_end == program->last)
                program->last = op;
                
            /* clean the replaced operations */
            free(op->next);
            free(op->next->next);
            free(op->next->next->next);
            free(op->next->next->next->next);
            free(loop_end);
            
            program->count -= 5;
            continue;
        }
        op = op->next;
    }
}

void optimize_scan_loops(IRProgram* program) 
{
    if (!program || !program->first)
        return;
        
    IROperation* op = program->first;
    
    while (op && op->next && op->next->next) 
    {
        /* check for [>] or [<] pattern */
        if (op->type == IR_LOOP_START &&
            (op->next->type == IR_PTR_ADD || op->next->type == IR_PTR_SUB) &&
            op->next->next->type == IR_LOOP_END && 
            op->next->next->loop_id == op->loop_id) 
        {
            /* replace with a SCAN_ZERO operation */
            IROperation* to_free1 = op->next;
            IROperation* to_free2 = op->next->next;
            IROperation* after_loop = to_free2->next;
            
            /* update type and value before modifying the linked list */
            op->type = IR_SCAN_ZERO;
            op->value = (to_free1->type == IR_PTR_ADD) ? to_free1->value : -to_free1->value;
            
            /* update the linked list */
            op->next = after_loop;
            
            if (to_free2 == program->last)
                program->last = op;
            
            free(to_free1);
            free(to_free2);
            program->count -= 2;
            continue;
        }
        op = op->next;
    }
}

IRProgram* optimize2(IRProgram* program)
{
    optimize_scan_loops(program);
    optimize_move_loops(program);
    optimize_combinable(program);
    return program;
}