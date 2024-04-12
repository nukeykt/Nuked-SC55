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
    "CM-300/SCC-1"
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
};

int romset = ROM_SET_MK2;

static const int ROM1_SIZE = 0x8000;
static const int ROM2_SIZE = 0x80000;
static const int RAM_SIZE = 0x400;
static const int SRAM_SIZE = 0x8000;
static const int ROMSM_SIZE = 0x1000;


static const int audio_buffer_size = 16384;
static const int audio_page_size = 512;

static short sample_buffer[audio_buffer_size];

static int sample_read_ptr;
static int sample_write_ptr;

static SDL_AudioDeviceID sdl_audio;

void MCU_ErrorTrap(void)
{
    printf("%.2x %.4x\n", mcu.cp, mcu.pc);
}

int mcu_mk1 = 0; // 0 - SC-55mkII, SC-55ST. 1 - SC-55, CM-300/SCC-1
int mcu_cm300 = 0; // 0 - SC-55, 1 - CM-300/SCC-1
int mcu_st = 0; // 0 - SC-55mk2, 1 - SC-55ST

static int ga_int[8];
static int ga_int_enable = 0;
static int ga_int_trigger = 0;
static int ga_lcd_counter = 0;


uint8_t dev_register[0x80];

static uint16_t ad_val[4];
static uint8_t ad_nibble = 0x00;
static uint8_t sw_pos = 0;
static uint8_t io_sd = 0x00;

SDL_atomic_t mcu_button_pressed = { 0 };

uint8_t RCU_Read(void)
{
    return 0;
}

enum {
    ANALOG_LEVEL_RCU_LOW = 0,
    ANALOG_LEVEL_RCU_HIGH = 0,
    ANALOG_LEVEL_SW_0 = 0,
    ANALOG_LEVEL_SW_1 = 0,
    ANALOG_LEVEL_SW_2 = 0,
    ANALOG_LEVEL_SW_3 = 0,
    ANALOG_LEVEL_BATTERY = 0x3ff,
};

uint16_t MCU_AnalogReadPin(uint32_t pin)
{
    if (mcu_mk1)
        return 0x0;
    return 0x3ff;
    uint8_t rcu;
    if (pin == 7)
    {
        switch ((io_sd >> 2) & 3)
        {
        case 0: // Battery voltage
            return ANALOG_LEVEL_BATTERY;
        case 1: // NC
            return 0;
        case 2: // SW
            switch (sw_pos)
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
            break;
        }
    }
    rcu = RCU_Read();
    if (rcu & (1 << pin))
        return ANALOG_LEVEL_RCU_HIGH;
    else
        return ANALOG_LEVEL_RCU_LOW;
}

void MCU_AnalogSample(int channel)
{
    int value = MCU_AnalogReadPin(channel);
    int dest = (channel << 1) & 6;
    dev_register[DEV_ADDRAH + dest] = value >> 2;
    dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

int adf_rd = 0;

uint64_t analog_end_time;

int ssr_rd = 0;

uint32_t uart_write_ptr;
uint32_t uart_read_ptr;
uint8_t uart_buffer[uart_buffer_size];

static uint8_t uart_rx_byte;
static uint64_t uart_rx_delay;
static uint64_t uart_tx_delay;

void MCU_DeviceWrite(uint32_t address, uint8_t data)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        TIMER_Write(address, data);
        return;
    }
    if (address >= 0x50 && address < 0x55)
    {
        TIMER2_Write(address, data);
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
        dev_register[address] &= ~0x7f;
        dev_register[address] |= data & 0x7f;
        if ((data & 0x80) == 0 && adf_rd)
        {
            dev_register[address] &= ~0x80;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
        }
        if ((data & 0x40) == 0)
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
        return;
    }
    case DEV_SSR:
    {
        if ((data & 0x80) == 0 && (ssr_rd & 0x80) != 0)
        {
            dev_register[address] &= ~0x80;
            uart_tx_delay = mcu.cycles + 1000;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_TX, 0);
        }
        if ((data & 0x40) == 0 && (ssr_rd & 0x40) != 0)
        {
            uart_rx_delay = mcu.cycles + 100;
            dev_register[address] &= ~0x40;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_RX, 0);
        }
        if ((data & 0x20) == 0 && (ssr_rd & 0x20) != 0)
        {
            dev_register[address] &= ~0x20;
        }
        if ((data & 0x10) == 0 && (ssr_rd & 0x10) != 0)
        {
            dev_register[address] &= ~0x10;
        }
        break;
    }
    default:
        address += 0;
        break;
    }
    dev_register[address] = data;
}

uint8_t MCU_DeviceRead(uint32_t address)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        return TIMER_Read(address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return TIMER_Read2(address);
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
        return dev_register[address];
    case DEV_ADCSR:
        adf_rd = (dev_register[address] & 0x80) != 0;
        return dev_register[address];
    case DEV_SSR:
        ssr_rd = dev_register[address];
        return dev_register[address];
    case DEV_RDR:
        return uart_rx_byte;
    case 0x00:
        return 0xff;
    case DEV_P9DR:
    {
        int cfg = 0;
        if (!mcu_mk1)
            cfg = 2; // bit 1: 0 - SC-155mk2 (???), 1 - SC-55mk2

        int dir = dev_register[DEV_P9DDR];

        int val = cfg & (dir ^ 0xff);
        val |= dev_register[DEV_P9DR] & dir;
        return val;
    }
    case DEV_SCR:
        return dev_register[address];
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
        return dev_register[address];
    }
    return dev_register[address];
}

void MCU_DeviceReset(void)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
    dev_register[DEV_RAME] = 0x80;
    dev_register[DEV_SSR] = 0x80;
}

void MCU_UpdateAnalog(uint64_t cycles)
{
    int ctrl = dev_register[DEV_ADCSR];
    int isscan = (ctrl & 16) != 0;

    if (ctrl & 0x20)
    {
        if (analog_end_time == 0)
            analog_end_time = cycles + 200;
        else if (analog_end_time < cycles)
        {
            if (isscan)
            {
                int base = ctrl & 4;
                for (int i = 0; i <= (ctrl & 3); i++)
                    MCU_AnalogSample(base + i);
                analog_end_time = cycles + 200;
            }
            else
            {
                MCU_AnalogSample(ctrl & 7);
                dev_register[DEV_ADCSR] &= ~0x20;
                analog_end_time = 0;
            }
            dev_register[DEV_ADCSR] |= 0x80;
            if (ctrl & 0x40)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        analog_end_time = 0;
}

mcu_t mcu;

uint8_t rom1[ROM1_SIZE];
uint8_t rom2[ROM2_SIZE];
uint8_t ram[RAM_SIZE];
uint8_t sram[SRAM_SIZE];

int rom2_mask = ROM2_SIZE - 1;

uint8_t MCU_Read(uint32_t address)
{
    uint32_t address_rom = address & 0x3ffff;
    if (address & 0x80000)
        address_rom |= 0x40000;
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    uint8_t ret = 0xff;
    switch (page)
    {
    case 0:
        if (!(address & 0x8000))
            ret = rom1[address & 0x7fff];
        else
        {
            if (!mcu_mk1)
            {
                if (address >= 0xe000 && address < 0xe400)
                {
                    ret = PCM_Read(address & 0x3f);
                }
                else if (address >= 0xec00 && address < 0xf000)
                {
                    ret = SM_SysRead(address & 0xff);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                    ret = ram[(address - 0xfb80) & 0x3ff];
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = sram[address & 0x7fff];
                }
                else if (address == 0xe402)
                {
                    ret = ga_int_trigger;
                    ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, 0);
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
                    ret = PCM_Read(address & 0x3f);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ret = ram[(address - 0xfb80) & 0x3ff];
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = sram[address & 0x7fff];
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    io_sd = address & 0xff;

                    if (mcu_cm300)
                        return 0xff;

                    LCD_Enable((io_sd & 8) != 0);

                    uint8_t data = 0xff;
                    uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu_button_pressed);

                    if ((io_sd & 1) == 0)
                        data &= 0x80 | (((button_pressed >> 0) & 127) ^ 127);
                    if ((io_sd & 2) == 0)
                        data &= 0x80 | (((button_pressed >> 7) & 127) ^ 127);
                    if ((io_sd & 4) == 0)
                        data &= 0x80 | (((button_pressed >> 14) & 127) ^ 127);
                    return data;
                }
                else if (address == 0xf106)
                {
                    ret = ga_int_trigger;
                    ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, 0);
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
        ret = rom2[address_rom & rom2_mask];
        break;
    case 2:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 3:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 4:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 8:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 9:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 14:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 15:
        ret = rom2[address_rom & rom2_mask];
        break;
    case 10:
        if (!mcu_mk1)
            ret = sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    case 5:
        if (mcu_mk1)
            ret = sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    default:
        ret = 0x00;
        break;
    }
    return ret;
}

uint16_t MCU_Read16(uint32_t address)
{
    address &= ~1;
    uint8_t b0, b1;
    b0 = MCU_Read(address);
    b1 = MCU_Read(address+1);
    return (b0 << 8) + b1;
}

uint32_t MCU_Read32(uint32_t address)
{
    address &= ~3;
    uint8_t b0, b1, b2, b3;
    b0 = MCU_Read(address);
    b1 = MCU_Read(address+1);
    b2 = MCU_Read(address+2);
    b3 = MCU_Read(address+3);
    return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

void MCU_Write(uint32_t address, uint8_t value)
{
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    if (page == 0)
    {
        if (address & 0x8000)
        {
            if (!mcu_mk1)
            {
                if (address >= 0xe400 && address < 0xe800)
                {
                    if (address == 0xe404 || address == 0xe405)
                        LCD_Write(address & 1, value);
                    else if (address == 0xe401)
                    {
                        io_sd = value;
                        LCD_Enable((value & 1) == 0);
                    }
                    else if (address == 0xe402)
                        ga_int_enable = (value << 1);
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
                else if (address >= 0xe000 && address < 0xe400)
                {
                    PCM_Write(address & 0x3f, value);
                }
                else if (address >= 0xec00 && address < 0xf000)
                {
                    SM_SysWrite(address & 0xff, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    sram[address & 0x7fff] = value;
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
                    PCM_Write(address & 0x3f, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    sram[address & 0x7fff] = value;
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    io_sd = address & 0xff;
                    LCD_Enable((io_sd & 8) != 0);
                }
                else if (address == 0xf105)
                {
                    LCD_Write(0, value);
                    ga_lcd_counter = 500;
                }
                else if (address == 0xf104)
                {
                    LCD_Write(1, value);
                    ga_lcd_counter = 500;
                }
                else if (address == 0xf107)
                {
                    io_sd = value;
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
    else if (page == 5 && mcu_mk1)
    {
        sram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 10 && !mcu_mk1)
    {
        sram[address & 0x7fff] = value; // FIXME
    }
    else
    {
        printf("Unknown write %x %x\n", (page << 16) | address, value);
    }
}

void MCU_Write16(uint32_t address, uint16_t value)
{
    address &= ~1;
    MCU_Write(address, value >> 8);
    MCU_Write(address + 1, value & 0xff);
}

void MCU_ReadInstruction(void)
{
    uint8_t operand = MCU_ReadCodeAdvance();

    MCU_Operand_Table[operand](operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_TRACE);
    }
}

void MCU_Init(void)
{
    memset(&mcu, 0, sizeof(mcu_t));
}

void MCU_Reset(void)
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

    uint32_t reset_address = MCU_GetVectorAddress(VECTOR_RESET);
    mcu.cp = (reset_address >> 16) & 0xff;
    mcu.pc = reset_address & 0xffff;

    mcu.exception_pending = -1;

    MCU_DeviceReset();

    if (mcu_mk1)
    {
        ga_int_enable = 255;
    }
}

void MCU_PostUART(uint8_t data)
{
    uart_buffer[uart_write_ptr] = data;
    uart_write_ptr = (uart_write_ptr + 1) % uart_buffer_size;
}

void MCU_UpdateUART_RX(void)
{
    if ((dev_register[DEV_SCR] & 16) == 0) // RX disabled
        return;
    if (uart_write_ptr == uart_read_ptr) // no byte
        return;

    if (dev_register[DEV_SSR] & 0x40)
        return;

    if (mcu.cycles < uart_rx_delay)
        return;

    uart_rx_byte = uart_buffer[uart_read_ptr];
    uart_read_ptr = (uart_read_ptr + 1) % uart_buffer_size;
    dev_register[DEV_SSR] |= 0x40;
    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_RX, (dev_register[DEV_SCR] & 0x40) != 0);
}

// dummy TX
void MCU_UpdateUART_TX(void)
{
    if ((dev_register[DEV_SCR] & 32) == 0) // TX disabled
        return;

    if (dev_register[DEV_SSR] & 0x80)
        return;

    if (mcu.cycles < uart_tx_delay)
        return;

    dev_register[DEV_SSR] |= 0x80;
    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_TX, (dev_register[DEV_SCR] & 0x80) != 0);

    // printf("tx:%x\n", dev_register[DEV_TDR]);
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
    work_thread_lock = SDL_CreateMutex();

    MCU_WorkThread_Lock();
    while (work_thread_run)
    {
        if (pcm.config_reg_3c & 0x40)
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
            MCU_Interrupt_Handle();
        else
            mcu.ex_ignore = 0;

        if (!mcu.sleep)
            MCU_ReadInstruction();

        mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

        // if (mcu.cycles % 24000000 == 0)
        //     printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

        PCM_Update(mcu.cycles);

        TIMER_Clock(mcu.cycles);

        if (!mcu_mk1)
            SM_Update(mcu.cycles);
        else
        {
            MCU_UpdateUART_RX();
            MCU_UpdateUART_TX();
        }

        MCU_UpdateAnalog(mcu.cycles);

        if (mcu_mk1)
        {
            if (ga_lcd_counter)
            {
                ga_lcd_counter--;
                if (ga_lcd_counter == 0)
                {
                    MCU_GA_SetGAInt(1, 0);
                    MCU_GA_SetGAInt(1, 1);
                }
            }
        }
    }
    MCU_WorkThread_Unlock();

    SDL_DestroyMutex(work_thread_lock);

    return 0;
}

static void MCU_Run()
{
    bool working = true;

    work_thread_run = true;
    SDL_Thread *thread = SDL_CreateThread(work_thread, "work thread", 0);

    while (working)
    {
        if(LCD_QuitRequested())
            working = false;

        LCD_Update();
        SDL_Delay(15);
    }

    work_thread_run = false;
    SDL_WaitThread(thread, 0);
}

void MCU_PatchROM(void)
{
    //rom2[0x1333] = 0x11;
    //rom2[0x1334] = 0x19;
    //rom1[0x622d] = 0x19;
}

uint8_t mcu_p0_data = 0x00;
uint8_t mcu_p1_data = 0x00;

uint8_t MCU_ReadP0(void)
{
    return 0xff;
}

uint8_t MCU_ReadP1(void)
{
    uint8_t data = 0xff;
    uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu_button_pressed);

    if ((mcu_p0_data & 1) == 0)
        data &= 0x80 | (((button_pressed >> 0) & 127) ^ 127);
    if ((mcu_p0_data & 2) == 0)
        data &= 0x80 | (((button_pressed >> 7) & 127) ^ 127);
    if ((mcu_p0_data & 4) == 0)
        data &= 0x80 | (((button_pressed >> 14) & 127) ^ 127);

    return data;
}

void MCU_WriteP0(uint8_t data)
{
    mcu_p0_data = data;
}

void MCU_WriteP1(uint8_t data)
{
    mcu_p1_data = data;
}

uint8_t tempbuf[0x200000];

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

int MCU_OpenAudio(int deviceIndex)
{
    SDL_AudioSpec spec = {};
    SDL_AudioSpec spec_actual = {};

    spec.format = AUDIO_S16SYS;
    spec.freq = mcu_mk1 ? 64000: 66207;
    spec.channels = 2;
    spec.callback = audio_callback;
    spec.samples = audio_page_size / 4;
    
    const char* audioDevicename = deviceIndex == -1 ? "Default device" : SDL_GetAudioDeviceName(deviceIndex, 0);

    sdl_audio = SDL_OpenAudioDevice(deviceIndex == -1 ? NULL : audioDevicename, 0, &spec, &spec_actual, 0);
    if (!sdl_audio)
    {
        return 0;
    }
    
    printf("Audio Device: %s\n", audioDevicename);

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

    sample_read_ptr = 0;
    sample_write_ptr = 0;

    SDL_PauseAudioDevice(sdl_audio, 0);

    return 1;
}

void MCU_CloseAudio(void)
{
    SDL_CloseAudio();
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

void MCU_GA_SetGAInt(int line, int value)
{
    // guesswork
    if (value && !ga_int[line] && (ga_int_enable & (1 << line)) != 0)
        ga_int_trigger = line;
    ga_int[line] = value;

    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, ga_int_trigger != 0);
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

int main(int argc, char *argv[])
{
    (void)argc;
    std::string basePath;

    int port = 0;
    int audioDeviceIndex = -1;
    bool autodetect = true;

    romset = ROM_SET_MK2;

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

    mcu_mk1 = false;
    mcu_cm300 = false;
    mcu_st = false;
    switch (romset)
    {
        case ROM_SET_ST:
            mcu_st = true;
            break;
        case ROM_SET_MK1:
            mcu_mk1 = true;
            mcu_st = false;
            break;
        case ROM_SET_CM300:
            mcu_mk1 = true;
            mcu_cm300 = true;
            break;
    }

    std::string rpaths[5] =
    {
        basePath + "/" + roms[romset][0],
        basePath + "/" + roms[romset][1],
        basePath + "/" + roms[romset][2],
        basePath + "/" + roms[romset][3],
        basePath + "/" + roms[romset][4]
    };

    bool r_ok = true;
    std::string errors_list;

    for(size_t i = 0; i < 5; ++i)
    {
        s_rf[i] = Files::utf8_fopen(rpaths[i].c_str(), "rb");
        r_ok &= (s_rf[i] != nullptr);
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

    LCD_SetBackPath(basePath + "/back.data");

    memset(&mcu, 0, sizeof(mcu_t));


    if (fread(rom1, 1, ROM1_SIZE, s_rf[0]) != ROM1_SIZE)
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM1.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    size_t rom2_read = fread(rom2, 1, ROM2_SIZE, s_rf[1]);

    if (rom2_read == ROM2_SIZE || rom2_read == ROM2_SIZE / 2)
    {
        rom2_mask = rom2_read - 1;
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM2.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    if (mcu_mk1)
    {
        if (fread(tempbuf, 1, 0x100000, s_rf[2]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, waverom1, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, waverom2, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[4]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom3.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, waverom3, 0x100000);
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

        unscramble(tempbuf, waverom1, 0x200000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, waverom2, 0x100000);

        if (fread(sm_rom, 1, ROMSM_SIZE, s_rf[4]) != ROMSM_SIZE)
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

    if (!MCU_OpenAudio(audioDeviceIndex))
    {
        fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
        fflush(stderr);
        return 2;
    }

    if(!MIDI_Init(port))
    {
        fprintf(stderr, "ERROR: Failed to initialize the MIDI Input.\nWARNING: Continuing without MIDI Input...\n");
        fflush(stderr);
    }

    LCD_Init();
    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    SM_Reset();
    PCM_Reset();

    MCU_Run();

    MCU_CloseAudio();
    MIDI_Quit();
    LCD_UnInit();
    SDL_Quit();

    return 0;
}
