// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023
 */
#include <common.h>
#include <div64.h>
#include <timer.h>
#include <linux/kernel.h>

static u64 timer_read_count(void)
{
    u32 high;
    u32 low;
    u32 next_high;

    do {
        asm volatile("rdcntvh.w %0" : "=r" (high));
        asm volatile("rdcntvl.w %0" : "=r" (low));
        asm volatile("rdcntvh.w %0" : "=r" (next_high));
    } while (high != next_high);

    return ((u64)high << 32) | low;
}

ulong notrace get_tbclk(void)
{
    return CONFIG_SYS_LOONGARCH_TIMER_FREQ;
}

uint64_t notrace get_ticks(void)
{
    return timer_read_count();
}

static u64 timer_get_ticks(unsigned long usec)
{
    u64 ticks = (u64)usec * get_tbclk();

    return DIV_ROUND_UP_ULL(ticks, 1000000);
}

void __udelay(unsigned long usec)
{
    u64 start = timer_read_count();
    u64 ticks = timer_get_ticks(usec);

    while (timer_read_count() - start < ticks)
        ;
}

unsigned long get_timer(unsigned long base)
{
    u64 now = timer_read_count();

    now *= CONFIG_SYS_HZ;
    do_div(now, get_tbclk());

    return (unsigned long)now - base;
}
