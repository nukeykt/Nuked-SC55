#pragma once
#include <stdint.h>

struct frt_t {
    uint16_t frc;
    uint16_t ocra;
    uint16_t ocrb;
};

void TIMER_Write(uint32_t address, uint8_t data);

