#include <stdbool.h>
#include <stdio.h>
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
            
            /* Update type and value before modifying the linked list */
            op->type = IR_SCAN_ZERO;
            op->value = (to_free1->type == IR_PTR_ADD) ? to_free1->value : -to_free1->value;
            
            /* Update the linked list */
            op->next = after_loop;
            
            /* Update the last pointer if needed */
            if (to_free2 == program->last)
                program->last = op;
            
            /* Free the memory */
            free(to_free1);
            free(to_free2);
            
            program->count -= 2;
            continue;
        }
        op = op->next;
    }
}

/* NOTE: extra careful because too aggressive = program break */
/* this is a double pass optimization subroutine */
/* the idea of this optimization this to detect if the IR instructions
 * are symmetric or not inside a loop that looks like this:
 * [>++<-].
 * the total pointer offset is 0, all pointer movements cancel each other out
 * one cell is incrementing, original is dec by 1
 */
 typedef struct 
 {
    int ptr_offset;           /* total pointer movement */
    int value_multiply;       /* value being multiplied */
    int decrement_count;      /* how many times the original cell is decremented */
    bool has_ptr_movement;    /* did we move pointers? */
    bool has_value_add;       /* did we add a value? */
    bool is_direct_multiply;  /* is this a confirmed multiplication pattern? */
} MultiplyLoopAnalysis;

MultiplyLoopAnalysis analyze_multiply_loop(IROperation* loopstart)
{
    MultiplyLoopAnalysis analysis = { 0 };

    if (!loopstart || !loopstart->next)
        return analysis;

    IROperation* current = loopstart->next;

    while (current && current->type != IR_LOOP_END) 
    {
        switch (current->type) 
        {
            case IR_PTR_ADD:
                analysis.ptr_offset += current->value;
                analysis.has_ptr_movement = true;
                break;
            case IR_PTR_SUB:
                analysis.ptr_offset -= current->value;
                analysis.has_ptr_movement = true;
                break;
            case IR_VAL_ADD: /* first value add, or adding to a non-zero offset */
                if (!analysis.has_value_add || analysis.ptr_offset != 0) 
                {
                    analysis.value_multiply = current->value;
                    analysis.has_value_add = true;
                }
                break;
            case IR_VAL_SUB:
                analysis.decrement_count++;
                break;
            default: /* reset optimizability if unexpected */
                analysis.is_direct_multiply = false;
                break;
        }
        current = current->next;
    }

    analysis.is_direct_multiply = 
        (abs(analysis.ptr_offset) <= 1) &&  /* mostly cancel out pointer movements */
        analysis.has_ptr_movement &&        /* some pointer movement */
        analysis.has_value_add &&           /* added a value */
        analysis.decrement_count > 0 &&     /* decremented original cell */
        analysis.value_multiply > 0;        /* positive multiplication value */

    return analysis;
}

void optimize_add_mul_loops(IRProgram* program) 
{
    if (!program || !program->first)
        return;
        
    IROperation* op = program->first;
    
    while (op && op->next) 
    {
        /* check if this is a loop start */
        if (op->type == IR_LOOP_START) 
        {
            int loop_id = op->loop_id; /* save the loop ID */
            MultiplyLoopAnalysis analysis = analyze_multiply_loop(op);
            
            if (analysis.is_direct_multiply) 
            {
                /* find the loop end with matching loop_id */
                IROperation* loop_end = op->next;
                int nesting = 1; /* track nesting level to handle nested loops */
                while (loop_end && nesting > 0) 
                {
                    if (loop_end->type == IR_LOOP_START) 
                    {
                        nesting++;
                    } 
                    else if (loop_end->type == IR_LOOP_END) 
                    {
                        nesting--;
                        if (nesting == 0 && loop_end->loop_id == loop_id) 
                        {
                            break; /* found the matching loop end */
                        }
                    }
                    loop_end = loop_end->next;
                }
                
                if (!loop_end) 
                {
                    op = op->next;
                    continue; /* couldn't find matching loop end */
                }
                
                /* save what comes after the loop */
                IROperation* after_loop = loop_end->next;
                
                /* create new IR operations to replace the loop */
                IROperation* add_mul = create_ir_op(IR_ADD_MUL, 
                                                   analysis.value_multiply, 
                                                   analysis.ptr_offset, 
                                                   -1);
                                                   
                IROperation* set_zero = create_ir_op(IR_SET_ZERO, 
                                                    0, 
                                                    0, 
                                                    -1);
                
                /* link them together */
                add_mul->next = set_zero;
                set_zero->next = after_loop;
                
                /* delete all operations between op and after_loop */
                IROperation* current = op->next;
                while (current != after_loop) 
                {
                    IROperation* to_free = current;
                    current = current->next;
                    free(to_free);
                    program->count--;
                }
                
                /* free the loop start operation and replace it with add_mul */
                add_mul->next = set_zero;
                
                /* update the previous operation to point to our new add_mul */
                if (op == program->first) 
                {
                    program->first = add_mul;
                } 
                else 
                {
                    IROperation* prev = program->first;
                    while (prev && prev->next != op) 
                    {
                        prev = prev->next;
                    }
                    
                    if (prev) 
                    {
                        prev->next = add_mul;
                    }
                }
                
                /* update the last pointer if needed */
                if (loop_end == program->last) 
                {
                    program->last = set_zero;
                }
                
                /* free the original loop start */
                free(op);
                program->count--; /* one less for the freed loop start */
                
                /* account for the two new operations */
                program->count += 2;
                
                /* continue from the new set_zero operation */
                op = set_zero;
            } 
            else 
            {
                op = op->next;
            }
        } 
        else 
        {
            op = op->next;
        }
    }
}

IRProgram* optimize1(IRProgram* program)
{
    if (!program)
        return NULL;

    /* basic optimization */
    optimize_combinable(program);
    optimize_clear_loops(program);

    /* advance; maybe moved to -O3 or optimize2() */
    optimize_move_loops(program);
    optimize_add_mul_loops(program);
    optimize_scan_loops(program);
    
    optimize_combinable(program); /* final pass */
    return program;
}