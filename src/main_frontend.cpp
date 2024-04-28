/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_timer.h"
#include "pcm.h"
#include "lcd.h"
#include "submcu.h"
#include "midi.h"
#include "utf8main.h"
#include "utils/files.h"
#include "emu.h"

#if __linux__
#include <unistd.h>
#include <limits.h>
#endif

const char* rs_name[ROM_SET_COUNT] = {
    "SC-55mk2",
    "SC-55st",
    "SC-55mk1",
    "CM-300/SCC-1",
    "JV-880",
    "SCB-55",
    "RLP-3237",
    "SC-155",
    "SC-155mk2"
};

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

static int audio_buffer_size;
static int audio_page_size;
static short *sample_buffer;

static int sample_read_ptr;
static int sample_write_ptr;

static SDL_AudioDeviceID sdl_audio;

void FE_StepBegin(void* userdata)
{
    mcu_t& mcu = *(mcu_t*)userdata;
    if (mcu.pcm->config_reg_3c & 0x40)
        sample_write_ptr &= ~3;
    else
        sample_write_ptr &= ~1;
}

bool FE_Wait(void* userdata)
{
    return sample_read_ptr == sample_write_ptr;
}

void FE_ReceiveSample(void* userdata, int *sample)
{
    sample[0] >>= 15;
    if (sample[0] > INT16_MAX)
        sample[0] = INT16_MAX;
    else if (sample[0] < INT16_MIN)
        sample[0] = INT16_MIN;
    sample[1] >>= 15;
    if (sample[1] > INT16_MAX)
        sample[1] = INT16_MAX;
    else if (sample[1] < INT16_MIN)
        sample[1] = INT16_MIN;
    sample_buffer[sample_write_ptr + 0] = sample[0];
    sample_buffer[sample_write_ptr + 1] = sample[1];
    sample_write_ptr = (sample_write_ptr + 2) % audio_buffer_size;
}

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

void audio_callback(void* /*userdata*/, Uint8* stream, int len)
{
    len /= 2;
    memcpy(stream, &sample_buffer[sample_read_ptr], len * 2);
    memset(&sample_buffer[sample_read_ptr], 0, len * 2);
    sample_read_ptr += len;
    sample_read_ptr %= audio_buffer_size;
}

static const char* audio_format_to_str(int format)
{
    switch(format)
    {
    case AUDIO_S8:
        return "S8";
    case AUDIO_U8:
        return "U8";
    case AUDIO_S16MSB:
        return "S16MSB";
    case AUDIO_S16LSB:
        return "S16LSB";
    case AUDIO_U16MSB:
        return "U16MSB";
    case AUDIO_U16LSB:
        return "U16LSB";
    case AUDIO_S32MSB:
        return "S32MSB";
    case AUDIO_S32LSB:
        return "S32LSB";
    case AUDIO_F32MSB:
        return "F32MSB";
    case AUDIO_F32LSB:
        return "F32LSB";
    }
    return "UNK";
}

int FE_OpenAudio(mcu_t& mcu, int deviceIndex, int pageSize, int pageNum)
{
    SDL_AudioSpec spec = {};
    SDL_AudioSpec spec_actual = {};

    audio_page_size = (pageSize/2)*2; // must be even
    audio_buffer_size = audio_page_size*pageNum;
    
    spec.format = AUDIO_S16SYS;
    spec.freq = (mcu.mcu_mk1 || mcu.mcu_jv880) ? 64000 : 66207;
    spec.channels = 2;
    spec.callback = audio_callback;
    spec.samples = audio_page_size / 4;
    
    sample_buffer = (short*)calloc(audio_buffer_size, sizeof(short));
    if (!sample_buffer)
    {
        printf("Cannot allocate audio buffer.\n");
        return 0;
    }
    sample_read_ptr = 0;
    sample_write_ptr = 0;
    
    int num = SDL_GetNumAudioDevices(0);
    if (num == 0)
    {
        printf("No audio output device found.\n");
        return 0;
    }
    
    if (deviceIndex < -1 || deviceIndex >= num)
    {
        printf("Out of range audio device index is requested. Default audio output device is selected.\n");
        deviceIndex = -1;
    }
    
    const char* audioDevicename = deviceIndex == -1 ? "Default device" : SDL_GetAudioDeviceName(deviceIndex, 0);
    
    sdl_audio = SDL_OpenAudioDevice(deviceIndex == -1 ? NULL : audioDevicename, 0, &spec, &spec_actual, 0);
    if (!sdl_audio)
    {
        return 0;
    }

    printf("Audio device: %s\n", audioDevicename);

    printf("Audio Requested: F=%s, C=%d, R=%d, B=%d\n",
           audio_format_to_str(spec.format),
           spec.channels,
           spec.freq,
           spec.samples);

    printf("Audio Actual: F=%s, C=%d, R=%d, B=%d\n",
           audio_format_to_str(spec_actual.format),
           spec_actual.channels,
           spec_actual.freq,
           spec_actual.samples);
    fflush(stdout);

    SDL_PauseAudioDevice(sdl_audio, 0);

    return 1;
}

void FE_CloseAudio(void)
{
    SDL_CloseAudio();
    if (sample_buffer) free(sample_buffer);
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

enum class ResetType {
    NONE,
    GS_RESET,
    GM_RESET,
};

void MIDI_Reset(mcu_t& mcu, ResetType resetType)
{
    const unsigned char gmReset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
    const unsigned char gsReset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
    
    if (resetType == ResetType::GS_RESET)
    {
        for (size_t i = 0; i < sizeof(gsReset); i++)
        {
            MCU_PostUART(mcu, gsReset[i]);
        }
    }
    else  if (resetType == ResetType::GM_RESET)
    {
        for (size_t i = 0; i < sizeof(gmReset); i++)
        {
            MCU_PostUART(mcu, gmReset[i]);
        }
    }

}

static bool work_thread_run = false;

int SDLCALL FE_RunMCU(void* data)
{
    mcu_t& mcu = *(mcu_t*)data;

    MCU_WorkThread_Lock(mcu);
    while (work_thread_run)
    {
        MCU_Step(mcu);
    }
    MCU_WorkThread_Unlock(mcu);

    return 0;
}

void FE_Run(mcu_t& mcu)
{
    bool working = true;

    work_thread_run = true;
    SDL_Thread *thread = SDL_CreateThread(FE_RunMCU, "work thread", &mcu);

    while (working)
    {
        if (LCD_QuitRequested(*mcu.lcd))
            working = false;

        LCD_Update(*mcu.lcd);

        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            LCD_HandleEvent(*mcu.lcd, sdl_event);
        }

        SDL_Delay(15);
    }

    work_thread_run = false;
    SDL_WaitThread(thread, 0);
}

int FE_Init()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "FATAL ERROR: Failed to initialize the SDL2: %s.\n", SDL_GetError());
        fflush(stderr);
        return 2;
    }

    return 0;
}

void FE_Quit()
{
    FE_CloseAudio();
    MIDI_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    (void)argc;
    std::string basePath;

    int port = 0;
    int audioDeviceIndex = -1;
    int pageSize = 512;
    int pageNum = 32;
    bool autodetect = true;
    ResetType resetType = ResetType::NONE;

    int romset = ROM_SET_MK2;

    {
        for (int i = 1; i < argc; i++)
        {
            if (!strncmp(argv[i], "-p:", 3))
            {
                port = atoi(argv[i] + 3);
            }
            else if (!strncmp(argv[i], "-a:", 3))
            {
                audioDeviceIndex = atoi(argv[i] + 3);
            }
            else if (!strncmp(argv[i], "-ab:", 4))
            {
                char* pColon = argv[i] + 3;
                
                if (pColon[1] != 0)
                {
                    pageSize = atoi(++pColon);
                    pColon = strchr(pColon, ':');
                    if (pColon && pColon[1] != 0)
                    {
                        pageNum = atoi(++pColon);
                    }
                }
                
                // reset both if either is invalid
                if (pageSize <= 0 || pageNum <= 0)
                {
                    pageSize = 512;
                    pageNum = 32;
                }
            }
            else if (!strcmp(argv[i], "-mk2"))
            {
                romset = ROM_SET_MK2;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-st"))
            {
                romset = ROM_SET_ST;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-mk1"))
            {
                romset = ROM_SET_MK1;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-cm300"))
            {
                romset = ROM_SET_CM300;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-jv880"))
            {
                romset = ROM_SET_JV880;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-scb55"))
            {
                romset = ROM_SET_SCB55;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-rlp3237"))
            {
                romset = ROM_SET_RLP3237;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-gs"))
            {
                resetType = ResetType::GS_RESET;
            }
            else if (!strcmp(argv[i], "-gm"))
            {
                resetType = ResetType::GM_RESET;
            }
            else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
            {
                // TODO: Might want to try to find a way to print out the executable's actual name (without any full paths).
                printf("Usage: nuked-sc55 [options]\n");
                printf("Options:\n");
                printf("  -h, -help, --help              Display this information.\n");
                printf("\n");
                printf("  -p:<port_number>               Set MIDI port.\n");
                printf("  -a:<device_number>             Set Audio Device index.\n");
                printf("  -ab:<page_size>:[page_count]   Set Audio Buffer size.\n");
                printf("\n");
                printf("  -mk2                           Use SC-55mk2 ROM set.\n");
                printf("  -st                            Use SC-55st ROM set.\n");
                printf("  -mk1                           Use SC-55mk1 ROM set.\n");
                printf("  -cm300                         Use CM-300/SCC-1 ROM set.\n");
                printf("  -jv880                         Use JV-880 ROM set.\n");
                printf("  -scb55                         Use SCB-55 ROM set.\n");
                printf("  -rlp3237                       Use RLP-3237 ROM set.\n");
                printf("\n");
                printf("  -gs                            Reset system in GS mode.\n");
                printf("  -gm                            Reset system in GM mode.\n");
                return 0;
            }
            else if (!strcmp(argv[i], "-sc155"))
            {
                romset = ROM_SET_SC155;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-sc155mk2"))
            {
                romset = ROM_SET_SC155MK2;
                autodetect = false;
            }
        }
    }

#if __linux__
    char self_path[PATH_MAX];
    memset(&self_path[0], 0, PATH_MAX);

    if(readlink("/proc/self/exe", self_path, PATH_MAX) == -1)
        basePath = Files::real_dirname(argv[0]);
    else
        basePath = Files::dirname(self_path);
#else
    basePath = Files::real_dirname(argv[0]);
#endif

    printf("Base path is: %s\n", argv[0]);

    if(Files::dirExists(basePath + "/../share/nuked-sc55"))
        basePath += "/../share/nuked-sc55";

    if (autodetect)
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
                romset = i;
                break;
            }
        }
        printf("ROM set autodetect: %s\n", rs_name[romset]);
    }

    if (FE_Init() != 0)
    {
        return 2;
    }

    emu_backend_t emu;
    EMU_Init(emu);
    // TODO: Clean up, shouldn't need to reach into emu for this
    emu.mcu->callback_userdata = emu.mcu;
    emu.mcu->step_begin_callback = FE_StepBegin;
    emu.mcu->sample_callback = FE_ReceiveSample;
    emu.mcu->wait_callback = FE_Wait;

    LCD_LoadBack(*emu.lcd, basePath + "/back.data");

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

    EMU_Reset(emu);

    if (!FE_OpenAudio(*emu.mcu, audioDeviceIndex, pageSize, pageNum))
    {
        fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
        fflush(stderr);
        return 2;
    }

    if(!MIDI_Init(*emu.mcu, port))
    {
        fprintf(stderr, "ERROR: Failed to initialize the MIDI Input.\nWARNING: Continuing without MIDI Input...\n");
        fflush(stderr);
    }

    if (resetType != ResetType::NONE) MIDI_Reset(*emu.mcu, resetType);
    
    FE_Run(*emu.mcu);

    EMU_Free(emu);

    FE_Quit();

    return 0;
}
