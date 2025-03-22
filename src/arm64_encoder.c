#include "arm64_encoder.h"

uint32_t encode_add_imm(int rd, int rn, int imm) 
{
    return (1u << 31) |            /* 64-bit */
            (0u << 30) |            /* ADD opcode */
            (0u << 29) |            /* no set flags */
            (0x11u << 24) |         /* encoding-specific opcode */
            ((imm & 0xFFF) << 10) | /* 12-bit immediate */
            (rn << 5) |             /* base register */
            rd;                     /* destination register */
}

uint32_t encode_sub_imm(int rd, int rn, int imm) 
{
    return (1u << 31) |            /* 64-bit */
            (1u << 30) |            /* SUB opcode */
            (0u << 29) |            /* no flags */
            (0x11u << 24) |         /* encoding-specific opcode */
            ((imm & 0xFFF) << 10) | /* 12-bit immediate */
            (rn << 5) |             /* base register */
            rd;                     /* destination register */
}

/* MOVZ variant for simplicity */
uint32_t encode_mov_imm(int rd, uint16_t imm) 
{
    return (1u << 31) |    /* 64-bit */
            (0x2u << 29) |  /* MOVZ opcode */
            (0x25u << 23) | /* fixed bits */
            (0u << 21) |    /* no shift */
            (imm << 5) |    /* 16-bit immediate */
            rd;             /* destination register */
}

/* MOV register (64-bit) */
uint32_t encode_mov_reg(int rd, int rn) 
{
    return (1u << 31) |   /* 64-bit */
            (0x2A0003E0) | /* ORR Xd, XZR, Xm */
            (rn << 16) |   /* source register */
            rd;            /* destination register */
}

uint32_t encode_ldrb(int rt, int rn) 
{
    return (0x00u << 30) | /* size=8-bit */
            (0x7u << 27) |  /* Load/Store opcode */
            (0x1u << 22) |  /* load (not store) */
            (0u << 10) |    /* zero offset */
            (rn << 5) |     /* base register */
            rt;             /* target register */
}

uint32_t encode_strb(int rt, int rn) 
{
    return (0x00u << 30) | /* size=8-bit */
            (0x7u << 27) |  /* Load/Store opcode */
            (0x0u << 22) |  /* Store (not load) */
            (0u << 10) |    /* zero offset */
            (rn << 5) |     /* base register */
            rt;             /* target register */
}

uint32_t encode_strb_offset(int rt, int rn, int offset) 
{
    return (0x00u << 30) |            /* size=8-bit */
            (0x7u << 27) |             /* Load/Store opcode */
            (0x0u << 22) |             /* store (not load) */
            ((offset & 0xFFF) << 10) | /* 12-bit immediate offset */
            (rn << 5) |                /* base register */
            rt;                        /* target register */
}

uint32_t encode_cbz(int rt, int32_t offset) 
{
    /* offset is in bytes * needs to be word aligned */
    int32_t imm19 = (offset / 4) & 0x7FFFF;
     return (1u << 31) |    /* 64-bit */
         (0xB4u << 24) | /* CBZ opcode */
         (imm19 << 5) |  /* 19-bit offset */
         rt;             /* test register */
}

uint32_t encode_cbnz(int rt, int32_t offset)
{
    int32_t imm19 = (offset / 4) & 0x7FFFF;
    return (1u << 31) |    /* 64-bit */
            (0xB5u << 24) | /* CBNZ opcode */
            (imm19 << 5) |  /* 19-bit offset */
            rt;             /* test register */
}

uint32_t encode_b(int32_t offset) 
{
    /* offset is in bytes * needs to be word aligned */
    int32_t imm26 = (offset / 4) & 0x3FFFFFF;
    return  (0x5u << 26) | /* B opcode */
            imm26;         /* 26-bit offset */
}

uint32_t encode_svc(uint16_t imm) 
{
    return (0xD4u << 24) |         /* SVC opcode */
            ((imm & 0xFFFF) << 5) | /* 16-bit immediate */
            0x1;                    /* fixed bits */
}

uint32_t encode_movk(int rd, uint16_t imm, int shift) 
{
    return  (1u << 31) |    /* 64-bit */
            (0x3u << 29) |  /* MOVK opcode */
            (0x25u << 23) | /* fixed bits */
            (shift << 21) | /* shift amount */
            (imm << 5) |    /* 16-bit immediate */
            rd;             /* destination register */
}

uint32_t encode_ret() 
{ 
    return 0xD65F03C0; /* RET instruction */ 
}

uint32_t encode_stur(int rt, int rn, int offset) 
{
    return (0x3u << 30) |             /* size=64-bit */
            (0x7u << 27) |             /* Load/Store opcode */
            (0x0u << 22) |             /* Store (not load) */
            ((offset & 0x1FF) << 12) | /* 9-bit immediate offset */
            (0x2u << 10) |             /* unscaled variant */
            (rn << 5) |                /* base register */
            rt;                        /* target register */
}

uint32_t encode_ldur(int rt, int rn, int offset) 
{
    return (0x3u << 30) |             /* size=64-bit */
            (0x7u << 27) |             /* Load/Store opcode */
            (0x1u << 22) |             /* Load (not store) */
            ((offset & 0x1FF) << 12) | /* 9-bit immediate offset */
            (0x2u << 10) |             /* unscaled variant */
            (rn << 5) |                /* base register */
            rt;                        /* target register */
}

uint32_t encode_stp(int rt, int rt2, int rn, int imm) 
{
    int opc = 2; /* for 64-bit regs */
    
    /* conv byte offset to register pair aligned offset (divide by 8) */
    int imm7 = (imm / 8) & 0x7F;
    
    return (opc << 30) |        /* opc=2 for 64-bit */
           (0xA4 << 22) |       /* fixed pattern */
           ((imm7 & 0x7F) << 15) | /* 7-bit immediate */
           (rt2 << 10) |        /* second register */
           (rn << 5) |          /* base register */
           rt;                  /* first register */
}

uint32_t encode_ldp(int rt, int rt2, int rn, int imm) 
{
    int opc = 2;
    int imm7 = (imm / 8) & 0x7F;
    
    return (opc << 30) |        /* opc=2 for 64-bit */
           (0xA5 << 22) |       /* fixed pattern */
           ((imm7 & 0x7F) << 15) | /* 7-bit immediate */
           (rt2 << 10) |        /* second register */
           (rn << 5) |          /* base register */
           rt;                  /* first register */
}

uint32_t encode_stp_pre(int rt, int rt2, int rn, int imm) 
{
    int opc = 2;
    int op2 = 3;
    int L = 0;
    int imm7 = (imm / 8) & 0x7F;

    return (opc << 30) |        /* opc=2 for 64-bit */
           (0xA << 24) |        /* fixed pattern */
           (op2 << 22) |        /* op2=3 for pre-indexed */
           (L << 21) |          /* L=0 for store */
           ((imm7 & 0x7F) << 15) | /* 7-bit immediate */
           (rt2 << 10) |        /* second register */
           (rn << 5) |          /* base register */
           rt;                  /* first register */
}

uint32_t encode_ldp_post(int rt, int rt2, int rn, int imm) 
{
    int opc = 2;
    int op2 = 1;
    int L = 1;
    int imm7 = (imm / 8) & 0x7F;
    
    return (opc << 30) |        /* opc=2 for 64-bit */
           (0xA << 24) |        /* fixed pattern */
           (op2 << 22) |        /* op2=1 for post-indexed */
           (L << 21) |          /* L=1 for load */
           ((imm7 & 0x7F) << 15) | /* 7-bit immediate */
           (rt2 << 10) |        /* second register */
           (rn << 5) |          /* base register */
           rt;                  /* first register */
}