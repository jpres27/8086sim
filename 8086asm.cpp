#include "8086asm.h"

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

size_t max_bytes  = 256;

static size_t load_memory_from_file(char *filename, u8 *buffer)
{
    size_t result = 0;

    FILE *file = fopen(filename, "rb");
    if(file)
    {
        result = fread(buffer, 1, max_bytes, file);
        fclose(file);
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to open %s.\n", filename);
    }

    return result;
}

static void wide_check(Instruction *inst, u8 *buffer, int i, b32 ir)
{
    if(!ir)
    {
        u8 wide = buffer[i] & w_mask;
        if((wide ^ w_bit) == 0) inst->w = true;
        else inst->w = false;
        #if 0
        fprintf(stdout, "\n\nu8 wide = %x\n", wide);
        fprintf(stdout, "w_mask = %x\n", w_mask);
        fprintf(stdout, "w_bit = %x\n", w_bit);
        fprintf(stdout, "\n\ninst->w = %u\n", inst->w);
        #endif
    }
    else if(ir)
    {
        u8 wide = buffer[i] & fifth_bit_mask;
        if((wide ^ fifth_bit_mask) == 0) inst->w = true;
        else inst->w = false;
    }
}

// d = 0, instruction source is in the REG field
// d = 1, instruction destination is in the REG field
static void dest_check(Instruction *inst, u8 *buffer, int i)
{
    u8 dest = buffer[i] & d_mask;
    if((dest ^ d_bit) == 0) inst->d = true;
    else inst->d = false;
}

static void sign_ext_check(Instruction *inst, u8 *buffer, int i)
{
    u8 sign_ext = buffer[i] & s_mask;
    if((sign_ext ^ s_bit) == 0) inst->s = true;
    else inst->s = false;
}

static void mod_check(Instruction *inst, u8 *buffer, int i)
{
    u8 mode = buffer[i] & mod_mask;
    if((mode ^ mem_no_disp_bits) == 0) inst->mod = MEM_NO_DISP;
    if((mode ^ mem_byte_disp_bits) == 0) inst->mod = MEM_BYTE_DISP;
    if((mode ^ mem_word_disp_bits) == 0) inst->mod = MEM_WORD_DISP;
    if((mode ^ reg_no_disp_bits) == 0) inst->mod = REG_NO_DISP;
}

// Reg check is used to check r/m when mod is 11 REG NO DISP
// To do so, supply a 3 as the shift arg and use the rm_mask as the mask arg
static void reg_check(Instruction *inst, u8 shift, u8 mask, u8 *buffer, int i)
{
    if(inst->w)
    {
        u8 reg = buffer[i] & mask;
        if((reg ^ (al_ax_bits >> shift)) == 0) fprintf(stdout, ax);
        if((reg ^ (cl_cx_bits >> shift)) == 0) fprintf(stdout, cx);
        if((reg ^ (dl_dx_bits >> shift)) == 0) fprintf(stdout, dx);
        if((reg ^ (bl_bx_bits >> shift)) == 0) fprintf(stdout, bx);
        if((reg ^ (ah_sp_bits >> shift)) == 0) fprintf(stdout, sp);
        if((reg ^ (ch_bp_bits >> shift)) == 0) fprintf(stdout, bp);
        if((reg ^ (dh_si_bits >> shift)) == 0) fprintf(stdout, si);
        if((reg ^ (bh_di_bits >> shift)) == 0) fprintf(stdout, di);
    }
    else 
    {
        u8 reg = buffer[i] & mask;
        if((reg ^ (al_ax_bits >> shift)) == 0) fprintf(stdout, al);
        if((reg ^ (cl_cx_bits >> shift)) == 0) fprintf(stdout, cl);
        if((reg ^ (dl_dx_bits >> shift)) == 0) fprintf(stdout, dl);
        if((reg ^ (bl_bx_bits >> shift)) == 0) fprintf(stdout, bl);
        if((reg ^ (ah_sp_bits >> shift)) == 0) fprintf(stdout, ah);
        if((reg ^ (ch_bp_bits >> shift)) == 0) fprintf(stdout, ch);
        if((reg ^ (dh_si_bits >> shift)) == 0) fprintf(stdout, dh);
        if((reg ^ (bh_di_bits >> shift)) == 0) fprintf(stdout, bh);
    }
}

static void rm_check(Instruction *inst, u8 *buffer, int i)
{
    u8 rm = buffer[i] & last_three_mask;
    if((rm ^ zzz) == 0) fprintf(stdout, "[bx + si"); 
    if((rm ^ zzo) == 0) fprintf(stdout, "[bx + di"); 
    if((rm ^ zoz) == 0) fprintf(stdout, "[bp + si"); 
    if((rm ^ zoo) == 0) fprintf(stdout, "[bp + di"); 
    if((rm ^ ozz) == 0) fprintf(stdout, "[si"); 
    if((rm ^ ozo) == 0) fprintf(stdout, "[di");  
    if((rm ^ ooz) == 0) fprintf(stdout, "[bp");
    if((rm ^ ooo) == 0) fprintf(stdout, "[bx"); 
}

static void direct_address_check(Instruction *inst, u8 *buffer, int i)
{
    u8 rm = buffer[i] & last_three_mask;
    if((inst->mod == MEM_NO_DISP) && ((rm ^ ooz) == 0)) inst->directaddress = true;
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
    else if (inst->mod == MEM_WORD_DISP)
    {
        s16 disp;
        memcpy(&disp, buffer + i, sizeof(disp));
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

static int rmr(Instruction *inst, u8 *buffer, int index)
{
    int i = index;
    dest_check(inst, buffer, i);
    wide_check(inst, buffer, i, false);
    mod_check(inst, buffer, i+1);
    direct_address_check(inst, buffer, i+1);

    if (inst->mod == REG_NO_DISP)
    {
        if (inst->d)
        {
            reg_check(inst, 0, reg_mask, buffer, i+1);
            fprintf(stdout, comma);
            reg_check(inst, 3, rm_mask, buffer, i+1);
        }
        else
        {
            reg_check(inst, 3, rm_mask, buffer, i+1);
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, buffer, i+1);
        }
    }

    else if (inst->mod == MEM_NO_DISP)
    {
        if (inst->directaddress)
        {
            reg_check(inst, 0, reg_mask, buffer, i+1);
            fprintf(stdout, comma);

            if (!inst->w)
            {
                u8 data = buffer[i+2];
                fprintf(stdout, "[%u]", data);
                ++i; // w is not set so a byte of data
            }
            else if (inst->w)
            {
                u16 data;
                memcpy(&data, buffer + (i+2), sizeof(data));
                fprintf(stdout, "[%u]", data);
                i = i+2;; // w is set so a word of data
            }
        }
        else if (inst->d)
        {
            reg_check(inst, 0, reg_mask, buffer, i+1);
            fprintf(stdout, comma);
            rm_check(inst, buffer, i + 1);
            fprintf(stdout, "]");
        }
        else
        {
            rm_check(inst, buffer, i+1);
            fprintf(stdout, "]");
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, buffer, i+1);
        }
    }

    else if (inst->mod == MEM_BYTE_DISP || inst->mod == MEM_WORD_DISP)
    {
        if (inst->d)
        {
            reg_check(inst, 0, reg_mask, buffer, i+1);
            fprintf(stdout, comma);
            rm_check(inst, buffer, i+1);
            rm_disp(inst, buffer, i+2);
            ++i; // one byte disp
            if (inst->mod == MEM_WORD_DISP)
            {
                ++i; // two byte disp
            }
        }
        else
        {
            rm_check(inst, buffer, i+1);
            rm_disp(inst, buffer, i+2);
            fprintf(stdout, comma);
            reg_check(inst, 0, reg_mask, buffer, i+1);
            ++i; // one byte disp
            if (inst->mod == MEM_WORD_DISP)
            {
                ++i; // two byte disp
            }
        }
    }

    fprintf(stdout, end_of_inst);
    ++i; // two byte instruction
    return(i);
}

// TODO: Reducing branching is possible
static int irm(Instruction *inst, u8 *buffer, int index)
{
    int i = index;
    if(inst->op == ADD || inst->op == SUB || inst->op == CMP)
    {
        sign_ext_check(inst, buffer, i);
    }
    wide_check(inst, buffer, i, false);
    mod_check(inst, buffer, i + 1);

    if (inst->mod == REG_NO_DISP)
    {
        reg_check(inst, 3, rm_mask, buffer, i+1);
    }

    else if (inst->mod == MEM_NO_DISP)
    {
        if(!inst->w) fprintf(stdout, "byte ");
        else if(inst->w) fprintf(stdout, "word ");
        rm_check(inst, buffer, i + 1);
        fprintf(stdout, "]");
    }

    else if (inst->mod == MEM_BYTE_DISP || inst->mod == MEM_WORD_DISP)
    {
        if(!inst->w) fprintf(stdout, "byte ");
        else if(inst->w) fprintf(stdout, "word ");
        rm_check(inst, buffer, i + 1);
        rm_disp(inst, buffer, i + 2);
        i++; // one byte disp plus at least one byte of disp
        if (inst->mod == MEM_WORD_DISP) ++i; // two byte disp
    }

    fprintf(stdout, comma);

    if (!inst->w)
    {
        u8 data = buffer[i + 2];
        fprintf(stdout, "%u", data);
        ++i; // w is not set so one byte of data
    }
    else if (inst->w)
    {
        if(inst->s)
        {
            s8 data = buffer[i + 2];
            s16 se_data = (s16)data;
            fprintf(stdout, "%d", se_data);
            ++i; // w is not set so one byte of data
        }
        else
        {
            u16 data;
            memcpy(&data, buffer + (i + 2), sizeof(data));
            fprintf(stdout, "%u", data);
            i = i+2; // w is set so two byts of data
        }
    }

    fprintf(stdout, end_of_inst);
    ++i;
    return(i);
}

static int ia(Instruction *inst, u8 *buffer, int index)
{
    int i = index;

    wide_check(inst, buffer, i, false);

    if(!inst->w)
    {
        fprintf(stdout, "al, ");
        u8 data = buffer[i+1];
        fprintf(stdout, "[%u]", data);
    }
    else if (inst->w)
    {
        fprintf(stdout, "ax, ");
        u16 data;
        memcpy(&data, buffer + (i+1), sizeof(data));
        fprintf(stdout, "[%u]", data);
        ++i; // three byte instruction since w=1
    }

    fprintf(stdout, end_of_inst);
    ++i;
    return(i);
}

static void disassemble(size_t byte_count, u8 *buffer)
{
    fprintf(stdout, opening);

    for(int i = 0; i < byte_count; ++i)
    {
        // fprintf(stdout, "\n-- Beginning of Loop Iteration i = %d --\n", i);
        Instruction inst = {};
        u8 op;

        op = buffer[i] & first_four_mask;
        if((op ^ mov_ir_bits) == 0) 
        {
            inst.op = MOV;
            fprintf(stdout, mov);

            wide_check(&inst, buffer, i, true);
            reg_check(&inst, 3, last_three_mask, buffer, i);
            fprintf(stdout, comma);
            if(!inst.w)
            {
                u8 data = buffer[i+1];
                fprintf(stdout, "%u", data);
            }
            else if (inst.w)
            {
                u16 data;
                memcpy(&data, buffer + i+1, sizeof(data));
                fprintf(stdout, "%u", data);
                ++i; // three byte instruction since w=1
            }
            ++i; // it was at least a two byte instruction
            fprintf(stdout, end_of_inst);
            continue;
        }

        if((op ^ conditional_jmp_bits) == 0)
        {
            u8 jmp_op = buffer[i] & last_four_mask;
            
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

            s8 data = buffer[i+1];
            fprintf(stdout, "%d", data);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        if((op ^ loop_category_bits) == 0)
        {
            u8 loop_op = buffer[i] & last_four_mask;
            
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

            s8 data = buffer[i+1];
            fprintf(stdout, "%d", data);
            fprintf(stdout, end_of_inst);
            ++i;
            continue;
        }

        op = buffer[i] & first_six_mask;

        if((op ^ add_rmr_bits) == 0)
        {
            fprintf(stdout, add);
            inst.op = ADD;
            i = rmr(&inst, buffer, i);
            continue;
        }

        if((op ^ sub_rmr_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = rmr(&inst, buffer, i);
            continue;
        }

        if((op ^ cmp_rmr_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = rmr(&inst, buffer, i);
            continue;
        }

        if((op ^ irm_bits) == 0)
        {
            u8 secondbyteop = buffer[i+1] & reg_mask;
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
            i = irm(&inst, buffer, i);
            continue;
        }

        if((op ^ mov_rmr_bits) == 0) 
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = rmr(&inst, buffer, i);
            continue;
        }

        op = buffer[i] & first_seven_mask;

        if((op ^ add_ia_bits) == 0)
        {
            fprintf(stdout, add);
            inst.op = ADD;
            i = ia(&inst, buffer, i);
            continue;
        }

        if((op ^ sub_ia_bits) == 0)
        {
            fprintf(stdout, sub);
            inst.op = SUB;
            i = ia(&inst, buffer, i);
            continue;
        }

        if((op ^ cmp_ia_bits) == 0)
        {
            fprintf(stdout, cmp);
            inst.op = CMP;
            i = ia(&inst, buffer, i);
            continue;
        }

        if((op ^ mov_irm_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;
            i = irm(&inst, buffer, i);
            continue;
        }

        if((op ^ mov_ma_bits) == 0)
        {
            fprintf(stdout, mov);
            inst.op = MOV;

            wide_check(&inst, buffer, i, false);
            fprintf(stdout, "ax, ");

            if(!inst.w)
            {
                u8 data = buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                u16 data;
                memcpy(&data, buffer + (i+1), sizeof(data));
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

            wide_check(&inst, buffer, i, false);
            if(!inst.w)
            {
                u8 data = buffer[i+1];
                fprintf(stdout, "[%u]", data);
            }
            else if (inst.w)
            {
                u16 data;
                memcpy(&data, buffer + (i+1), sizeof(data));
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
    u8 buffer[256];
    memset(buffer, 0, 256);

    if(arg_count > 1)
    {
        char *filename = args[1];
        size_t bytes_read = load_memory_from_file(filename, buffer);
        disassemble(bytes_read, buffer);
    }
}