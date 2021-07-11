#pragma once
#include <stdint.h>

struct dsp_t {
    uint32_t cycles;
};

void DSP_Write(uint32_t address, uint8_t data);
uint8_t DSP_Read(uint32_t address);

