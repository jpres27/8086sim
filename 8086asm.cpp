// 8086asm.cpp

static void wide_check(Instruction *inst, u8 *buffer, int i, b32 ir)
{
    if(!ir)
    {
        u8 wide = buffer[i] & w_mask;
        if((wide ^ w_bit) == 0) inst->w = true;
        else inst->w = false;
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
        if((reg ^ (al_ax_bits >> shift)) == 0) 
        {
            inst->reg = AX;
            fprintf(stdout, ax);
        }    
        else if((reg ^ (cl_cx_bits >> shift)) == 0) 
        {
            inst->reg = CX;
            fprintf(stdout, cx);
        }    
        else if((reg ^ (dl_dx_bits >> shift)) == 0) 
        {
            inst->reg = DX;
            fprintf(stdout, dx);
        }    
        else if((reg ^ (bl_bx_bits >> shift)) == 0) 
        {
            inst->reg = BX;
            fprintf(stdout, bx);
        }
        else if((reg ^ (ah_sp_bits >> shift)) == 0) 
        {
            inst->reg = SP;
            fprintf(stdout, sp);
        }
        else if((reg ^ (ch_bp_bits >> shift)) == 0)
        {
            inst->reg = BP;
            fprintf(stdout, bp);
        }
        else if((reg ^ (dh_si_bits >> shift)) == 0) 
        {
            inst->reg = SI;
            fprintf(stdout, si);
        }
        else if((reg ^ (bh_di_bits >> shift)) == 0) 
        {
            inst->reg = DI;
            fprintf(stdout, di);
        }
        else
        {
            fprintf(stdout, " -- ERROR REG BITS NOT DECODABLE -- \n");
        }
    }
    else 
    {
        u8 reg = buffer[i] & mask;
        if((reg ^ (al_ax_bits >> shift)) == 0) 
        {
            inst->reg = AL;
            fprintf(stdout, al);
        }
        else if((reg ^ (cl_cx_bits >> shift)) == 0) 
        {
            inst->reg = CL;
            fprintf(stdout, cl);
        }
        else if((reg ^ (dl_dx_bits >> shift)) == 0) 
        {
            inst->reg = DL;
            fprintf(stdout, dl);
        }
        else if((reg ^ (bl_bx_bits >> shift)) == 0) 
        {
            inst->reg = BL;
            fprintf(stdout, bl);
        }
        else if((reg ^ (ah_sp_bits >> shift)) == 0) 
        {
            inst->reg = AH;
            fprintf(stdout, ah);
        }
        else if((reg ^ (ch_bp_bits >> shift)) == 0) 
        {
            inst->reg = CH;
            fprintf(stdout, ch);
        }
        else if((reg ^ (dh_si_bits >> shift)) == 0) 
        {
            inst->reg = DH;
            fprintf(stdout, dh);
        }
        else if((reg ^ (bh_di_bits >> shift)) == 0) 
        {
            inst->reg = BH;
            fprintf(stdout, bh);
        }
        else
        {
            fprintf(stdout, " -- ERROR REG BITS NOT DECODABLE -- \n");
        }
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

