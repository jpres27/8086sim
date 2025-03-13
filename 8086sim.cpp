#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "8086sim.h"
#include "8086asm.h"
#include "8086asm.cpp"
#include "8086execute.cpp"

size_t max_bytes  = 256;

static size_t load_memory_from_file(char *filename, Memory *memory)
{
    size_t result = 0;

    FILE *file = fopen(filename, "rb");
    if(file)
    {
        result = fread(memory->buffer, 1, max_bytes, file);
        fclose(file);
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to open %s.\n", filename);
    }

    return result;
}

static void decode_and_execute(size_t byte_count, Memory *memory)
{
    fprintf(stdout, opening);

    for(int i = 0; i < byte_count; ++i)
    {
        Instruction inst = {};
        u8 op;

        op = memory->buffer[i] & first_four_mask;
        if((op ^ mov_ir_bits) == 0) 
        {
            inst.op = MOV;
            fprintf(stdout, mov);

            wide_check(&inst, memory->buffer, i, true);
            reg_check(&inst, 3, last_three_mask, memory->buffer, i);
            fprintf(stdout, comma);
            if(!inst.w)
            {
                memory->regs[inst.reg] = memory->buffer[i+1];
                fprintf(stdout, "%u", memory->regs[inst.reg]);
            }
            else if (inst.w)
            {
                memcpy((u16 *)&memory->regs[inst.reg], memory->buffer + i+1, sizeof(u16));
                fprintf(stdout, "%u", memory->regs[inst.reg]);
                ++i; // three byte instruction since w=1
            }
            ++i; // it was at least a two byte instruction
            fprintf(stdout, end_of_inst);
            continue;
        }

        if((op ^ conditional_jmp_bits) == 0)
        {
            u8 jmp_op = memory->buffer[i] & last_four_mask;
            
            if((jmp_op ^ je_bits) == 0)
            {
                    fprintf(stdout, "je ");
                    inst.op = JE;
            }
            if((jmp_op ^ jl_bits) == 0)
            {
                    fprintf(stdout, "jl ");
                    inst.op = JL;
            }
            if((jmp_op ^ jle_bits) == 0)
            {
                    fprintf(stdout, "jle ");
                    inst.op = JLE;
            }
            if((jmp_op ^ jb_bits) == 0)
            {
                    fprintf(stdout, "jb ");
                    inst.op = JB;
            }
            if((jmp_op ^ jbe_bits) == 0)
            {
                    fprintf(stdout, "jbe ");
                    inst.op = JBE;
            }
            if((jmp_op ^ jp_bits) == 0)
            {
                    fprintf(stdout, "jp ");
                    inst.op = JP;
            }
            if((jmp_op ^ jo_bits) == 0)
            {
                    fprintf(stdout, "jo ");
                    inst.op = JO;
            }
            if((jmp_op ^ js_bits) == 0)
            {
                    fprintf(stdout, "js ");
                    inst.op = JS;
            }
            if((jmp_op ^ jne_bits) == 0)
            {
                    fprintf(stdout, "jne ");
                    inst.op = JNE;
            }
            if((jmp_op ^ jnl_bits) == 0)
            {
                    fprintf(stdout, "jnl ");
                    inst.op = JNL;
            }
            if((jmp_op ^ jnle_bits) == 0)
            {
                    fprintf(stdout, "jnle ");
                    inst.op = JNLE;
            }
            if((jmp_op ^ jnb_bits) == 0)
            {
                    fprintf(stdout, "jnb ");
                    inst.op = JNB;
            }
            if((jmp_op ^ jnbe_bits) == 0)
            {
                    fprintf(stdout, "jnbe ");
                    inst.op = JNBE;
            }
            if((jmp_op ^ jnp_bits) == 0)
            {
                    fprintf(stdout, "jnp ");
                    inst.op = JNP;
            }
            if((jmp_op ^ jno_bits) == 0)
            {
                    fprintf(stdout, "jno ");
                    inst.op = JNO;
            }
            if((jmp_op ^ jns_bits) == 0)
            {
                    fprintf(stdout, "jns ");
                    inst.op = JNS;
            }

            s8 data = memory->buffer[i+1];
            fprintf(stdout, "%d", data);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        if((op ^ loop_category_bits) == 0)
        {
            u8 loop_op = memory->buffer[i] & last_four_mask;
            
            if((loop_op ^ loop_bits) == 0)
            {
                    fprintf(stdout, "loop ");
                    inst.op = LOOP;
            }
            if((loop_op ^ loopz_bits) == 0)
            {
                    fprintf(stdout, "loopz ");
                    inst.op = LOOPZ;
            }
            if((loop_op ^ loopnz_bits) == 0)
            {
                    fprintf(stdout, "loopnz ");
                    inst.op = LOOPNZ;
            }
            if((loop_op ^ jcxz_bits) == 0)
            {
                    fprintf(stdout, "jcxz ");
                    inst.op = JCXZ;
            }

            s8 data = memory->buffer[i+1];
            fprintf(stdout, "%d", data);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        op = memory->buffer[i] & first_six_mask;

        if((op ^ add_rmr_bits) == 0)
        {
            fprintf(stdout, add);
            inst.op = ADD;
            i = rmr(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ sub_rmr_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = rmr(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ cmp_rmr_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = rmr(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ irm_bits) == 0)
        {
            u8 secondbyteop = memory->buffer[i+1] & reg_mask;
            if((secondbyteop ^ add_irm_bits) == 0)
            {
                fprintf(stdout, add);
                inst.op = ADD; 
            }
            else if((secondbyteop ^ sub_irm_bits) == 0)
            {
                fprintf(stdout, sub);
                inst.op = SUB; 
            }
            else if((secondbyteop ^ cmp_irm_bits) == 0)
            {
                fprintf(stdout, cmp);
                inst.op = CMP;
            }
            i = irm(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ mov_rmr_bits) == 0) 
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = rmr(&inst, memory->buffer, i);
            continue;
        }

        op = memory->buffer[i] & first_seven_mask;

        if((op ^ add_ia_bits) == 0)
        {
            fprintf(stdout, add);
            inst.op = ADD;
            i = ia(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ sub_ia_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = ia(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ cmp_ia_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = ia(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ mov_irm_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = irm(&inst, memory->buffer, i);
            continue;
        }

        if((op ^ mov_ma_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;

            wide_check(&inst, memory->buffer, i, false);
            fprintf(stdout, "ax, ");

            if(!inst.w)
            {
                u8 data = memory->buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                u16 data;
                memcpy(&data, memory->buffer + (i+1), sizeof(data));
                fprintf(stdout, "[%u]", data);
                ++i; // three byte instruction since w=1
            }
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        if((op ^ mov_am_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;

            wide_check(&inst, memory->buffer, i, false);
            if(!inst.w)
            {
                u8 data = memory->buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                u16 data;
                memcpy(&data, memory->buffer + (i+1), sizeof(data));
                fprintf(stdout, "[%u]", data);
                ++i; // three byte instruction since w=1
            }
            fprintf(stdout, ", ax");
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }
    }
}

int main(int arg_count, char **args)
{
    Memory memory = {};

    if(arg_count > 1)
    {
        char *filename = args[1];
        size_t bytes_read = load_memory_from_file(filename, &memory);
        decode_and_execute(bytes_read, &memory);
    }

    char* reg_names[] = 
    {
        "AX",
        "BX",
        "CX",
        "DX",
        "SP",
        "BP",
        "SI",
        "DI"
    };
    fprintf(stdout, "\n\nMEMORY / REGISTER STATE AT END OF EXECUTION:\n");
    int k = 0;
    for(int i = 0; i < 16; i = i + 2)
    {
        fprintf(stdout, "%hS: 0x%04x\n", reg_names[k], memory.regs[i]);
        ++k;
    }
}