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
#include "mcu.h"
#include "mcu_interrupt.h"

void MCU_Interrupt_Start(MCU* mcu, int32_t mask)
{
    mcu->MCU_PushStack(mcu->mcu.pc);
    mcu->MCU_PushStack(mcu->mcu.cp);
    mcu->MCU_PushStack(mcu->mcu.sr);
    mcu->mcu.sr &= ~STATUS_T;
    if (mask >= 0)
    {
        mcu->mcu.sr &= ~STATUS_INT_MASK;
        mcu->mcu.sr |= mask << 8;
    }
    mcu->mcu.sleep = 0;
}

void MCU_Interrupt_SetRequest(MCU* mcu, uint32_t interrupt, uint32_t value)
{
    mcu->mcu.interrupt_pending[interrupt] = value;
}

void MCU_Interrupt_Exception(MCU* mcu, uint32_t exception)
{
#if 0
    if (interrupt == INTERRUPT_SOURCE_IRQ0 && (mcu->dev_register[DEV_P1CR] & 0x20) == 0)
        return;
    if (interrupt == INTERRUPT_SOURCE_IRQ1 && (mcu->dev_register[DEV_P1CR] & 0x40) == 0)
        return;
#endif
    mcu->mcu.exception_pending = exception;
}

void MCU_Interrupt_TRAPA(MCU* mcu, uint32_t vector)
{
    mcu->mcu.trapa_pending[vector] = 1;
}

void MCU_Interrupt_StartVector(MCU* mcu, uint32_t vector, int32_t mask)
{
    uint32_t address = mcu->MCU_GetVectorAddress(vector);
    MCU_Interrupt_Start(mcu, mask);
    mcu->mcu.cp = address >> 16;
    mcu->mcu.pc = address;
}

void MCU_Interrupt_Handle(MCU* mcu)
{
#if 0
    if (mcu->mcu.cycles % 2000 == 0 && mcu->mcu.sleep)
    {
        MCU_Interrupt_StartVector(mcu, VECTOR_INTERNAL_INTERRUPT_94);
        return;
    }
    if (mcu->mcu.cycles % 2000 == 1000 && mcu->mcu.sleep)
    {
        MCU_Interrupt_StartVector(mcu, VECTOR_INTERNAL_INTERRUPT_A4);
        return;
    }
    if (mcu->mcu.cycles % 2000 == 1500 && mcu->mcu.sleep)
    {
        MCU_Interrupt_StartVector(mcu, VECTOR_INTERNAL_INTERRUPT_B4);
        return;
    }
#endif
    uint32_t i;
    for (i = 0; i < 16; i++)
    {
        if (mcu->mcu.trapa_pending[i])
        {
            mcu->mcu.trapa_pending[i] = 0;
            MCU_Interrupt_StartVector(mcu, VECTOR_TRAPA_0 + i, -1);
            return;
        }
    }
    if (mcu->mcu.exception_pending >= 0)
    {
        switch (mcu->mcu.exception_pending)
        {
            case EXCEPTION_SOURCE_ADDRESS_ERROR:
                MCU_Interrupt_StartVector(mcu, VECTOR_ADDRESS_ERROR, -1);
                break;
            case EXCEPTION_SOURCE_INVALID_INSTRUCTION:
                MCU_Interrupt_StartVector(mcu, VECTOR_INVALID_INSTRUCTION, -1);
                break;
            case EXCEPTION_SOURCE_TRACE:
                MCU_Interrupt_StartVector(mcu, VECTOR_TRACE, -1);
                break;

        }
        mcu->mcu.exception_pending = -1;
        return;
    }
    if (mcu->mcu.interrupt_pending[INTERRUPT_SOURCE_NMI])
    {
        // mcu->mcu.interrupt_pending[INTERRUPT_SOURCE_NMI] = 0;
        MCU_Interrupt_StartVector(mcu, VECTOR_NMI, 7);
        return;
    }
    uint32_t mask = (mcu->mcu.sr >> 8) & 7;
    for (i = INTERRUPT_SOURCE_NMI + 1; i < INTERRUPT_SOURCE_MAX; i++)
    {
        int32_t vector = -1;
        int32_t level = 0;
        if (!mcu->mcu.interrupt_pending[i])
            continue;
        switch (i)
        {
            case INTERRUPT_SOURCE_IRQ0:
                if ((mcu->dev_register[DEV_P1CR] & 0x20) == 0)
                    continue;
                vector = VECTOR_IRQ0;
                level = (mcu->dev_register[DEV_IPRA] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_IRQ1:
                if ((mcu->dev_register[DEV_P1CR] & 0x40) == 0)
                    continue;
                vector = VECTOR_IRQ1;
                level = (mcu->dev_register[DEV_IPRA] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_94;
                level = (mcu->dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_98;
                level = (mcu->dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT0_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_9C;
                level = (mcu->dev_register[DEV_IPRB] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_A4;
                level = (mcu->dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_A8;
                level = (mcu->dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT1_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_AC;
                level = (mcu->dev_register[DEV_IPRB] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_OCIA:
                vector = VECTOR_INTERNAL_INTERRUPT_B4;
                level = (mcu->dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_OCIB:
                vector = VECTOR_INTERNAL_INTERRUPT_B8;
                level = (mcu->dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_FRT2_FOVI:
                vector = VECTOR_INTERNAL_INTERRUPT_BC;
                level = (mcu->dev_register[DEV_IPRC] >> 4) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_CMIA:
                vector = VECTOR_INTERNAL_INTERRUPT_C0;
                level = (mcu->dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_CMIB:
                vector = VECTOR_INTERNAL_INTERRUPT_C4;
                level = (mcu->dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_TIMER_OVI:
                vector = VECTOR_INTERNAL_INTERRUPT_C8;
                level = (mcu->dev_register[DEV_IPRC] >> 0) & 7;
                break;
            case INTERRUPT_SOURCE_ANALOG:
                vector = VECTOR_INTERNAL_INTERRUPT_E0;
                level = (mcu->dev_register[DEV_IPRD] >> 0) & 7;
                break;
            default:
                break;
        }

        if ((int32_t)mask < level)
        {
            // mcu->mcu.interrupt_pending[INTERRUPT_SOURCE_NMI] = 0;
            MCU_Interrupt_StartVector(mcu, vector, level);
            return;
        }
    }
}
