static u16 pit_counter(void)
{
    u8 low;
    u8 high;
    out8(0x43, 0x00);
    low = in8(0x40);
    high = in8(0x40);
    return (u16)low | ((u16)high << 8);
}

static u32 clock_millis(void)
{
    u16 current = pit_counter();
    u16 elapsed = (u16)(clock_previous - current);
    clock_previous = current;
    clock_cycles += elapsed;
    return clock_cycles / 1193UL;
}
