#pragma once

typedef char unsigned u8;
typedef short unsigned u16;
typedef int unsigned u32;
typedef long long unsigned u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef s32 b32;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct Registers
{
    u16 ax;
    u16 cx;
    u16 dx;
    u16 bx;
    u16 sp;
    u16 bp;
    u16 si;
    u16 di;
};

struct Memory 
{
    Registers regs;
    u8 buffer[256];
};