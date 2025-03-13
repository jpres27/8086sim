#pragma once

u8 first_four_mask = 0xf0;
u8 first_six_mask = 0xfc;
u8 first_seven_mask = 0xfe;
u8 fifth_bit_mask = 0x8;
u8 last_three_mask = 0x7;
u8 last_four_mask = 0xf;
u8 d_mask = 0x2; 
u8 d_bit = 0x2;
u8 s_mask = 0x2;
u8 s_bit = 0x2;
u8 w_mask = 0x1;
u8 w_bit = 0x1;
u8 mod_mask = 0xc0;
u8 reg_mask = 0x38;
u8 rm_mask = 0x7;

u8 mov_ir_bits = 0xb0;
u8 mov_irm_bits = 0xc6;
u8 mov_ma_bits = 0xa0;
u8 mov_am_bits = 0xa2;
u8 mov_rmr_bits = 0x88;

u8 irm_bits = 0x80;
u8 add_irm_bits = 0x0;
u8 sub_irm_bits = 0x28;
u8 cmp_irm_bits = 0x38;

u8 add_rmr_bits = 0x0;
u8 add_ia_bits = 0x4;

u8 sub_rmr_bits = 0x28;
u8 sub_ia_bits = 0x2c;

u8 cmp_rmr_bits = 0x38;
u8 cmp_ia_bits = 0x3c;

u8 conditional_jmp_bits = 0x70;
u8 loop_category_bits = 0xe0;

u8 je_bits = 0x4;
u8 jl_bits = 0xc;
u8 jle_bits = 0xe;
u8 jb_bits = 0x2;
u8 jbe_bits = 0x6;
u8 jp_bits = 0xa;
u8 jo_bits = 0x0;
u8 js_bits = 0x8;
u8 jne_bits = 0x5;
u8 jnl_bits = 0xd;
u8 jnle_bits = 0xf;
u8 jnb_bits = 0x3;
u8 jnbe_bits = 0x7;
u8 jnp_bits = 0xb;
u8 jno_bits = 0x1;
u8 jns_bits = 0x9;

u8 loop_bits = 0x2;
u8 loopz_bits = 0x1;
u8 loopnz_bits = 0x0;
u8 jcxz_bits = 0x3;

u8 mem_no_disp_bits = 0x0;
u8 mem_byte_disp_bits = 0x40;
u8 mem_word_disp_bits = 0x80;
u8 reg_no_disp_bits = 0xc0;

u8 al_ax_bits = 0x0;
u8 cl_cx_bits = 0x8;
u8 dl_dx_bits = 0x10;
u8 bl_bx_bits = 0x18;
u8 ah_sp_bits = 0x20;
u8 ch_bp_bits = 0x28;
u8 dh_si_bits = 0x30;
u8 bh_di_bits = 0x38;

u8 rm_al_ax_bits = al_ax_bits >> 3;
u8 rm_cl_cx_bits = cl_cx_bits >> 3;
u8 rm_dl_dx_bits = dl_dx_bits >> 3;
u8 rm_bl_bx_bits = bl_bx_bits >> 3;
u8 rm_ah_sp_bits = ah_sp_bits >> 3;
u8 rm_ch_bp_bits = ch_bp_bits >> 3;
u8 rm_dh_si_bits = dh_si_bits >> 3;
u8 rm_bh_di_bits = bh_di_bits >> 3;

u8 zzz = 0x0;
u8 zzo = 0x1;
u8 zoz = 0x2;
u8 zoo = 0x3;
u8 ozz = 0x4;
u8 ozo = 0x5;
u8 ooz = 0x6;
u8 ooo = 0x7;

char *opening = "bits 16\n\n";
char *comma = ", ";

char *mov = "mov ";
char *add = "add ";
char *sub = "sub ";
char *cmp = "cmp ";

char *al = "al";
char *ax = "ax";
char *cl = "cl";
char *cx = "cx";
char *dl = "dl";
char *dx = "dx";
char *bl = "bl";
char *bx = "bx";
char *ah = "ah";
char *sp = "sp";
char *ch = "ch";
char *bp = "bp";
char *dh = "dh";
char *si = "si";
char *bh = "bh";
char *di = "di";
char *end_of_inst = "\n";

enum Opcode
{
    MOV,
    ADD,
    SUB,
    CMP,
    JE,
    JL,
    JLE,
    JB,
    JBE,
    JP,
    JO,
    JS,
    JNE,
    JNL,
    JNLE,
    JNB,
    JNBE,
    JNP,
    JNO,
    JNS,
    LOOP,
    LOOPZ,
    LOOPNZ,
    JCXZ
};

enum Mod
{
    MEM_NO_DISP,
    MEM_BYTE_DISP,
    MEM_WORD_DISP,
    REG_NO_DISP
};

enum Reg
{
    AX = 0,
    AL = 0,
    AH = 1,
    BX = 2,
    BL = 2,
    BH = 3,
    CX = 4,
    CL = 4,
    CH = 5,
    DX = 6,
    DL = 6,
    DH = 7,
    SP = 8,
    BP = 10,
    SI = 12,
    DI = 14
};

struct Instruction
{
    Opcode op;
    b32 d;
    b32 s;
    b32 w;
    Mod mod;
    Reg reg;
    // TODO: We aren't using rm or storing any state about it
    // so we should update an rm enum or something with all possible
    // states of rm that we can set in conjunction with mod checking
    // or something just to store all relevant information, so that
    // the contents of an instruction could be known from this struct
    // alone, without reference to the assembly we print. It may simply be
    // that we directly store the state of the rm bits and then whatever 
    // needs to interpret it can do so in light of mod and we dont need 
    // to worry about that here.
    Reg rm;
    b32 directaddress;
};