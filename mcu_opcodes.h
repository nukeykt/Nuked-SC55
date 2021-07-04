#pragma once

#include <stdint.h>

extern void (*MCU_Opcode_Table[256])(uint8_t prefix);
