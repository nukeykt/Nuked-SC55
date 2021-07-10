#include "mcu.h"
#include "mcu_interrupt.h"


void MCU_Interrupt_Request(uint32_t interrupt)
{
    mcu.interrupt_pending[interrupt] = 1;
}


void MCU_Interrupt_Handle(void)
{
    if (mcu.exmode)
        return;
}



