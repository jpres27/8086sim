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

static void mov_op(Instruction *inst, u8 *dest, u8 *source)
{
    if(inst->w) 
    {
        *((u16*)dest) = *((u16*)source);
    }
    else 
    {
        *dest = *source;
    }
}

static void add_op(Instruction *inst, Memory *memory, u8 *operand_1, u8 *operand_2)
{
    if(inst->w) 
    {
        *((u16*)operand_1) = *((u16*)operand_1) + *((u16*)operand_2);
    }
    else 
    {
        *operand_1 = *operand_1 + *operand_2;
    }

    flag_check(inst, memory, operand_1);
}

static void sub_op(Instruction *inst, Memory *memory, u8 *operand_1, u8 *operand_2)
{
    if(inst->w) 
    {
        *((u16*)operand_1) = *((u16*)operand_1) - *((u16*)operand_2);
    }
    else 
    {
        *operand_1 = *operand_1 - *operand_2;
    }

    flag_check(inst, memory, operand_1);
}

static void cmp_op(Instruction *inst, Memory *memory, u8 *operand_1, u8 *operand_2)
{
    if(inst->w) 
    {
        *((u16*)(&memory->cmp_buffer[0])) = *((u16*)operand_1) - *((u16*)operand_2);
        fprintf(stdout, "\nCMP BUFFER CONTENTS: %u\n", TwoByteAccess(memory->cmp_buffer[0]));
    }
    else 
    {
        memory->cmp_buffer[0] = *operand_1 - *operand_2;
        fprintf(stdout, "\nCMP BUFFER CONTENTS: %u\n", memory->cmp_buffer[0]);
    }

    flag_check(inst, memory, &memory->cmp_buffer[0]);
}

static void rm_disp(Instruction *inst, u8 *buffer, int i)
{
    if(inst->mod == MEM_BYTE_DISP)
    {
        s8 disp = (s8)buffer[i];
        if(disp != 0)
        {
            if(disp < 0)
            {
                fprintf(stdout, " - %d]", (-1*disp));
            }
            else
            {
                fprintf(stdout, " + %d]", disp);
            }
        }
        else
        {
            fprintf(stdout, "]");
        }
    }
    else if (inst->mod == MEM_WORD_DISP)
    {
        s16 disp = *((s16*)buffer[i]);
        if(disp != 0)
        {
            if(disp < 0)
            {
                fprintf(stdout, " - %d]", (disp*-1));
            }
            else
            {
                fprintf(stdout, " + %d]", disp);
            }
        }
        else
        {
            fprintf(stdout, "]");
        }
    }
}

static u16 rm_mem_calc(Instruction *inst, Memory *memory, int i)
{
    u16 address = 0;
    u16 displacement = 0;

    if(inst->mod == MEM_BYTE_DISP) displacement = (u16)memory->buffer[i+2];
    if(inst->mod == MEM_WORD_DISP) displacement = TwoByteAccess(memory->buffer[i+2]);

    switch(inst->rm)
    {
    case 0:
        address = TwoByteAccess(memory->regs[BX]) + TwoByteAccess(memory->regs[SI]) + displacement;
        break;

    case 1:
        address = TwoByteAccess(memory->regs[BX]) + TwoByteAccess(memory->regs[DI]) + displacement;
        break;

    case 2:
        address = TwoByteAccess(memory->regs[BP]) + TwoByteAccess(memory->regs[SI]) + displacement;
        break;

    case 3:
        address = TwoByteAccess(memory->regs[BP]) + TwoByteAccess(memory->regs[DI]) + displacement;
        break;

    case 4:
        address = TwoByteAccess(memory->regs[SI]) + displacement;
        break;

    case 5:
        address = TwoByteAccess(memory->regs[DI]) + displacement;
        break;

    case 6:
        if(inst->mod == MEM_BYTE_DISP || inst->mod == MEM_WORD_DISP) 
        {
            address = TwoByteAccess(memory->regs[BP]) + displacement;
        }
        else if(inst->mod == MEM_NO_DISP) 
        {
            address = TwoByteAccess(memory->buffer[i+2]);
        }
        break;

    case 7:
        address = TwoByteAccess(memory->regs[BX]) + displacement;
        break;
    default:
        fprintf(stdout, "ERROR: Couldn't calculate effective address\n");
    }

    //fprintf(stdout, " | ADDRESS: %u |", address);
    return(address);
}

// TODO: Once we stop needing to print instructions, a lot of this function can be
// greatly simplified, as much of what is here is solely to print out the parts
// of the instruction in the correct order
static int rmr(Instruction *inst, Memory *memory, int index)
{
    int i = index;
    dest_check(inst, memory->buffer, i);
    wide_check(inst, memory->buffer, i, false);
    mod_check(inst, memory->buffer, i+1);
    rm_check(inst, memory->buffer, i+1);

    if (inst->mod == REG_NO_DISP)
    {
        memory->instruction_pointer = i+2;
        if (inst->d)
        {
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
            fprintf(stdout, comma);
            reg_check(inst, 3, rm_mask, memory->buffer, i+1, true);
        }
        else
        {
            reg_check(inst, 3, rm_mask, memory->buffer, i+1, true);
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
        }

        if(inst->op == MOV) 
        {
           if(inst->d) mov_op(inst, &memory->regs[inst->reg], &memory->regs[inst->rm]);
           else mov_op(inst, &memory->regs[inst->rm], &memory->regs[inst->reg]);
        }
        else if(inst->op == ADD) 
        {
            if(inst->d) add_op(inst, memory, &memory->regs[inst->reg], &memory->regs[inst->rm]);
            else add_op(inst, memory, &memory->regs[inst->rm], &memory->regs[inst->reg]);
        }
        else if(inst->op == SUB) 
        {
            if(inst->d) sub_op(inst, memory, &memory->regs[inst->reg], &memory->regs[inst->rm]);
            else sub_op(inst, memory, &memory->regs[inst->rm], &memory->regs[inst->reg]);
        }
        else if(inst->op == CMP)
        {
            if(inst->d) cmp_op(inst, memory, &memory->regs[inst->reg], &memory->regs[inst->rm]);
            else cmp_op(inst, memory, &memory->regs[inst->rm], &memory->regs[inst->reg]);
        }
    }

    else if (inst->mod == MEM_NO_DISP)
    {
        u16 address = rm_mem_calc(inst, memory, i);
        if (inst->directaddress)
        {
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
            fprintf(stdout, comma);

            // NOTE: If we refactor and remove printing, we just need to preserve the 
            // index handling and register checking going on in this code below
            if (!inst->w)
            {
                memory->instruction_pointer = i+3;
                u8 data = memory->buffer[i+2];
                fprintf(stdout, "[%u]", data);
                ++i; // w is not set so a byte of data
            }
            else if (inst->w)
            {
                memory->instruction_pointer = i+4;
                u16 data;
                memcpy(&data, memory->buffer + (i+2), sizeof(data));
                fprintf(stdout, "[%u]", data);
                i = i+2;; // w is set so a word of data
            }
        }
        else if (inst->d)
        {
            memory->instruction_pointer = i+2;
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
            fprintf(stdout, comma);
            fprintf(stdout, "[");
            rm_print(inst, memory->buffer, i + 1);
            fprintf(stdout, "]");

        }
        else
        {
            memory->instruction_pointer = i+2;
            fprintf(stdout, "[");
            rm_print(inst, memory->buffer, i+1);
            fprintf(stdout, "]");
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
        }

        if(inst->op == MOV)
        {
            if(inst->d) mov_op(inst, &memory->regs[inst->reg], &memory->mem[address]);
            else mov_op(inst, &memory->mem[address], &memory->regs[inst->reg]);
        }
        else if(inst->op == ADD)
        {
            if(inst->d) add_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else add_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
        else if(inst->op == SUB)
        {
            if(inst->d) sub_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else sub_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
        else if(inst->op == SUB)
        {
            if(inst->d) cmp_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else cmp_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
    }

    else if (inst->mod == MEM_BYTE_DISP || inst->mod == MEM_WORD_DISP)
    {
        if(inst->mod == MEM_BYTE_DISP) memory->instruction_pointer = i+3;
        else if(inst->mod == MEM_WORD_DISP) memory->instruction_pointer = i+4;
        u16 address = rm_mem_calc(inst, memory, i);

        if (inst->d)
        {
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
            fprintf(stdout, comma);
            rm_print(inst, memory->buffer, i+1);
            rm_disp(inst, memory->buffer, i+2);
            ++i; // one byte disp

            if (inst->mod == MEM_WORD_DISP)
            {
                ++i; // two byte disp
            }
        }
        else
        {
            rm_print(inst, memory->buffer, i+1);
            rm_disp(inst, memory->buffer, i+2);
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, memory->buffer, i+1, false);
            ++i; // one byte disp
            if (inst->mod == MEM_WORD_DISP)
            {
                ++i; // two byte disp
            }
        }

        if(inst->op == MOV)
        {
            if(inst->d) mov_op(inst, &memory->regs[inst->reg], &memory->mem[address]);
            else mov_op(inst, &memory->mem[address], &memory->regs[inst->reg]);
        }
        else if(inst->op == ADD)
        {
            if(inst->d) add_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else add_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
        if(inst->op == SUB)
        {
            if(inst->d) sub_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else sub_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
        if(inst->op == CMP)
        {
            if(inst->d) cmp_op(inst, memory, &memory->regs[inst->reg], &memory->mem[address]);
            else cmp_op(inst, memory, &memory->mem[address], &memory->regs[inst->reg]);
        }
    }

    fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
    fprintf(stdout, end_of_inst);
    ++i; // two byte instruction
    return(i);
}

// TODO: Reducing branching is possible
// NOTE: There is nothing in the reg bits in these instructions so
// all registers are stored in the rm bits
// TODO: This function probably needs to be almost totally rewriten
// now that we are implementing memory ops. This structure just makes
// no sense beyond printing properly.
static int irm(Instruction *inst, Memory *memory, int index)
{
    int i = index;
    if(inst->op == ADD || inst->op == SUB || inst->op == CMP)
    {
        sign_ext_check(inst, memory->buffer, i);
    }
    wide_check(inst, memory->buffer, i, false);
    mod_check(inst, memory->buffer, i + 1);

    if (inst->mod == REG_NO_DISP)
    {
        reg_check(inst, 3, rm_mask, memory->buffer, i+1, true);
        fprintf(stdout, comma);

        if (!inst->w)
        {
            memory->instruction_pointer = i+3;
            u8 data = memory->buffer[i + 2];
            fprintf(stdout, "%u", data);
    
            if(inst->op == MOV)
            {
                mov_op(inst, &memory->regs[inst->rm], &data);
            }
            else if(inst->op == ADD)
            {
                add_op(inst, memory, &memory->regs[inst->rm], &data);
            }
            else if(inst->op == SUB)
            {
                sub_op(inst, memory, &memory->regs[inst->rm], &data);
            }
            else if(inst->op == CMP)
            {
                cmp_op(inst, memory, &memory->regs[inst->rm], &data);
            }
    
            ++i; // w is not set so one byte of data
        }
        else if (inst->w)
        {
            if(inst->s)
            {
                memory->instruction_pointer = i+3;
    
                u8 data = memory->buffer[i+2];
                s16 se_data = (s16)data;
                fprintf(stdout, "%d", se_data);
    
                if(inst->op == ADD)
                {
                    add_op(inst, memory, &memory->regs[inst->rm], (u8*)(&se_data));
                }
                else if(inst->op == SUB)
                {
                    sub_op(inst, memory, &memory->regs[inst->rm], (u8*)(&se_data));
                }
                else if(inst->op == CMP)
                {
                    cmp_op(inst, memory, &memory->regs[inst->rm], (u8*)(&se_data));
                }
    
                ++i; // w is not set so one byte of data
            }
            else
            {
                memory->instruction_pointer = i+4;
    
                u16 data;
                memcpy(&data, memory->buffer + (i + 2), sizeof(data));
                fprintf(stdout, "%u", data);
    
                if(inst->op == MOV)
                {
                    mov_op(inst, &memory->regs[inst->rm], &memory->buffer[i+2]);
                }
    
                i = i+2; // w is set so two bytes of data
            }
        }
    }

    else if (inst->mod == MEM_NO_DISP)
    {
        if(!inst->w) fprintf(stdout, "byte ");
        else if(inst->w) fprintf(stdout, "word ");
        rm_check(inst, memory->buffer, i+1);
        u16 address = rm_mem_calc(inst, memory, i);

        fprintf(stdout, "[");
        rm_print(inst, memory->buffer, i + 1);
        fprintf(stdout, "]");
        fprintf(stdout, comma);

        if(inst->directaddress)
        {
            if(inst->w)
            {
                memory->instruction_pointer = i+6;
                u16 data = *((u16*)&memory->buffer[i+4]);
                fprintf(stdout, "%u", data);
            }
            else if (!inst->w)
            {
                memory->instruction_pointer = i+5;
                u8 data = memory->buffer[i+4];
                fprintf(stdout, "%u", data);
            }

            if(inst->op == MOV)
            {
                mov_op(inst, &memory->mem[address], &memory->buffer[i+4]);
            }
        }

        else if (!inst->w)
        {
            memory->instruction_pointer = i+3;
            u8 data = memory->buffer[i + 2];
            fprintf(stdout, "%u", data);
    
            if(inst->op == MOV)
            {
                mov_op(inst, &memory->mem[address], &memory->buffer[i+2]);
            }

            ++i; // w is not set so one byte of data
        }
        else if (inst->w)
        {
            if(inst->s)
            {
                memory->instruction_pointer = i+3;
    
                u8 data = memory->buffer[i + 2];
                s16 se_data = (s16)data;
                fprintf(stdout, "%d", se_data);
    
                ++i; // w is not set so one byte of data
            }
            else
            {
                memory->instruction_pointer = i+4;
    
                u16 data;
                // TODO: If the casting method is working as expected then
                // it would be good to standardize all these memcpys to just
                // do that instead
                memcpy(&data, memory->buffer + (i + 2), sizeof(data));
                fprintf(stdout, "%u", data);
    
                if(inst->op == MOV)
                {
                    mov_op(inst, &memory->mem[address], &memory->buffer[i+2]);
                }
    
                i = i+2; // w is set so two bytes of data
            }
        }
    }

    else if (inst->mod == MEM_BYTE_DISP || inst->mod == MEM_WORD_DISP)
    {
        if(!inst->w) fprintf(stdout, "byte ");
        else if(inst->w) fprintf(stdout, "word ");
        rm_check(inst, memory->buffer, i+1);
        rm_print(inst, memory->buffer, i+1);
        rm_disp(inst, memory->buffer, i+2);
        u16 address = rm_mem_calc(inst, memory, i);
        i++; // one byte disp plus at least one byte of disp
        if (inst->mod == MEM_WORD_DISP) ++i; // two byte disp

        fprintf(stdout, comma);

        if (!inst->w)
        {
            memory->instruction_pointer = i+3;
            u8 data = memory->buffer[i + 2];
            fprintf(stdout, "%u", data);
    
            if(inst->op == MOV)
            {
                mov_op(inst, &memory->mem[address], &memory->buffer[i+2]);
            }

            ++i; // w is not set so one byte of data
        }
        else if (inst->w)
        {
            if(inst->s)
            {
                memory->instruction_pointer = i+3;
    
                u8 data = memory->buffer[i + 2];
                s16 se_data = (s16)data;
                fprintf(stdout, "%d", se_data);
    
                ++i; // w is not set so one byte of data
            }
            else
            {
                memory->instruction_pointer = i+4;
    
                u16 data;
                memcpy(&data, memory->buffer + (i + 2), sizeof(data));
                fprintf(stdout, "%u", data);
    
                if(inst->op == MOV)
                {
                    mov_op(inst, &memory->mem[address], &memory->buffer[i+2]);
                }
    
                i = i+2; // w is set so two bytes of data
            }
        }
    }

    fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
    fprintf(stdout, end_of_inst);
    ++i;
    return(i);
}

static int ia(Instruction *inst, Memory *memory, u8 *buffer, int index)
{
    int i = index;

    wide_check(inst, buffer, i, false);

    if(!inst->w)
    {
        memory->instruction_pointer = i+2;
        fprintf(stdout, "al, ");
        u8 data = buffer[i+1];
        fprintf(stdout, "[%u]", data);
    }
    else if (inst->w)
    {
        memory->instruction_pointer = i+3;
        fprintf(stdout, "ax, ");
        u16 data;
        memcpy(&data, buffer + (i+1), sizeof(data));
        fprintf(stdout, "[%u]", data);
        ++i; // three byte instruction since w=1
    }

    fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
    fprintf(stdout, end_of_inst);
    ++i;
    return(i);
}

static void decode_and_execute(size_t byte_count, Memory *memory)
{
    fprintf(stdout, opening);
    memory->instruction_pointer = 0;

    while(memory->instruction_pointer < byte_count)
    {
        fprintf(stdout, "\nCX REGISTER CONTENTS: %u\n", TwoByteAccess(memory->regs[CX]));

        Instruction inst = {};
        u8 op;
        int i = memory->instruction_pointer;

        op = memory->buffer[i] & first_four_mask;
        if((op ^ mov_ir_bits) == 0) 
        {
            inst.op = MOV;
            fprintf(stdout, mov);

            wide_check(&inst, memory->buffer, i, true);
            reg_check(&inst, 3, last_three_mask, memory->buffer, i, false);
            fprintf(stdout, comma);
            mov_op(&inst, &memory->regs[inst.reg], &memory->buffer[i+1]);
            if(!inst.w)
            {
                memory->instruction_pointer = i+2;
                fprintf(stdout, "0x%02x", memory->regs[inst.reg]);
            }
            else if (inst.w)
            {
                memory->instruction_pointer = i+3;
                fprintf(stdout, "0x%04x", *((u16*)&memory->regs[inst.reg]));
                ++i; // three byte instruction since w=1
            }
            ++i; // it was at least a two byte instruction
            fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
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
                memory->instruction_pointer = memory->instruction_pointer + 2;
                fprintf(stdout, "jne ");
                inst.op = JNE;
                if(!memory->flags[ZF])
                {
                    s8 offset = memory->buffer[i+1];
                    memory->instruction_pointer = memory->instruction_pointer + offset;
                }

                fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
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
            i = rmr(&inst, memory, i);
            continue;
        }

        if((op ^ sub_rmr_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = rmr(&inst, memory, i);
            continue;
        }

        if((op ^ cmp_rmr_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = rmr(&inst, memory, i);
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
            i = irm(&inst, memory, i);
            continue;
        }

        if((op ^ mov_rmr_bits) == 0) 
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = rmr(&inst, memory, i);
            continue;
        }

        op = memory->buffer[i] & first_seven_mask;

        if((op ^ add_ia_bits) == 0)
        {
            fprintf(stdout, add);
            inst.op = ADD;
            i = ia(&inst, memory, memory->buffer, i);
            continue;
        }

        if((op ^ sub_ia_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = ia(&inst, memory, memory->buffer, i);
            continue;
        }

        if((op ^ cmp_ia_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = ia(&inst, memory, memory->buffer, i);
            continue;
        }

        if((op ^ mov_irm_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = irm(&inst, memory, i);
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
                memory->instruction_pointer = i+2;
                u8 data = memory->buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                memory->instruction_pointer = i+3;
                u16 data;
                memcpy(&data, memory->buffer + (i+1), sizeof(data));
                fprintf(stdout, "[%u]", data);
                ++i; // three byte instruction since w=1
            }

            fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
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
                memory->instruction_pointer = i+2;
                u8 data = memory->buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                memory->instruction_pointer = i+3;
                u16 data;
                memcpy(&data, memory->buffer + (i+1), sizeof(data));
                fprintf(stdout, "[%u]", data);
                ++i; // three byte instruction since w=1
            }
            fprintf(stdout, ", ax");

            fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        op = memory->buffer[i];
        if((op ^ mov_rms_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            mod_check(&inst, memory->buffer, i+1);
            // Since moves with segment registers only happen in 16 bits, we automatically
            // set the wide bit so that the rm check later will work properly
            inst.w = true;

            if(inst.mod == REG_NO_DISP)
            {
                memory->instruction_pointer = i+2;
                segreg_check(&inst, memory->buffer, i+1);
                fprintf(stdout, comma);
                reg_check(&inst, 3, rm_mask, memory->buffer, i+1, true);
                mov_op(&inst, &memory->regs[inst.reg], &memory->regs[inst.rm]);
            }

            fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        if((op ^ mov_srm_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            mod_check(&inst, memory->buffer, i+1);
            inst.w = true;

            if(inst.mod == REG_NO_DISP)
            {
                memory->instruction_pointer = i+2;
                reg_check(&inst, 3, rm_mask, memory->buffer, i+1, true);
                fprintf(stdout, comma);
                segreg_check(&inst, memory->buffer, i+1);
                mov_op(&inst, &memory->regs[inst.rm], &memory->regs[inst.reg]);
            }

            fprintf(stdout, "     /// IP: 0x%04x   ///     ", memory->instruction_pointer);
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

    FILE *f = fopen("img.data", "wb");
    fwrite(memory.mem, sizeof(u8), sizeof(memory.mem), f);
    fclose(f);

    char* reg_names[] = 
    {
        "AX",
        "BX",
        "CX",
        "DX",
        "SP",
        "BP",
        "SI",
        "DI",
        "ES",
        "CS",
        "SS",
        "DS"
    };
    fprintf(stdout, "\n\nMEMORY / REGISTER STATE AT END OF EXECUTION:\n");
    int k = 0;
    for(int i = 0; i < 24; i = i + 2)
    {
        if(*((u16*)&memory.regs[i]) != 0)
        {
            fprintf(stdout, "%hS: 0x%04x\n", reg_names[k], *((u16*)&memory.regs[i]));
        }
        ++k;
    }
}