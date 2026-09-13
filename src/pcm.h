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
 *  Thanks:
 *      John McMaster (https://siliconprawn.org):
 *          PCM chip decap
 *
 */
#pragma once
#include <stdint.h>

struct pcm_t {
    uint32_t ram1[32][8];
    uint16_t ram2[32][16];
    uint32_t select_channel;
    uint32_t voice_mask;
    uint32_t voice_mask_pending;
    uint32_t voice_mask_updating;
    uint32_t write_latch;
    uint32_t wave_read_address;
    uint8_t wave_byte_latch;
    uint32_t read_latch;
    uint8_t config_reg_3c; // SC55:c3 JV880:c0
    uint8_t config_reg_3d;
    uint32_t irq_channel;
    uint32_t irq_assert;

    uint32_t nfs;

    uint32_t tv_counter;

    uint64_t cycles;

    uint16_t eram[0x4000];

    int accum_l;
    int accum_r;
    int rcsum[2];
};

extern pcm_t pcm;
extern uint8_t waverom1[];
extern uint8_t waverom2[];
extern uint8_t waverom3[];
extern uint8_t waverom4[];
extern uint8_t waverom_card[];
extern uint8_t waverom_exp[];

void PCM_Write(uint32_t address, uint8_t data);
uint8_t PCM_Read(uint32_t address);
void PCM_Reset(void);
void PCM_Update(uint64_t cycles);
