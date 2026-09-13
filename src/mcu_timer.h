/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 * This file is part of Nuked-SC55.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
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

extern uint8_t dev_WDT_TCSR;
extern uint8_t dev_WDT_TCNT;

void TIMER_Reset();

void TIMER_Write(uint32_t address, uint8_t data);
uint8_t TIMER_Read(uint32_t address);
void TIMER_Clock(uint64_t cycles);

void TIMER2_Write(uint32_t address, uint8_t data);
uint8_t TIMER_Read2(uint32_t address);

