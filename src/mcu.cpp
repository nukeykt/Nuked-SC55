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
#include <stdio.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_timer.h"
#include "pcm.h"
#include "lcd.h"
#include "submcu.h"
#include "midi.h"
#include "utf8main.h"
#include "utils/files.h"

#if __linux__
#include <unistd.h>
#include <limits.h>
#endif

const char* rs_name[ROM_SET_COUNT] = {
    "SC-55mk2",
    "SC-55st",
    "SC-55mk1",
    "CM-300/SCC-1",
    "JV-880",
    "SCB-55",
    "RLP-3237",
    "SC-155",
    "SC-155mk2"
};

const char* roms[ROM_SET_COUNT][5] =
{
    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",

    "rom1.bin",
    "rom2_st.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",

    "sc55_rom1.bin",
    "sc55_rom2.bin",
    "sc55_waverom1.bin",
    "sc55_waverom2.bin",
    "sc55_waverom3.bin",

    "cm300_rom1.bin",
    "cm300_rom2.bin",
    "cm300_waverom1.bin",
    "cm300_waverom2.bin",
    "cm300_waverom3.bin",

    "jv880_rom1.bin",
    "jv880_rom2.bin",
    "jv880_waverom1.bin",
    "jv880_waverom2.bin",
    "jv880_waverom_expansion.bin",

    "scb55_rom1.bin",
    "scb55_rom2.bin",
    "scb55_waverom1.bin",
    "scb55_waverom2.bin",
    "",

    "rlp3237_rom1.bin",
    "rlp3237_rom2.bin",
    "rlp3237_waverom1.bin",
    "",
    "",

    "sc155_rom1.bin",
    "sc155_rom2.bin",
    "sc155_waverom1.bin",
    "sc155_waverom2.bin",
    "sc155_waverom3.bin",

    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",
};

static int audio_buffer_size;
static int audio_page_size;
static short *sample_buffer;

static int sample_read_ptr;
static int sample_write_ptr;

static SDL_AudioDeviceID sdl_audio;

void MCU_ErrorTrap(mcu_t& mcu)
{
    printf("%.2x %.4x\n", mcu.cp, mcu.pc);
}

uint8_t RCU_Read(void)
{
    return 0;
}

enum {
    ANALOG_LEVEL_RCU_LOW = 0,
    ANALOG_LEVEL_RCU_HIGH = 0,
    ANALOG_LEVEL_SW_0 = 0,
    ANALOG_LEVEL_SW_1 = 0x155,
    ANALOG_LEVEL_SW_2 = 0x2aa,
    ANALOG_LEVEL_SW_3 = 0x3ff,
    ANALOG_LEVEL_BATTERY = 0x2a0,
};

uint16_t MCU_SC155Sliders(mcu_t& mcu, uint32_t index)
{
    // 0 - 1/9
    // 1 - 2/10
    // 2 - 3/11
    // 3 - 4/12
    // 4 - 5/13
    // 5 - 6/14
    // 6 - 7/15
    // 7 - 8/16
    // 8 - ALL
    return 0x0;
}

uint16_t MCU_AnalogReadPin(mcu_t& mcu, uint32_t pin)
{
    if (mcu.mcu_cm300)
        return 0;
    if (mcu.mcu_jv880)
    {
        if (pin == 1)
            return ANALOG_LEVEL_BATTERY;
        return 0x3ff;
    }
    if (0)
    {
READ_RCU:
        uint8_t rcu = RCU_Read();
        if (rcu & (1 << pin))
            return ANALOG_LEVEL_RCU_HIGH;
        else
            return ANALOG_LEVEL_RCU_LOW;
    }
    if (mcu.mcu_mk1)
    {
        if (mcu.mcu_sc155 && (mcu.dev_register[DEV_P9DR] & 1) != 0)
        {
            return MCU_SC155Sliders(mcu, pin);
        }
        if (pin == 7)
        {
            if (mcu.mcu_sc155 && (mcu.dev_register[DEV_P9DR] & 2) != 0)
                return MCU_SC155Sliders(mcu, 8);
            else
                return ANALOG_LEVEL_BATTERY;
        }
        else
            goto READ_RCU;
    }
    else
    {
        if (mcu.mcu_sc155 && (mcu.io_sd & 16) != 0)
        {
            return MCU_SC155Sliders(mcu, pin);
        }
        if (pin == 7)
        {
            if (mcu.mcu_mk1)
                return ANALOG_LEVEL_BATTERY;
            switch ((mcu.io_sd >> 2) & 3)
            {
            case 0: // Battery voltage
                return ANALOG_LEVEL_BATTERY;
            case 1: // NC
                if (mcu.mcu_sc155)
                    return MCU_SC155Sliders(mcu, 8);
                return 0;
            case 2: // SW
                switch (mcu.sw_pos)
                {
                case 0:
                default:
                    return ANALOG_LEVEL_SW_0;
                case 1:
                    return ANALOG_LEVEL_SW_1;
                case 2:
                    return ANALOG_LEVEL_SW_2;
                case 3:
                    return ANALOG_LEVEL_SW_3;
                }
            case 3: // RCU
                goto READ_RCU;
            }
        }
        else
            goto READ_RCU;
    }
}

void MCU_AnalogSample(mcu_t& mcu, int channel)
{
    int value = MCU_AnalogReadPin(mcu, channel);
    int dest = (channel << 1) & 6;
    mcu.dev_register[DEV_ADDRAH + dest] = value >> 2;
    mcu.dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

void MCU_DeviceWrite(mcu_t& mcu, uint32_t address, uint8_t data)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        TIMER_Write(*mcu.timer, address, data);
        return;
    }
    if (address >= 0x50 && address < 0x55)
    {
        TIMER2_Write(*mcu.timer, address, data);
        return;
    }
    switch (address)
    {
    case DEV_P1DDR: // P1DDR
        break;
    case DEV_P5DDR:
        break;
    case DEV_P6DDR:
        break;
    case DEV_P7DDR:
        break;
    case DEV_SCR:
        break;
    case DEV_WCR:
        break;
    case DEV_P9DDR:
        break;
    case DEV_RAME: // RAME
        break;
    case DEV_P1CR: // P1CR
        break;
    case DEV_DTEA:
        break;
    case DEV_DTEB:
        break;
    case DEV_DTEC:
        break;
    case DEV_DTED:
        break;
    case DEV_SMR:
        break;
    case DEV_BRR:
        break;
    case DEV_IPRA:
        break;
    case DEV_IPRB:
        break;
    case DEV_IPRC:
        break;
    case DEV_IPRD:
        break;
    case DEV_PWM1_DTR:
        break;
    case DEV_PWM1_TCR:
        break;
    case DEV_PWM2_DTR:
        break;
    case DEV_PWM2_TCR:
        break;
    case DEV_PWM3_DTR:
        break;
    case DEV_PWM3_TCR:
        break;
    case DEV_P7DR:
        break;
    case DEV_TMR_TCNT:
        break;
    case DEV_TMR_TCR:
        break;
    case DEV_TMR_TCSR:
        break;
    case DEV_TMR_TCORA:
        break;
    case DEV_TDR:
        break;
    case DEV_ADCSR:
    {
        mcu.dev_register[address] &= ~0x7f;
        mcu.dev_register[address] |= data & 0x7f;
        if ((data & 0x80) == 0 && mcu.adf_rd)
        {
            mcu.dev_register[address] &= ~0x80;
            MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_ANALOG, 0);
        }
        if ((data & 0x40) == 0)
            MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_ANALOG, 0);
        return;
    }
    case DEV_SSR:
    {
        if ((data & 0x80) == 0 && (mcu.ssr_rd & 0x80) != 0)
        {
            mcu.dev_register[address] &= ~0x80;
            mcu.uart_tx_delay = mcu.cycles + 3000;
            MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_UART_TX, 0);
        }
        if ((data & 0x40) == 0 && (mcu.ssr_rd & 0x40) != 0)
        {
            mcu.uart_rx_delay = mcu.cycles + 3000;
            mcu.dev_register[address] &= ~0x40;
            MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_UART_RX, 0);
        }
        if ((data & 0x20) == 0 && (mcu.ssr_rd & 0x20) != 0)
        {
            mcu.dev_register[address] &= ~0x20;
        }
        if ((data & 0x10) == 0 && (mcu.ssr_rd & 0x10) != 0)
        {
            mcu.dev_register[address] &= ~0x10;
        }
        break;
    }
    default:
        address += 0;
        break;
    }
    mcu.dev_register[address] = data;
}

uint8_t MCU_DeviceRead(mcu_t& mcu, uint32_t address)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        return TIMER_Read(*mcu.timer, address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return TIMER_Read2(*mcu.timer, address);
    }
    switch (address)
    {
    case DEV_ADDRAH:
    case DEV_ADDRAL:
    case DEV_ADDRBH:
    case DEV_ADDRBL:
    case DEV_ADDRCH:
    case DEV_ADDRCL:
    case DEV_ADDRDH:
    case DEV_ADDRDL:
        return mcu.dev_register[address];
    case DEV_ADCSR:
        mcu.adf_rd = (mcu.dev_register[address] & 0x80) != 0;
        return mcu.dev_register[address];
    case DEV_SSR:
        mcu.ssr_rd = mcu.dev_register[address];
        return mcu.dev_register[address];
    case DEV_RDR:
        return mcu.uart_rx_byte;
    case 0x00:
        return 0xff;
    case DEV_P7DR:
    {
        if (!mcu.mcu_jv880) return 0xff;

        uint8_t data = 0xff;
        uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu.mcu_button_pressed);

        if (mcu.io_sd == 0b11111011)
            data &= ((button_pressed >> 0) & 0b11111) ^ 0xFF;
        if (mcu.io_sd == 0b11110111)
            data &= ((button_pressed >> 5) & 0b11111) ^ 0xFF;
        if (mcu.io_sd == 0b11101111)
            data &= ((button_pressed >> 10) & 0b1111) ^ 0xFF;

        data |= 0b10000000;
        return data;
    }
    case DEV_P9DR:
    {
        int cfg = 0;
        if (!mcu.mcu_mk1)
            cfg = mcu.mcu_sc155 ? 0 : 2; // bit 1: 0 - SC-155mk2 (???), 1 - SC-55mk2

        int dir = mcu.dev_register[DEV_P9DDR];

        int val = cfg & (dir ^ 0xff);
        val |= mcu.dev_register[DEV_P9DR] & dir;
        return val;
    }
    case DEV_SCR:
    case DEV_TDR:
    case DEV_SMR:
        return mcu.dev_register[address];
    case DEV_IPRC:
    case DEV_IPRD:
    case DEV_DTEC:
    case DEV_DTED:
    case DEV_FRT2_TCSR:
    case DEV_FRT1_TCSR:
    case DEV_FRT1_TCR:
    case DEV_FRT1_FRCH:
    case DEV_FRT1_FRCL:
    case DEV_FRT3_TCSR:
    case DEV_FRT3_OCRAH:
    case DEV_FRT3_OCRAL:
        return mcu.dev_register[address];
    }
    return mcu.dev_register[address];
}

void MCU_DeviceReset(mcu_t& mcu)
{
    // mcu.dev_register[0x00] = 0x03;
    // mcu.dev_register[0x7c] = 0x87;
    mcu.dev_register[DEV_RAME] = 0x80;
    mcu.dev_register[DEV_SSR] = 0x80;
}

void MCU_UpdateAnalog(mcu_t& mcu, uint64_t cycles)
{
    int ctrl = mcu.dev_register[DEV_ADCSR];
    int isscan = (ctrl & 16) != 0;

    if (ctrl & 0x20)
    {
        if (mcu.analog_end_time == 0)
            mcu.analog_end_time = cycles + 200;
        else if (mcu.analog_end_time < cycles)
        {
            if (isscan)
            {
                int base = ctrl & 4;
                for (int i = 0; i <= (ctrl & 3); i++)
                    MCU_AnalogSample(mcu, base + i);
                mcu.analog_end_time = cycles + 200;
            }
            else
            {
                MCU_AnalogSample(mcu, ctrl & 7);
                mcu.dev_register[DEV_ADCSR] &= ~0x20;
                mcu.analog_end_time = 0;
            }
            mcu.dev_register[DEV_ADCSR] |= 0x80;
            if (ctrl & 0x40)
                MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        mcu.analog_end_time = 0;
}

uint8_t MCU_Read(mcu_t& mcu, uint32_t address)
{
    uint32_t address_rom = address & 0x3ffff;
    if (address & 0x80000 && !mcu.mcu_jv880)
        address_rom |= 0x40000;
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    uint8_t ret = 0xff;
    switch (page)
    {
    case 0:
        if (!(address & 0x8000))
            ret = mcu.rom1[address & 0x7fff];
        else
        {
            if (!mcu.mcu_mk1)
            {
                uint16_t base = mcu.mcu_jv880 ? 0xf000 : 0xe000;
                if (address >= base && address < (base | 0x400))
                {
                    ret = PCM_Read(*mcu.pcm, address & 0x3f);
                }
                else if (!mcu.mcu_scb55 && address >= 0xec00 && address < 0xf000)
                {
                    ret = SM_SysRead(*mcu.sm, address & 0xff);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(mcu, address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (mcu.dev_register[DEV_RAME] & 0x80) != 0)
                    ret = mcu.ram[(address - 0xfb80) & 0x3ff];
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = mcu.sram[address & 0x7fff];
                }
                else if (address == (base | 0x402))
                {
                    ret = mcu.ga_int_trigger;
                    mcu.ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(mcu, mcu.mcu_jv880 ? INTERRUPT_SOURCE_IRQ0 : INTERRUPT_SOURCE_IRQ1, 0);
                }
                else
                {
                    printf("Unknown read %x\n", address);
                    ret = 0xff;
                }
                //
                // e402:2-0 irq source
                //
            }
            else
            {
                if (address >= 0xe000 && address < 0xe040)
                {
                    ret = PCM_Read(*mcu.pcm, address & 0x3f);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(mcu, address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (mcu.dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ret = mcu.ram[(address - 0xfb80) & 0x3ff];
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = mcu.sram[address & 0x7fff];
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    mcu.io_sd = address & 0xff;

                    if (mcu.mcu_cm300)
                        return 0xff;

                    LCD_Enable(*mcu.lcd, (mcu.io_sd & 8) != 0);

                    uint8_t data = 0xff;
                    uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu.mcu_button_pressed);

                    if ((mcu.io_sd & 1) == 0)
                        data &= ((button_pressed >> 0) & 255) ^ 255;
                    if ((mcu.io_sd & 2) == 0)
                        data &= ((button_pressed >> 8) & 255) ^ 255;
                    if ((mcu.io_sd & 4) == 0)
                        data &= ((button_pressed >> 16) & 255) ^ 255;
                    if ((mcu.io_sd & 8) == 0)
                        data &= ((button_pressed >> 24) & 255) ^ 255;
                    return data;
                }
                else if (address == 0xf106)
                {
                    ret = mcu.ga_int_trigger;
                    mcu.ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_IRQ1, 0);
                }
                else
                {
                    printf("Unknown read %x\n", address);
                    ret = 0xff;
                }
                //
                // f106:2-0 irq source
                //
            }
        }
        break;
#if 0
    case 3:
        ret = rom2[address | 0x30000];
        break;
    case 4:
        ret = rom2[address];
        break;
    case 10:
        ret = rom2[address | 0x60000]; // FIXME
        break;
    case 1:
        ret = rom2[address | 0x10000];
        break;
#endif
    case 1:
        ret = mcu.rom2[address_rom & mcu.rom2_mask];
        break;
    case 2:
        ret = mcu.rom2[address_rom & mcu.rom2_mask];
        break;
    case 3:
        ret = mcu.rom2[address_rom & mcu.rom2_mask];
        break;
    case 4:
        ret = mcu.rom2[address_rom & mcu.rom2_mask];
        break;
    case 8:
        if (!mcu.mcu_jv880)
            ret = mcu.rom2[address_rom & mcu.rom2_mask];
        else
            ret = 0xff;
        break;
    case 9:
        if (!mcu.mcu_jv880)
            ret = mcu.rom2[address_rom & mcu.rom2_mask];
        else
            ret = 0xff;
        break;
    case 14:
    case 15:
        if (!mcu.mcu_jv880)
            ret = mcu.rom2[address_rom & mcu.rom2_mask];
        else
            ret = mcu.cardram[address & 0x7fff]; // FIXME
        break;
    case 10:
    case 11:
        if (!mcu.mcu_mk1)
            ret = mcu.sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    case 12:
    case 13:
        if (mcu.mcu_jv880)
            ret = mcu.nvram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    case 5:
        if (mcu.mcu_mk1)
            ret = mcu.sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    default:
        ret = 0x00;
        break;
    }
    return ret;
}

uint16_t MCU_Read16(mcu_t& mcu, uint32_t address)
{
    address &= ~1;
    uint8_t b0, b1;
    b0 = MCU_Read(mcu, address);
    b1 = MCU_Read(mcu, address+1);
    return (b0 << 8) + b1;
}

uint32_t MCU_Read32(mcu_t& mcu, uint32_t address)
{
    address &= ~3;
    uint8_t b0, b1, b2, b3;
    b0 = MCU_Read(mcu, address);
    b1 = MCU_Read(mcu, address+1);
    b2 = MCU_Read(mcu, address+2);
    b3 = MCU_Read(mcu, address+3);
    return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

void MCU_Write(mcu_t& mcu, uint32_t address, uint8_t value)
{
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    if (page == 0)
    {
        if (address & 0x8000)
        {
            if (!mcu.mcu_mk1)
            {
                uint16_t base = mcu.mcu_jv880 ? 0xf000 : 0xe000;
                if (address >= (base | 0x400) && address < (base | 0x800))
                {
                    if (address == (base | 0x404) || address == (base | 0x405))
                        LCD_Write(*mcu.lcd, address & 1, value);
                    else if (address == (base | 0x401))
                    {
                        mcu.io_sd = value;
                        LCD_Enable(*mcu.lcd, (value & 1) == 0);
                    }
                    else if (address == (base | 0x402))
                        mcu.ga_int_enable = (value << 1);
                    else
                        printf("Unknown write %x %x\n", address, value);
                    //
                    // e400: always 4?
                    // e401: SC0-6?
                    // e402: enable/disable IRQ?
                    // e403: always 1?
                    // e404: LCD
                    // e405: LCD
                    // e406: 0 or 40
                    // e407: 0, e406 continuation?
                    //
                }
                else if (address >= (base | 0x000) && address < (base | 0x400))
                {
                    PCM_Write(*mcu.pcm, address & 0x3f, value);
                }
                else if (!mcu.mcu_scb55 && address >= 0xec00 && address < 0xf000)
                {
                    SM_SysWrite(*mcu.sm, address & 0xff, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(mcu, address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (mcu.dev_register[DEV_RAME] & 0x80) != 0)
                {
                    mcu.ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    mcu.sram[address & 0x7fff] = value;
                }
                else
                {
                    printf("Unknown write %x %x\n", address, value);
                }
            }
            else
            {
                if (address >= 0xe000 && address < 0xe040)
                {
                    PCM_Write(*mcu.pcm, address & 0x3f, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(mcu, address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (mcu.dev_register[DEV_RAME] & 0x80) != 0)
                {
                    mcu.ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    mcu.sram[address & 0x7fff] = value;
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    mcu.io_sd = address & 0xff;
                    LCD_Enable(*mcu.lcd, (mcu.io_sd & 8) != 0);
                }
                else if (address == 0xf105)
                {
                    LCD_Write(*mcu.lcd, 0, value);
                    mcu.ga_lcd_counter = 500;
                }
                else if (address == 0xf104)
                {
                    LCD_Write(*mcu.lcd, 1, value);
                    mcu.ga_lcd_counter = 500;
                }
                else if (address == 0xf107)
                {
                    mcu.io_sd = value;
                }
                else
                {
                    printf("Unknown write %x %x\n", address, value);
                }
            }
        }
        else
        {
            printf("Unknown write %x %x\n", address, value);
        }
    }
    else if (page == 5 && mcu.mcu_mk1)
    {
        mcu.sram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 10 && !mcu.mcu_mk1)
    {
        mcu.sram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 12 && mcu.mcu_jv880)
    {
        mcu.nvram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 14 && mcu.mcu_jv880)
    {
        mcu.cardram[address & 0x7fff] = value; // FIXME
    }
    else
    {
        printf("Unknown write %x %x\n", (page << 16) | address, value);
    }
}

void MCU_Write16(mcu_t& mcu, uint32_t address, uint16_t value)
{
    address &= ~1;
    MCU_Write(mcu, address, value >> 8);
    MCU_Write(mcu, address + 1, value & 0xff);
}

void MCU_ReadInstruction(mcu_t& mcu)
{
    uint8_t operand = MCU_ReadCodeAdvance(mcu);

    MCU_Operand_Table[operand](mcu, operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(mcu, EXCEPTION_SOURCE_TRACE);
    }
}

void MCU_Init(mcu_t& mcu, submcu_t& sm, pcm_t& pcm, mcu_timer_t& timer, lcd_t& lcd)
{
    memset(&mcu, 0, sizeof(mcu_t));
    mcu.sw_pos = 3;
    mcu.sm = &sm;
    mcu.pcm = &pcm;
    mcu.timer = &timer;
    mcu.lcd = &lcd;
    mcu.romset = ROM_SET_MK2;
    mcu.rom2_mask = ROM2_SIZE - 1;
}

void MCU_Reset(mcu_t& mcu)
{
    mcu.r[0] = 0;
    mcu.r[1] = 0;
    mcu.r[2] = 0;
    mcu.r[3] = 0;
    mcu.r[4] = 0;
    mcu.r[5] = 0;
    mcu.r[6] = 0;
    mcu.r[7] = 0;

    mcu.pc = 0;

    mcu.sr = 0x700;

    mcu.cp = 0;
    mcu.dp = 0;
    mcu.ep = 0;
    mcu.tp = 0;
    mcu.br = 0;

    uint32_t reset_address = MCU_GetVectorAddress(mcu, VECTOR_RESET);
    mcu.cp = (reset_address >> 16) & 0xff;
    mcu.pc = reset_address & 0xffff;

    mcu.exception_pending = -1;

    MCU_DeviceReset(mcu);

    if (mcu.mcu_mk1)
    {
        mcu.ga_int_enable = 255;
    }
}

void MCU_PostUART(mcu_t& mcu, uint8_t data)
{
    mcu.uart_buffer[mcu.uart_write_ptr] = data;
    mcu.uart_write_ptr = (mcu.uart_write_ptr + 1) % uart_buffer_size;
}

void MCU_UpdateUART_RX(mcu_t& mcu)
{
    if ((mcu.dev_register[DEV_SCR] & 16) == 0) // RX disabled
        return;
    if (mcu.uart_write_ptr == mcu.uart_read_ptr) // no byte
        return;

    if (mcu.dev_register[DEV_SSR] & 0x40)
        return;

    if (mcu.cycles < mcu.uart_rx_delay)
        return;

    mcu.uart_rx_byte = mcu.uart_buffer[mcu.uart_read_ptr];
    mcu.uart_read_ptr = (mcu.uart_read_ptr + 1) % uart_buffer_size;
    mcu.dev_register[DEV_SSR] |= 0x40;
    MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_UART_RX, (mcu.dev_register[DEV_SCR] & 0x40) != 0);
}

// dummy TX
void MCU_UpdateUART_TX(mcu_t& mcu)
{
    if ((mcu.dev_register[DEV_SCR] & 32) == 0) // TX disabled
        return;

    if (mcu.dev_register[DEV_SSR] & 0x80)
        return;

    if (mcu.cycles < mcu.uart_tx_delay)
        return;

    mcu.dev_register[DEV_SSR] |= 0x80;
    MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_UART_TX, (mcu.dev_register[DEV_SCR] & 0x80) != 0);

    // printf("tx:%x\n", mcu.dev_register[DEV_TDR]);
}

static bool work_thread_run = false;

static SDL_mutex *work_thread_lock;

void MCU_WorkThread_Lock(void)
{
    SDL_LockMutex(work_thread_lock);
}

void MCU_WorkThread_Unlock(void)
{
    SDL_UnlockMutex(work_thread_lock);
}

int SDLCALL work_thread(void* data)
{
    mcu_t& mcu = *(mcu_t*)data;
    work_thread_lock = SDL_CreateMutex();

    MCU_WorkThread_Lock();
    while (work_thread_run)
    {
        if (mcu.pcm->config_reg_3c & 0x40)
            sample_write_ptr &= ~3;
        else
            sample_write_ptr &= ~1;
        if (sample_read_ptr == sample_write_ptr)
        {
            MCU_WorkThread_Unlock();
            while (sample_read_ptr == sample_write_ptr)
            {
                SDL_Delay(1);
            }
            MCU_WorkThread_Lock();
        }

        if (!mcu.ex_ignore)
            MCU_Interrupt_Handle(mcu);
        else
            mcu.ex_ignore = 0;

        if (!mcu.sleep)
            MCU_ReadInstruction(mcu);

        mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

        // if (mcu.cycles % 24000000 == 0)
        //     printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

        PCM_Update(*mcu.pcm, mcu.cycles);

        TIMER_Clock(*mcu.timer, mcu.cycles);

        if (!mcu.mcu_mk1 && !mcu.mcu_jv880 && !mcu.mcu_scb55)
            SM_Update(*mcu.sm, mcu.cycles);
        else
        {
            MCU_UpdateUART_RX(mcu);
            MCU_UpdateUART_TX(mcu);
        }

        MCU_UpdateAnalog(mcu, mcu.cycles);

        if (mcu.mcu_mk1)
        {
            if (mcu.ga_lcd_counter)
            {
                mcu.ga_lcd_counter--;
                if (mcu.ga_lcd_counter == 0)
                {
                    MCU_GA_SetGAInt(mcu, 1, 0);
                    MCU_GA_SetGAInt(mcu, 1, 1);
                }
            }
        }
    }
    MCU_WorkThread_Unlock();

    SDL_DestroyMutex(work_thread_lock);

    return 0;
}

static void MCU_Run(mcu_t& mcu)
{
    bool working = true;

    work_thread_run = true;
    SDL_Thread *thread = SDL_CreateThread(work_thread, "work thread", &mcu);

    while (working)
    {
        if(LCD_QuitRequested(*mcu.lcd))
            working = false;

        LCD_Update(*mcu.lcd);

        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            LCD_HandleEvent(*mcu.lcd, sdl_event);
        }

        SDL_Delay(15);
    }

    work_thread_run = false;
    SDL_WaitThread(thread, 0);
}

void MCU_PatchROM(mcu_t& mcu)
{
    //rom2[0x1333] = 0x11;
    //rom2[0x1334] = 0x19;
    //rom1[0x622d] = 0x19;
}

uint8_t MCU_ReadP0(mcu_t& mcu)
{
    return 0xff;
}

uint8_t MCU_ReadP1(mcu_t& mcu)
{
    uint8_t data = 0xff;
    uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu.mcu_button_pressed);

    if ((mcu.mcu_p0_data & 1) == 0)
        data &= ((button_pressed >> 0) & 255) ^ 255;
    if ((mcu.mcu_p0_data & 2) == 0)
        data &= ((button_pressed >> 8) & 255) ^ 255;
    if ((mcu.mcu_p0_data & 4) == 0)
        data &= ((button_pressed >> 16) & 255) ^ 255;
    if ((mcu.mcu_p0_data & 8) == 0)
        data &= ((button_pressed >> 24) & 255) ^ 255;

    return data;
}

void MCU_WriteP0(mcu_t& mcu, uint8_t data)
{
    mcu.mcu_p0_data = data;
}

void MCU_WriteP1(mcu_t& mcu, uint8_t data)
{
    mcu.mcu_p1_data = data;
}

uint8_t tempbuf[0x800000];

void unscramble(uint8_t *src, uint8_t *dst, int len)
{
    for (int i = 0; i < len; i++)
    {
        int address = i & ~0xfffff;
        static const int aa[] = {
            2, 0, 3, 4, 1, 9, 13, 10, 18, 17, 6, 15, 11, 16, 8, 5, 12, 7, 14, 19
        };
        for (int j = 0; j < 20; j++)
        {
            if (i & (1 << j))
                address |= 1<<aa[j];
        }
        uint8_t srcdata = src[address];
        uint8_t data = 0;
        static const int dd[] = {
            2, 0, 4, 5, 7, 6, 3, 1
        };
        for (int j = 0; j < 8; j++)
        {
            if (srcdata & (1 << dd[j]))
                data |= 1<<j;
        }
        dst[i] = data;
    }
}

void audio_callback(void* /*userdata*/, Uint8* stream, int len)
{
    len /= 2;
    memcpy(stream, &sample_buffer[sample_read_ptr], len * 2);
    memset(&sample_buffer[sample_read_ptr], 0, len * 2);
    sample_read_ptr += len;
    sample_read_ptr %= audio_buffer_size;
}

static const char* audio_format_to_str(int format)
{
    switch(format)
    {
    case AUDIO_S8:
        return "S8";
    case AUDIO_U8:
        return "U8";
    case AUDIO_S16MSB:
        return "S16MSB";
    case AUDIO_S16LSB:
        return "S16LSB";
    case AUDIO_U16MSB:
        return "U16MSB";
    case AUDIO_U16LSB:
        return "U16LSB";
    case AUDIO_S32MSB:
        return "S32MSB";
    case AUDIO_S32LSB:
        return "S32LSB";
    case AUDIO_F32MSB:
        return "F32MSB";
    case AUDIO_F32LSB:
        return "F32LSB";
    }
    return "UNK";
}

int MCU_OpenAudio(mcu_t& mcu, int deviceIndex, int pageSize, int pageNum)
{
    SDL_AudioSpec spec = {};
    SDL_AudioSpec spec_actual = {};

    audio_page_size = (pageSize/2)*2; // must be even
    audio_buffer_size = audio_page_size*pageNum;
    
    spec.format = AUDIO_S16SYS;
    spec.freq = (mcu.mcu_mk1 || mcu.mcu_jv880) ? 64000 : 66207;
    spec.channels = 2;
    spec.callback = audio_callback;
    spec.samples = audio_page_size / 4;
    
    sample_buffer = (short*)calloc(audio_buffer_size, sizeof(short));
    if (!sample_buffer)
    {
        printf("Cannot allocate audio buffer.\n");
        return 0;
    }
    sample_read_ptr = 0;
    sample_write_ptr = 0;
    
    int num = SDL_GetNumAudioDevices(0);
    if (num == 0)
    {
        printf("No audio output device found.\n");
        return 0;
    }
    
    if (deviceIndex < -1 || deviceIndex >= num)
    {
        printf("Out of range audio device index is requested. Default audio output device is selected.\n");
        deviceIndex = -1;
    }
    
    const char* audioDevicename = deviceIndex == -1 ? "Default device" : SDL_GetAudioDeviceName(deviceIndex, 0);
    
    sdl_audio = SDL_OpenAudioDevice(deviceIndex == -1 ? NULL : audioDevicename, 0, &spec, &spec_actual, 0);
    if (!sdl_audio)
    {
        return 0;
    }

    printf("Audio device: %s\n", audioDevicename);

    printf("Audio Requested: F=%s, C=%d, R=%d, B=%d\n",
           audio_format_to_str(spec.format),
           spec.channels,
           spec.freq,
           spec.samples);

    printf("Audio Actual: F=%s, C=%d, R=%d, B=%d\n",
           audio_format_to_str(spec_actual.format),
           spec_actual.channels,
           spec_actual.freq,
           spec_actual.samples);
    fflush(stdout);

    SDL_PauseAudioDevice(sdl_audio, 0);

    return 1;
}

void MCU_CloseAudio(void)
{
    SDL_CloseAudio();
    if (sample_buffer) free(sample_buffer);
}

void MCU_PostSample(int *sample)
{
    sample[0] >>= 15;
    if (sample[0] > INT16_MAX)
        sample[0] = INT16_MAX;
    else if (sample[0] < INT16_MIN)
        sample[0] = INT16_MIN;
    sample[1] >>= 15;
    if (sample[1] > INT16_MAX)
        sample[1] = INT16_MAX;
    else if (sample[1] < INT16_MIN)
        sample[1] = INT16_MIN;
    sample_buffer[sample_write_ptr + 0] = sample[0];
    sample_buffer[sample_write_ptr + 1] = sample[1];
    sample_write_ptr = (sample_write_ptr + 2) % audio_buffer_size;
}

void MCU_GA_SetGAInt(mcu_t& mcu, int line, int value)
{
    // guesswork
    if (value && !mcu.ga_int[line] && (mcu.ga_int_enable & (1 << line)) != 0)
        mcu.ga_int_trigger = line;
    mcu.ga_int[line] = value;

    if (mcu.mcu_jv880)
        MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_IRQ0, mcu.ga_int_trigger != 0);
    else
        MCU_Interrupt_SetRequest(mcu, INTERRUPT_SOURCE_IRQ1, mcu.ga_int_trigger != 0);
}

void MCU_EncoderTrigger(mcu_t& mcu, int dir)
{
    if (!mcu.mcu_jv880) return;
    MCU_GA_SetGAInt(mcu, dir == 0 ? 3 : 4, 0);
    MCU_GA_SetGAInt(mcu, dir == 0 ? 3 : 4, 1);
}


static const size_t rf_num = 5;
static FILE *s_rf[rf_num] =
{
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static void closeAllR()
{
    for(size_t i = 0; i < rf_num; ++i)
    {
        if(s_rf[i])
            fclose(s_rf[i]);
        s_rf[i] = nullptr;
    }
}

enum class ResetType {
    NONE,
    GS_RESET,
    GM_RESET,
};

void MIDI_Reset(mcu_t& mcu, ResetType resetType)
{
    const unsigned char gmReset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
    const unsigned char gsReset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
    
    if (resetType == ResetType::GS_RESET)
    {
        for (size_t i = 0; i < sizeof(gsReset); i++)
        {
            MCU_PostUART(mcu, gsReset[i]);
        }
    }
    else  if (resetType == ResetType::GM_RESET)
    {
        for (size_t i = 0; i < sizeof(gmReset); i++)
        {
            MCU_PostUART(mcu, gmReset[i]);
        }
    }

}

int main(int argc, char *argv[])
{
    (void)argc;
    std::string basePath;

    int port = 0;
    int audioDeviceIndex = -1;
    int pageSize = 512;
    int pageNum = 32;
    bool autodetect = true;
    ResetType resetType = ResetType::NONE;

    int romset = ROM_SET_MK2;

    mcu_t mcu;
    submcu_t sm;
    mcu_timer_t timer;
    // allocate pcm because the bitmaps will overflow the stack
    lcd_t* lcd = (lcd_t*)malloc(sizeof(lcd_t));
    // allocate pcm because the waveroms will overflow the stack
    pcm_t* pcm = (pcm_t*)malloc(sizeof(pcm_t));
    PCM_Reset(*pcm, mcu);
    MCU_Init(mcu, sm, *pcm, timer, *lcd);
    TIMER_Init(timer, mcu);

    {
        for (int i = 1; i < argc; i++)
        {
            if (!strncmp(argv[i], "-p:", 3))
            {
                port = atoi(argv[i] + 3);
            }
            else if (!strncmp(argv[i], "-a:", 3))
            {
                audioDeviceIndex = atoi(argv[i] + 3);
            }
            else if (!strncmp(argv[i], "-ab:", 4))
            {
                char* pColon = argv[i] + 3;
                
                if (pColon[1] != 0)
                {
                    pageSize = atoi(++pColon);
                    pColon = strchr(pColon, ':');
                    if (pColon && pColon[1] != 0)
                    {
                        pageNum = atoi(++pColon);
                    }
                }
                
                // reset both if either is invalid
                if (pageSize <= 0 || pageNum <= 0)
                {
                    pageSize = 512;
                    pageNum = 32;
                }
            }
            else if (!strcmp(argv[i], "-mk2"))
            {
                romset = ROM_SET_MK2;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-st"))
            {
                romset = ROM_SET_ST;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-mk1"))
            {
                romset = ROM_SET_MK1;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-cm300"))
            {
                romset = ROM_SET_CM300;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-jv880"))
            {
                romset = ROM_SET_JV880;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-scb55"))
            {
                romset = ROM_SET_SCB55;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-rlp3237"))
            {
                romset = ROM_SET_RLP3237;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-gs"))
            {
                resetType = ResetType::GS_RESET;
            }
            else if (!strcmp(argv[i], "-gm"))
            {
                resetType = ResetType::GM_RESET;
            }
            else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
            {
                // TODO: Might want to try to find a way to print out the executable's actual name (without any full paths).
                printf("Usage: nuked-sc55 [options]\n");
                printf("Options:\n");
                printf("  -h, -help, --help              Display this information.\n");
                printf("\n");
                printf("  -p:<port_number>               Set MIDI port.\n");
                printf("  -a:<device_number>             Set Audio Device index.\n");
                printf("  -ab:<page_size>:[page_count]   Set Audio Buffer size.\n");
                printf("\n");
                printf("  -mk2                           Use SC-55mk2 ROM set.\n");
                printf("  -st                            Use SC-55st ROM set.\n");
                printf("  -mk1                           Use SC-55mk1 ROM set.\n");
                printf("  -cm300                         Use CM-300/SCC-1 ROM set.\n");
                printf("  -jv880                         Use JV-880 ROM set.\n");
                printf("  -scb55                         Use SCB-55 ROM set.\n");
                printf("  -rlp3237                       Use RLP-3237 ROM set.\n");
                printf("\n");
                printf("  -gs                            Reset system in GS mode.\n");
                printf("  -gm                            Reset system in GM mode.\n");
                return 0;
            }
            else if (!strcmp(argv[i], "-sc155"))
            {
                romset = ROM_SET_SC155;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-sc155mk2"))
            {
                romset = ROM_SET_SC155MK2;
                autodetect = false;
            }
        }
    }

#if __linux__
    char self_path[PATH_MAX];
    memset(&self_path[0], 0, PATH_MAX);

    if(readlink("/proc/self/exe", self_path, PATH_MAX) == -1)
        basePath = Files::real_dirname(argv[0]);
    else
        basePath = Files::dirname(self_path);
#else
    basePath = Files::real_dirname(argv[0]);
#endif

    printf("Base path is: %s\n", argv[0]);

    if(Files::dirExists(basePath + "/../share/nuked-sc55"))
        basePath += "/../share/nuked-sc55";

    if (autodetect)
    {
        for (size_t i = 0; i < ROM_SET_COUNT; i++)
        {
            bool good = true;
            for (size_t j = 0; j < 5; j++)
            {
                if (roms[i][j][0] == '\0')
                    continue;
                std::string path = basePath + "/" + roms[i][j];
                auto h = Files::utf8_fopen(path.c_str(), "rb");
                if (!h)
                {
                    good = false;
                    break;
                }
                fclose(h);
            }
            if (good)
            {
                romset = i;
                break;
            }
        }
        printf("ROM set autodetect: %s\n", rs_name[romset]);
    }

    mcu.romset = romset;
    mcu.mcu_mk1 = false;
    mcu.mcu_cm300 = false;
    mcu.mcu_st = false;
    mcu.mcu_jv880 = false;
    mcu.mcu_scb55 = false;
    mcu.mcu_sc155 = false;
    switch (romset)
    {
        case ROM_SET_MK2:
        case ROM_SET_SC155MK2:
            if (romset == ROM_SET_SC155MK2)
                mcu.mcu_sc155 = true;
            break;
        case ROM_SET_ST:
            mcu.mcu_st = true;
            break;
        case ROM_SET_MK1:
        case ROM_SET_SC155:
            mcu.mcu_mk1 = true;
            mcu.mcu_st = false;
            if (romset == ROM_SET_SC155)
                mcu.mcu_sc155 = true;
            break;
        case ROM_SET_CM300:
            mcu.mcu_mk1 = true;
            mcu.mcu_cm300 = true;
            break;
        case ROM_SET_JV880:
            mcu.mcu_jv880 = true;
            mcu.rom2_mask /= 2; // rom is half the size
            break;
        case ROM_SET_SCB55:
        case ROM_SET_RLP3237:
            mcu.mcu_scb55 = true;
            break;
    }

    std::string rpaths[5];

    bool r_ok = true;
    std::string errors_list;

    for(size_t i = 0; i < 5; ++i)
    {
        if (roms[romset][i][0] == '\0')
        {
            rpaths[i] = "";
            continue;
        }
        rpaths[i] = basePath + "/" + roms[romset][i];
        s_rf[i] = Files::utf8_fopen(rpaths[i].c_str(), "rb");
        bool optional = mcu.mcu_jv880 && i == 4;
        r_ok &= optional || (s_rf[i] != nullptr);
        if(!s_rf[i])
        {
            if(!errors_list.empty())
                errors_list.append(", ");

            errors_list.append(rpaths[i]);
        }
    }

    if (!r_ok)
    {
        fprintf(stderr, "FATAL ERROR: One of required data ROM files is missing: %s.\n", errors_list.c_str());
        fflush(stderr);
        closeAllR();
        return 1;
    }

    LCD_SetBackPath(*lcd, basePath + "/back.data");

    if (fread(mcu.rom1, 1, ROM1_SIZE, s_rf[0]) != ROM1_SIZE)
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM1.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    size_t rom2_read = fread(mcu.rom2, 1, ROM2_SIZE, s_rf[1]);

    if (rom2_read == ROM2_SIZE || rom2_read == ROM2_SIZE / 2)
    {
        mcu.rom2_mask = rom2_read - 1;
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM2.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    if (mcu.mcu_mk1)
    {
        if (fread(tempbuf, 1, 0x100000, s_rf[2]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom1, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom2, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[4]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom3.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom3, 0x100000);
    }
    else if (mcu.mcu_jv880)
    {
        if (fread(tempbuf, 1, 0x200000, s_rf[2]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom1, 0x200000);

        if (fread(tempbuf, 1, 0x200000, s_rf[3]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom2, 0x200000);

        if (s_rf[4] && fread(tempbuf, 1, 0x800000, s_rf[4]))
            unscramble(tempbuf, pcm->waverom_exp, 0x800000);
        else
            printf("WaveRom EXP not found, skipping it.\n");
    }
    else
    {
        if (fread(tempbuf, 1, 0x200000, s_rf[2]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm->waverom1, 0x200000);

        if (s_rf[3])
        {
            if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
            {
                fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
                fflush(stderr);
                closeAllR();
                return 1;
            }

            unscramble(tempbuf, mcu.mcu_scb55 ? pcm->waverom3 : pcm->waverom2, 0x100000);
        }

        if (s_rf[4] && fread(sm.sm_rom, 1, ROMSM_SIZE, s_rf[4]) != ROMSM_SIZE)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the sub mcu ROM.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }
    }

    // Close all files as they no longer needed being open
    closeAllR();

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "FATAL ERROR: Failed to initialize the SDL2: %s.\n", SDL_GetError());
        fflush(stderr);
        return 2;
    }

    if (!MCU_OpenAudio(mcu, audioDeviceIndex, pageSize, pageNum))
    {
        fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
        fflush(stderr);
        return 2;
    }

    if(!MIDI_Init(mcu, port))
    {
        fprintf(stderr, "ERROR: Failed to initialize the MIDI Input.\nWARNING: Continuing without MIDI Input...\n");
        fflush(stderr);
    }

    LCD_Init(*lcd, mcu);
    MCU_PatchROM(mcu);
    MCU_Reset(mcu);
    SM_Reset(sm, mcu);

    if (resetType != ResetType::NONE) MIDI_Reset(mcu, resetType);
    
    MCU_Run(mcu);

    MCU_CloseAudio();
    MIDI_Quit();
    LCD_UnInit(*lcd);
    SDL_Quit();

    free(pcm);

    return 0;
}
