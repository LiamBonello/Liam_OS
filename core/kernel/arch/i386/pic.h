#ifndef LIAM_OS_PIC_H
#define LIAM_OS_PIC_H

#include "../../core/types.h"


void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_clear_masks(void);

#endif