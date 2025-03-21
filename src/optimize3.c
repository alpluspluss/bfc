#include <stdbool.h>
#include <stdlib.h>
#include "bfc.h"

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

IRProgram* optimize3(IRProgram* program)
{
    optimize_add_mul_loops(program);
    optimize_combinable(program);
    return program;
}