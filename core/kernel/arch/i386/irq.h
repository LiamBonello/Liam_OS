#ifndef LIAM_OS_IRQ_H
#define LIAM_OS_IRQ_H

#include "../../core/types.h"


uint32_t irq_save(void);
void irq_restore(uint32_t flags);
void irq_disable(void);
void irq_enable(void);
uint8_t irq_are_enabled(void);

#endif