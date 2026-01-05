#include <stdio.h>
#include <stdlib.h>

#define BITS 16

// OP CODE BIT MASKS
const unsigned char OP_4_BIT_MASK = 0xf0;
const unsigned char OP_6_BIT_MASK = 0xfc;
const unsigned char OP_7_BIT_MASK = 0xfe;
const unsigned char OP_DIRECTION_MASK = 2; // 00000010
const unsigned char OP_WORD_MASK = 1;      // 00000001

// MODE BIT MASK
const unsigned char MODE_MASK = 0xc0;   // 11000000
const unsigned char REG_OP_MASK = 0x38; // 00111000
const unsigned char RM_MASK = 7;        // 00000111

// MOV codes
const unsigned char MOV_RM_TO_FROM_REG = 0x88; // 100010dw
const unsigned char MOV_IMM_TO_RM = 0xc6;      // 1100011w
const unsigned char MOV_IMM_TO_REG = 0xb0;     // 1011wreg
const unsigned char MOV_MEM_TO_ACC = 0xa0;     // 1010000w
const unsigned char MOV_ACC_TO_MEM = 0xa2;     // 1010001w
const unsigned char MOV_RM_TO_SR = 0x8e;       // 10001110
const unsigned char MOV_SR_TO_RM = 0x8c;       // 10001100

// PUSH codes

// POP codes

// ADD codes
const unsigned char ADD_RM_TO_FROM_REG = 0x0; // 000000dw
const unsigned char ADD_IMM_TO_REG = 0x80;    // 100000sw
const unsigned char ADD_IMM_TO_ACC = 0x4;     // 0000010w

// SUB codes
const unsigned char SUB_RM_TO_FROM_REG = 0x28; // 001010dw
// const unsigned char SUB_IMM_FROM_RM = 0x80;
const unsigned char SUB_IMM_FROM_ACC = 0x2c; // 0010110w

// CMP codes
const unsigned char CMP_RM_TO_REG = 0x38; // 001110dw
// const unsigned char CMP_IMM_W_REG = 0x80;
const unsigned char CMP_IMM_W_ACC = 0x3c; // 0011110w

// Conditional Jump codes
#define JE_JZ 0x74
#define JL_JNGE 0x7c
#define JLE_JNG 0x7e
#define JB_JNAE 0x72
#define JBE_JNA 0x76
#define JP_JPE 0x7a
#define JO 0x70
#define JS 0x78
#define JNE_JNZ 0x75
#define JNL_JGE 0x7d
#define JNLE_JG 0x7f
#define JNB_JAE 0x73
#define JNBE_JA 0x77
#define JNP_JPO 0x7b
#define JNO 0x71
#define JNS 0x79
#define LOOP 0xe2
#define LOOPZ_LOOPE 0xe1
#define LOOPNZ_LOOPNE 0xe0
#define JCXZ 0xe3

// 01110111 11111110 -- nasm
// 11100111 11111110

// Register tables
const char *BYTE_REGISTERS[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char *WORD_REGISTERS[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

// R/M table
const char *RM_REGISTERS[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

// Segment register table
const char *SEGMENT_REGISTERS[4] = {"es", "cs", "ss", "ds"};

const char *MATH_OPS[8] = {"add", NULL, NULL, NULL, NULL, "sub", NULL, "cmp"};

typedef struct
{
    const char *buf; // The buffered program
    size_t len;      // The length of the program in bytes
    size_t pos;      // The position of the parser
} Prog;

static void signedDisplacement(unsigned short displacement, char *str_buf, size_t len)
{
    // don't print +/- 0 displacements
    if (displacement == 0)
    {
        str_buf[0] = '\0';
        return;
    }
    unsigned short sign_mask = (128 << 8);
    if (displacement <= 255)
    {
        if ((displacement & 128) == 128)
        {
// warning: unknown conversion type character 'h' in format [-Wformat=]
#if __GNUC__ >= 6
            __mingw_snprintf(str_buf, len, " - %hhu", ~displacement + 1);
#else
            snprintf(str_buf, len, "- %hhu", ~displacement + 1);
#endif
        }
        else
        {
#if __GNUC__ >= 6
            __mingw_snprintf(str_buf, len, " + %hhu", displacement);
#else
            snprintf(str_buf, len, "+ %hhu", displacement);
#endif
        }
    }
    else
    {
        if ((displacement & sign_mask) == sign_mask)
        {
            snprintf(str_buf, len, "- %hu", ~displacement + 1);
        }
        else
        {
            snprintf(str_buf, len, "+ %hu", displacement);
        }
    }
}

static void printInstructions(Prog *prog)
{
    if (prog->pos >= prog->len)
    {
        return;
    }
    char *disp_str = malloc(32);
    printf("bits %zu\n\n", BITS);
    // 00000001 01011110 00000000
    while (prog->pos < prog->len)
    {
        const unsigned char op = prog->buf[prog->pos];
        if (op == JE_JZ || op == JL_JNGE || op == JLE_JNG || op == JB_JNAE || op == JBE_JNA || op == JP_JPE || op == JO || op == JS || op == JNE_JNZ || op == JNL_JGE || op == JNLE_JG || op == JNB_JAE || op == JNBE_JA || op == JNP_JPO || op == JNO || op == JNS || op == LOOP || op == LOOPZ_LOOPE || op == LOOPNZ_LOOPNE || op == JCXZ)
        {
            signed char ip = prog->buf[++prog->pos];
            switch (op)
            {
            case JE_JZ:
            {
                // How do i know when to JZ or JE?
                // Does it actually matter?
                printf("je ");
            }
            break;
            case JL_JNGE:
            {
                printf("jl ");
            }
            break;
            case JLE_JNG:
            {
                printf("jle ");
            }
            break;
            case JB_JNAE:
            {
                printf("jb ");
            }
            break;
            case JBE_JNA:
            {
                printf("jbe ");
            }
            break;
            case JP_JPE:
            {
                printf("jp ");
            }
            break;
            case JO:
            {
                printf("jo ");
            }
            break;
            case JS:
            {
                printf("js ");
            }
            break;
            case JNE_JNZ:
            {
                printf("jnz ");
            }
            break;
            case JNL_JGE:
            {
                printf("jnl ");
            }
            break;
            case JNLE_JG:
            {
                printf("jnle ");
            }
            break;
            case JNB_JAE:
            {
                printf("jnb ");
            }
            break;
            case JNBE_JA:
            {
                printf("jnbe ");
            }
            break;
            case JNP_JPO:
            {
                printf("jnp ");
            }
            break;
            case JNO:
            {
                printf("jno ");
            }
            break;
            case JNS:
            {
                printf("jns ");
            }
            break;
            case LOOP:
            {
                printf("loop ");
            }
            break;
            case LOOPZ_LOOPE:
            {
                printf("loopz ");
            }
            break;
            case LOOPNZ_LOOPNE:
            {
                printf("loopnz ");
            }
            break;
            case JCXZ:
            {
                printf("jcxz ");
            }
            break;
            }

            // I don't know if the +2 is right all the time
            // it works for the examples
            if (ip < 0)
            {
                printf("$%hhd+2\n", ip);
            }
            else
            {
                printf("$+%hhd+2\n", ip);
            }
        }
        else if ((op & OP_4_BIT_MASK) == MOV_IMM_TO_REG)
        {
            unsigned char wide = (op & 8) >> 3;
            unsigned char reg = (op & 7);
            unsigned char lo = prog->buf[++prog->pos];
            unsigned short hi = wide == 0 ? 0 : prog->buf[++prog->pos];
            const char *dest = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
            unsigned short value = 0;

            // got only the hi 8 bits
            if (wide == 0 && reg > 3)
            {
                value = (255 << 8) + lo;
            }
            else if (wide == 1 && reg < 4)
            {
                // 16 bit immediate value
                value = (hi << 8) + lo;
            }
            else
            {
                // 8 bit lo value
                value = lo;
            }
            printf("mov %s, %hu\n", dest, value);
        }

        else if ((op & OP_7_BIT_MASK) == MOV_ACC_TO_MEM || (op & OP_7_BIT_MASK) == ADD_IMM_TO_ACC || (op & OP_7_BIT_MASK) == SUB_IMM_FROM_ACC || (op & OP_7_BIT_MASK) == CMP_IMM_W_ACC)
        {
            // Note: due to the encoding this op must be checked before MOV_MEM_TO_ACC
            /**
             * 0xa3 & 0xa0 == 0xa0 && 0xa1 & 0xa0 == 0xa0
             * mov ax, [2555] && mov ax, [16] respectivly
             *
             * 0xa3 & 0xa2 == 0xa2 && 0xa1 & 0xa2 == 0xa0
             * mov [2554], ax && mov [15], ax respectively
             */
            unsigned char instruction = (op & OP_7_BIT_MASK);
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char addr_lo = prog->buf[++prog->pos];
            if (instruction == MOV_ACC_TO_MEM)
            {
                unsigned short addr_hi = prog->buf[++prog->pos];
                unsigned short value = (addr_hi << 8) + addr_lo;
                if (wide == 0)
                {
                    printf("mov [%hu], al\n", value);
                }
                else
                {
                    printf("mov [%hu], ax\n", value);
                }
            }
            else
            {
                unsigned char reg = (op & REG_OP_MASK) >> 3;
                const char *math_op = MATH_OPS[reg];
                if (!math_op)
                {
                    printf("Unexpected math op pos %zu: %x\n", prog->pos, op);
                    return;
                }

                if (wide == 0)
                {
                    printf("%s al, %hhi\n", math_op, addr_lo);
                }
                else
                {
                    unsigned short addr_hi = prog->buf[++prog->pos];
                    unsigned short value = (addr_hi << 8) + addr_lo;
                    printf("%s ax, %hi\n", math_op, value);
                }
            }
        }
        else if ((op & OP_7_BIT_MASK) == MOV_IMM_TO_RM || (op & OP_6_BIT_MASK) == ADD_IMM_TO_REG)
        {
            unsigned char instruction = (op & OP_7_BIT_MASK) == MOV_IMM_TO_RM ? MOV_IMM_TO_RM : ADD_IMM_TO_REG;
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char sign = instruction == ADD_IMM_TO_REG ? (op & 2) >> 1 : 0;
            unsigned char op2 = prog->buf[++prog->pos];
            unsigned char mode = (op2 & MODE_MASK) >> 6;
            unsigned char reg = (op2 & REG_OP_MASK) >> 3; // Math OP if not MOV
            unsigned char rm = (op2 & RM_MASK);
            const char *rm_v = RM_REGISTERS[rm];

            switch (mode)
            {
            case 0:
            {
                // no displacement unless rm = 110
                if (rm == 6)
                {
                    unsigned char disp_lo = prog->buf[++prog->pos];
                    unsigned short disp_hi = prog->buf[++prog->pos];
                    unsigned short value = (disp_hi << 8) + disp_lo;
                    unsigned char data_lo = prog->buf[++prog->pos];
                    const char *math_op = instruction == MOV_IMM_TO_RM ? "mov" : MATH_OPS[reg];
                    // 10000011 00 111 110
                    unsigned char sw = (sign << 1) + wide;
                    switch (sw)
                    {
                    case 0:
                    {
                        // usigned 8 bit data
                        printf("%s byte [%hu], %hhu\n", math_op, value, data_lo);
                    }
                    break;
                    case 1:
                    {
                        // usigned 16 bit data
                        unsigned short data_hi = prog->buf[++prog->pos];
                        unsigned short data_val = (data_hi << 8) + data_lo;
                        printf("%s word [%hu], %hu\n", math_op, value, data_val);
                    }
                    break;
                    case 3:
                    {
                        // sign extened 8 bit data
                        printf("%s word [%hu], %hhd\n", math_op, value, data_lo);
                    }
                    break;
                    }
                }
                else if (instruction == MOV_IMM_TO_RM)
                {
                    unsigned char lo = prog->buf[++prog->pos];
                    printf("mov [%s], ", rm_v);
                    if (wide == 0)
                    {
                        printf("byte %hu\n", lo);
                    }
                    else
                    {
                        unsigned short hi = prog->buf[++prog->pos];
                        unsigned short value = (hi << 8) + lo;
                        printf("word %hhu\n", value);
                    }
                }
                else
                {
                    unsigned char data_lo = prog->buf[++prog->pos];
                    const char *math_op = MATH_OPS[reg];
                    unsigned char sw = (sign << 1) + wide;
                    switch (sw)
                    {
                    case 0:
                    {
                        // usigned 8 bit data
                        printf("%s byte [%s], %hhu\n", math_op, rm_v, data_lo);
                    }
                    break;
                    case 1:
                    {
                        // usinged 16 bit data
                        unsigned short data_hi = prog->buf[++prog->pos];
                        unsigned short value = (data_hi << 8) + data_lo;

                        printf("%s word [%s], %hhu\n", math_op, rm_v, value);
                    }
                    break;
                    case 3:
                    {
                        // sign extended 8 bit data
                        printf("%s byte [%s], %hhd\n", math_op, rm_v, data_lo);
                    }
                    break;
                    default:
                    {
                        printf("sign %hhu, wide %hhu, %s not implemented pos %zu: %x %x\n", sign, wide, math_op, prog->pos, op, op2);
                        return;
                    }
                    }
                }
            }
            break;
            case 1:
            {
                // 1 displacement
                unsigned char disp_lo = prog->buf[++prog->pos];
                unsigned char data_lo = prog->buf[++prog->pos];
                printf("mov [%s ", rm_v);
                // signed displacement
                if ((disp_lo & 128) == 0)
                {
                    printf("+ %hhu], ", disp_lo);
                }
                else
                {
                    printf("- %hhu], ", ~disp_lo + 1);
                }
                // data
                if (wide == 0)
                {
                    printf(" byte %hhu\n", data_lo);
                }
                else
                {
                    unsigned short data_hi = prog->buf[++prog->pos];
                    unsigned short value = (data_hi << 8) + data_lo;
                    printf(" word %hu\n", value);
                }
            }
            break;
            case 2:
            {
                // 2 displacement
                unsigned char disp_lo = prog->buf[++prog->pos];
                unsigned short disp_hi = prog->buf[++prog->pos];
                unsigned char data_lo = prog->buf[++prog->pos];
                unsigned short disp_value = (disp_hi << 8) + disp_lo;
                signedDisplacement(disp_value, disp_str, 32);
                const char *rm_v = RM_REGISTERS[rm];

                if (instruction == MOV_IMM_TO_RM)
                {
                    printf("mov [%s%s], ", rm_v, disp_str);
                    if (wide == 0)
                    {
                        printf("byte %hhu\n", data_lo);
                    }
                    else
                    {
                        unsigned short data_hi = prog->buf[++prog->pos];
                        unsigned short data_value = (data_hi << 8) + data_lo;
                        printf("word %hu\n", data_value);
                    }
                }
                else
                {
                    unsigned char sw = (sign << 1) + wide;
                    const char *math_op = MATH_OPS[reg];
                    if (!math_op)
                    {
                        printf("unexpected math op pos %zu: %x %x\n", prog->pos, op, op2);
                        return;
                    }
                    switch (sw)
                    {
                    case 0:
                    {
                        printf("%s word [%s%s], %hhu", math_op, rm_v, disp_str, data_lo);
                    }
                    break;
                    case 1:
                    {
                        unsigned short data_hi = prog->buf[++prog->pos];
                        unsigned short value = (data_hi << 8) + data_lo;
                        printf("%s word [%s%s], %hi\n", math_op, rm_v, disp_str, value);
                    }
                    break;
                    case 3:
                    {
                        // Sign extended byte
                        printf("%s word [%s%s], %hhd\n", math_op, rm_v, disp_str, data_lo);
                    }
                    break;
                    default:
                    {
                        printf("default sw case pos %zu: %x%x\n", prog->pos, op, op2);
                        return;
                    }
                    }
                }
            }
            break;
            case 3:
            {
                // no displacement
                if (instruction == MOV_IMM_TO_RM)
                {
                    printf("mode 3 for MOV_IMM_TO_RM not implemented pos %zu: %x %x\n", prog->pos, op, op2);
                    return;
                }
                else
                {
                    unsigned char data_lo = prog->buf[++prog->pos];
                    unsigned short data_hi = (op & 3) == 1 ? prog->buf[++prog->pos] : 0;
                    unsigned short value = (data_hi << 8) + data_lo;
                    rm_v = wide == 0 ? BYTE_REGISTERS[rm] : WORD_REGISTERS[rm];
                    switch (reg)
                    {
                    case 0:
                    {
                        // add
                        printf("add %s, ", rm_v);
                    }
                    break;
                    case 5:
                        // sub
                        printf("sub %s, ", rm_v);
                        break;
                    case 7:
                        // cmp
                        printf("cmp %s, ", rm_v);
                        break;
                    }

                    if (sign == 1)
                    {
                        printf("%hd\n", (short)value);
                    }
                    else
                    {
                        printf("%hhu\n", value);
                    }
                }
            }
            break;
            }
        }
        else if ((op & OP_7_BIT_MASK) == MOV_MEM_TO_ACC)
        {
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char addr_lo = prog->buf[++prog->pos];
            unsigned short addr_hi = prog->buf[++prog->pos];
            unsigned short value = (addr_hi << 8) + addr_lo;
            if (wide == 0)
            {
                printf("mov al, [%hu]\n", value);
            }
            else
            {
                printf("mov ax, [%hu]\n", value);
            }
        }
        else if (op == MOV_RM_TO_SR)
        {
        }
        else if (op == MOV_SR_TO_RM)
        {
        }
        else if ((op & OP_6_BIT_MASK) == MOV_RM_TO_FROM_REG || (op & OP_6_BIT_MASK) == ADD_RM_TO_FROM_REG || (op & OP_6_BIT_MASK) == SUB_RM_TO_FROM_REG || (op & OP_6_BIT_MASK) == CMP_RM_TO_REG)
        {
            const unsigned char instruction = (op & OP_6_BIT_MASK);

            unsigned char dir = (op & OP_DIRECTION_MASK) >> 1;
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char op2 = prog->buf[++prog->pos];
            unsigned char mode = (op2 & MODE_MASK) >> 6;
            unsigned char reg = (op2 & REG_OP_MASK) >> 3;
            unsigned char rm = (op2 & RM_MASK);

            if (instruction == MOV_RM_TO_FROM_REG)
            {
                printf("mov ");
            }
            else if (instruction == ADD_RM_TO_FROM_REG)
            {
                printf("add ");
            }
            else if (instruction == SUB_RM_TO_FROM_REG)
            {
                printf("sub ");
            }
            else if (instruction == CMP_RM_TO_REG)
            {
                printf("cmp ");
            }

            switch (mode)
            {
            // Memory mode, no displacment unless rm == 110, then 16 bit displacement
            case 0:
            {
                // Direct addressing
                if (rm == 6)
                {
                    unsigned char disp_lo = prog->buf[++prog->pos];
                    unsigned short disp_hi = prog->buf[++prog->pos];
                    unsigned short value = (disp_hi << 8) + disp_lo;
                    const char *dest = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                    printf("%s, [%hu]\n", dest, value);
                }
                else
                {
                    const char *rm_v = RM_REGISTERS[rm];
                    if (dir == 0)
                    {
                        const char *src = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                        printf("[%s], %s\n", rm_v, src);
                    }
                    else
                    {
                        const char *dest = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];

                        printf("%s, [%s]\n", dest, rm_v);
                    }
                }
            }
            break;
            // Memory mode, 8 bit displacement
            case 1:
            {
                // 00000001 01011110 00000000
                unsigned char lo = prog->buf[++prog->pos];
                const char *rm_v = RM_REGISTERS[rm];
                const char *reg_v = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                signedDisplacement(lo, disp_str, 32);
                // src is in reg
                if (dir == 0)
                {
                    printf("[%s%s], %s\n", rm_v, disp_str, reg_v);
                }
                else
                {
                    printf("%s, [%s%s]\n", reg_v, rm_v, disp_str);
                }
            }
            break;
            // Memory mode, 16 bit displacement
            case 2:
            {
                unsigned char lo = prog->buf[++prog->pos];
                unsigned char hi = prog->buf[++prog->pos];
                unsigned short value = ((short)hi << 8) + lo;

                const char *reg_v = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                const char *rm_v = RM_REGISTERS[rm];
                signedDisplacement(value, disp_str, 32);
                // src is in reg
                if (dir == 0)
                {
                    printf("[%s%s], %s\n", rm_v, disp_str, reg_v);
                }
                else
                {
                    printf("%s, [%s%s]\n", reg_v, rm_v, disp_str);
                }
            }
            break;
            // Register Mode
            case 3:
            {
                const char *rm_v = wide == 0 ? BYTE_REGISTERS[rm] : WORD_REGISTERS[rm];
                const char *reg_v = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                // src is in reg
                if (dir == 0)
                {
                    printf("%s, %s\n", rm_v, reg_v);
                }
                else
                {
                    // dest is in reg
                    printf("%s, %s\n", reg_v, rm_v);
                }
            }
            break;
            }
        }

        prog->pos++;
    }

    free(disp_str);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s path/to/binary\n", argv[0]);
        return 1;
    }

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

    printInstructions(&prog);

    return 0;
}
