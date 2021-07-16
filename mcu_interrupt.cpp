#include "mcu.h"
#include "mcu_interrupt.h"

void MCU_Interrupt_Start(void)
{
    MCU_PushStack(mcu.pc);
    MCU_PushStack(mcu.cp);
    MCU_PushStack(mcu.sr);
    mcu.sr &= ~STATUS_T;
    mcu.sleep = 0;
}

void MCU_Interrupt_Request(uint32_t interrupt)
{
    if (interrupt == INTERRUPT_SOURCE_IRQ0 && (dev_register[DEV_P1CR] & 0x20) == 0)
        return;
    if (interrupt == INTERRUPT_SOURCE_IRQ1 && (dev_register[DEV_P1CR] & 0x40) == 0)
        return;
    mcu.interrupt_pending[interrupt] = 1;
}

void MCU_Interrupt_TRAPA(uint32_t vector)
{
    mcu.trapa_pending[vector] = 1;
}

void MCU_Interrupt_StartVector(uint32_t vector)
{
    uint32_t address = MCU_GetVectorAddress(vector);
    MCU_Interrupt_Start();
    mcu.cp = address >> 16;
    mcu.pc = address;
}

void MCU_Interrupt_Handle(void)
{
    if (mcu.cycles % 2000 == 0 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_94);
        return;
    }
    if (mcu.cycles % 2000 == 1000 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_A4);
        return;
    }
    if (mcu.cycles % 2000 == 1500 && mcu.sleep)
    {
        MCU_Interrupt_StartVector(VECTOR_INTERNAL_INTERRUPT_B4);
        return;
    }
    uint32_t i;
    for (i = 0; i < 16; i++)
    {
        if (mcu.trapa_pending[i])
        {
            mcu.trapa_pending[i] = 0;
            MCU_Interrupt_StartVector(VECTOR_TRAPA_0 + i);
            return;
        }
    }
    if (mcu.interrupt_pending[INTERRUPT_SOURCE_NMI])
    {
        mcu.interrupt_pending[INTERRUPT_SOURCE_NMI] = 0;
        MCU_Interrupt_StartVector(VECTOR_NMI);
        return;
    }
}
