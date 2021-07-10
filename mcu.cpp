#include <stdio.h>
#include <string.h>
#include "mcu.h"
#include "mcu_opcodes.h"

const int ROM1_SIZE = 0x8000;
const int ROM2_SIZE = 0x80000;
const int RAM_SIZE = 0x8000;


void MCU_ErrorTrap(void)
{
    printf("%x %x", mcu.cp, mcu.pc);
}

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
        if (!(address & 0x8000))
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

    mcu.cycles++;
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
}

void MCU_Update(int32_t cycles)
{
    while (mcu.cycles < cycles)
    {
        MCU_ReadInstruction();
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

    MCU_Init();
    MCU_Reset();
    MCU_Update(10000);

    fclose(r1);
    fclose(r2);
    return 0;
}
