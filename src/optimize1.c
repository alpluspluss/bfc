#include <stdlib.h>
#include "bfc.h"

/* this function tries to optimize operations of the same type */
/* also is some sort of dead code elimination */
void optimize_combinable(IRProgram* program)
{
    if (!program || !program->first)
        return;

    IROperation* current = program->first;
    while (current && current->next)
    {
        IROperation* next = current->next;

        /* if the next ops are mergable */
        if ((current->type == IR_PTR_ADD && next->type == IR_PTR_ADD) ||
            (current->type == IR_PTR_SUB && next->type == IR_PTR_SUB) ||
            (current->type == IR_VAL_ADD && next->type == IR_VAL_ADD) ||
            (current->type == IR_VAL_SUB && next->type == IR_VAL_SUB)) 
        {
            current->value += next->value; /* combine by adding values */

            current->next = next->next; /* remove the next operation */
            if (next == program->last)
                program->last = current;

            free(next);
            program->count--;
            
            /* don't advance current since we need to check the new next */
            continue;
        }
        else if ((current->type == IR_PTR_ADD && next->type == IR_PTR_SUB) ||
                (current->type == IR_PTR_SUB && next->type == IR_PTR_ADD) ||
                (current->type == IR_VAL_ADD && next->type == IR_VAL_SUB) ||
                (current->type == IR_VAL_SUB && next->type == IR_VAL_ADD))
        {
            int net_value = current->value - next->value;
            if (net_value == 0) 
            {
                /* operations cancel out completely */
                IROperation* next_next = next->next;
                
                /* special case: if removing the first element */
                if (current == program->first) 
                {
                    program->first = next_next;
                    if (!program->first)
                        program->last = NULL;
                } 
                else
                {
                    /* find the previous operation */
                    IROperation* prev = program->first;
                    while (prev && prev->next != current) 
                        prev = prev->next;
                    
                    if (prev) 
                    {
                        prev->next = next_next;
                        if (next == program->last)
                            program->last = prev;
                    }
                }
                
                free(current);
                free(next);
                program->count -= 2;
                
                current = (program->first) ? program->first : NULL;
            }
            else if (net_value > 0) /* first operation dominates */
            {
                current->value = net_value;
                current->next = next->next;
                if (next == program->last)
                    program->last = current;

                free(next);
                program->count--;
            }
            else /* second dominates */
            {
                current->type = next->type;
                current->value = -net_value;
                
                /* remove next operation */
                current->next = next->next;
                if (next == program->last)
                    program->last = current;

                free(next);
                program->count--;
            }
        }
        current = current->next;
    }
}

/* detect and optimize clear cell loops [-]*/
void optimize_clear_loops(IRProgram* program) 
{
    if (!program || !program->first)
        return;
    
    IROperation* current = program->first;
    while (current && current->next && current->next->next) 
    {
        if (current->type == IR_LOOP_START &&
            current->next->type == IR_VAL_SUB && current->next->value == 1 &&
            current->next->next->type == IR_LOOP_END && 
            current->next->next->loop_id == current->loop_id) 
        {
            
            /* replace with SET_ZERO so we can generate more specialized instructions */
            IROperation* loop_end = current->next->next;
            IROperation* after_loop = loop_end->next;
            
            current->type = IR_SET_ZERO;
            current->value = 0;
            current->next = after_loop;
            
            if (loop_end == program->last)
                program->last = current;
            
            /* cleanup */
            free(current->next);
            free(loop_end);
            program->count -= 2;
        
            continue; /* don't advance */
        }
        
        current = current->next;
    }
}

IRProgram* optimize1(IRProgram* program)
{
    if (!program)
        return NULL;

    /* basic optimization */
    optimize_combinable(program);
    optimize_clear_loops(program);

    /* 
     * there are many operations such as: 
     * - move loops [->+<-]
     * - add-mul loops [->++<]
     * - scan loops [>] OR [<]
     */
    
    optimize_combinable(program); /* final pass; maybe later changed to a specialized functions */
    return program;
}