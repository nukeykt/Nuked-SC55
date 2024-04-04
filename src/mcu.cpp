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
#include <chrono>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_timer.h"
#include "pcm.h"
#include "lcd.h"
#include "submcu.h"
#include "utf8main.h"
#include "utils/files.h"

#if __linux__
#include <unistd.h>
#include <limits.h>
#endif

void MCU::MCU_ErrorTrap(void)
{
    printf("trap %.2x %.4x\n", mcu.cp, mcu.pc);
}

uint8_t MCU::RCU_Read(void)
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

uint32_t MCU::MCU_GetAddress(uint8_t page, uint16_t address) {
    return (page << 16) + address;
}

uint8_t MCU::MCU_ReadCode(void) {
    return MCU_Read(MCU_GetAddress(mcu.cp, mcu.pc));
}

uint8_t MCU::MCU_ReadCodeAdvance(void) {
    uint8_t ret = MCU_ReadCode();
    mcu.pc++;
    return ret;
}

void MCU::MCU_SetRegisterByte(uint8_t reg, uint8_t val)
{
    mcu.r[reg] = val;
}

uint32_t MCU::MCU_GetVectorAddress(uint32_t vector)
{
    return MCU_Read32(vector * 4);
}

uint32_t MCU::MCU_GetPageForRegister(uint32_t reg)
{
    if (reg >= 6)
        return mcu.tp;
    else if (reg >= 4)
        return mcu.ep;
    return mcu.dp;
}

void MCU::MCU_ControlRegisterWrite(uint32_t reg, uint32_t siz, uint32_t data)
{
    if (siz)
    {
        if (reg == 0)
        {
            mcu.sr = data;
            mcu.sr &= sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            mcu.dp = data & 0xff;
        }
        else if (reg == 4) // FIXME: undocumented
        {
            mcu.ep = data & 0xff;
        }
        else if (reg == 3) // FIXME: undocumented
        {
            mcu.br = data & 0xff;
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else
    {
        if (reg == 1)
        {
            mcu.sr &= ~0xff;
            mcu.sr |= data & 0xff;
            mcu.sr &= sr_mask;
        }
        else if (reg == 3)
        {
            mcu.br = data;
        }
        else if (reg == 4)
        {
            mcu.ep = data;
        }
        else if (reg == 5)
        {
            mcu.dp = data;
        }
        else if (reg == 7)
        {
            mcu.tp = data;
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
}

uint32_t MCU::MCU_ControlRegisterRead(uint32_t reg, uint32_t siz)
{
    uint32_t ret = 0;
    if (siz)
    {
        if (reg == 0)
        {
            ret = mcu.sr & sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            ret = mcu.dp | (mcu.dp << 8);
        }
        else if (reg == 4) // FIXME: undocumented
        {
            ret = mcu.ep | (mcu.ep << 8);
        }
        else if (reg == 3) // FIXME: undocumented
        {
            ret = mcu.br | (mcu.br << 8);;
        }
        else
        {
            MCU_ErrorTrap();
        }
        ret &= 0xffff;
    }
    else
    {
        if (reg == 1)
        {
            ret = mcu.sr & sr_mask;
        }
        else if (reg == 3)
        {
            ret = mcu.br;
        }
        else if (reg == 4)
        {
            ret = mcu.ep;
        }
        else if (reg == 5)
        {
            ret = mcu.dp;
        }
        else if (reg == 7)
        {
            ret = mcu.tp;
        }
        else
        {
            MCU_ErrorTrap();
        }
        ret &= 0xff;
    }
    return ret;
}

void MCU::MCU_SetStatus(uint32_t condition, uint32_t mask)
{
    if (condition)
        mcu.sr |= mask;
    else
        mcu.sr &= ~mask;
}

void MCU::MCU_PushStack(uint16_t data)
{
    if (mcu.r[7] & 1)
        MCU_Interrupt_Exception(this, EXCEPTION_SOURCE_ADDRESS_ERROR);
    mcu.r[7] -= 2;
    MCU_Write16(mcu.r[7], data);
}

uint16_t MCU::MCU_PopStack(void)
{
    uint16_t ret;
    if (mcu.r[7] & 1)
        MCU_Interrupt_Exception(this, EXCEPTION_SOURCE_ADDRESS_ERROR);
    ret = MCU_Read16(mcu.r[7]);
    mcu.r[7] += 2;
    return ret;
}

uint16_t MCU::MCU_AnalogReadPin(uint32_t pin)
{
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

void MCU::MCU_AnalogSample(int channel)
{
    int value = MCU_AnalogReadPin(channel);
    int dest = (channel << 1) & 6;
    dev_register[DEV_ADDRAH + dest] = value >> 2;
    dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

void MCU::MCU_DeviceWrite(uint32_t address, uint8_t data)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        mcu_timer.TIMER_Write(address, data);
        return;
    }
    if (address >= 0x50 && address < 0x55)
    {
        mcu_timer.TIMER2_Write(address, data);
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
    case DEV_ADCSR:
    {
        dev_register[address] &= ~0x7f;
        dev_register[address] |= data & 0x7f;
        if ((data & 0x80) == 0 && adf_rd)
        {
            dev_register[address] &= ~0x80;
            MCU_Interrupt_SetRequest(this, INTERRUPT_SOURCE_ANALOG, 0);
        }
        if ((data & 0x40) == 0)
            MCU_Interrupt_SetRequest(this, INTERRUPT_SOURCE_ANALOG, 0);
        return;
    }
    default:
        address += 0;
        break;
    }
    dev_register[address] = data;
}

uint8_t MCU::MCU_DeviceRead(uint32_t address)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        return mcu_timer.TIMER_Read(address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return mcu_timer.TIMER_Read(address);
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
    case DEV_RDR:
        return 0x00;
    case 0x00:
        return 0xff;
    case DEV_P9DR:
        return 0x02;
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

void MCU::MCU_DeviceReset(void)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
    dev_register[DEV_RAME] = 0x80;
}

void MCU::MCU_UpdateAnalog(uint64_t cycles)
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
                MCU_Interrupt_SetRequest(this, INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        analog_end_time = 0;
}

uint8_t MCU::MCU_Read(uint32_t address)
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
                    ret = pcm.PCM_Read(address & 0x3f);
                }
                else if (address >= 0xec00 && address < 0xf000)
                {
                    ret = sub_mcu.SM_SysRead(address & 0xff);
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
                    MCU_Interrupt_SetRequest(this, INTERRUPT_SOURCE_IRQ1, 0);
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
                    ret = pcm.PCM_Read(address & 0x3f);
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
                else
                {
                    printf("Unknown read %x\n", address);
                    ret = 0xff;
                }
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
    case 2:
    case 3:
    case 4:
    case 8:
    case 9:
    case 14:
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

uint16_t MCU::MCU_Read16(uint32_t address)
{
    address &= ~1;
    uint8_t b0, b1;
    b0 = MCU_Read(address);
    b1 = MCU_Read(address+1);
    return (b0 << 8) + b1;
}

uint32_t MCU::MCU_Read32(uint32_t address)
{
    address &= ~3;
    uint8_t b0, b1, b2, b3;
    b0 = MCU_Read(address);
    b1 = MCU_Read(address+1);
    b2 = MCU_Read(address+2);
    b3 = MCU_Read(address+3);
    return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

void MCU::MCU_Write(uint32_t address, uint8_t value)
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
                        lcd.LCD_Write(address & 1, value);
                    else if (address == 0xe401)
                    {
                        io_sd = value;
                        lcd.LCD_Enable((value & 1) == 0);
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
                    pcm.PCM_Write(address & 0x3f, value);
                }
                else if (!mcu_mk1 && address >= 0xec00 && address < 0xf000)
                {
                    sub_mcu.SM_SysWrite(address & 0xff, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                    ram[(address - 0xfb80) & 0x3ff] = value;
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
                    pcm.PCM_Write(address & 0x3f, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                    ram[(address - 0xfb80) & 0x3ff] = value;
                else if (address >= 0x8000 && address < 0xe000)
                {
                    sram[address & 0x7fff] = value;
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
    else if (page == 10)
    {
        sram[address & 0x7fff] = value; // FIXME
    }
    else
    {
        printf("Unknown write %x %x\n", (page << 16) | address, value);
    }
}

void MCU::MCU_Write16(uint32_t address, uint16_t value)
{
    address &= ~1;
    MCU_Write(address, value >> 8);
    MCU_Write(address + 1, value & 0xff);
}

void MCU::MCU_ReadInstruction(void)
{
    uint8_t operand = MCU_ReadCodeAdvance();

    if (mcu.cycles == 0x45)
    {
        mcu.cycles += 0;
    }

    MCU_Operand_Table[operand](this, operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(this, EXCEPTION_SOURCE_TRACE);
    }
}

void MCU::MCU_Init(void)
{
    memset(&mcu, 0, sizeof(mcu_t));
}

void MCU::MCU_Reset(void)
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

    // uint32_t reset_address = MCU_GetVectorAddress(VECTOR_RESET);
    uint32_t reset_address = 0x016C;
    mcu.cp = (reset_address >> 16) & 0xff;
    mcu.pc = reset_address & 0xffff;

    mcu.exception_pending = -1;

    MCU_DeviceReset();
}

void MCU::MCU_PatchROM(void)
{
    //rom2[0x1333] = 0x11;
    //rom2[0x1334] = 0x19;
    //rom1[0x622d] = 0x19;
}

uint8_t MCU::MCU_ReadP0(void)
{
    return 0xff;
}

uint8_t MCU::MCU_ReadP1(void)
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

void MCU::MCU_WriteP0(uint8_t data)
{
    mcu_p0_data = data;
}

void MCU::MCU_WriteP1(uint8_t data)
{
    mcu_p1_data = data;
}

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

void MCU::MCU_PostSample(int *sample)
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

void MCU::MCU_GA_SetGAInt(int line, int value)
{
    // guesswork
    if (value && !ga_int[line] && (ga_int_enable & (1 << line)) != 0)
        ga_int_trigger = line;
    ga_int[line] = value;

    MCU_Interrupt_SetRequest(this, INTERRUPT_SOURCE_IRQ1, ga_int_trigger != 0);
}

void MCU::closeAllR()
{
    for(size_t i = 0; i < rf_num; ++i)
    {
        if(s_rf[i])
            fclose(s_rf[i]);
        s_rf[i] = nullptr;
    }
}

MCU::MCU() : pcm(this), lcd(), sub_mcu(this), mcu_timer(this) {}

int MCU::startSC55(std::string *basePath)
{
    if (init_lock == nullptr)
        init_lock = SDL_CreateMutex();

    SDL_LockMutex(init_lock);

    uint8_t* tempbuf = (uint8_t*) malloc(0x200000);

    printf("Base path is: %s\n", basePath->c_str());

    std::string rpaths[5] =
    {
        *basePath + "/rom1.bin",
        *basePath + "/rom2.bin",
        *basePath + "/waverom1.bin",
        *basePath + "/waverom2.bin",
        *basePath + "/rom_sm.bin"
    };

    if (mcu_mk1)
    {
        //rpaths[0] = *basePath + "/sc55_rom1.bin";
        //rpaths[1] = *basePath + "/sc55_rom2.bin";
        //rpaths[2] = *basePath + "/sc55_waverom1.bin";
        //rpaths[3] = *basePath + "/sc55_waverom2.bin";
        //rpaths[4] = *basePath + "/sc55_waverom3.bin";
        rpaths[0] = *basePath + "/cm300_rom1.bin";
        rpaths[1] = *basePath + "/cm300_rom2.bin";
        rpaths[2] = *basePath + "/cm300_waverom1.bin";
        rpaths[3] = *basePath + "/cm300_waverom2.bin";
        rpaths[4] = *basePath + "/cm300_waverom3.bin";
    }

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

    lcd.LCD_SetBackPath(*basePath + "/back.data");

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

        unscramble(tempbuf, pcm.waverom1, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm.waverom2, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[4]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom3.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm.waverom2, 0x100000);
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

        unscramble(tempbuf, pcm.waverom1, 0x200000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, pcm.waverom2, 0x100000);

        if (fread(sub_mcu.sm_rom, 1, ROMSM_SIZE, s_rf[4]) != ROMSM_SIZE)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the sub mcu ROM.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }
    }

    // Close all files as they no longer needed being open
    closeAllR();

    lcd.LCD_Init(this);
    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    sub_mcu.SM_Reset();
    pcm.PCM_Reset();

    sample_write_ptr = 0;

    free(tempbuf);
    SDL_UnlockMutex(init_lock);
    
    return 0;
}

int MCU::updateSC55(int16_t *data, unsigned int dataSize) {
    if (init_lock == nullptr)
        init_lock = SDL_CreateMutex();

    SDL_LockMutex(init_lock);

    // auto start = std::chrono::high_resolution_clock::now();

    dataSize /= 2;

    if (audio_buffer_size < dataSize) {
        printf("Audio buffer size is too small. (%d requested)\n", dataSize);
        fflush(stdout);
        return 0;
    }

    sample_write_ptr = 0;
    // memset(sample_buffer, 0, audio_buffer_size * 2);

    int maxCycles = dataSize * 256;

    for (int i = 0; sample_write_ptr < dataSize; i++) {
        if (i > maxCycles) {
            printf("Not enough samples!\n");
            fflush(stdout);
            break;
        }

        if (!mcu.ex_ignore)
            MCU_Interrupt_Handle(this);
        else
            mcu.ex_ignore = 0;

        if (!mcu.sleep)
            MCU_ReadInstruction();

        mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

        // if (mcu.cycles % 24000000 == 0)
        //     printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

        pcm.PCM_Update(mcu.cycles);

        mcu_timer.TIMER_Clock(mcu.cycles);

        if (!mcu_mk1)
            sub_mcu.SM_Update(mcu.cycles);

        MCU_UpdateAnalog(mcu.cycles);
    }

    memcpy(data, sample_buffer, dataSize * 2);

    // auto stop = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    // if (duration > 8) {
    //     printf("updateSC55 took %lld ms\n", duration);
    //     fflush(stdout);
    // }

    SDL_UnlockMutex(init_lock);

    return 0;
}

void MCU::SC55_Reset() {
    lcd.LCD_Init(this);
    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    sub_mcu.SM_Reset();
    pcm.PCM_Reset();

    sample_write_ptr = 0;
}

void MCU::postMidiSC55(uint8_t* message, int length) {
    for (int i = 0; i < length; i++) {
        sub_mcu.SM_PostUART(message[i]);
    }
}
