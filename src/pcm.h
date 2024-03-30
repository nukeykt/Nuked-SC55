#pragma once
#include <stdint.h>

struct pcm_t {
    uint32_t ram1[32][8]; // only 6 actually used?
    uint16_t ram2[32][16]; // only 12 actually used?
    uint32_t select_channel;
    uint32_t voice_mask;
    uint32_t voice_mask_pending;
    uint32_t voice_mask_updating;
    uint32_t write_latch;
    uint32_t wave_read_address;
    uint8_t wave_byte_latch;
    uint32_t read_latch;
    uint8_t config_reg_3c;
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

extern uint8_t waverom1[];
extern uint8_t waverom2[];

void PCM_Write(uint32_t address, uint8_t data);
uint8_t PCM_Read(uint32_t address);
void PCM_Reset(void);
void PCM_Update(uint64_t cycles);
