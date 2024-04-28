#pragma once

struct mcu_t;
struct submcu_t;
struct mcu_timer_t;
struct lcd_t;
struct pcm_t;

struct emu_backend_t {
    mcu_t*       mcu;
    submcu_t*    sm;
    mcu_timer_t* timer;
    lcd_t*       lcd;
    pcm_t*       pcm;
};

void EMU_Init(emu_backend_t& emu);
void EMU_Free(emu_backend_t& emu);

// Should be called after loading roms
void EMU_Reset(emu_backend_t& emu);

