#pragma once

#include <stdint.h>


enum {
    VECTOR_RESET = 0,
    VECTOR_RESERVED1, // UNUSED
    VECTOR_INVALID_INSTRUCTION,
    VECTOR_DIVZERO,
    VECTOR_TRAP,
    VECTOR_RESERVED2, // UNUSED
    VECTOR_RESERVED3, // UNUSED
    VECTOR_RESERVED4, // UNUSED
    VECTOR_ADDRESS_ERROR,
    VECTOR_TRACE,
    VECTOR_RESERVED5, // UNUSED
    VECTOR_NMI,
    VECTOR_RESERVED6, // UNUSED
    VECTOR_RESERVED7, // UNUSED
    VECTOR_RESERVED8, // UNUSED
    VECTOR_RESERVED9, // UNUSED
    VECTOR_TRAPA_0,
    VECTOR_TRAPA_1,
    VECTOR_TRAPA_2,
    VECTOR_TRAPA_3,
    VECTOR_TRAPA_4,
    VECTOR_TRAPA_5,
    VECTOR_TRAPA_6,
    VECTOR_TRAPA_7,
    VECTOR_TRAPA_8,
    VECTOR_TRAPA_9,
    VECTOR_TRAPA_A,
    VECTOR_TRAPA_B,
    VECTOR_TRAPA_C,
    VECTOR_TRAPA_D,
    VECTOR_TRAPA_E,
    VECTOR_TRAPA_F,
    VECTOR_IRQ0,
    VECTOR_IRQ1,
    VECTOR_INTERNAL_INTERRUPT_88, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_8C, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_90, // FRT1 ICI
    VECTOR_INTERNAL_INTERRUPT_94, // FRT1 OCIA
    VECTOR_INTERNAL_INTERRUPT_98, // FRT1 OCIB
    VECTOR_INTERNAL_INTERRUPT_9C, // FRT1 FOVI
    VECTOR_INTERNAL_INTERRUPT_A0, // FRT2 ICI
    VECTOR_INTERNAL_INTERRUPT_A4, // FRT2 OCIA
    VECTOR_INTERNAL_INTERRUPT_A8, // FRT2 OCIB
    VECTOR_INTERNAL_INTERRUPT_AC, // FRT2 FOVI
    VECTOR_INTERNAL_INTERRUPT_B0, // FRT3 ICI
    VECTOR_INTERNAL_INTERRUPT_B4, // FRT3 OCIA
    VECTOR_INTERNAL_INTERRUPT_B8, // FRT3 OCIB
    VECTOR_INTERNAL_INTERRUPT_BC, // FRT3 FOVI
    VECTOR_INTERNAL_INTERRUPT_C0, // CMIA
    VECTOR_INTERNAL_INTERRUPT_C4, // CMIB
    VECTOR_INTERNAL_INTERRUPT_C8, // OVI
    VECTOR_INTERNAL_INTERRUPT_CC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_D0, // ERI
    VECTOR_INTERNAL_INTERRUPT_D4, // RXI
    VECTOR_INTERNAL_INTERRUPT_D8, // TXI
    VECTOR_INTERNAL_INTERRUPT_DC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_E0, // ADI
};

struct mcu_t {
    uint16_t r[8];
    uint16_t pc;
    uint16_t sr;
    uint8_t cp, dp, ep, tp, br;
    uint32_t cycles;
};

extern mcu_t mcu;

void MCU_ErrorTrap(void);

uint8_t MCU_Read(uint32_t address);
uint16_t MCU_Read16(uint32_t address);
uint32_t MCU_Read32(uint32_t address);
void MCU_Write(uint32_t address, uint8_t value);
void MCU_Write16(uint32_t address, uint16_t value);

inline uint32_t MCU_GetAddress(uint8_t page, uint16_t address) {
    return (page << 16) + address;
}

inline uint8_t MCU_ReadCode(void) {
    return MCU_Read(MCU_GetAddress(mcu.cp, mcu.pc));
}

inline uint8_t MCU_ReadCodeAdvance(void) {
    uint8_t ret = MCU_ReadCode();
    mcu.pc++;
    return ret;
}

inline void MCU_SetRegisterByte(uint8_t reg, uint8_t val)
{
    mcu.r[reg] = val;
}
