#include "emu.h"
#include "mcu.h"
#include "submcu.h"
#include "mcu_timer.h"
#include "lcd.h"
#include "pcm.h"
#include "utils/files.h"

void EMU_Init(emu_backend_t& emu)
{
    emu.mcu   = (mcu_t*)malloc(sizeof(mcu_t));
    emu.sm    = (submcu_t*)malloc(sizeof(submcu_t));
    emu.timer = (mcu_timer_t*)malloc(sizeof(mcu_timer_t));
    emu.lcd   = (lcd_t*)malloc(sizeof(lcd_t));
    emu.pcm   = (pcm_t*)malloc(sizeof(pcm_t));

    PCM_Reset(*emu.pcm, *emu.mcu);
    MCU_Init(*emu.mcu, *emu.sm, *emu.pcm, *emu.timer, *emu.lcd);
    TIMER_Init(*emu.timer, *emu.mcu);

    // TODO: LCD should be optional for use with other frontends
    LCD_Init(*emu.lcd, *emu.mcu);
}

void EMU_Free(emu_backend_t& emu)
{
    LCD_UnInit(*emu.lcd);
    free(emu.pcm);
    free(emu.lcd);
    free(emu.timer);
    free(emu.sm);
    free(emu.mcu);
}

// Should be called after loading roms
void EMU_Reset(emu_backend_t& emu)
{
    MCU_PatchROM(*emu.mcu);
    MCU_Reset(*emu.mcu);
    SM_Reset(*emu.sm, *emu.mcu);
}

const char* roms[ROM_SET_COUNT][5] =
{
    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",

    "rom1.bin",
    "rom2_st.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",

    "sc55_rom1.bin",
    "sc55_rom2.bin",
    "sc55_waverom1.bin",
    "sc55_waverom2.bin",
    "sc55_waverom3.bin",

    "cm300_rom1.bin",
    "cm300_rom2.bin",
    "cm300_waverom1.bin",
    "cm300_waverom2.bin",
    "cm300_waverom3.bin",

    "jv880_rom1.bin",
    "jv880_rom2.bin",
    "jv880_waverom1.bin",
    "jv880_waverom2.bin",
    "jv880_waverom_expansion.bin",

    "scb55_rom1.bin",
    "scb55_rom2.bin",
    "scb55_waverom1.bin",
    "scb55_waverom2.bin",
    "",

    "rlp3237_rom1.bin",
    "rlp3237_rom2.bin",
    "rlp3237_waverom1.bin",
    "",
    "",

    "sc155_rom1.bin",
    "sc155_rom2.bin",
    "sc155_waverom1.bin",
    "sc155_waverom2.bin",
    "sc155_waverom3.bin",

    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",
};

uint8_t tempbuf[0x800000];

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

static const size_t rf_num = 5;
static FILE *s_rf[rf_num] =
{
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static void closeAllR()
{
    for(size_t i = 0; i < rf_num; ++i)
    {
        if(s_rf[i])
            fclose(s_rf[i]);
        s_rf[i] = nullptr;
    }
}

int EMU_DetectRoms(const std::string& basePath)
{
    for (size_t i = 0; i < ROM_SET_COUNT; i++)
    {
        bool good = true;
        for (size_t j = 0; j < 5; j++)
        {
            if (roms[i][j][0] == '\0')
                continue;
            std::string path = basePath + "/" + roms[i][j];
            auto h = Files::utf8_fopen(path.c_str(), "rb");
            if (!h)
            {
                good = false;
                break;
            }
            fclose(h);
        }
        if (good)
        {
            return i;
        }
    }
    return 0;
}

int EMU_LoadRoms(emu_backend_t& emu, int romset, const std::string& basePath)
{
    emu.mcu->romset = romset;
    emu.mcu->mcu_mk1 = false;
    emu.mcu->mcu_cm300 = false;
    emu.mcu->mcu_st = false;
    emu.mcu->mcu_jv880 = false;
    emu.mcu->mcu_scb55 = false;
    emu.mcu->mcu_sc155 = false;
    switch (romset)
    {
        case ROM_SET_MK2:
        case ROM_SET_SC155MK2:
            if (romset == ROM_SET_SC155MK2)
                emu.mcu->mcu_sc155 = true;
            break;
        case ROM_SET_ST:
            emu.mcu->mcu_st = true;
            break;
        case ROM_SET_MK1:
        case ROM_SET_SC155:
            emu.mcu->mcu_mk1 = true;
            emu.mcu->mcu_st = false;
            if (romset == ROM_SET_SC155)
                emu.mcu->mcu_sc155 = true;
            break;
        case ROM_SET_CM300:
            emu.mcu->mcu_mk1 = true;
            emu.mcu->mcu_cm300 = true;
            break;
        case ROM_SET_JV880:
            emu.mcu->mcu_jv880 = true;
            emu.mcu->rom2_mask /= 2; // rom is half the size
            break;
        case ROM_SET_SCB55:
        case ROM_SET_RLP3237:
            emu.mcu->mcu_scb55 = true;
            break;
    }

    std::string rpaths[5];

    bool r_ok = true;
    std::string errors_list;

    for(size_t i = 0; i < 5; ++i)
    {
        if (roms[romset][i][0] == '\0')
        {
            rpaths[i] = "";
            continue;
        }
        rpaths[i] = basePath + "/" + roms[romset][i];
        s_rf[i] = Files::utf8_fopen(rpaths[i].c_str(), "rb");
        bool optional = emu.mcu->mcu_jv880 && i == 4;
        r_ok &= optional || (s_rf[i] != nullptr);
        if(!s_rf[i])
        {
            if(!errors_list.empty())
                errors_list.append(", ");

            errors_list.append(rpaths[i]);
        }
    }

    if (!r_ok)
    {
        fprintf(stderr, "FATAL ERROR: One of required data ROM files is missing: %s.\n", errors_list.c_str());
        fflush(stderr);
        closeAllR();
        return 1;
    }

    if (fread(emu.mcu->rom1, 1, ROM1_SIZE, s_rf[0]) != ROM1_SIZE)
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM1.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    size_t rom2_read = fread(emu.mcu->rom2, 1, ROM2_SIZE, s_rf[1]);

    if (rom2_read == ROM2_SIZE || rom2_read == ROM2_SIZE / 2)
    {
        emu.mcu->rom2_mask = rom2_read - 1;
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM2.\n");
        fflush(stderr);
        closeAllR();
        return 1;
    }

    if (emu.mcu->mcu_mk1)
    {
        if (fread(tempbuf, 1, 0x100000, s_rf[2]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom1, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom2, 0x100000);

        if (fread(tempbuf, 1, 0x100000, s_rf[4]) != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom3.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom3, 0x100000);
    }
    else if (emu.mcu->mcu_jv880)
    {
        if (fread(tempbuf, 1, 0x200000, s_rf[2]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom1, 0x200000);

        if (fread(tempbuf, 1, 0x200000, s_rf[3]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom2, 0x200000);

        if (s_rf[4] && fread(tempbuf, 1, 0x800000, s_rf[4]))
            unscramble(tempbuf, emu.pcm->waverom_exp, 0x800000);
        else
            printf("WaveRom EXP not found, skipping it.\n");
    }
    else
    {
        if (fread(tempbuf, 1, 0x200000, s_rf[2]) != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }

        unscramble(tempbuf, emu.pcm->waverom1, 0x200000);

        if (s_rf[3])
        {
            if (fread(tempbuf, 1, 0x100000, s_rf[3]) != 0x100000)
            {
                fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
                fflush(stderr);
                closeAllR();
                return 1;
            }

            unscramble(tempbuf, emu.mcu->mcu_scb55 ? emu.pcm->waverom3 : emu.pcm->waverom2, 0x100000);
        }

        if (s_rf[4] && fread(emu.sm->sm_rom, 1, ROMSM_SIZE, s_rf[4]) != ROMSM_SIZE)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the sub mcu ROM.\n");
            fflush(stderr);
            closeAllR();
            return 1;
        }
    }

    // Close all files as they no longer needed being open
    closeAllR();

    return 0;
}
