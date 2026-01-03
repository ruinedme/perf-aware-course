#include <stdio.h>

#define BITS 16

// OP CODE BIT MASKS
const unsigned char OP_4_BIT_MASK = 0Xf0;  // 11110000
const unsigned char OP_5_BIT_MASK = 0Xf8;  // 11111000
const unsigned char OP_6_BIT_MASK = 0Xfc;  // 11111100
const unsigned char OP_7_BIT_MASK = 0Xfe;  // 11111110
const unsigned char OP_DIRECTION_MASK = 2; // 00000010
const unsigned char OP_WORD_MASK = 1;      // 00000001

// MODE BIT MASK
const unsigned char MODE_MASK = 0xc0;   // 11000000
const unsigned char REG_OP_MASK = 0x38; // 00111000
const unsigned char RM_MASK = 7;        // 00000111

// MOV codes
const unsigned char MOV_RM_TO_FROM_REG = 0x88; // 10001000
const unsigned char MOV_IMM_TO_RM = 0xc6;      // 11000110
const unsigned char MOV_IMM_TO_REG = 0xb0;     // 10110000
const unsigned char MOV_MEM_TO_ACC = 0xa0;     // 10100000
const unsigned char MOV_ACC_TO_MEM = 0xa2;     // 10100010
const unsigned char MOV_RM_TO_SR = 0x8e;       // 10001110
const unsigned char MOV_SR_TO_RM = 0x8c;       // 10001100

// PUSH codes

// POP codes

// Register tables
const char *BYTE_REGISTERS[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char *WORD_REGISTERS[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

// R/M table
const char *RM_REGSITERS[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

// Segment register table
const char *SEGMENT_REGISTERS[4] = {"es", "cs", "ss", "ds"};

typedef struct
{
    const char *buf; // The buffered program
    size_t len;      // The length of the program in bytes
    size_t pos;      // The position of the parser
} Prog;

static void printInstructions(Prog *prog)
{
    if (prog->pos >= prog->len)
    {
        return;
    }

    printf("bits %zu\n\n", BITS);
    while (prog->pos < prog->len)
    {
        unsigned char op = prog->buf[prog->pos];

        if ((op & MOV_IMM_TO_RM) == MOV_IMM_TO_RM)
        {
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char op2 = prog->buf[++prog->pos];
            unsigned char mode = (op2 & MODE_MASK) >> 6;
            // unsigned char reg = (op2 & REG_OP_MASK) >> 3;
            unsigned char rm = (op2 & RM_MASK);

            const char *rm_v = RM_REGSITERS[rm];

            switch (mode)
            {
            case 0:
            {
                // no displacement unless rm = 110
                if (rm == 6)
                {
                    printf("Direct addressing not implemented pos %zu: %x %x\n", prog->pos, op, op2);
                    return;
                }
                printf("mov [%s], ", rm_v);
                unsigned char lo = prog->buf[++prog->pos];
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
                const char *rm_v = RM_REGSITERS[rm];
                printf("mov [%s ", rm_v);
                // signed displacement
                if ((disp_hi & 128) == 0)
                {
                    printf("+ %hu], ", disp_value);
                }
                else
                {
                    printf("- %hu], ", ~disp_value + 1);
                }

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
            break;
            case 3:{
                // no displacement
                printf("mode 3 for MOV_IMM_TO_RM not implemented pos %zu: %zu %zu\n", prog->pos, op, op2);
                return;

            }
            break;
            }
        }
        else if ((op & MOV_IMM_TO_REG) == MOV_IMM_TO_REG)
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
            printf("mov %s, %zu\n", dest, value);
        }
        else if ((op & MOV_ACC_TO_MEM) == MOV_ACC_TO_MEM)
        {
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char addr_lo = prog->buf[++prog->pos];
            unsigned short addr_hi = prog->buf[++prog->pos];
            unsigned short value = (addr_hi << 8) + addr_lo;
            if(wide == 0){
                printf("mov [%hu], al\n", value);
            }else {
                printf("mov [%hu], ax\n", value);
            }
        }
        else if ((op & MOV_MEM_TO_ACC) == MOV_MEM_TO_ACC)
        {
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char addr_lo = prog->buf[++prog->pos];
            unsigned short addr_hi = prog->buf[++prog->pos];
            unsigned short value = (addr_hi << 8) + addr_lo;
            if(wide == 0){
                printf("mov al, [%hu]\n", value);
            }else {
                printf("mov ax, [%hu]\n", value);
            }
        }
        else if ((op & MOV_RM_TO_SR) == MOV_RM_TO_SR)
        {
        }
        else if ((op & MOV_SR_TO_RM) == MOV_SR_TO_RM)
        {
        }
        else if ((op & MOV_RM_TO_FROM_REG) == MOV_RM_TO_FROM_REG)
        {
            unsigned char dir = (op & OP_DIRECTION_MASK) >> 1;
            unsigned char wide = (op & OP_WORD_MASK);
            unsigned char op2 = prog->buf[++prog->pos];
            unsigned char mode = (op2 & MODE_MASK) >> 6;
            unsigned char reg = (op2 & REG_OP_MASK) >> 3;
            unsigned char rm = (op2 & RM_MASK);

            printf("mov ");
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
                    const char *rm_v = RM_REGSITERS[rm];
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
                unsigned char lo = prog->buf[++prog->pos];
                // src is in reg
                if (dir == 0)
                {
                    const char *dest = RM_REGSITERS[rm];
                    const char *src = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                    if (lo == 0)
                    {
                        printf("[%s], %s\n", dest, src);
                    }
                    else
                    {
                        if ((lo & 128) == 0)
                        {
                            printf("[%s + %zu], %s\n", dest, src, lo);
                        }
                        else
                        {
                            printf("[%s - %zu], %s\n", dest, src, ~lo + 1);
                        }
                    }
                }
                else
                {
                    const char *dest = wide == 0 ? BYTE_REGISTERS[reg] : WORD_REGISTERS[reg];
                    const char *src = RM_REGSITERS[rm];
                    if (lo == 0)
                    {
                        printf("%s, [%s]\n", dest, src);
                    }
                    else
                    {
                        // sign
                        if ((lo & 128) == 0)
                        {
                            printf("%s, [%s + %hhu]\n", dest, src, lo);
                        }
                        else
                        {
                            printf("%s, [%s - %hhu]\n", dest, src, (~lo) + 1);
                        }
                    }
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
                const char *rm_v = RM_REGSITERS[rm];
                // src is in reg
                if (dir == 0)
                {
                    if ((hi & 128) == 0)
                    {
                        printf("[%s + %hu], %s\n", rm_v, value, rm_v);
                    }
                    else
                    {
                        printf("[%s - %hu], %s\n", rm_v, ~value + 1, reg_v);
                    }
                }
                else
                {
                    if ((hi & 128) == 0)
                    {
                        printf("%s, [%s + %zu]\n", reg_v, rm_v, value);
                    }
                    else
                    {
                        printf("%s, [%s - %zu]\n", reg_v, rm_v, ~value + 1);
                    }
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
                    // if (wide == 0)
                    // {
                    //     printf("%s, ", BYTE_REGISTERS[reg]);
                    // }
                    // else
                    // {
                    //     printf("%s, ", WORD_REGISTERS[reg]);
                    // }
                }
            }
            break;
            }
        }
    
        prog->pos++;
    }
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
