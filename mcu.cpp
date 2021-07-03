#include <stdio.h>
#include "mcu.h"

const int ROM1_SIZE = 0x8000;
const int ROM2_SIZE = 0x80000;
const int RAM_SIZE = 0x8000;

struct mcu_t {
    uint16_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint16_t pc;
    uint8_t cp, dp, ep, fp;
    uint32_t cycles;
};


mcu_t mcu;

uint8_t rom1[ROM1_SIZE];
uint8_t rom2[ROM2_SIZE];
uint8_t ram[RAM_SIZE];

uint8_t MCU_Read(uint32_t address)
{
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    uint8_t ret = 0xff;
    switch (page)
    {
    case 0:
        if (address & 0x8000)
            ret = rom1[address & 0x7fff];
        else
        {
            ret = ram[address & 0x7fff];
        }
        break;
    case 3:
        ret = rom2[address | 0x30000];
        break;
    case 4:
        ret = rom2[address];
        break;
    }
    return 0;
}

void MCU_Write(uint32_t address, uint8_t value)
{
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    if (page != 0)
        return;
    if (address & 0x8000)
    {
        if (0)
        {

        }
        else
            ram[address & 0x7fff] = value;
    }
}

void MCU_Update(int32_t cycles)
{
    while (mcu.cycles < cycles)
    {
        mcu.cycles++;
    }
}

int main()
{
    FILE *r1 = fopen("rom1.bin", "rb");
    FILE *r2 = fopen("rom2.bin", "rb");

    if (!r1 || !r2)
        return 0;

    if (fread(rom1, 1, ROM1_SIZE, r1) != ROM1_SIZE)
        return 0;

    if (fread(rom2, 1, ROM2_SIZE, r2) != ROM2_SIZE)
        return 0;

    fclose(r1);
    fclose(r2);
    return 0;
}
