#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arm64_encoder.h"
#include "bfc.h"

#define REG_TAPE_PTR 0 /* X0 = ptr to current cell */
#define REG_TEMP 1     /* X1 = temp register for operations */
#define REG_SYSCALL 8  /* X8 = syscall number */
#define REG_STDOUT 1   /* X0 = stdout file descriptor */
#define REG_BUF_PTR 1  /* X1 = buffer pointer for I/O */
#define REG_BUF_SIZE 2 /* X2 = buffer size for I/O */

CodeBuffer* create_code_buffer(size_t capacity) 
{
    CodeBuffer* buf = malloc(sizeof(CodeBuffer));
    if (!buf)
        return NULL;

    buf->code = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    if (!buf->code) 
    {
        free(buf);
        return NULL;
    }

    buf->capacity = capacity;
    buf->size = 0;
    return buf;
}

void free_code_buffer(CodeBuffer* buf) 
{
    if (buf) 
    {
        if (buf->code)
            free(buf->code);
        free(buf);
    }
}

void emit_instr(CodeBuffer* buf, uint32_t instr) 
{
    if (buf->size >= buf->capacity) /* resize if bigger */
    {
        size_t new_capacity = buf->capacity * 2;
        uint32_t* new_code = (uint32_t*)realloc(buf->code, new_capacity * sizeof(uint32_t));
        if (!new_code) 
        {
            fprintf(stderr, "Failed to expand code buffer\n");
            exit(1);
        }

        buf->code = new_code;
        buf->capacity = new_capacity;
    }

    buf->code[buf->size++] = instr;
}

int32_t compute_br_offset(CodeBuffer* buf, int src_offset, int dest_offset) 
{
    /* ARM64 branch offsets are relative to the branch instruction's position
     * and measured in 4-byte units since instructions are 4 bytes */
    return (dest_offset - src_offset) * 4;
}

void patch_br(CodeBuffer* buf, int offset, int32_t branch_offset) 
{
    if (offset < 0 || (size_t)offset >= buf->size) /* buffer overflow check */
    {
        fprintf(stderr, "Error: Attempt to patch branch at invalid offset %d (buffer size: %zu)\n", 
                offset, buf->size);
        return;
    }

    if (buf->code[offset] & (1u << 31)) /* 64-bit conditional branch (CBZ/CBNZ) */
    {
        /* offset is encoded in bits 5-23 for CCBZ/CBNZ */
        int imm19 = (branch_offset / 4) & 0x7FFFF;
        buf->code[offset] &= ~(0x7FFFF << 5); /* clear old offset */
        buf->code[offset] |= (imm19 << 5);  /* set new offset */
    } 
    else 
    {
        /* for unconditional B instruction, offset is encoded in bits 0-25 */
        int imm26 = (branch_offset / 4) & 0x3FFFFFF;
        buf->code[offset] &= ~0x3FFFFFF; /* Clear old offset */
        buf->code[offset] |= imm26; /* Set new offset */
    }
}

void emit_io_op(CodeBuffer* buf, int is_output)
{
    /* preserve X0 in X9 (a caller-saved register we can use) */
    emit_instr(buf, encode_mov_reg(9, REG_TAPE_PTR));

    if (is_output)  /* stdout */
    {
        emit_instr(buf, encode_mov_imm(REG_SYSCALL, 64));
        emit_instr(buf, encode_mov_imm(0, 1));
    }
    else /* stdin */
    {
        emit_instr(buf, encode_mov_imm(REG_SYSCALL, 63));
        emit_instr(buf, encode_mov_imm(0, 0));
    }

    emit_instr(buf, encode_mov_reg(REG_BUF_PTR, 9)); /* X1 = buffer address (current byte pointer) */
    emit_instr(buf, encode_mov_imm(REG_BUF_SIZE, 1)); /* X2 = length (1) */
    emit_instr(buf, encode_svc(0)); /* make syscall */
    emit_instr(buf, encode_mov_reg(REG_TAPE_PTR, 9)); /* restore X0 from X9 */
}

CodeBuffer* codegen(IRProgram* program) 
{
    if (!program || !program->first) 
    {
        fprintf(stderr, "Empty program to compile\n");
        return NULL;
    }
    
    CodeBuffer* buf = create_code_buffer(5000);
    if (!buf) 
    {
        fprintf(stderr, "Failed to create code buffer\n");
        return NULL;
    }

    /* allocate memory for loop tracking */
    int max_loop_id = -1;
    IROperation* op = program->first;
    while (op) 
    {
        if ((op->type == IR_LOOP_START || op->type == IR_LOOP_END) && 
            op->loop_id > max_loop_id) 
        {
            max_loop_id = op->loop_id;
        }
        op = op->next;
    }
    
    int* loop_start_offsets = malloc((max_loop_id + 1) * sizeof(int));
    int* loop_end_patches = malloc((max_loop_id + 1) * sizeof(int));

    if (!loop_start_offsets || !loop_end_patches) 
    {
        fprintf(stderr, "Memory allocation error\n");
        free_code_buffer(buf);
        free(loop_start_offsets);
        free(loop_end_patches);
        return NULL;
    }

    memset(loop_start_offsets, -1, (max_loop_id + 1) * sizeof(int));
    memset(loop_end_patches, -1, (max_loop_id + 1) * sizeof(int));

    op = program->first;
    while (op)
    {
        switch (op->type) 
        {
            case IR_PTR_ADD:
            {
                    emit_instr(buf, encode_add_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->value));
                    break;
            }
                
            case IR_PTR_SUB:
            { 
                emit_instr(buf, encode_sub_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->value));
                break;
            }
                
            case IR_VAL_ADD:
            {    
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR));
                emit_instr(buf, encode_add_imm(REG_TEMP, REG_TEMP, op->value));
                emit_instr(buf, encode_strb(REG_TEMP, REG_TAPE_PTR));
                break;
            }
                
            case IR_VAL_SUB:
            { 
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR));
                emit_instr(buf, encode_sub_imm(REG_TEMP, REG_TEMP, op->value));
                emit_instr(buf, encode_strb(REG_TEMP, REG_TAPE_PTR));
                break; 
            }
                
            case IR_OUTPUT: 
            {
                emit_io_op(buf, 1); /* output = true */
                break;
            }
                
            case IR_INPUT:
            {
                emit_io_op(buf, 0); /* output = false */
                break;
            }

            case IR_LOOP_START:
            {   
                loop_start_offsets[op->loop_id] = buf->size; /* record the start position of this loop */
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* then load value at pointer */
                
                /* branch to end of loop if zero; will be patched later in the second pass */
                emit_instr(buf, encode_cbz(REG_TEMP, 0));
                loop_end_patches[op->loop_id] = buf->size - 1;
                break;
            }
                
            case IR_LOOP_END:
            {  
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* load value at pointer */
                
                /* branch back to start of loop if not zero */
                int32_t backwards_offset = compute_br_offset(
                    buf, buf->size, loop_start_offsets[op->loop_id]);

                emit_instr(buf, encode_cbnz(REG_TEMP, backwards_offset));
                
                /* patch the forward jump from loop start */
                if (loop_end_patches[op->loop_id] >= 0)
                {
                    int32_t forwards_offset = compute_br_offset(
                        buf, loop_end_patches[op->loop_id], buf->size);
                    patch_br(buf, loop_end_patches[op->loop_id], forwards_offset);
                } 
                else 
                {
                    fprintf(stderr, "Warning: Unmatched loop end for loop ID %d\n", op->loop_id);
                }
                break;
            }
                
            case IR_SET_ZERO:
            {
                emit_instr(buf, encode_strb_offset(31, REG_TAPE_PTR, 0)); /* XZR = zero register */
                break;
            }
                
            case IR_SET_VAL: /* set current cell to a specific value */
            {    
                emit_instr(buf, encode_mov_imm(REG_TEMP, op->value));
                emit_instr(buf, encode_strb(REG_TEMP, REG_TAPE_PTR));
                break;
            }
                
            case IR_ADD_MUL: /* add multiplication: cell[ptr+offset] += cell[ptr] * factor */
            {
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* load source value */
                emit_instr(buf, encode_mov_reg(9, REG_TAPE_PTR)); /* save the original stack pointer */
                emit_instr(buf, encode_add_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->offset)); /* move to target cell */
                emit_instr(buf, encode_ldrb(2, REG_TAPE_PTR)); /* load target */
                for (int i = 0; i < op->value; i++) 
                    emit_instr(buf, encode_add_imm(2, 2, REG_TEMP)); /* perform mult by repeated addition */
                
                emit_instr(buf, encode_strb(2, REG_TAPE_PTR)); /* store result*/
                emit_instr(buf, encode_mov_reg(REG_TAPE_PTR, 9)); /* restore the pointer */
                emit_instr(buf, encode_strb_offset(31, REG_TAPE_PTR, 0)); /* clear the source cell buffer */
                break;
            }
                
            case IR_MOVE_VAL: /* move operation: cell[ptr+offset] += cell[ptr], cell[ptr] = 0 */
            {
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* load source value */
                emit_instr(buf, encode_mov_reg(9, REG_TAPE_PTR)); /* save stack pointer */
                emit_instr(buf, encode_add_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->offset)); /* move to target cell */
                emit_instr(buf, encode_ldrb(2, REG_TAPE_PTR)); /* load target value */
                emit_instr(buf, encode_add_imm(2, 2, REG_TEMP)); /* add source to target */
                emit_instr(buf, encode_strb(2, REG_TAPE_PTR)); /* store result */
                
                emit_instr(buf, encode_mov_reg(REG_TAPE_PTR, 9)); /* restore pointer */
                emit_instr(buf, encode_strb_offset(31, REG_TAPE_PTR, 0)); /* clear the source cell buffer */
                break;
            }  
            case IR_SCAN_ZERO: /* scan until finding a zero: while (*ptr) ptr += step */
            {
                int scan_start = buf->size;
                
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* load current cell */
                emit_instr(buf, encode_cbz(REG_TEMP, 0)); /* if zero then exit loop */
                int scan_patch = buf->size - 1;
                
                /* move pointer in steps */
                if (op->value > 0) 
                {
                    emit_instr(buf, encode_add_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->value));
                } 
                else 
                {
                    emit_instr(buf, encode_sub_imm(REG_TAPE_PTR, REG_TAPE_PTR, -op->value));
                }
                    
                /* jump back to start of scan */
                int32_t scan_back = compute_br_offset(buf, buf->size, scan_start);
                emit_instr(buf, encode_b(scan_back));
                    
                /* then patch the exit point */
                int32_t scan_exit = compute_br_offset(buf, scan_patch, buf->size);
                patch_br(buf, scan_patch, scan_exit);

                break;
            }
                
            case IR_SCAN_NONZERO: /* scan until finding non-zero: while (!*ptr) ptr += step */
            {
                int scan_start = buf->size;
                emit_instr(buf, encode_ldrb(REG_TEMP, REG_TAPE_PTR)); /* load current cell */
                
                emit_instr(buf, encode_cbnz(REG_TEMP, 0)); /* if non-zero, exit loop */
                int scan_patch = buf->size - 1;
                    
                /* move pointer by step */
                if (op->value > 0) 
                {
                    emit_instr(buf, encode_add_imm(REG_TAPE_PTR, REG_TAPE_PTR, op->value));
                } 
                else 
                {
                    emit_instr(buf, encode_sub_imm(REG_TAPE_PTR, REG_TAPE_PTR, -op->value));
                }
                    
                /* jump back to start of scan */
                int32_t scan_back = compute_br_offset(buf, buf->size, scan_start);
                emit_instr(buf, encode_b(scan_back));
                    
                /* patch the exit point */
                int32_t scan_exit = compute_br_offset(buf, scan_patch, buf->size);
                patch_br(buf, scan_patch, scan_exit);
                break;
            }
                
            case IR_CONDITIONAL: /* for advanced optimizations */
                fprintf(stderr, "Warning: IR_CONDITIONAL is yet to be implemented\n");
                break;
        }
        
        op = op->next;
    }
    
    /* add return instruction */
    emit_instr(buf, encode_ret());
    
    /* clean up */
    free(loop_start_offsets);
    free(loop_end_patches);
    return buf;
}