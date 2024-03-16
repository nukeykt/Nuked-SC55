#pragma once
#include <stdint.h>

struct frt_t {
    uint8_t tcr;
    uint8_t tcsr;
    uint16_t frc;
    uint16_t ocra;
    uint16_t ocrb;
    uint16_t icr;
    uint8_t status_rd;
};

struct mcu_timer_t {
    uint8_t tcr;
    uint8_t tcsr;
    uint8_t tcora;
    uint8_t tcorb;
    uint8_t tcnt;
    uint8_t status_rd;
};

void TIMER_Write(uint32_t address, uint8_t data);
uint8_t TIMER_Read(uint32_t address);
void TIMER_Clock(uint64_t cycles);

void TIMER2_Write(uint32_t address, uint8_t data);
uint8_t TIMER_Read2(uint32_t address);

