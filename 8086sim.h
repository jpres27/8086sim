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
#define TwoByteAccess(a) *((u16*)&a)
#define OneByteAccess(a) *((u8*)&a)

struct Memory 
{
    s32 instruction_pointer;
    u32 clocks_total;
    u16 cx_loop_counter;
    u8 regs[24];
    b32 flags[2];
    u8 buffer[512];
    u8 cmp_buffer[2];
    u8 mem[68608];
};