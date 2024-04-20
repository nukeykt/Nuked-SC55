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
#include <stdint.h>
#include <string.h>
#include "mcu.h"
#include "mcu_timer.h"

uint64_t timer_cycles;
uint8_t timer_tempreg;

frt_t frt[3];
mcu_timer_t timer;

uint8_t timer_lut1[4] = {3,7,31,1};
uint8_t timer_mk1_lut1[4] = {3,7,31,3};
uint8_t* p_timer_lut1;
uint16_t timer_lut2[8] = {0,7,63,1023,0,1,1,1};
uint16_t timer_mk1_lut2[8] = {0,7,63,1023,0,3,3,3};
uint16_t* p_timer_lut2;

enum {
    REG_TCR = 0x00,
    REG_TCSR = 0x01,
    REG_FRCH = 0x02,
    REG_FRCL = 0x03,
    REG_OCRAH = 0x04,
    REG_OCRAL = 0x05,
    REG_OCRBH = 0x06,
    REG_OCRBL = 0x07,
    REG_ICRH = 0x08,
    REG_ICRL = 0x09,
};

void TIMER_Reset(void)
{
    timer_cycles = 0;
    timer_tempreg = 0;
    memset(frt, 0, sizeof(frt));
    memset(&timer, 0, sizeof(timer));
    p_timer_lut1 = mcu_mk1 ? timer_mk1_lut1 : timer_lut1;
    p_timer_lut2 = mcu_mk1 ? timer_mk1_lut2 : timer_lut2;
}

void TIMER_Write(uint32_t address, uint8_t data)
{
    uint32_t t = (address >> 4) - 1;
    if (t > 2)
        return;
    address &= 0x0f;
    frt_t *timer = &frt[t];
    switch (address)
    {
    case REG_TCR:
        timer->tcr = data;
        break;
    case REG_TCSR:
        timer->tcsr &= ~0xf;
        timer->tcsr |= data & 0xf;
        if ((data & 0x10) == 0 && (timer->status_rd & 0x10) != 0)
        {
            timer->tcsr &= ~0x10;
            timer->status_rd &= ~0x10;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_FOVI + t * 4, 0);
        }
        if ((data & 0x20) == 0 && (timer->status_rd & 0x20) != 0)
        {
            timer->tcsr &= ~0x20;
            timer->status_rd &= ~0x20;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIA + t * 4, 0);
        }
        if ((data & 0x40) == 0 && (timer->status_rd & 0x40) != 0)
        {
            timer->tcsr &= ~0x40;
            timer->status_rd &= ~0x40;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIB + t * 4, 0);
        }
        break;
    case REG_FRCH:
    case REG_OCRAH:
    case REG_OCRBH:
    case REG_ICRH:
        timer_tempreg = data;
        break;
    case REG_FRCL:
        timer->frc = (timer_tempreg << 8) | data;
        break;
    case REG_OCRAL:
        timer->ocra = (timer_tempreg << 8) | data;
        break;
    case REG_OCRBL:
        timer->ocrb = (timer_tempreg << 8) | data;
        break;
    case REG_ICRL:
        timer->icr = (timer_tempreg << 8) | data;
        break;
    }
}

uint8_t TIMER_Read(uint32_t address)
{
    uint32_t t = (address >> 4) - 1;
    if (t > 2)
        return 0xff;
    address &= 0x0f;
    frt_t *timer = &frt[t];
    switch (address)
    {
    case REG_TCR:
        return timer->tcr;
    case REG_TCSR:
    {
        uint8_t ret = timer->tcsr;
        timer->status_rd |= timer->tcsr & 0xf0;
        //timer->status_rd |= 0xf0;
        return ret;
    }
    case REG_FRCH:
        timer_tempreg = timer->frc & 0xff;
        return timer->frc >> 8;
    case REG_OCRAH:
        timer_tempreg = timer->ocra & 0xff;
        return timer->ocra >> 8;
    case REG_OCRBH:
        timer_tempreg = timer->ocrb & 0xff;
        return timer->ocrb >> 8;
    case REG_ICRH:
        timer_tempreg = timer->icr & 0xff;
        return timer->icr >> 8;
    case REG_FRCL:
    case REG_OCRAL:
    case REG_OCRBL:
    case REG_ICRL:
        return timer_tempreg;
    }
    return 0xff;
}

void TIMER2_Write(uint32_t address, uint8_t data)
{
    switch (address)
    {
    case DEV_TMR_TCR:
        timer.tcr = data;
        break;
    case DEV_TMR_TCSR:
        timer.tcsr &= ~0xf;
        timer.tcsr |= data & 0xf;
        if ((data & 0x20) == 0 && (timer.status_rd & 0x20) != 0)
        {
            timer.tcsr &= ~0x20;
            timer.status_rd &= ~0x20;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_OVI, 0);
        }
        if ((data & 0x40) == 0 && (timer.status_rd & 0x40) != 0)
        {
            timer.tcsr &= ~0x40;
            timer.status_rd &= ~0x40;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_CMIA, 0);
        }
        if ((data & 0x80) == 0 && (timer.status_rd & 0x80) != 0)
        {
            timer.tcsr &= ~0x80;
            timer.status_rd &= ~0x80;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_CMIB, 0);
        }
        break;
    case DEV_TMR_TCORA:
        timer.tcora = data;
        break;
    case DEV_TMR_TCORB:
        timer.tcorb = data;
        break;
    case DEV_TMR_TCNT:
        timer.tcnt = data;
        break;
    }
}
uint8_t TIMER_Read2(uint32_t address)
{
    switch (address)
    {
    case DEV_TMR_TCR:
        return timer.tcr;
    case DEV_TMR_TCSR:
    {
        uint8_t ret = timer.tcsr;
        timer.status_rd |= timer.tcsr & 0xe0;
        return ret;
    }
    case DEV_TMR_TCORA:
        return timer.tcora;
    case DEV_TMR_TCORB:
        return timer.tcorb;
    case DEV_TMR_TCNT:
        return timer.tcnt;
    }
    return 0xff;
}

void TIMER_Clock(uint64_t cycles)
{
    frt_t *timer1 = &frt[0];
    frt_t *timer2 = &frt[1];
    frt_t *timer3 = &frt[2];
    uint8_t lut1_1_val = p_timer_lut1[timer1->tcr & 3];
    uint8_t lut1_2_val = p_timer_lut1[timer2->tcr & 3];
    uint8_t lut1_3_val = p_timer_lut1[timer3->tcr & 3];
    uint16_t lut2_val = p_timer_lut2[timer.tcr & 7];

    while (timer_cycles*2 < cycles) // FIXME
    {
        if (!(timer_cycles & lut1_1_val)) {
            uint8_t matcha = timer1->frc == timer1->ocra;
            uint8_t matchb = timer1->frc == timer1->ocrb;
            timer1->frc = ((timer1->tcsr & 1) != 0 && matcha) ? 0 : timer1->frc + 1;

            // flags
            timer1->tcsr |= (((timer1->frc >> 16) & 1) ? 0x10 : 0) | (matcha ? 0x20 : 0) | (matchb ? 0x40 : 0);
            if ((timer1->tcr & 0x10) != 0 && (timer1->tcsr & 0x10) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_FOVI, 1);
            if ((timer1->tcr & 0x20) != 0 && (timer1->tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIA, 1);
            if ((timer1->tcr & 0x40) != 0 && (timer1->tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIB, 1);
        }
        if (!(timer_cycles & lut1_2_val)) {
            uint8_t matcha = timer2->frc == timer2->ocra;
            uint8_t matchb = timer2->frc == timer2->ocrb;
            timer2->frc = ((timer2->tcsr & 1) != 0 && matcha) ? 0 : timer2->frc + 1;

            // flags
            timer2->tcsr |= (((timer2->frc >> 16) & 1) ? 0x10 : 0) | (matcha ? 0x20 : 0) | (matchb ? 0x40 : 0);
            if ((timer2->tcr & 0x10) != 0 && (timer2->tcsr & 0x10) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_FOVI + 4, 1);
            if ((timer2->tcr & 0x20) != 0 && (timer2->tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIA + 4, 1);
            if ((timer2->tcr & 0x40) != 0 && (timer2->tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIB + 4, 1);
        }
        if (!(timer_cycles & lut1_3_val)) {
            uint8_t matcha = timer3->frc == timer3->ocra;
            uint8_t matchb = timer3->frc == timer3->ocrb;
            timer3->frc = ((timer3->tcsr & 1) != 0 && matcha) ? 0 : timer3->frc + 1;

            // flags
            timer3->tcsr |= (((timer3->frc >> 16) & 1) ? 0x10 : 0) | (matcha ? 0x20 : 0) | (matchb ? 0x40 : 0);
            if ((timer3->tcr & 0x10) != 0 && (timer3->tcsr & 0x10) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_FOVI + 8, 1);
            if ((timer3->tcr & 0x20) != 0 && (timer3->tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIA + 8, 1);
            if ((timer3->tcr & 0x40) != 0 && (timer3->tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_FRT0_OCIB + 8, 1);
        }

        if (lut2_val && !(timer_cycles & lut2_val))
        {
            uint16_t value = timer.tcnt;
            uint8_t matcha = value == timer.tcora;
            uint8_t matchb = value == timer.tcorb;
            uint8_t match24 = timer.tcr & 24;
            value = matcha && match24 == 8 ? 0
                : (matchb && match24 == 16 ? 0 : value + 1);
            timer.tcnt = value;

            // flags
            timer.tcsr |= (((value >> 8) & 1) ? 0x20 : 0) | (matcha ? 0x40 : 0) | (matchb ? 0x80 : 0);
            if ((timer.tcr & 0x20) != 0 && (timer.tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_OVI, 1);
            if ((timer.tcr & 0x40) != 0 && (timer.tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_CMIA, 1);
            if ((timer.tcr & 0x80) != 0 && (timer.tcsr & 0x80) != 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_TIMER_CMIB, 1);
        }

        timer_cycles++;
    }
}
