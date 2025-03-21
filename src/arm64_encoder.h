#pragma once

#include <stdint.h>

/* arm64 instruction encoder; in arm64_encoder.c */
uint32_t encode_add_imm(int rd, int rn, int imm); /* ADD immediate 64-bit */

uint32_t encode_sub_imm(int rd, int rn, int imm); /* SUB immediate */

uint32_t encode_mov_imm(int rd, uint16_t imm); /* MOV immediate */

uint32_t encode_mov_reg(int rd, int rn); /* MOV register */

uint32_t encode_ldrb(int rt, int rn); /* load byte */

uint32_t encode_strb(int rt, int rn); /* store byte */

uint32_t encode_strb_offset(int rt, int rn, int offset); /* store byte with 12-bit unsigned offset */

uint32_t encode_cbz(int rt, int32_t offset); /* compare and branch if zero */

uint32_t encode_cbnz(int rt, int32_t offset); /* compare and branch if not zero */

uint32_t encode_b(int32_t offset); /* unconditional branch */

uint32_t encode_svc(uint16_t imm); /* supervisor call */

uint32_t encode_movk(int rd, uint16_t imm, int shift); /* imm 16-bit mov to register with keep */

uint32_t encode_ret(); /* return from subroutine */

uint32_t encode_stur(int rt, int rn, int offset); /* stor register (unscaled offset) */