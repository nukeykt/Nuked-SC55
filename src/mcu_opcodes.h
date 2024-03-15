#pragma once

#include <stdint.h>

extern void (*MCU_Operand_Table[256])(uint8_t operand);
extern void (*MCU_Opcode_Table[32])(uint8_t opcode, uint8_t opcode_reg);
