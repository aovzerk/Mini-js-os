static u32 clock_millis(void)
{
    return *(volatile u32 *)0x70004UL;
}
