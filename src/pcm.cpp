#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mcu.h"
#include "mcu_interrupt.h"
#include "pcm.h"

pcm_t pcm;
uint8_t waverom1[0x200000];
uint8_t waverom2[0x100000];

uint8_t PCM_ReadROM(uint32_t address)
{
    int bank;
    if (pcm.config_reg_3d & 0x20)
        bank = (address >> 21) & 7;
    else
        bank = (address >> 19) & 7;
    switch (bank)
    {
        case 0:
            return waverom1[address & 0x1fffff];
        case 1:
            return waverom2[address & 0xfffff];
        default:
            break;
    }
    return 0;
}

void PCM_Write(uint32_t address, uint8_t data)
{
    address &= 0x3f;
    //printf("PCM Write: %.2x %.2x\n", address, data);
    if (address < 0x4) // voice enable
    {
        switch (address & 3)
        {
            case 0:
                pcm.voice_mask_pending &= ~0xf000000;
                pcm.voice_mask_pending |= (data & 0xf) << 24;
                break;
            case 1:
                pcm.voice_mask_pending &= ~0xff0000;
                pcm.voice_mask_pending |= (data & 0xff) << 16;
                break;
            case 2:
                pcm.voice_mask_pending &= ~0xff00;
                pcm.voice_mask_pending |= (data & 0xff) << 8;
                break;
            case 3:
                pcm.voice_mask_pending &= ~0xff;
                pcm.voice_mask_pending |= (data & 0xff) << 0;
                break;
        }
        pcm.voice_mask_updating = 1;
    }
    else if (address >= 0x20 && address < 0x24) // wave rom
    {
        switch (address & 3)
        {
            case 1:
                pcm.wave_read_address &= ~0xff0000;
                pcm.wave_read_address |= (data & 0xff) << 16;
                break;
            case 2:
                pcm.wave_read_address &= ~0xff00;
                pcm.wave_read_address |= (data & 0xff) << 8;
                break;
            case 3:
                pcm.wave_read_address &= ~0xff;
                pcm.wave_read_address |= (data & 0xff) << 0;
                pcm.wave_byte_latch = PCM_ReadROM(pcm.wave_read_address);
                break;
        }
    }
    else if (address == 0x3c)
    {
        pcm.config_reg_3c = data;
    }
    else if (address == 0x3d)
    {
        pcm.config_reg_3d = data;
    }
    else if (address == 0x3e)
    {
        pcm.select_channel = data & 0x1f;
    }
    else if ((address >= 0x4 && address < 0x10) || (address >= 0x24 && address < 0x30))
    {
        switch (address & 3)
        {
            case 1:
                pcm.write_latch &= ~0xf0000;
                pcm.write_latch |= (data & 0xf) << 16;
                break;
            case 2:
                pcm.write_latch &= ~0xff00;
                pcm.write_latch |= (data & 0xff) << 8;
                break;
            case 3:
                pcm.write_latch &= ~0xff;
                pcm.write_latch |= (data & 0xff) << 0;
                break;
        }
        if ((address & 3) == 3)
        {
            int ix = 0;
            if (address & 32)
                ix |= 1;
            if ((address & 8) == 0)
                ix |= 4;
            if ((address & 4) == 0)
                ix |= 2;

            pcm.ram1[pcm.select_channel][ix] = pcm.write_latch;
        }
    }
    else if ((address >= 0x10 && address < 0x20) || (address >= 0x30 && address < 0x38))
    {
        switch (address & 1)
        {
        case 0:
            pcm.write_latch &= ~0xff00;
            pcm.write_latch |= (data & 0xff) << 8;
            break;
        case 1:
            pcm.write_latch &= ~0xff;
            pcm.write_latch |= (data & 0xff) << 0;
            break;
        }
        if ((address & 1) == 1)
        {
            int ix = (address >> 1) & 7;
            if (address & 32)
                ix |= 8;

            //if (ix == 0)
            //    printf("pitch %i %x\n", pcm.select_channel, pcm.write_latch & 0xffff);
            // if (ix == 5)
            //     printf("tv3 %i %x\n", pcm.select_channel, pcm.write_latch & 0xffff);

            pcm.ram2[pcm.select_channel][ix] = pcm.write_latch;
        }
    }
}

uint8_t PCM_Read(uint32_t address)
{
    address &= 0x3f;
    //printf("PCM Read: %.2x\n", address);

    if (address < 0x4)
    {
        if (pcm.voice_mask_updating)
            pcm.voice_mask = pcm.voice_mask_pending;
        pcm.voice_mask_updating = 0;
    }
    else if (address == 0x3c || address == 0x3e) // status
    {
        uint8_t status = 0;
        if (address == 0x3e && pcm.irq_assert)
        {
            pcm.irq_assert = 0;
            MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ0, 0);
        }

        status |= pcm.irq_channel;
        if (pcm.voice_mask_updating)
            status |= 32;

        return status;
    }
    else if (address == 0x3f)
    {
        return pcm.wave_byte_latch;
    }
    else if ((address >= 0x4 && address < 0x10) || (address >= 0x24 && address < 0x30))
    {
        if ((address & 3) == 1)
        {
            int ix = 0;
            if (address & 32)
                ix |= 1;
            if ((address & 8) == 0)
                ix |= 4;
            if ((address & 4) == 0)
                ix |= 2;

            pcm.read_latch = pcm.ram1[pcm.select_channel][ix];
        }
    }
    else if ((address >= 0x10 && address < 0x20) || (address >= 0x30 && address < 0x38))
    {
        if ((address & 1) == 0)
        {
            int ix = (address >> 1) & 7;
            if (address & 32)
                ix |= 8;

            pcm.read_latch = pcm.ram2[pcm.select_channel][ix];
            // if (pcm.select_channel >= 30)
            //     printf("read common %i %i\n", pcm.select_channel, ix);

            // hack
            //if (ix == 7)
            //    pcm.read_latch |= 0x20;
            // if (ix == 7 && ((pcm.voice_mask & pcm.voice_mask_pending) & (1<< pcm.select_channel)) != 0)
            //     pcm.read_latch |= 0x20;
        }
    }
    else if (address >= 0x39 && address <= 0x3b)
    {
        switch (address & 3)
        {
            case 1:
                return (pcm.read_latch >> 16) & 0xf;
            case 2:
                return (pcm.read_latch >> 8) & 0xff;
            case 3:
                return (pcm.read_latch >> 0) & 0xff;
        }
    }

    return 0;
}

void PCM_Reset(void)
{
    memset(&pcm, 0, sizeof(pcm));
}

inline uint32_t addclip20(uint32_t add1, uint32_t add2, uint32_t cin)
{
    uint32_t sum = (add1 + add2 + cin) & 0xfffff;
    if ((add1 & 0x80000) != 0 && (add2 & 0x80000) != 0 && (sum & 0x80000) == 0)
        sum = 0x80000;
    else if ((add1 & 0x80000) == 0 && (add2 & 0x80000) == 0 && (sum & 0x80000) != 0)
        sum = 0x7ffff;
    return sum;
}

inline int32_t multi(int32_t val1, int8_t val2)
{
    if (val1 & 0x80000)
        val1 |= ~0xfffff;
    else
        val1 &= 0x7ffff;

    val1 *= val2;
    if (val1 & 0x8000000)
        val1 |= ~0x1ffffff;
    else
        val1 &= 0x1ffffff;
    return val1;
}

static const int interp_lut[3][128] = {
    3385, 3401, 3417, 3432, 3448, 3463, 3478, 3492, 3506, 3521, 3534, 3548, 3562, 3575, 3588, 3601,
    3614, 3626, 3638, 3650, 3662, 3673, 3685, 3696, 3707, 3718, 3728, 3739, 3749, 3759, 3768, 3778,
    3787, 3796, 3805, 3814, 3823, 3831, 3839, 3847, 3855, 3863, 3870, 3878, 3885, 3892, 3899, 3905,
    3912, 3918, 3924, 3930, 3936, 3942, 3948, 3953, 3958, 3963, 3968, 3973, 3978, 3983, 3987, 3991,
    3995, 4000, 4004, 4007, 4011, 4015, 4018, 4022, 4025, 4028, 4031, 4034, 4037, 4040, 4042, 4045,
    4047, 4050, 4052, 4054, 4057, 4059, 4061, 4063, 4064, 4066, 4068, 4070, 4071, 4073, 4074, 4076,
    4077, 4078, 4079, 4081, 4082, 4083, 4084, 4085, 4086, 4086, 4087, 4088, 4089, 4089, 4090, 4091,
    4091, 4092, 4092, 4093, 4093, 4094, 4094, 4094, 4094, 4095, 4095, 4095, 4095, 4095, 4095, 4095,

    710, 726, 742, 758, 775, 792, 809, 826, 844, 861, 879, 897, 915, 933, 952, 971,
    990, 1009, 1028, 1047, 1067, 1087, 1106, 1126, 1147, 1167, 1188, 1208, 1229, 1250, 1271, 1292,
    1314, 1335, 1357, 1379, 1400, 1423, 1445, 1467, 1489, 1512, 1534, 1557, 1580, 1602, 1625, 1648,
    1671, 1695, 1718, 1741, 1764, 1788, 1811, 1835, 1858, 1882, 1906, 1929, 1953, 1977, 2000, 2024,
    2048, 2069, 2095, 2119, 2143, 2166, 2190, 2214, 2237, 2261, 2284, 2308, 2331, 2355, 2378, 2401,
    2425, 2448, 2471, 2494, 2517, 2539, 2562, 2585, 2607, 2630, 2652, 2674, 2696, 2718, 2740, 2762,
    2783, 2805, 2826, 2847, 2868, 2889, 2910, 2931, 2951, 2971, 2991, 3011, 3031, 3051, 3070, 3089,
    3108, 3127, 3146, 3164, 3182, 3200, 3218, 3236, 3253, 3271, 3288, 3304, 3321, 3338, 3354, 3370,

    0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 8, 8, 9, 10, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 22, 23, 24, 26, 27, 29, 30, 32, 34, 36, 38, 40, 42, 44, 46,
    49, 51, 53, 56, 59, 62, 65, 68, 71, 74, 77, 81, 84, 88, 92, 96,
    100, 104, 109, 113, 118, 122, 127, 132, 137, 143, 148, 154, 160, 165, 171, 178,
    184, 191, 197, 204, 211, 219, 226, 234, 241, 249, 257, 266, 274, 283, 292, 301,
    310, 319, 329, 339, 349, 359, 369, 380, 391, 402, 413, 424, 436, 448, 460, 472,
    484, 497, 510, 523, 536, 549, 563, 577, 591, 605, 619, 634, 648, 663, 679, 694,
};

void PCM_Update(uint64_t cycles)
{
    int slots = (pcm.config_reg_3d & 31) + 1;
    int voice_active = pcm.voice_mask & pcm.voice_mask_pending;
    while (pcm.cycles < cycles)
    {
        int tt[2] = {};

        { //?
            int shifter = pcm.ram2[30][10];
            int xr = ((shifter >> 0) ^ (shifter >> 1) ^ (shifter >> 7) ^ (shifter >> 12)) & 1;
            shifter = (shifter >> 1) | (xr << 15);
            pcm.ram2[30][10] = shifter;
            if (pcm.config_reg_3c & 0x40)
            {
                xr = ((shifter >> 0) ^ (shifter >> 1) ^ (shifter >> 7) ^ (shifter >> 12)) & 1;
                shifter = (shifter >> 1) | (xr << 15);
                pcm.ram2[30][10] = shifter;
            }
        }

        { // global counter for envelopes
            if (!pcm.nfs)
                pcm.tv_counter = pcm.ram2[31][8]; // fixme

            pcm.tv_counter += 1;

            pcm.tv_counter &= 0x3fff;
        }

        // accummulator
        pcm.ram1[31][1] = 0;
        pcm.ram1[31][3] = 0;

        for (int slot = 0; slot < slots; slot++)
        {
            uint32_t *ram1 = pcm.ram1[slot];
            uint16_t *ram2 = pcm.ram2[slot];
            int okey = (ram2[7] & 0x20) != 0;
            int key = (voice_active >> slot) & 1;

            //if (key && !okey)
            //    printf("KON %i\n", slot);

            int active = okey && key;
            int kon = key && !okey;

            // address generator

            int b15 = (ram2[8] & 0x8000) != 0; // 0
            int b6 = (ram2[7] & 0x40) != 0; // 1
            int b7 = (ram2[7] & 0x80) != 0; // 1
            int hiaddr = (ram2[7] >> 8) & 15; // 1
            int old_nibble = (ram2[7] >> 12) & 15; // 1
            //b6 = 0;
            //b7 = 0;

            int address = ram1[4]; // 0
            int address_end = ram1[0]; // 1 or 2
            int address_loop = ram1[2]; // 2 or 1

            int cmp1 = b15 ? address_loop : address_end;
            int cmp2 = address;
            int nibble_cmp1 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 2
            int irq_flag = 0;

            // fixme:
            if (kon)
                irq_flag = ((cmp1 + address_loop) & 0x100000) != 0;
            else
                irq_flag = ((address + ((-address_loop) & 0xfffff)) & 0x100000) != 0;
            irq_flag ^= b7;

            //int nibble_address = (kon || (!b6 && nibble_cmp1)) ?
            //    ((!b6 && nibble_cmp1) ? address_loop : address) :
            //    address;
            int nibble_address = (!b6 && nibble_cmp1) ? address_loop : address; // 3
            int address_b4 = (nibble_address & 0x10) != 0;
            int wave_address = nibble_address >> 5;
            int xor2 = (address_b4 ^ b7);
            int check1 = xor2 && active;
            int xor1 = (b15 ^ !nibble_cmp1);
            int nibble_add = b6 ? check1 && xor1 : (!nibble_cmp1 && check1);
            int nibble_subtract = b6 && !xor1 && active && !xor2;
            wave_address += nibble_add - nibble_subtract;
            wave_address &= 0xfffff;

            int newnibble = PCM_ReadROM((hiaddr << 20) | wave_address);
            int newnibble_sel = address_b4 ^ ((b6 || !nibble_cmp1) && okey);
            if (newnibble_sel)
                newnibble = (newnibble >> 4) & 15;
            else
                newnibble &= 15;

            int sub_phase = (ram2[8] & 0x3fff); // 1
            int interp_ratio = (sub_phase >> 7) & 127;
            sub_phase += pcm.ram2[ram2[7] & 31][0]; // 5
            int sub_phase_of = (sub_phase >> 14) & 7;
            ram2[8] &= ~0x3fff;
            ram2[8] |= sub_phase & 0x3fff;


            // address 0
            int address_cnt = address;
            int samp0 = (int8_t)PCM_ReadROM((hiaddr << 20) | address_cnt); // 18

            cmp1 = address;
            cmp2 = address_cnt;
            int nibble_cmp2 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 8
            cmp1 = b15 ? address_loop : address_end;
            cmp2 = address_cnt;
            int address_cmp = (cmp1 & 0xfffff) == (cmp2 & 0xfffff); // 9

            int next_address = address_cnt; // 11
            int usenew = !nibble_cmp2;
            int next_b15 = b15;

            cmp1 = (!b6 && address_cmp) ? address_loop : address_cnt;
            cmp2 = address_cnt;
            int address_cnt2 = (kon || (!b6 && address_cmp)) ? cmp1 : cmp2;

            int address_add = (!address_cmp && b6 && !b15) || (!address_cmp && !b6);
            int address_sub = !address_cmp && b6 && b15;
            if (b7)
                address_cnt2 -= address_add - address_sub;
            else
                address_cnt2 += address_add - address_sub;
            address_cnt = address_cnt2 & 0xfffff; // 11
            b15 = b6 && (b15 ^ address_cmp); // 11

            int samp1 = (int8_t)PCM_ReadROM((hiaddr << 20) | address_cnt); // 20

            cmp1 = address;
            cmp2 = address_cnt;
            int nibble_cmp3 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 12
            cmp1 = b15 ? address_loop : address_end;
            cmp2 = address_cnt;
            address_cmp = (cmp1 & 0xfffff) == (cmp2 & 0xfffff); // 13

            if (sub_phase_of >= 1)
            {
                next_address = address_cnt; // 13
                usenew = !nibble_cmp3;
                next_b15 = b15;
            }

            cmp1 = (!b6 && address_cmp) ? address_loop : address_cnt;
            cmp2 = address_cnt;
            address_cnt2 = (kon || (!b6 && address_cmp)) ? cmp1 : cmp2;

            address_add = (!address_cmp && b6 && !b15) || (!address_cmp && !b6);
            address_sub = !address_cmp && b6 && b15;
            if (b7)
                address_cnt2 -= address_add - address_sub;
            else
                address_cnt2 += address_add - address_sub;
            address_cnt = address_cnt2 & 0xfffff; // 15
            b15 = b6 && (b15 ^ address_cmp); // 15

            int samp2 = (int8_t)PCM_ReadROM((hiaddr << 20) | address_cnt); // 1

            cmp1 = address;
            cmp2 = address_cnt;
            int nibble_cmp4 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 16
            cmp1 = b15 ? address_loop : address_end;
            cmp2 = address_cnt;
            address_cmp = (cmp1 & 0xfffff) == (cmp2 & 0xfffff); // 17

            if (sub_phase_of >= 2)
            {
                next_address = address_cnt; // 17
                usenew = !nibble_cmp4;
                next_b15 = b15;
            }

            cmp1 = (!b6 && address_cmp) ? address_loop : address_cnt;
            cmp2 = address_cnt;
            address_cnt2 = (kon || (!b6 && address_cmp)) ? cmp1 : cmp2;

            address_add = (!address_cmp && b6 && !b15) || (!address_cmp && !b6);
            address_sub = !address_cmp && b6 && b15;
            if (b7)
                address_cnt2 -= address_add - address_sub;
            else
                address_cnt2 += address_add - address_sub;
            address_cnt = address_cnt2 & 0xfffff; // 19
            b15 = b6 && (b15 ^ address_cmp); // 19

            int samp3 = (int8_t)PCM_ReadROM((hiaddr << 20) | address_cnt); // 5

            cmp1 = address;
            cmp2 = address_cnt;
            int nibble_cmp5 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 20
            cmp1 = b15 ? address_loop : address_end;
            cmp2 = address_cnt;
            address_cmp = (cmp1 & 0xfffff) == (cmp2 & 0xfffff); // 21

            if (sub_phase_of >= 3)
            {
                next_address = address_cnt; // 21
                usenew = !nibble_cmp5;
                next_b15 = b15;
            }

            cmp1 = (!b6 && address_cmp) ? address_loop : address_cnt;
            cmp2 = address_cnt;
            address_cnt2 = (kon || (!b6 && address_cmp)) ? cmp1 : cmp2;

            address_add = (!address_cmp && b6 && !b15) || (!address_cmp && !b6);
            address_sub = !address_cmp && b6 && b15;
            if (b7)
                address_cnt2 -= address_add - address_sub;
            else
                address_cnt2 += address_add - address_sub;
            address_cnt = address_cnt2 & 0xfffff; // 23
            // b15 = b6 && (b15 ^ address_cmp); // 23

            cmp1 = address;
            cmp2 = address_cnt;
            int nibble_cmp6 = (cmp1 & 0xffff0) == (cmp2 & 0xffff0); // 24

            if (sub_phase_of >= 4)
            {
                next_address = address_cnt; // 1
                usenew = !nibble_cmp6;
                // b15 is not updated?
            }

            if (active && pcm.nfs)
                ram1[4] = next_address;

            ram2[8] &= ~0x8000;
            ram2[8] |= next_b15 << 15;

            // dpcm

            // 18
            int reference = ram1[5];

            // 19
            int preshift = samp0 << 10;
            int select_nibble = nibble_cmp2 ? old_nibble : newnibble;
            int shift = (10 - select_nibble) & 15;

            int shifted = (preshift << 1) >> shift;

            if (sub_phase_of >= 1)
            {

                //int sampt = (int8_t)PCM_ReadROM((hiaddr << 20) | address); // 20
                //int sampt2 = (int8_t)PCM_ReadROM((hiaddr << 20) | (address >> 5)); // 20
                //if (address & 0x10)
                //    sampt2 = (sampt2 >> 4) & 15;
                //else
                //    sampt2 = (sampt2 >> 0) & 15;
                //printf("samp %i %i\n", samp0, select_nibble);
                //printf("should %i %i\n", sampt, sampt2);
                //if ((sampt2 != select_nibble || samp0 != sampt) && active)
                //    printf("hmm");
                reference = addclip20(reference, shifted >> 1, shifted & 1);
            }

            //if (ram2[0])
            //    printf("%i %i\n", address, old_nibble);

            preshift = samp1 << 10;
            select_nibble = nibble_cmp3 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;

            shifted = (preshift << 1) >> shift;

            if (sub_phase_of >= 2)
            {

                //int sampt = (int8_t)PCM_ReadROM((hiaddr << 20) | (address + 1)); // 20
                //int sampt2 = (int8_t)PCM_ReadROM((hiaddr << 20) | ((address + 1) >> 5)); // 20
                //if ((address + 1) & 0x10)
                //    sampt2 = (sampt2 >> 4) & 15;
                //else
                //    sampt2 = (sampt2 >> 0) & 15;
                //printf("samp %i %i\n", samp0, select_nibble);
                //printf("should %i %i\n", sampt, sampt2);
                //if ((sampt2 != select_nibble || samp1 != sampt) && active)
                //    printf("hmm");
                reference = addclip20(reference, shifted >> 1, shifted & 1);
            }

            preshift = samp2 << 10;
            select_nibble = nibble_cmp4 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;

            shifted = (preshift << 1) >> shift;

            if (sub_phase_of >= 3)
                reference = addclip20(reference, shifted >> 1, shifted & 1);

            preshift = samp3 << 10;
            select_nibble = nibble_cmp5 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;

            shifted = (preshift << 1) >> shift;

            if (sub_phase_of >= 4)
                reference = addclip20(reference, shifted >> 1, shifted & 1);

            // interpolation

            int test = ram1[5];

            int step0 = multi(interp_lut[0][interp_ratio] << 6, samp0) >> 8;
            select_nibble = nibble_cmp2 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;
            step0 =  (step0 << 1) >> shift;

            test = addclip20(test, step0 >> 1, step0 & 1);


            int step1 = multi(interp_lut[1][interp_ratio] << 6, samp1) >> 8;
            select_nibble = nibble_cmp3 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;
            step1 = (step1 << 1) >> shift;

            test = addclip20(test, step1 >> 1, step1 & 1);

            int step2 = multi(interp_lut[2][interp_ratio] << 6, samp2) >> 8;
            select_nibble = nibble_cmp4 ? old_nibble : newnibble;
            shift = (10 - select_nibble) & 15;
            step2 = (step2 << 1) >> shift;

            int reg1 = ram1[1];
            int reg3 = ram1[3];
            int reg2_6 = (ram2[6] >> 8) & 127;

            test = addclip20(test, step2 >> 1, step2 & 1);

            int filter = ram2[11];

            int mult1 = multi(reg1, filter >> 8); // 8
            int mult2 = multi(reg1, (filter >> 1) & 127); // 9
            int mult3 = multi(reg1, reg2_6); // 10

            int v2 = addclip20(reg3, mult1 >> 6, (mult1 >> 5) & 1); // 9
            int v1 = addclip20(v2, mult2 >> 13, (mult2 >> 12) & 1); // 10
            int subvar = addclip20(v1, (mult3 >> 6), (mult3 >> 5) & 1); // 11

            ram1[3] = v1;

            int v3 = addclip20(test, subvar ^ 0xfffff, 1); // 12

            int mult4 = multi(v3, filter >> 8);
            int mult5 = multi(v3, (filter >> 1) & 127);
            int v4 = addclip20(reg1, mult4 >> 6, (mult4 >> 5) & 1); // 14
            int v5 = addclip20(v4, mult5 >> 13, (mult5 >> 12) & 1); // 15

            ram1[5] = reference;
            ram1[1] = v5;

            ram2[7] &= ~0xf000;

            ram2[7] |= ((usenew || kon) ? newnibble : old_nibble) << 12;

#if 0
            int sub_cnt = ram2[8] & 0x3fff;
            sub_cnt += (pcm.ram2[ram2[7] & 31][0])&0xffff;
            int sub_cnt_of = (sub_cnt >> 14) & 7;

            // check irq
            int irq_condition = ram1[4] >= ram1[2]; // HACK
            irq_condition = 0;

            ram2[8] &= ~0x3fff;
            ram2[8] |= sub_cnt & 0x3fff;
#endif

            if (active && (ram2[6] & 1) != 0 && (ram2[8] & 0x4000) == 0 && !pcm.irq_assert && irq_flag)
            {
                //printf("irq voice %i\n", slot);
                ram2[8] |= 0x4000;
                pcm.irq_assert = 1;
                pcm.irq_channel = slot;
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ0, 1);
            }

#if 0
            for (int j = 0; j < sub_cnt_of; j++)
            {
                if (ram1[4] >= ram1[0])
                    continue;
                int bank = (ram2[7] & 0xf00) << 12;
                int sh = PCM_ReadROM(bank | (ram1[4] >> 5));
                int nibble = (ram1[4] & 16) != 0 ? (sh >> 4) : sh & 15;
                int samp = PCM_ReadROM(bank | ram1[4]);
                int shift = (10 - nibble) & 15;
                int diff = ((int8_t)samp << 11) >> shift;

                ram1[5] = addclip20(ram1[5], diff);

                ram1[4]++;
                if (ram1[4] > ram1[0])
                    ram1[4] = ram1[0];
                if (ram1[4] == ram1[2])
                    ram1[7] = ram1[5];
                if (ram1[4] == ram1[0])
                {
                    ram1[4] = ram1[2];
                    ram1[5] = ram1[7];
                }
            }
#endif

#if 0
            //int ss1 = (int)(ram1[5] << 12) >> 8;
            //int ss1 = (int)(test << 12) >> 8;
            int ss1 = (int)(((ram2[6] & 2) == 0 ? ram1[3] : v3) << 12) >> 8;
            int ss = ss1;
            ss *= (ram2[10] >> 8);
            ss >>= 7;
            //
            ss *= (ram2[9] >> 8);
            ss >>= 6;

            {
                ss >>= 8;
                int r = (ram2[1] & 0xff);
                int l = (ram2[1] >> 8) & 0xff;
                tt[0] += (ss * l)>> 4;
                tt[1] += (ss * r) >> 4;
            }
#endif

            int volmul1 = 0;
            int volmul2 = 0;
#if 1
            for (int e = 0; e < 3; e++)
            {
                int adjust = ram2[3+e];
                int levelcur = ram2[9+e] & 0x7fff;
                int speed = adjust & 0xff;
                int target = (adjust >> 8) & 0xff;

                // speed 0-175: tick always
                // speed 176-192:
                // speed 192-255: tick slow
                //
                //

                
                int w1 = (speed & 0xf0) == 0;
                int w2 = w1 || (speed & 0x10) != 0;
                int w3 = pcm.nfs &&
                    ((speed & 0x80) == 0 || ((speed & 0x40) == 0 && (!w2 || (speed & 0x20) == 0)));

                // w3 = speed < 160;
                // 
                // int b4 = w2;
                // int b5 = (speed & 0x20) != 0;
                // int b6 = (speed & 0x40) != 0;
                // int b7 = (speed & 0x80) != 0;
                // 
                // w3 = !(((b4 && b5) || b6) && b7);

                int type = w2 | (w3 << 3);
                if (speed & 0x20)
                    type |= 2;
                if ((speed & 0x80) == 0 || (speed & 0x40) == 0)
                    type |= 4;


                int write = !active;
                int addlow = 0;
                if (type & 4)
                {
                    if (pcm.tv_counter & 8)
                        addlow |= 1;
                    if (pcm.tv_counter & 4)
                        addlow |= 2;
                    if (pcm.tv_counter & 2)
                        addlow |= 4;
                    if (pcm.tv_counter & 1)
                        addlow |= 8;
                    write |= 1;
                }
                else
                {
                    switch (type & 3)
                    {
                    case 0:
                        if (pcm.tv_counter & 0x20)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x10)
                            addlow |= 2;
                        if (pcm.tv_counter & 8)
                            addlow |= 4;
                        if (pcm.tv_counter & 4)
                            addlow |= 8;
                        write |= (pcm.tv_counter & 3) == 0;
                        break;
                    case 1:
                        if (pcm.tv_counter & 0x80)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x40)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x20)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x10)
                            addlow |= 8;
                        write |= (pcm.tv_counter & 15) == 0;
                        break;
                    case 2:
                        if (pcm.tv_counter & 0x200)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x100)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x80)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x40)
                            addlow |= 8;
                        write |= (pcm.tv_counter & 63) == 0;
                        break;
                    case 3:
                        if (pcm.tv_counter & 0x800)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x400)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x200)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x100)
                            addlow |= 8;
                        write |= (pcm.tv_counter & 127) == 0;
                        break;
                    }
                }

                if ((type & 8) == 0)
                {
                    int shift = speed & 15;
                    shift = (10 - shift) & 15;

                    int sum1 = (target << 11); // 5
                    if (e != 2 || active)
                        sum1 -= (levelcur << 4); // 6
                    int neg = (sum1 & 0x80000) != 0;

                    int preshift = sum1;

                    int shifted = preshift >> shift;
                    shifted -= sum1;

                    int sum2 = (target << 11) + addlow + shifted;
                    if (write)
                        ram2[9 + e] = (sum2 >> 4) & 0x7fff;

                    if (e == 0)
                    {
                        volmul1 = (sum2 >> 4) & 0x7ffe;
                    }
                    else if (e == 1)
                    {
                        volmul2 = (sum2 >> 4) & 0x7ffe;
                    }
                }
                else
                {
                    int shift = (speed >> 4) & 14;
                    shift |= w2;
                    shift = (10 - shift) & 15;

                    int sum1 = target << 11; // 5
                    if (e != 2 || active)
                        sum1 -= (levelcur << 4); // 6
                    int neg = (sum1 & 0x80000) != 0;
                    int preshift = (speed & 15) << 9;
                    if (!w1)
                        preshift |= 0x2000;
                    if (neg)
                        preshift ^= ~0x3f;
#if 0
                    int preshift = neg ? ~0x3e3f0 : 0;

                    if (neg ^ (speed & 1))
                        preshift |= 0x200;
                    if (neg ^ ((speed & 2) != 0))
                        preshift |= 0x400;
                    if (neg ^ ((speed & 4) != 0))
                        preshift |= 0x800;
                    if (neg ^ ((speed & 8) != 0))
                        preshift |= 0x1000;
                    if ((neg ^ w1) == 0)
                        preshift |= 0x2000;
#endif

                    int shifted = preshift >> shift;
                    int sum2 = shifted;
                    if (e != 2 || active)
                        sum2 += (levelcur << 4) | addlow;

                    int sum2_l = (sum2 >> 4);

                    int sum3 = (target << 11) - (sum2_l << 4);

                    int neg2 = (sum3 & 0x80000) != 0;
                    int xnor = !(neg2 ^ neg);

                    if (write)
                    {
                        if (xnor)
                            ram2[9 + e] = sum2_l & 0x7fff;
                        else
                            ram2[9 + e] = target << 7;
                    }

                    if (e == 0)
                    {
                        volmul1 = sum2_l & 0x7ffe;
                    }
                    else if (e == 1)
                    {
                        if (xnor)
                            volmul2 = sum2_l & 0x7ffe;
                        else
                            volmul2 = target << 7;
                    }
                }
                //if (e == 1 && levelcur)
                //    printf("slot%i s%i t%i %i->%i\n", slot, speed, target << 7, levelcur, ram2[9 + e]);

#if 0
                int update = (type >> 2) & 1;
                switch (type & 3)
                {
                    case 0:
                        update = (pcm.tv_counter & 3) == 0;
                        break;
                    case 1:
                        update = (pcm.tv_counter & 15) == 0;
                        break;
                    case 2:
                        update = (pcm.tv_counter & 63) == 0;
                        break;
                    case 3:
                        update = (pcm.tv_counter & 127) == 0;
                        break;
                }

                shift = (10 - shift) & 15;

                int addlow = 0;
                if (type & 4)
                {
                    if (pcm.tv_counter & 8)
                        addlow |= 1;
                    if (pcm.tv_counter & 4)
                        addlow |= 2;
                    if (pcm.tv_counter & 2)
                        addlow |= 4;
                    if (pcm.tv_counter & 1)
                        addlow |= 8;
                }
                else
                {
                    switch (type & 3)
                    {
                    case 0:
                        if (pcm.tv_counter & 0x20)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x10)
                            addlow |= 2;
                        if (pcm.tv_counter & 8)
                            addlow |= 4;
                        if (pcm.tv_counter & 4)
                            addlow |= 8;
                        break;
                    case 1:
                        if (pcm.tv_counter & 0x80)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x40)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x20)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x10)
                            addlow |= 8;
                        break;
                    case 2:
                        if (pcm.tv_counter & 0x200)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x100)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x80)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x40)
                            addlow |= 8;
                        break;
                    case 3:
                        if (pcm.tv_counter & 0x800)
                            addlow |= 1;
                        if (pcm.tv_counter & 0x400)
                            addlow |= 2;
                        if (pcm.tv_counter & 0x200)
                            addlow |= 4;
                        if (pcm.tv_counter & 0x100)
                            addlow |= 8;
                        break;
                    }
                }

                int cnt = ram2[9 + e] << 4;
                int sum = (target << 11) - ((active || e != 2) ? cnt : 0);
                int sum2 = 0;
                int flag = 0;
                if (type & 8)
                {
                    int neg = (sum >> 19) & 1;
                    int preshift = neg ? 0xfc1c0 : 0;

                    if (neg ^ (speed & 1))
                        preshift |= 0x200;
                    if (neg ^ ((speed & 2) != 0))
                        preshift |= 0x400;
                    if (neg ^ ((speed & 4) != 0))
                        preshift |= 0x800;
                    if (neg ^ ((speed & 8) != 0))
                        preshift |= 0x1000;
                    if ((neg ^ w1) == 0)
                        preshift |= 0x2000;

                    int shifted = (preshift << 1) >> shift;

                    sum2 = ((shifted >> 1) & 0xffff0) + ((active || e != 2) ? cnt : 0);

                    sum2 += (target << 11);

                    flag = (sum2 & 0x80000) == (sum & 0x80000);

                    sum2 >>= 4;
                }
                else
                {
                    sum += addlow;

                    int preshift = sum;

                    int shifted = (preshift << 1) >> shift;

                    sum2 = ((shifted >> 1) & 0xffff0) - sum;

                    int add = (target << 11);

                    sum2 = (sum2 + add) >> 4;
                }

                if (!active || update)
                {
                    int val;
                    if ((type & 8) == 0 || flag)
                        val = sum2;
                    else
                        val = target << 7;

                    ram2[9+e] = val & 0x7fff;
                }
#endif
            }
#else
            for (int e = 0; e < 3; e++)
            {
                int dst = (ram2[3 + e] >> 1) & 0x7f80;
                if (ram2[9 + e] < dst)
                    ram2[9 + e]++;
                else if (ram2[9 + e] > dst)
                    ram2[9 + e]--;
            }
#endif
            int sample = (ram2[6] & 2) == 0 ? ram1[3] : v3;

            int multiv1 = multi(sample, volmul1 >> 8);
            int multiv2 = multi(sample, (volmul1 >> 1) & 127);

            int sample2 = addclip20(multiv1 >> 6, multiv2 >> 13, ((multiv2 >> 12) | (multiv1 >> 5)) & 1);

            int multiv3 = multi(sample2, volmul2 >> 8);
            int multiv4 = multi(sample2, (volmul2 >> 1) & 127);

            int sample3 = addclip20(multiv3 >> 6, multiv4 >> 13, ((multiv4 >> 12) | (multiv3 >> 5)) & 1);

            int pan = active ? ram2[1] : 0;
            int rc = active ? ram2[2] : 0;

            int sampl = multi(sample3, (pan >> 8) & 255);
            int sampr = multi(sample3, (pan >> 0) & 255);

            int rc1 = multi(sample3, (rc >> 8) & 255);
            int rc2 = multi(sample3, (rc >> 0) & 255);

            rc1 = addclip20(rc1 >> 6, rc1 >> 6, (rc1 >> 5) & 1);
            rc2 = addclip20(rc2 >> 6, rc2 >> 6, (rc2 >> 5) & 1);

            pcm.ram1[31][1] = addclip20(pcm.ram1[31][1], sampl >> 6, (sampl >> 5) & 1);
            pcm.ram1[31][3] = addclip20(pcm.ram1[31][3], sampr >> 6, (sampr >> 5) & 1);

            // update key
            ram2[7] &= ~0x20;
            ram2[7] |= key << 5;

            if (!active)
            {
                if (pcm.nfs)
                {
                    ram1[1] = 0;
                    ram1[3] = 0;
                    ram1[5] = 0;
                }

                ram2[8] = 0;
                ram2[9] = 0;
                ram2[10] = 0;
            }
        }

        tt[0] = (int)(pcm.ram1[31][1] << 12) >> 16;
        tt[1] = (int)(pcm.ram1[31][3] << 12) >> 16;

        tt[0] <<= 2;
        if (tt[0] > INT16_MAX)
            tt[0] = INT16_MAX;
        else if (tt[0] < INT16_MIN)
            tt[0] = INT16_MIN;
        tt[1] <<= 2;
        if (tt[1] > INT16_MAX)
            tt[1] = INT16_MAX;
        else if (tt[1] < INT16_MIN)
            tt[1] = INT16_MIN;

        MCU_PostSample(tt);

        pcm.nfs = 1;

        pcm.cycles += (slots + 1) * 25;
    }
}
