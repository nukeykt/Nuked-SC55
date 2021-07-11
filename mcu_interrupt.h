#pragma once

void MCU_Interrupt_Request(uint32_t interrupt);
void MCU_Interrupt_TRAPA(uint32_t vector);
void MCU_Interrupt_Handle(void);