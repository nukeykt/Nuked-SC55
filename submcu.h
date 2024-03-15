#pragma once
#include <stdint.h>

enum {
    SM_STATUS_C = 1,
    SM_STATUS_Z = 2,
    SM_STATUS_I = 4,
    SM_STATUS_D = 8,
    SM_STATUS_B = 16,
    SM_STATUS_T = 32,
    SM_STATUS_V = 64,
    SM_STATUS_N = 128
};

struct submcu_t {
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t sr;
    uint64_t cycles;
    uint8_t sleep;
};

extern submcu_t sm;

extern uint8_t sm_rom[4096];

void SM_Reset(void);
void SM_Update(uint64_t cycles);
void SM_SysWrite(uint32_t address, uint8_t data);
uint8_t SM_SysRead(uint32_t address);
void SM_PostUART(uint8_t data);
