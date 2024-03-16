#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_timer.h"
#include "pcm.h"
#include "lcd.h"
#include "submcu.h"
#include "midi.h"

const int ROM1_SIZE = 0x8000;
const int ROM2_SIZE = 0x80000;
const int RAM_SIZE = 0x400;
const int SRAM_SIZE = 0x8000;
const int ROMSM_SIZE = 0x1000;


static const int audio_buffer_size = 8192;
static const int audio_page_size = 256;

short sample_buffer[audio_buffer_size];

int sample_read_ptr;
int sample_write_ptr;

SDL_AudioDeviceID sdl_audio;

void MCU_ErrorTrap(void)
{
    printf("%.2x %.4x\n", mcu.cp, mcu.pc);
}


int ga_int[8];
int ga_int_enable;
int ga_int_trigger;


uint8_t dev_register[0x80];

uint16_t ad_val[4];
uint8_t ad_nibble;
uint8_t sw_pos = 0;
uint8_t io_sd;

uint8_t RCU_Read(void)
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

uint16_t MCU_AnalogReadPin(uint32_t pin)
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

void MCU_AnalogSample(int channel)
{
    int value = MCU_AnalogReadPin(channel);
    int dest = (channel << 1) & 6;
    dev_register[DEV_ADDRAH + dest] = value >> 2;
    dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

int adf_rd = 0;

uint64_t analog_end_time;

void MCU_DeviceWrite(uint32_t address, uint8_t data)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        TIMER_Write(address, data);
        return;
    }
    if (address >= 0x50 && address < 0x55)
    {
        TIMER2_Write(address, data);
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
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
        }
        if ((data & 0x40) == 0)
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
        return;
    }
    default:
        address += 0;
        break;
    }
    dev_register[address] = data;
}

uint8_t MCU_DeviceRead(uint32_t address)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        return TIMER_Read(address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return TIMER_Read(address);
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

void MCU_DeviceReset(void)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
    dev_register[DEV_RAME] = 0x80;
}

void MCU_UpdateAnalog(uint64_t cycles)
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
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        analog_end_time = 0;
}

mcu_t mcu;

uint8_t rom1[ROM1_SIZE];
uint8_t rom2[ROM2_SIZE];
uint8_t ram[RAM_SIZE];
uint8_t sram[SRAM_SIZE];

uint8_t MCU_Read(uint32_t address)
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
            if (address >= 0xe000 && address < 0xe400)
            {
                ret = PCM_Read(address & 0x3f);
            }
            else if (address >= 0xec00 && address < 0xf000)
            {
                ret = SM_SysRead(address & 0xff);
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
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, 0);
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
        ret = rom2[address_rom];
        break;
    case 10:
        ret = sram[address & 0x7fff]; // FIXME
        break;
    default:
        ret = 0x00;
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
    if (page == 0)
    {
        if (address & 0x8000)
        {
            if (address >= 0xe400 && address < 0xe800)
            {
                if (address == 0xe404 || address == 0xe405)
                    LCD_Write(address & 1, value);
                else if (address == 0xe401)
                {
                    io_sd = value;
                    LCD_Enable((value & 1) == 0);
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
                PCM_Write(address & 0x3f, value);
            }
            else if (address >= 0xec00 && address < 0xf000)
            {
                SM_SysWrite(address & 0xff, value);
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
            printf("Unknown write %x %x\n", address, value);
        }
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

void MCU_Write16(uint32_t address, uint16_t value)
{
    address &= ~1;
    MCU_Write(address, value >> 8);
    MCU_Write(address + 1, value & 0xff);
}

void MCU_ReadInstruction(void)
{
    uint8_t operand = MCU_ReadCodeAdvance();

    if (mcu.cycles == 0x45)
    {
        mcu.cycles += 0;
    }

    MCU_Operand_Table[operand](operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_TRACE);
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

    mcu.exception_pending = -1;

    MCU_DeviceReset();
}

void MCU_Update(int32_t cycles)
{
    SDL_Event event = {};
    bool working = true;

    while (working)
    {
        while (sample_read_ptr == sample_write_ptr)
            SDL_Delay(1);

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                working = false;
                break;
            default:
                break;
            }
        }

        if (!mcu.ex_ignore)
            MCU_Interrupt_Handle();
        else
            mcu.ex_ignore = 0;
        if (!mcu.sleep)
            MCU_ReadInstruction();

        mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

        if (mcu.cycles % 1200000 == 0)
            LCD_Update();

        if (mcu.cycles % 24000000 == 0)
            printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

        PCM_Update(mcu.cycles);

        TIMER_Clock(mcu.cycles);

        SM_Update(mcu.cycles);

        MCU_UpdateAnalog(mcu.cycles);
    }
}

void MCU_PatchROM(void)
{
    //rom2[0x1333] = 0x11;
    //rom2[0x1334] = 0x19;
    //rom1[0x622d] = 0x19;
}

uint32_t mcu_button_pressed;

uint8_t mcu_p0_data;

uint8_t MCU_ReadP0(void)
{
    return 0xff;
}

uint8_t MCU_ReadP1(void)
{
    uint8_t data = 0xff;
    if ((mcu_p0_data & 1) == 0)
        data &= 0x80 | (((mcu_button_pressed >> 0) & 127) ^ 127);
    if ((mcu_p0_data & 2) == 0)
        data &= 0x80 | (((mcu_button_pressed >> 7) & 127) ^ 127);
    if ((mcu_p0_data & 4) == 0)
        data &= 0x80 | (((mcu_button_pressed >> 14) & 127) ^ 127);
    return data;
}

void MCU_WriteP0(uint8_t data)
{
    mcu_p0_data = data;
}

void MCU_WriteP1(uint8_t data)
{
}

uint8_t tempbuf[0x200000];

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

void audio_callback(void* userdata, Uint8* stream, int len)
{
    len /= 2;
    memcpy(stream, &sample_buffer[sample_read_ptr], len * 2);
    memset(&sample_buffer[sample_read_ptr], 0, len * 2);
    sample_read_ptr += len;
    sample_read_ptr %= audio_buffer_size;
}

int MCU_OpenAudio(void)
{
    SDL_AudioSpec spec = {};
    SDL_AudioSpec spec_actual = {};

    spec.format = AUDIO_S16SYS;
    spec.freq = 33103;
    spec.channels = 2;
    spec.callback = audio_callback;
    spec.samples = audio_page_size / 4;

    sdl_audio = SDL_OpenAudioDevice(NULL, 0, &spec, &spec_actual, 0);
    if (!sdl_audio)
    {
        return 0;
    }

    sample_read_ptr = 0;
    sample_write_ptr = 0;

    SDL_PauseAudioDevice(sdl_audio, 0);

    return 1;
}

void MCU_PostSample(int *sample)
{
    sample_buffer[sample_write_ptr + 0] = sample[0];
    sample_buffer[sample_write_ptr + 1] = sample[1];
    sample_write_ptr = (sample_write_ptr + 2) % audio_buffer_size;
}

void MCU_GA_SetGAInt(int line, int value)
{
    // guesswork
    if (value && !ga_int[line] && (ga_int_enable & (1 << line)) != 0)
        ga_int_trigger = line;
    ga_int[line] = value;

    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, ga_int_trigger != 0);
}

int main(int argc, char *args[])
{
    FILE *r1 = fopen("rom1.bin", "rb");
    FILE *r2 = fopen("rom2.bin", "rb");
    FILE* r3 = fopen("rom_sm.bin", "rb");
    FILE* r4 = fopen("waverom1.bin", "rb");
    FILE* r5 = fopen("waverom2.bin", "rb");

    if (!r1 || !r2 || !r3 || !r4 || !r5)
        return 0;

    if (fread(rom1, 1, ROM1_SIZE, r1) != ROM1_SIZE)
        return 0;

    if (fread(rom2, 1, ROM2_SIZE, r2) != ROM2_SIZE)
        return 0;

    if (fread(sm_rom, 1, ROMSM_SIZE, r3) != ROMSM_SIZE)
        return 0;

    if (fread(tempbuf, 1, 0x200000, r4) != 0x200000)
        return 0;

    unscramble(tempbuf, waverom1, 0x200000);

    if (fread(tempbuf, 1, 0x100000, r5) != 0x100000)
        return 0;

    unscramble(tempbuf, waverom2, 0x100000);

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fclose(r1);
        fclose(r2);
        fclose(r3);
        fclose(r4);
        fclose(r5);
        return 0;
    }

    if (!MCU_OpenAudio())
    {
        return 0;
    }

    MIDI_Init();

    LCD_Init();
    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    SM_Reset();
    PCM_Reset();
    MCU_Update(100000000);

    fclose(r1);
    fclose(r2);
    fclose(r3);
    fclose(r4);
    fclose(r5);

    SDL_Quit();
    return 0;
}
