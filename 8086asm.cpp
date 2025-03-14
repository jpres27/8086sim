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

static void segreg_check(Instruction *inst, u8 *buffer, int i)
{
    u8 segreg = (buffer[i] >> 3) & 0x3;
    if((segreg ^ 0x0) == 0) 
    {
        inst->reg = ES;
        fprintf(stdout, "es");
    }
    else if((segreg ^ 0x1) == 0) 
    {
        inst->reg = CS;
        fprintf(stdout, "cs");
    }
    else if((segreg ^ 0x2) == 0) 
    {
        inst->reg = SS;
        fprintf(stdout, "ss");
    }
    else if((segreg ^ 0x3) == 0)
    {
        inst->reg = DS;
        fprintf(stdout, "ds");
    }
    else
    {
        fprintf(stdout, "ERROR: CANNOT DECIDE SEG REG\n");
    }
}

// Reg check is used to check r/m when mod is 11 REG NO DISP
// To do so, supply a 3 as the shift arg and use the rm_mask as the mask arg
static void reg_check(Instruction *inst, u8 shift, u8 mask, u8 *buffer, int i, b32 rm_field)
{
    if(inst->w)
    {
        u8 reg = buffer[i] & mask;
        if((reg ^ (al_ax_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = AX;
            else inst->rm = AX;
            fprintf(stdout, ax);
        }    
        else if((reg ^ (cl_cx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = CX;
            else inst->rm = CX;
            fprintf(stdout, cx);
        }    
        else if((reg ^ (dl_dx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = DX;
            else inst->rm = DX;
            fprintf(stdout, dx);
        }    
        else if((reg ^ (bl_bx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = BX;
            else inst->rm = BX;
            fprintf(stdout, bx);
        }
        else if((reg ^ (ah_sp_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = SP;
            else inst->rm = SP;
            fprintf(stdout, sp);
        }
        else if((reg ^ (ch_bp_bits >> shift)) == 0)
        {
            if(!rm_field) inst->reg = BP;
            else inst->rm = BP;
            fprintf(stdout, bp);
        }
        else if((reg ^ (dh_si_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = SI;
            else inst->rm = SI;
            fprintf(stdout, si);
        }
        else if((reg ^ (bh_di_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = DI;
            else inst->rm = DI;
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
            if(!rm_field) inst->reg = AL;
            else inst->rm = AL;
            fprintf(stdout, al);
        }
        else if((reg ^ (cl_cx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = CL;
            else inst->rm = CL;
            fprintf(stdout, cl);
        }
        else if((reg ^ (dl_dx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = DL;
            else inst->rm = DL;
            fprintf(stdout, dl);
        }
        else if((reg ^ (bl_bx_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = BL;
            else inst->rm = BL;
            fprintf(stdout, bl);
        }
        else if((reg ^ (ah_sp_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = AH;
            else inst->rm = AH;
            fprintf(stdout, ah);
        }
        else if((reg ^ (ch_bp_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = CH;
            else inst->rm = CH;
            fprintf(stdout, ch);
        }
        else if((reg ^ (dh_si_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = DH;
            else inst->rm = DH;
            fprintf(stdout, dh);
        }
        else if((reg ^ (bh_di_bits >> shift)) == 0) 
        {
            if(!rm_field) inst->reg = BH;
            else inst->rm = BH;
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
