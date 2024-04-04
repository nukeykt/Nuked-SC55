/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <stdint.h>
#include <vector>
#include "mcu_interrupt.h"
#include <SDL2/SDL_atomic.h>
#include "SDL.h"
#include "pcm.h"
#include "lcd.h"
#include "mcu_timer.h"
#include "submcu.h"

#ifdef __APPLE__
#include <sys/syslimits.h> // PATH_MAX
#include <mach-o/dyld.h>
#endif

enum {
    DEV_P1DDR = 0x00,
    DEV_P5DDR = 0x08,
    DEV_P6DDR = 0x09,
    DEV_P7DDR = 0x0c,
    DEV_P7DR = 0x0e,
    DEV_FRT1_TCR = 0x10,
    DEV_FRT1_TCSR = 0x11,
    DEV_FRT1_FRCH = 0x12,
    DEV_FRT1_FRCL = 0x13,
    DEV_FRT1_OCRAH = 0x14,
    DEV_FRT1_OCRAL = 0x15,
    DEV_FRT2_TCR = 0x20,
    DEV_FRT2_TCSR = 0x21,
    DEV_FRT2_FRCH = 0x22,
    DEV_FRT2_FRCL = 0x23,
    DEV_FRT2_OCRAH = 0x24,
    DEV_FRT2_OCRAL = 0x25,
    DEV_FRT3_TCR = 0x30,
    DEV_FRT3_TCSR = 0x31,
    DEV_FRT3_FRCH = 0x32,
    DEV_FRT3_FRCL = 0x33,
    DEV_FRT3_OCRAH = 0x34,
    DEV_FRT3_OCRAL = 0x35,
    DEV_PWM1_TCR = 0x40,
    DEV_PWM1_DTR = 0x41,
    DEV_PWM2_TCR = 0x44,
    DEV_PWM2_DTR = 0x45,
    DEV_PWM3_TCR = 0x48,
    DEV_PWM3_DTR = 0x49,
    DEV_TMR_TCR = 0x50,
    DEV_TMR_TCSR = 0x51,
    DEV_TMR_TCORA = 0x52,
    DEV_TMR_TCORB = 0x53,
    DEV_TMR_TCNT = 0x54,
    DEV_SMR = 0x58,
    DEV_BRR = 0x59,
    DEV_SCR = 0x5a,
    DEV_RDR = 0x5d,
    DEV_ADDRAH = 0x60,
    DEV_ADDRAL = 0x61,
    DEV_ADDRBH = 0x62,
    DEV_ADDRBL = 0x63,
    DEV_ADDRCH = 0x64,
    DEV_ADDRCL = 0x65,
    DEV_ADDRDH = 0x66,
    DEV_ADDRDL = 0x67,
    DEV_ADCSR = 0x68,
    DEV_IPRA = 0x70,
    DEV_IPRB = 0x71,
    DEV_IPRC = 0x72,
    DEV_IPRD = 0x73,
    DEV_DTEA = 0x74,
    DEV_DTEB = 0x75,
    DEV_DTEC = 0x76,
    DEV_DTED = 0x77,
    DEV_WCR = 0x78,
    DEV_RAME = 0x79,
    DEV_P1CR = 0x7c,
    DEV_P9DDR = 0x7e,
    DEV_P9DR = 0x7f,
};

const uint16_t sr_mask = 0x870f;
enum {
    STATUS_T = 0x8000,
    STATUS_N = 0x08,
    STATUS_Z = 0x04,
    STATUS_V = 0x02,
    STATUS_C = 0x01,
    STATUS_INT_MASK = 0x700
};

enum {
    VECTOR_RESET = 0,
    VECTOR_RESERVED1, // UNUSED
    VECTOR_INVALID_INSTRUCTION,
    VECTOR_DIVZERO,
    VECTOR_TRAP,
    VECTOR_RESERVED2, // UNUSED
    VECTOR_RESERVED3, // UNUSED
    VECTOR_RESERVED4, // UNUSED
    VECTOR_ADDRESS_ERROR,
    VECTOR_TRACE,
    VECTOR_RESERVED5, // UNUSED
    VECTOR_NMI,
    VECTOR_RESERVED6, // UNUSED
    VECTOR_RESERVED7, // UNUSED
    VECTOR_RESERVED8, // UNUSED
    VECTOR_RESERVED9, // UNUSED
    VECTOR_TRAPA_0,
    VECTOR_TRAPA_1,
    VECTOR_TRAPA_2,
    VECTOR_TRAPA_3,
    VECTOR_TRAPA_4,
    VECTOR_TRAPA_5,
    VECTOR_TRAPA_6,
    VECTOR_TRAPA_7,
    VECTOR_TRAPA_8,
    VECTOR_TRAPA_9,
    VECTOR_TRAPA_A,
    VECTOR_TRAPA_B,
    VECTOR_TRAPA_C,
    VECTOR_TRAPA_D,
    VECTOR_TRAPA_E,
    VECTOR_TRAPA_F,
    VECTOR_IRQ0,
    VECTOR_IRQ1,
    VECTOR_INTERNAL_INTERRUPT_88, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_8C, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_90, // FRT1 ICI
    VECTOR_INTERNAL_INTERRUPT_94, // FRT1 OCIA
    VECTOR_INTERNAL_INTERRUPT_98, // FRT1 OCIB
    VECTOR_INTERNAL_INTERRUPT_9C, // FRT1 FOVI
    VECTOR_INTERNAL_INTERRUPT_A0, // FRT2 ICI
    VECTOR_INTERNAL_INTERRUPT_A4, // FRT2 OCIA
    VECTOR_INTERNAL_INTERRUPT_A8, // FRT2 OCIB
    VECTOR_INTERNAL_INTERRUPT_AC, // FRT2 FOVI
    VECTOR_INTERNAL_INTERRUPT_B0, // FRT3 ICI
    VECTOR_INTERNAL_INTERRUPT_B4, // FRT3 OCIA
    VECTOR_INTERNAL_INTERRUPT_B8, // FRT3 OCIB
    VECTOR_INTERNAL_INTERRUPT_BC, // FRT3 FOVI
    VECTOR_INTERNAL_INTERRUPT_C0, // CMIA
    VECTOR_INTERNAL_INTERRUPT_C4, // CMIB
    VECTOR_INTERNAL_INTERRUPT_C8, // OVI
    VECTOR_INTERNAL_INTERRUPT_CC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_D0, // ERI
    VECTOR_INTERNAL_INTERRUPT_D4, // RXI
    VECTOR_INTERNAL_INTERRUPT_D8, // TXI
    VECTOR_INTERNAL_INTERRUPT_DC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_E0, // ADI
};


struct mcu_t {
    uint16_t r[8];
    uint16_t pc;
    uint16_t sr;
    uint8_t cp, dp, ep, tp, br;
    uint8_t sleep;
    uint8_t ex_ignore;
    int32_t exception_pending;
    uint8_t interrupt_pending[INTERRUPT_SOURCE_MAX];
    uint8_t trapa_pending[16];
    uint64_t cycles;
};

enum {
    MCU_BUTTON_POWER = 0,
    MCU_BUTTON_INST_L = 3,
    MCU_BUTTON_INST_R = 4,
    MCU_BUTTON_INST_MUTE = 5,
    MCU_BUTTON_INST_ALL = 6,
    MCU_BUTTON_MIDI_CH_L = 7,
    MCU_BUTTON_MIDI_CH_R = 8,
    MCU_BUTTON_CHORUS_L = 9,
    MCU_BUTTON_CHORUS_R = 10,
    MCU_BUTTON_PAN_L = 11,
    MCU_BUTTON_PAN_R = 12,
    MCU_BUTTON_PART_R = 13,
    MCU_BUTTON_KEY_SHIFT_L = 14,
    MCU_BUTTON_KEY_SHIFT_R = 15,
    MCU_BUTTON_REVERB_L = 16,
    MCU_BUTTON_REVERB_R = 17,
    MCU_BUTTON_LEVEL_L = 18,
    MCU_BUTTON_LEVEL_R = 19,
    MCU_BUTTON_PART_L = 20
};

static const int ROM1_SIZE = 0x8000;
static const int ROM2_SIZE = 0x80000;
static const int RAM_SIZE = 0x400;
static const int SRAM_SIZE = 0x8000;
static const int ROMSM_SIZE = 0x1000;

static const int audio_buffer_size = 4096 * 8;
static const int audio_page_size = 512;

const size_t rf_num = 5;

struct MCU {
    int mcu_mk1 = 0; // 0 - SC-55mkII, SC-55ST. 1 - SC-55, CM-300/SCC-1

    SDL_atomic_t mcu_button_pressed = {0};

    uint8_t mcu_p0_data = 0x00;
    uint8_t mcu_p1_data = 0x00;

    mcu_t mcu;

    uint8_t rom1[ROM1_SIZE];
    uint8_t rom2[ROM2_SIZE];
    uint8_t ram[RAM_SIZE];
    uint8_t sram[SRAM_SIZE];

    int rom2_mask = ROM2_SIZE - 1;

    short sample_buffer[audio_buffer_size] = {0};
    int sample_write_ptr = 0;

    int ga_int[8] = {0};
    int ga_int_enable = 0;
    int ga_int_trigger = 0;

    uint8_t dev_register[0x80] = {0};

    uint16_t ad_val[4] = {0};
    uint8_t ad_nibble = 0x00;
    uint8_t sw_pos = 0;
    uint8_t io_sd = 0x00;

    int adf_rd = 0;
    uint64_t analog_end_time = 0;

    SDL_mutex *init_lock;

    FILE *s_rf[rf_num] =
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    Pcm pcm;
    LCD lcd;
    MCU_Timer mcu_timer;
    SubMcu sub_mcu;

    MCU();

    void MCU_ErrorTrap(void);

    uint8_t MCU_Read(uint32_t address);
    uint16_t MCU_Read16(uint32_t address);
    uint32_t MCU_Read32(uint32_t address);
    void MCU_Write(uint32_t address, uint8_t value);
    void MCU_Write16(uint32_t address, uint16_t value);

    uint8_t MCU_ReadP0(void);
    uint8_t MCU_ReadP1(void);
    void MCU_WriteP0(uint8_t data);
    void MCU_WriteP1(uint8_t data);
    void MCU_GA_SetGAInt(int line, int value);

    void MCU_PostSample(int *sample);

    int startSC55(std::string *basepath);
    int updateSC55(int16_t *data, unsigned int dataSize);
    void postMidiSC55(uint8_t* message, int length);
    void SC55_Reset();

    uint8_t RCU_Read(void);
    uint16_t MCU_AnalogReadPin(uint32_t pin);
    void MCU_AnalogSample(int channel);
    void MCU_DeviceWrite(uint32_t address, uint8_t data);
    uint8_t MCU_DeviceRead(uint32_t address);
    void MCU_DeviceReset(void);
    void MCU_UpdateAnalog(uint64_t cycles);
    void MCU_ReadInstruction(void);
    void MCU_Init(void);
    void MCU_Reset(void);
    void MCU_PatchROM(void);
    void closeAllR();

    uint32_t MCU_GetAddress(uint8_t page, uint16_t address);
    uint8_t MCU_ReadCode(void);
    uint8_t MCU_ReadCodeAdvance(void);
    void MCU_SetRegisterByte(uint8_t reg, uint8_t val);
    uint32_t MCU_GetVectorAddress(uint32_t vector);
    uint32_t MCU_GetPageForRegister(uint32_t reg);
    void MCU_ControlRegisterWrite(uint32_t reg, uint32_t siz, uint32_t data);
    uint32_t MCU_ControlRegisterRead(uint32_t reg, uint32_t siz);
    void MCU_SetStatus(uint32_t condition, uint32_t mask);
    void MCU_PushStack(uint16_t data);
    uint16_t MCU_PopStack(void);
};
