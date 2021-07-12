#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_dsp.h"
#include "mcu_uart.h"
#include "lcd.h"

const int ROM1_SIZE = 0x8000;
const int ROM2_SIZE = 0x80000;
const int RAM_SIZE = 0x8000;


void MCU_ErrorTrap(void)
{
    printf("%.2x %.4x\n", mcu.cp, mcu.pc);
}

uint8_t dev_register[0x80];

void MCU_DeviceWrite(uint32_t address, uint8_t data)
{
    address &= 0x7f;
    dev_register[address] = data;
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
    case DEV_FRT1_FRCH:
        break;
    case DEV_FRT1_FRCL:
        break;
    case DEV_FRT1_OCRAH:
        break;
    case DEV_FRT1_OCRAL:
        break;
    case DEV_FRT1_TCR:
        break;
    case DEV_FRT1_TCSR:
        break;
    case DEV_FRT2_FRCH:
        break;
    case DEV_FRT2_FRCL:
        break;
    case DEV_FRT2_OCRAH:
        break;
    case DEV_FRT2_OCRAL:
        break;
    case DEV_FRT2_TCR:
        break;
    case DEV_FRT2_TCSR:
        break;
    case DEV_FRT3_FRCH:
        break;
    case DEV_FRT3_FRCL:
        break;
    case DEV_FRT3_OCRAH:
        break;
    case DEV_FRT3_OCRAL:
        break;
    case DEV_FRT3_TCR:
        break;
    case DEV_FRT3_TCSR:
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
        break;
    default:
        address += 0;
        break;
    }
}

uint8_t MCU_DeviceRead(uint32_t address)
{
    address &= 0x7f;
    switch (address)
    {
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
    case DEV_ADCSR:
        return dev_register[address];
    }
    return dev_register[address];
}

void MCU_DeviceReset(void)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
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
            if (address >= 0xe000 && address < 0xe400)
            {
                ret = DSP_Read(address & 0x3f);
            }
            else if (address >= 0xec00 && address < 0xf000)
            {
                ret = UART_Read(address & 0xff);
            }
            else if (address >= 0xff80)
            {
                ret = MCU_DeviceRead(address & 0x7f);
            }
            else
                ret = ram[address & 0x7fff];
        }
        break;
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
    default:
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
        if (address >= 0xe404 && address < 0xe406)
        {
            LCD_Write(address & 1, value);
        }
        else if (address >= 0xe000 && address < 0xe400)
        {
            DSP_Write(address & 0x3f, value);
        }
        else if (address >= 0xec00 && address < 0xf000)
        {
            UART_Write(address & 0xff, value);
        }
        else if (address >= 0xff80)
        {
            MCU_DeviceWrite(address & 0x7f, value);
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
    if (mcu.pc == 0x622d)
    {
        mcu.pc += 0;
    }
    uint8_t operand = MCU_ReadCodeAdvance();

    if (mcu.cycles == 0x45)
    {
        mcu.cycles += 0;
    }

    MCU_Operand_Table[operand](operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Request(INTERRUPT_SOURCE_TRACE);
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
}

void MCU_Update(int32_t cycles)
{
    while (mcu.cycles < cycles)
    {
        if (mcu.cycles % 1000 == 0)
            LCD_Update();
        if (!mcu.ex_ignore)
            MCU_Interrupt_Handle();
        else
            mcu.ex_ignore = 0;
        if (!mcu.sleep)
            MCU_ReadInstruction();
        mcu.cycles++;
    }
}

void MCU_PatchROM(void)
{
    rom2[0x1333] = 0x11;
    rom2[0x1334] = 0x19;
    rom1[0x622d] = 0x19;
}

int main(int argc, char *args[])
{
    FILE *r1 = fopen("rom1.bin", "rb");
    FILE *r2 = fopen("rom2.bin", "rb");

    if (!r1 || !r2)
        return 0;

    if (fread(rom1, 1, ROM1_SIZE, r1) != ROM1_SIZE)
        return 0;

    if (fread(rom2, 1, ROM2_SIZE, r2) != ROM2_SIZE)
        return 0;

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fclose(r1);
        fclose(r2);
        return 0;
    }

    LCD_Init();
    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    MCU_Update(100000000);

    fclose(r1);
    fclose(r2);

    SDL_Quit();
    return 0;
}
