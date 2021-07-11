#include "mcu.h"
#include "mcu_interrupt.h"

void MCU_Interrupt_Start(void)
{
    MCU_PushStack(mcu.pc);
    MCU_PushStack(mcu.cp);
    MCU_PushStack(mcu.sr);
    mcu.sr &= ~STATUS_T;
}

void MCU_Interrupt_Request(uint32_t interrupt)
{
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
    uint32_t i;
    if (mcu.exmode)
        return;
    for (i = 0; i < 16; i++)
    {
        if (mcu.trapa_pending[i])
        {
            mcu.trapa_pending[i] = 0;
            MCU_Interrupt_StartVector(VECTOR_TRAPA_0 + i);
            return;
        }
    }
}



