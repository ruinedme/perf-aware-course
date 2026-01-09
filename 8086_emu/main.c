#include <stdio.h>
#include <stdlib.h>

#define BITS 16

typedef unsigned char u8;
typedef unsigned short u16;

// MODE BIT MASK
const u8 MODE_MASK = 0xc0;   // 11000000
const u8 REG_OP_MASK = 0x38; // 00111000
const u8 RM_MASK = 0x07;     // 00000111

// Register tables
const char *BYTE_REGISTERS[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char *WORD_REGISTERS[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

// R/M table
const char *RM_REGISTERS[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

// Segment register table
const char *SEGMENT_REGISTERS[4] = {"es", "cs", "ss", "ds"};

// operator tables
const char *IMM_OPS[8] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
const char *SHIFT_OPS[8] = {"rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar"};
const char *GROUP1[8] = {"test", NULL, "not", "neg", "mul", "imul", "div", "idiv"};
const char *GROUP2[8] = {"inc", "dec", "call", "call", "jmp", "jmp", "push", NULL};

// Mnemonic table
const char *MNEMONIC[256] = {
    "add", "add", "add", "add", "add", "add", "push", "pop", "or", "or", "or", "or", "or", "or", "push", NULL,
    "adc", "adc", "adc", "adc", "adc", "adc", "push", "pop", "sbb", "sbb", "sbb", "sbb", "sbb", "sbb", "push", "pop",
    "and", "and", "and", "and", "and", "and", "es:", "daa", "sub", "sub", "sub", "sub", "sub", "sub", "cs:", "das",
    "xor", "xor", "xor", "xor", "xor", "xor", "ss:", "aaa", "cmp", "cmp", "cmp", "cmp", "cmp", "cmp", "ds:", "aas",
    "inc", "inc", "inc", "inc", "inc", "inc", "inc", "inc", "dec", "dec", "dec", "dec", "dec", "dec", "dec", "dec",
    "push", "push", "push", "push", "push", "push", "push", "push", "pop", "pop", "pop", "pop", "pop", "pop", "pop", "pop",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "jo", "jno", "jb", "jnb", "je", "jne", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle",
    NULL, NULL, NULL, NULL, "test", "test", "xchg", "xchg", "mov", "mov", "mov", "mov", "mov", "lea", "mov", "pop",
    "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "cbw", "cwd", "call", "wait", "pushf", "popf", "sahf", "lahf",
    "mov", "mov", "mov", "mov", "movs", "movs", "cmps", "cmps", "test", "test", "stos", "stos", "lods", "lods", "scas", "scas",
    "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",
    NULL, NULL, "ret", "ret", "les", "lds", "mov", "mov", NULL, NULL, "retf", "retf", "int3", "int", "into", "iret",
    NULL, NULL, NULL, NULL, "aam", "aad", NULL, "xlat", "esc", "esc", "esc", "esc", "esc", "esc", "esc", "esc",
    "loopnz", "loopz", "loop", "jcxz", "in", "in", "out", "out", "call", "jmp", "jmp", "jmp", "in", "in", "out", "out",
    "lock", NULL, "rep", "rep", "hlt", "cmc", NULL, NULL, "clc", "stc", "cli", "sti", "cld", "std", NULL, NULL};

typedef struct
{
    const char *buf; // The buffered program
    size_t len;      // The length of the program in bytes
    size_t pos;      // The position of the parser
} Prog;

typedef struct
{
    u8 mod;
    u8 reg;
    u8 rm;
} Mod_Reg_RM;

static inline u8 abs_u8(u8 value)
{
    return (value & 0x80) == 0x80 ? ~value + 1 : value;
}

static inline u16 abs_u16(u16 value)
{
    return ((value & 0x8000) == 0x8000) ? ~value + 1 : value;
}

/// @brief Advance pos field of prog and return the next byte in program buffer
/// @param prog
/// @return The next byte
static u8 consume(Prog *prog)
{
    return (u8)prog->buf[++prog->pos];
}

static u8 peek_offset(Prog *prog, size_t offset)
{
    return prog->buf[prog->pos + offset];
}

/// @brief Returns the current byte in the buffer without consuming it
/// @param prog
/// @return The current byte at pos
static u8 peek(Prog *prog)
{
    return prog->buf[prog->pos];
}

/// @brief Consumes 2 bytes in the buffer and returns an unsigned short
/// @param prog
/// @return
static u16 consumeShort(Prog *prog)
{
    u8 disp_lo = consume(prog);
    u16 disp_hi = consume(prog);
    u16 value = (disp_hi << 8) | disp_lo;
    return value;
}

static void get_mod_reg_rm(Mod_Reg_RM *dest, Prog *prog)
{
    u8 op = consume(prog);
    dest->mod = (op & MODE_MASK) >> 6;
    dest->reg = (op & REG_OP_MASK) >> 3;
    dest->rm = (op & RM_MASK);
}

static inline u8 mod_reg_rm_to_u8(Mod_Reg_RM *op)
{
    return (u8)((op->mod << 6) | (op->reg << 3) | (op->rm));
}

static void print_sign_extended_data(Prog *prog, u8 sw)
{
    u16 data = 0;
    switch (sw)
    {
    case 0:
    {
        // 8-bit unsigned
        data = consume(prog);
        printf("%hhu", data);
    }
    break;
    case 1:
    {
        // 16 bit signed
        data = consumeShort(prog);
        printf("%hd", data);
    }
    break;
    case 2:
    {
        // sign extended 8 bit
        data = consume(prog);
        printf("%hhd", data);
    }
    break;

    case 3:
    {
        // 8 bit signed
        data = consume(prog);
        printf("%hhd", data);
    }
    break;
    }
}

static void single_byte_w_ops(u8 op)
{
    u8 wide = (op & 1);
    if (wide)
    {
        switch (op)
        {
        case 0xed: // IN variable
            printf("%s ax, dx\n", MNEMONIC[op]);
            break;
        case 0xef: // OUT variable
            printf("%s dx, ax\n", MNEMONIC[op]);
            break;
        case 0xa5:
        case 0xa7:
        case 0xab:
        case 0xad:
        case 0xaf:
            printf("%sw\n", MNEMONIC[op]);
            break;
        default:
            printf("Invalid op in single_byte_w_ops %x\n", op);
        }
    }
    else
    {
        switch (op)
        {
        case 0xec:
            printf("%s ax, dl\n", MNEMONIC[op]);
            break;
        case 0xee:
            printf("%s dx, al\n", MNEMONIC[op]);
            break;
        case 0xa4:
        case 0xa6:
        case 0xaa:
        case 0xac:
        case 0xae:
            printf("%sb\n", MNEMONIC[op]);
            break;
        default:
            printf("Invalid op in single_byte_w_ops %x\n", op);
        }
    }
}

/// @brief Displays the ASM of the next consumed OP
/// @param prog
static void displayInstruction(Prog *prog)
{
    u8 op1 = peek(prog);
    Mod_Reg_RM op2 = {0};
    char *seg_reg = "";

    // Segment register access
    if (op1 == 0x26 || op1 == 0x36 || op1 == 0x2e || op1 == 0x3e)
    {
        seg_reg = MNEMONIC[op1];
        op1 = consume(prog);
    }

    switch (op1)
    {
    // Single byte ops
    case 0xf3:
    case 0xf0:
        printf("%s ", MNEMONIC[op1]); // Note: No new line
        break;
    case 0x27:
    case 0x2f:
    case 0x37:
    case 0x3f:
    case 0x98:
    case 0x99:
    case 0x9b:
    case 0x9c:
    case 0x9d:
    case 0x9e:
    case 0x9f:
    case 0xc3:
    case 0xcb:
    case 0xcc:
    case 0xce:
    case 0xcf:
    case 0xd4:
    case 0xd5:
    case 0xd7:
    case 0xf4:
    case 0xf5:
    case 0xf8:
    case 0xf9:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfd:
    {
        printf("%s\n", MNEMONIC[op1]);
        if ((op1 == 0xd4 || op1 == 0xd5))
        {
            if (peek_offset(prog, 1) == (u8)0x0a)
            {
                consume(prog);
            }
            else
            {
                printf("AAD/AAM displacement not supported %zu: %x %x\n", prog->pos, op1, peek_offset(prog, 1));
                exit(1);
            }
        }
    }
    break;
    // Single byte variable ops XXXXXYYY
    // PUSH/POP/XCHG/INC
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b:
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5d:
    case 0x5e:
    case 0x5f:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    {
        u8 reg = (op1 & 7);
        printf("%s ", MNEMONIC[op1]);
        // exchange reg with acc
        if (op1 >= 0x90 && op1 <= 0x97)
        {
            printf("ax, ");
        }

        printf("%s\n", WORD_REGISTERS[reg]);
    }
    break;
    // Single byte variable OPs XXXXXXXW
    // IN/OUT variable
    case 0xec:
    case 0xed:
    case 0xee:
    case 0xef:
    // MOVS
    case 0xa4:
    case 0xa5:
    // CMPS
    case 0xa6:
    case 0xa7:
    // STOS
    case 0xaa:
    case 0xab:
    // LODS
    case 0xac:
    case 0xad:
    // SCAS
    case 0xae:
    case 0xaf:
        single_byte_w_ops(op1);
        break;
    // Single byte segment register ops XXXYYXXX
    // PUSH/POP
    case 0x6:
    case 0x7:
    case 0xe:
    case 0x16:
    case 0x17:
    case 0x1e:
    case 0x1f:
    {
        u8 sr = (op1 & 0x18) >> 3;
        printf("%s %s\n", MNEMONIC[op1], SEGMENT_REGISTERS[sr]);
    }
    break;
    // 2-4 byte rm to/from register ops XXXXXXDW YYREGRM AAAAAAAA BBBBBBBB
    // MOV/ADD/ADC/SUB/SSB/CMP/AND/OR/XOR
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x88:
    case 0x89:
    case 0x8a:
    case 0x8b:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    {
        u8 dir = (op1 & 2) >> 1;
        u8 wide = (op1 & 1);
        get_mod_reg_rm(&op2, prog);

        const char *reg_str = wide == 0 ? BYTE_REGISTERS[op2.reg] : WORD_REGISTERS[op2.reg];

        u8 disp_lo = 0;
        u16 value = 0;
        printf("%s ", MNEMONIC[op1]);
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                // direct memory addressing 16 displacement
                value = consumeShort(prog);
                // 10001001 00 011 110 00011100 00000000 ; mov [28], bx
                // 10001011 00 011 110 00011100 00000000 ; mov bx, [28]
                if (dir)
                {
                    printf("%s, %s[%hd]\n", reg_str, seg_reg, value);
                }
                else
                {
                    printf("%s[%d], %s\n", seg_reg, value, reg_str);
                }
            }
            else
            {
                // Memory mode no displacement
                const char *rm_str = RM_REGISTERS[op2.rm];
                if (dir)
                {
                    printf("%s, %s[%s]\n", reg_str, seg_reg, rm_str);
                }
                else
                {
                    printf("%s[%s], %s\n", seg_reg, rm_str, reg_str);
                }
            }
        }
        break;
        case 1:
        {
            // 10001011 01000001 11011011
            disp_lo = consume(prog);
            const char *sign = (disp_lo & 0x80) == 0x80 ? "-" : "+";
            const char *rm_str = RM_REGISTERS[op2.rm];
            disp_lo = abs_u8(disp_lo);
            if (disp_lo == 0 && dir)
            {
                printf("%s, %s[%s]\n", reg_str, seg_reg, rm_str);
            }
            else if (disp_lo == 0 && dir == 0)
            {
                printf("%s[%s], %s\n", seg_reg, rm_str, reg_str);
            }
            else if (dir)
            {
                printf("%s, %s[%s %s %hhu]\n", reg_str, seg_reg, rm_str, sign, disp_lo);
            }
            else
            {
                printf("%s[%s %s %hhu], %s\n", seg_reg, rm_str, sign, disp_lo, reg_str);
            }
        }
        break;
        case 2:
        {
            // 10001010 10 000 000 10000111 00010011
            value = consumeShort(prog);
            const char *sign = (value & 0x8000) == 0x8000 ? "-" : "+";
            const char *rm_str = RM_REGISTERS[op2.rm];
            value = abs_u16(value);
            if (value == 0 && dir)
            {
                printf("%s, %s[%s]\n", reg_str, seg_reg, rm_str);
            }
            else if (value == 0 && dir == 0)
            {
                printf("%s[%s], %s\n", seg_reg, rm_str, reg_str);
            }
            else if (dir)
            {
                printf("%s, %s[%s %s %hu]\n", reg_str, seg_reg, rm_str, sign, value);
            }
            else
            {

                printf("%s[%s %s %hu], %s\n", seg_reg, rm_str, sign, value, reg_str);
            }
        }
        break;
        case 3:
        {
            // Register mode no displacement
            const char *rm_str = wide == 0 ? BYTE_REGISTERS[op2.rm] : WORD_REGISTERS[op2.rm];
            if (dir)
            {
                printf("%s, %s\n", reg_str, rm_str);
            }
            else
            {
                printf("%s, %s\n", rm_str, reg_str);
            }
        }
        }
    }
    break;
    // 2-6 byte imm to rm XXXXXXXW AAYYYBBB LLLLLLLL HHHHHHHH CCCCCCCC DDDDDDDD
    // MOV
    case 0xc6:
    case 0xc7:
    {
        // 11000111 01000000 00001000 00101100 00000001
        u8 wide = (op1 & 1);
        get_mod_reg_rm(&op2, prog);
        if (op2.reg != 0)
        {
            printf("expected reg to be 00, got %02x at pos: %zu, %x %x\n", op2.reg, prog->pos, op1, op2);
            exit(1);
        }
        printf("%s ", MNEMONIC[op1]);
        u16 disp = 0;
        u16 data = 0;
        const char *byte_or_word = wide == 0 ? "byte" : "word";
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                // direct memory addressing
                // 11000111 00000110 11101000 00000011 00101100 00000001
                disp = consumeShort(prog);

                data = consumeShort(prog);
                printf("[%hd], %s %hd\n", disp, byte_or_word, data);
            }
            else
            {
                // memory mode no displacement
                // 11000111 00000100 00101100 00000001
                // 11000110 00000111 11111000
                const char *rm_str = RM_REGISTERS[op2.rm];
                data = wide == 0 ? consume(prog) : consumeShort(prog);
                if (wide == 0)
                {
                    printf("[%s], %s %hhd\n", rm_str, byte_or_word, (u8)data);
                }
                else
                {
                    printf("[%s], %s %hd\n", rm_str, byte_or_word, data);
                }
            }
        }
        break;
        case 1:
        {
            disp = consume(prog);
            data = wide == 0 ? consume(prog) : consumeShort(prog);
            const char *rm_str = RM_REGISTERS[op2.rm];
            const char *sign = (disp & 0x80) == 0x80 ? "-" : "+";
            disp = abs_u16(disp);
            printf("[%s %s %hhu], %s %hd\n", rm_str, sign, (u8)disp, byte_or_word, data);
        }
        break;
        case 2:
        {
            disp = consumeShort(prog);
            data = wide == 0 ? consume(prog) : consumeShort(prog);
            const char *rm_str = RM_REGISTERS[op2.rm];
            const char *sign = (disp & 0x8000) == 0x8000 ? "-" : "+";
            disp = abs_u16(disp);
            printf("[%s %s %hu], %s %hd\n", rm_str, sign, disp, byte_or_word, data);
        }
        break;
        case 3:
        {
            printf("Mode 3 not implemented for MOV IMM to reg\n");
            exit(1);
        }
        break;
        }
    }
    break;
    // 2-3 byte imm to reg XXXXWREG YYYYYYYY ZZZZZZZZ
    case 0xb0:
    case 0xb1:
    case 0xb2:
    case 0xb3:
    case 0xb4:
    case 0xb5:
    case 0xb6:
    case 0xb7:
    case 0xb8:
    case 0xb9:
    case 0xba:
    case 0xbb:
    case 0xbc:
    case 0xbd:
    case 0xbe:
    case 0xbf:
    {
        u8 wide = (op1 & 8) >> 3;
        u8 reg = (op1 & 7);
        u16 data = wide == 0 ? consume(prog) : consumeShort(prog);
        const char *byte_or_word = wide == 0 ? "byte" : "word";
        const char *reg_str = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
        printf("%s %s, %s ", MNEMONIC[op1], reg_str, byte_or_word);
        if (wide == 0)
        {
            printf("%hhd\n", (u8)data);
        }
        else
        {
            printf("%hd\n", data);
        }
    }
    break;
    // 3 bytes mov acc to mem/ mem to acc
    case 0xa0:
    case 0xa1:
    case 0xa2:
    case 0xa3:
    {
        u16 addr = consumeShort(prog);
        u8 wide = (op1 & 1);
        u8 dir = (op1 & 2) >> 1;
        printf("%s ", MNEMONIC[op1]);
        if (wide == 0)
        {
            // al
            if (dir == 0)
            {
                printf("al [%hu]\n", addr);
            }
            else
            {
                printf("[%hu], al\n", addr);
            }
        }
        else
        {
            // ax
            if (dir == 0)
            {
                printf("ax, [%hu]\n", addr);
            }
            else
            {
                printf("[%hu], ax\n", addr);
            }
        }
    }
    break;
    // imm to rm 3-6 bytes ADD/OR/ADC/SBB/AND/SUB/XOR/CMP
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    {
        u8 wide = (op1 & 1);
        u8 sw = (op1 & 3);
        get_mod_reg_rm(&op2, prog);
        u16 disp = 0;
        // const char *rm_str = WORD_REGISTERS[op2.rm];
        const char *rm_str = RM_REGISTERS[op2.rm];
        const char *byte_or_word = wide == 0 ? "byte" : "word";
        printf("%s ", IMM_OPS[op2.reg]);
        // 10000001 01000000 00001000 00101100 00000001 ; add [bx + si + 8], word 300
        // 10000011 00000110 11101000 00000011 11111110 ; add [1000], word -2
        // 10000011 11000110 00000101 ; add si, 5
        // 10000000011001101101100111101111 ; and byte [bp - 39], 239
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                disp = consumeShort(prog);
                printf("%s[%hu], %s ", seg_reg, disp, byte_or_word);
            }
            else
            {
                printf("%s[%s], %s ", seg_reg, rm_str, byte_or_word);
            }
        }
        break;
        case 1:
        {
            disp = consume(prog);
            const char *sign = (disp & 0x80) == 0x80 ? "-" : "+";
            disp = abs_u8(disp);
            printf("%s[%s %s %hhu], %s ", seg_reg, rm_str, sign, disp, byte_or_word);
        }
        break;
        case 2:
        {
            disp = consumeShort(prog);
            const char *sign = (disp & 0x8000) == 0x8000 ? "-" : "+";
            disp = abs_u16(disp);
            // 10000001 10 100 000 00010100 11101111 01011000 00101000
            printf("%s[%s %s %hu], %s ", seg_reg, rm_str, sign, disp, byte_or_word);
        }
        break;
        case 3:
        {
            printf("%s, %s ", rm_str, byte_or_word);
        }
        break;
        }
        print_sign_extended_data(prog, sw);
        printf("\n");
    }
    break;
    // 2-4 bytes mov segment register to/from
    case 0x8c:
    case 0x8e:
    {
        // 10001110 10000111 00101100 00000001
        get_mod_reg_rm(&op2, prog);
        u8 dir = (op1 & 2) >> 1;
        if (op2.reg > 4)
        {
            printf("Invalid op at pos %zu: %x %x\n", prog->pos, op1, op2);
        }
        const char *sr_str = SEGMENT_REGISTERS[op2.reg];
        const char *rm_str = RM_REGISTERS[op2.rm];
        u16 value = 0;
        printf("%s ", MNEMONIC[op1]);
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                // direct addressing 16 bit displacement
                value = consumeShort(prog);
                if (dir == 0)
                {
                    printf("%s[%hu], %s\n", seg_reg, value, sr_str);
                }
                else
                {
                    printf("%s, %s[%hu]\n", sr_str, seg_reg, value);
                }
            }
            else
            {
                // memory mode no displacement
                if (dir == 0)
                {
                    printf("%s[%s], %s\n", seg_reg, rm_str, sr_str);
                }
                else
                {
                    printf("%s, %s[%s]\n", sr_str, seg_reg, rm_str);
                }
            }
        }
        break;
        case 1:
        {
            value = consume(prog);
            const char *sign = (value & 0x80) == 0x80 ? "-" : "+";
            value = (value & 0x80) == 0x80 ? ~value + 1 : value;

            if (dir == 0)
            {
                printf("%s[%s %s %hhu], %s\n", seg_reg, rm_str, sign, value, sr_str);
            }
            else
            {
                printf("%s, %s[%s %s %hhu]\n", seg_reg, sr_str, rm_str, sign, value);
            }
        }
        break;
        case 2:
        {
            value = consumeShort(prog);
            const char *sign = (value & 0x8000) == 0x8000 ? "-" : "+";
            value = (value & 0x8000) == 0x8000 ? ~value + 1 : value;
            if (dir == 0)
            {
                printf("%s[%s %s %hu], %s\n", seg_reg, rm_str, sign, value, sr_str);
            }
            else
            {
                printf("%s, %s[%s %s %hu]\n", sr_str, seg_reg, rm_str, sign, value);
            }
        }
        break;
        case 3:
        {
            printf("mov sr to/from reg mod 3 is not implemented at pos %zu: %x %x\n", prog->pos, op1, op2);
            exit(1);
        }
        break;
        }
    }
    break;
    // pop rm 2-4 bytes
    case 0x8f:
    {
        get_mod_reg_rm(&op2, prog);
        if (op2.reg != 0)
        {
            printf("Invalid op at pos %zu: %x %x\n", prog->pos, op1, op2);
            exit(1);
        }
        u16 disp = 0;
        const char *rm_str = RM_REGISTERS[op2.rm];
        printf("pop word ");
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                disp = consumeShort(prog);
                printf("[%hu]\n", disp);
            }
            else
            {
                printf("[%s]\n", rm_str);
            }
        }
        break;
        case 1:
        {
            disp = consume(prog);
            const char *sign = (disp & 0x80) == 0x80 ? "-" : "+";
            printf("[%s %s %hhu]\n", rm_str, sign, abs_u8(disp));
        }
        break;
        case 2:
        {
            disp = consumeShort(prog);
            const char *sign = (disp & 0x8000) == 0x8000 ? "-" : "+";
            printf("[%s %s %hu]\n", rm_str, sign, abs_u16(disp));
        }
        break;
        case 3:
        {
            printf("pop rm mod 3 not implemented at pos %zu: %x %x\n", prog->pos, op1, op2);
            exit(1);
        }
        break;
        }
    }
    break;
    // in/out fixed
    case 0xe4:
    case 0xe5:
    case 0xe6:
    case 0xe7:
    {
        u8 wide = (op1 & 1);
        u8 data = consume(prog);
        char reg[3] = {0};
        reg[0] = (op1 & 7) > 5 ? 'd' : 'a';
        reg[1] = wide == 0 ? 'l' : 'x';
        printf("%s ", MNEMONIC[op1]);
        if ((op1 & 7) > 5)
        {
            printf("%hhu, %s\n", data, reg);
        }
        else
        {
            printf("%s, %hhu\n", reg, data);
        }
    }
    break;
    // 2-4 byte INC/DEC/CALL/JMP/PUSH
    case 0xfe:
    case 0xff:
    {
        get_mod_reg_rm(&op2, prog);
        u8 wide = (op1 & 1);
        u16 value = 0;
        if (!GROUP2[op2.reg])
        {
            printf("Invalid op at pos %zu: %x %x\n", prog->pos, op1, op2);
            exit(1);
        }
        char *byte_or_word = NULL;
        // 00 100 101
        if(op2.reg == 3 || op2.reg == 5){
            byte_or_word = "far";
        }else if(op2.reg == 2 || op2.reg == 4){
            byte_or_word = "near";
        }
        else{
            byte_or_word = wide == 0 ? "byte" : "word";
        }
        const char *rm_str = RM_REGISTERS[op2.rm];
        printf("%s ", GROUP2[op2.reg]);
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                value = consumeShort(prog);
                printf("%s %s[%hu]\n", byte_or_word, seg_reg, value);
            }
            else
            {
                printf("%s %s[%hs]\n", byte_or_word, seg_reg, rm_str);
            }
        }
        break;
        case 1:
        {
            value = consume(prog);
            const char *sign = (value & 0x80) == 0x80 ? "-" : "+";
            value = abs_u8(value);
            printf("%s %s[%hs %s %hhu]\n", byte_or_word, seg_reg, rm_str, sign, value);
        }
        break;
        case 2:
        {
            // 11111111 10000011 11000100 11011000
            value = consumeShort(prog);
            const char *sign = (value & 0x8000) == 0x8000 ? "-" : "+";
            value = abs_u16(value);
            printf("%s %s[%hs %s %hu]\n", byte_or_word, seg_reg, rm_str, sign, value);
        }
        break;
        case 3:
        {
            const char *reg_str = wide == 0 ? BYTE_REGISTERS[op2.rm] : WORD_REGISTERS[op2.rm];
            printf("%s\n", reg_str);
        }
        break;
        }
    }
    break;
    // 2-4 byte LEA/LDS/LES
    case 0x8d:
    case 0xc4:
    case 0xc5:
    {
        get_mod_reg_rm(&op2, prog);
        u16 value = 0;
        const char *reg_str = WORD_REGISTERS[op2.reg];
        const char *rm_str = RM_REGISTERS[op2.rm];

        printf("%s %s, ", MNEMONIC[op1], reg_str);
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                value = consumeShort(prog);
                printf("%s[%hu]\n", seg_reg, value);
            }
            else
            {
                printf("%s[%s]\n", seg_reg, rm_str);
            }
        }
        break;
        case 1:
        {
            value = consume(prog);
            const char *sign = (value & 0x80) == 0x80 ? "-" : "+";
            value = abs_u8(value);
            printf("%s[%s %s %hhu]\n", seg_reg, rm_str, sign, value);
        }
        break;
        case 2:
        {
            value = consumeShort(prog);
            const char *sign = (value & 0x8000) == 0x8000 ? "-" : "+";
            value = abs_u16(value);
            printf("%s[%s %s %hu]\n", seg_reg, rm_str, sign, value);
        }
        break;
        case 3:
            printf("Invalid LEA/LES/LDS at %zu: %x %x\n", prog->pos, op1, mod_reg_rm_to_u8(&op2));
            exit(1);
            break;
        }
    }
    break;
    // 2-3 byte imm to acc ADD/ADC/AND/XOR/OR/SBB/SUB/CMP/TEST
    case 0x4:
    case 0x5:
    case 0x14:
    case 0x15:
    case 0x24:
    case 0x25:
    case 0x34:
    case 0x35:
    case 0x0c:
    case 0x0d:
    case 0x1c:
    case 0x1d:
    case 0x2c:
    case 0x2d:
    case 0x3c:
    case 0x3d:
    case 0xa9:
    {
        u8 wide = (op1 & 1);
        u16 data = 0;
        data = wide == 0 ? consume(prog) : consumeShort(prog);
        char reg[3] = {'a', 0, 0};
        reg[1] = wide == 0 ? 'l' : 'x';
        if (wide == 0)
        {
            printf("%s %s, %hhd\n", MNEMONIC[op1], reg, data);
        }
        else
        {
            printf("%s %s, %hd\n", MNEMONIC[op1], reg, data);
        }
    }
    break;
    // 3-6 byte imm to reg TEST/NOT/NEG/MUL/IMUL/DIV/IDIV
    case 0xf6:
    case 0xf7:
    {
        u8 wide = (op1 & 1);
        get_mod_reg_rm(&op2, prog);

        if (op2.reg == 1)
        {
            printf("Invalid op at pos %zu: %x %x\n", prog->pos, op1, mod_reg_rm_to_u8(&op2));
            exit(1);
        }

        const char *byte_or_word = wide == 0 ? "byte" : "word";
        const char *rm_str = RM_REGISTERS[op2.rm];
        u16 disp = 0;
        u16 data = 0;
        printf("%s ", GROUP1[op2.reg]);
        // 11110110 11 00 0011 00010100
        switch (op2.mod)
        {
        case 0:
        {
            // 11110110 00 101 111
            if (op2.rm == 6)
            {
                disp = consumeShort(prog);
                printf("%s %s[%hd]\n", byte_or_word, seg_reg, disp);
            }
            else
            {
                if (op2.reg == 0)
                {
                    data = wide == 0 ? consume(prog) : consumeShort(prog);
                    printf("%s %s[%s], %hd\n", byte_or_word, seg_reg, rm_str, data);
                }
                else
                {
                    printf("%s %s[%s]\n", byte_or_word, seg_reg, rm_str);
                }
            }
        }
        break;
        case 1:
        {
            disp = consume(prog);
            const char *sign = (disp & 0x80) == 0x80 ? "-" : "+";
            disp = abs_u8(disp);
            if (op2.reg == 0)
            {
                data = wide == 0 ? consume(prog) : consumeShort(prog);
                printf("%s %s[%s %s %hhu], %hd\n", byte_or_word, seg_reg, rm_str, sign, disp, data);
            }
            else
            {
                printf("%s %s[%s %s %hhu]\n", byte_or_word, seg_reg, rm_str, sign, disp);
            }
        }
        break;
        case 2:
        {
            disp = consumeShort(prog);
            const char *sign = (disp & 0x8000) == 0x8000 ? "-" : "+";
            disp = abs_u16(disp);
            if (op2.reg == 0)
            {
                data = wide == 0 ? consume(prog) : consumeShort(prog);
                printf("%s %s[%s %s %hu], %hd\n", byte_or_word, seg_reg, rm_str, sign, disp, data);
            }
            else
            {
                printf("%s %s[%s %s %hu]\n", byte_or_word, seg_reg, rm_str, sign, disp);
            }
        }
        break;
        case 3:
        {
            const char *reg_str = wide == 0 ? BYTE_REGISTERS[op2.rm] : WORD_REGISTERS[op2.rm];
            if (op2.reg == 0)
            {
                data = wide == 0 ? consume(prog) : consumeShort(prog);
                printf("%s, %hd\n", reg_str, data);
            }
            else
            {
                printf("%s\n", reg_str);
            }
        }
        break;
        }
    }
    break;
    // 2-4 byte shift ops ROL/ROR/RCL/RCR/SHL/SHR/SAR
    case 0xd0:
    case 0xd1:
    case 0xd2:
    case 0xd3:
    {
        get_mod_reg_rm(&op2, prog);
        u8 wide = (op1 & 1);
        u8 variable = (op1 & 2) >> 1;
        // u8 variable = (op1 & 2)>>1;
        // 11010000 11 100 100 -- shl ah, 1
        // 11010010 11 100 100 -- shl ah, cl
        if (op2.reg == 6)
        {
            printf("Inavlid op at pos %zu: %x %x\n", prog->pos, op1, mod_reg_rm_to_u8(&op2));
            exit(1);
        }
        const char *byte_or_word = wide == 0 ? "byte" : "word";
        const char *rm_str = RM_REGISTERS[op2.rm];
        u16 disp = 0;
        printf("%s ", SHIFT_OPS[op2.reg]);
        const char *src_str = variable == 0 ? "1" : "cl";
        switch (op2.mod)
        {
        case 0:
        {
            if (op2.rm == 6)
            {
                disp = consumeShort(prog);
                // 11010010 00 001 110 01001010 00010011 ror byte [4938], cl
                printf("%s %s[%hu], %s\n", byte_or_word, seg_reg, disp, src_str);
            }
            else
            {
                printf("%s %S[%s], %s\n", byte_or_word, seg_reg, rm_str, src_str);
            }
        }
        break;
        case 1:
        {
            disp = consume(prog);
            const char *sign = (disp & 0x80) == 0x80 ? "-" : "+";
            disp = abs_u8(disp);
            printf("%s %s[%s %s %hhu], %s\n", byte_or_word, seg_reg, rm_str, sign, disp, src_str);
        }
        break;
        case 2:
        {
            disp = consumeShort(prog);
            const char *sign = (disp & 0x8000) == 0x8000 ? "-" : "+";
            disp = abs_u16(disp);
            printf("%s %s[%s %s %hu], %s\n", byte_or_word, seg_reg, rm_str, sign, disp, src_str);
        }
        break;
        case 3:
        {
            const char *reg_str = wide == 0 ? BYTE_REGISTERS[op2.rm] : WORD_REGISTERS[op2.rm];
            printf("%s, %s\n", reg_str, src_str);
        }
        break;
        }
    }
    break;
    // 3 byte Retrun within seg adding imm to sp
    case 0xc2:
    case 0xca:
    {
        u16 data = consumeShort(prog);
        printf("%s %hd\n", MNEMONIC[op1], data);
    }
    break;
    // 2 byte conditional jumps
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7a:
    case 0x7b:
    case 0x7c:
    case 0x7d:
    case 0x7e:
    case 0x7f:
    case 0xe0:
    case 0xe1:
    case 0xe2:
    case 0xe3:
    case 0xcd:
    {
        u8 offset = consume(prog);
        if (op1 == 0xcd)
        {
            printf("%s %hhu\n", MNEMONIC[op1], offset);
        }
        else
        {
            printf("%s $+2+%hhd\n", MNEMONIC[op1], offset);
        }
    }
    break;
    // 3 byte call/jmp/retf direct
    case 0xe8:
    case 0xe9: {
        // 10011010 11001000 00000001 01111011 00000000 ; call 123:456 call imm direct
        u16 value = consumeShort(prog);
        // Need to multiply type by 4? what is type?
        // From what i can tell the direct value of call needs to be divisible by 4
        // Increment value to the nearest number that is divisible by 4
        // This works for the examples, no idea if this is actually correct tho.
        value = (value + 4) - (value % 4);
        printf("%s %hd\n", MNEMONIC[op1], value);
    }break;
    // 5 byte call/jmp direct intersegment 
    case 0x9a:
    case 0xea:{
        u16 ip = consumeShort(prog);
        u16 cs = consumeShort(prog);
        printf("%s %hd:%hd\n", MNEMONIC[op1], cs, ip);
    }break;
    default:
        printf("Invalid op at %zu: %x\n", prog->pos, op1);
        exit(1);
    }

    // bump to the next op
    consume(prog);
}

static void disassemble(Prog *prog)
{
    while (prog->pos < prog->len)
    {
        displayInstruction(prog);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return printf("Usage: %s path/to/binary\n", argv[0]);

    const char *path = argv[1];
    char buf[4096] = {0};
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    size_t bytes_read = fread(buf, 1, 4096, f);
    if (bytes_read == 0)
    {
        fclose(f);
        printf("Could not read file\n");
        perror("fread");
        return 1;
    }
    // printf("Bytes read: %zu\n", bytes_read);

    fclose(f);

    Prog prog = {
        .buf = buf,
        .len = bytes_read,
        .pos = 0};

    disassemble(&prog);

    return 0;
}
