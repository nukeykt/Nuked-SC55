#include <stdint.h>
#include <stdio.h>
#include "mcu.h"
#include "mcu_dsp.h"

dsp_t dsp;

void DSP_Write(uint32_t address, uint8_t data)
{
    printf("DSP Write: %.2x %.2x\n", address, data);
}

uint8_t DSP_Read(uint32_t address)
{
    printf("DSP Read: %.2x\n", address);
    return 0x7f;
}

void DSP_Update(uint32_t cycles)
{

}
