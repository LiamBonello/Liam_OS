typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;

#define VGA_TEXT_BUFFER ((volatile u16 *)0xB8000ULL)
#define VGA_WIDTH 80ULL
#define VGA_ATTR 0x0FULL

static u16 vga_entry(char c)
{
    return (u16)c | (u16)(VGA_ATTR << 8);
}

static void vga_clear(void)
{
    volatile u16 *vga = VGA_TEXT_BUFFER;
    for (u64 i = 0; i < VGA_WIDTH * 25ULL; ++i) {
        vga[i] = vga_entry(' ');
    }
}

static void vga_write_at(const char *message, u64 row, u64 column)
{
    volatile u16 *vga = VGA_TEXT_BUFFER + (row * VGA_WIDTH) + column;
    while (*message != '\0') {
        *vga++ = vga_entry(*message++);
    }
}

void kernel_main_x86_64(void)
{
    vga_clear();
    vga_write_at("Liam_OS x86_64 C kernel entry online", 0, 0);
    vga_write_at("Stage: long mode -> freestanding C", 1, 0);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
