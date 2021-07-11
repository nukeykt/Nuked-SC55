#pragma once

#include <stdint.h>

void MCU_Interrupt_Request(uint32_t interrupt);
void MCU_Interrupt_TRAPA(uint32_t vector);
void MCU_Interrupt_Handle(void);

enum {
    INTERRUPT_SOURCE_NMI = 0,
    INTERRUPT_SOURCE_IRQ0,
    INTERRUPT_SOURCE_IRQ1,
    INTERRUPT_SOURCE_ADDRESS_ERROR,
    INTERRUPT_SOURCE_INVALID_INSTRUCTION,
    INTERRUPT_SOURCE_TRACE,
    INTERRUPT_SOURCE_MAX
};
