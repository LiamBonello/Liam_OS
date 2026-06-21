#include "keyboard.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/io.h"
#include "../core/log.h"
#include "../core/shell.h"
#include "../core/types.h"


#define KEYBOARD_DATA_PORT 0x60
#define IRQ_KEYBOARD 1

#define SCANCODE_LEFT_SHIFT_PRESS 0x2A
#define SCANCODE_RIGHT_SHIFT_PRESS 0x36
#define SCANCODE_LEFT_SHIFT_RELEASE 0xAA
#define SCANCODE_RIGHT_SHIFT_RELEASE 0xB6

static uint8_t shift_pressed = 0;

static const char scancode_ascii_map[128] = {
    0,
    27,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
};

static const char scancode_shift_ascii_map[128] = {
    0,
    27,
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    '_',
    '+',
    '\b',
    '\t',
    'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '{',
    '}',
    '\n',
    0,
    'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ':',
    '"',
    '~',
    0,
    '|',
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    '<',
    '>',
    '?',
    0,
    '*',
    0,
    ' ',
};

static void keyboard_irq_handler(interrupt_registers_t *registers)
{
    (void)registers;

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == SCANCODE_LEFT_SHIFT_PRESS || scancode == SCANCODE_RIGHT_SHIFT_PRESS) {
        shift_pressed = 1;
        return;
    }

    if (scancode == SCANCODE_LEFT_SHIFT_RELEASE || scancode == SCANCODE_RIGHT_SHIFT_RELEASE) {
        shift_pressed = 0;
        return;
    }

    if ((scancode & 0x80) != 0) {
        return;
    }

    if (scancode >= 128) {
        return;
    }

    char character = shift_pressed
        ? scancode_shift_ascii_map[scancode]
        : scancode_ascii_map[scancode];

    if (character == 0) {
        return;
    }

    shell_handle_character(character);
}

void keyboard_initialize(void)
{
    idt_register_irq_handler(IRQ_KEYBOARD, keyboard_irq_handler);
    log_success("Keyboard initialized");
}