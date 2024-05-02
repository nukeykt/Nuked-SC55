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
#include "ringbuffer.h"
#include "math_util.h"

#if __linux__
#include <unistd.h>
#include <limits.h>
#endif

struct fe_emu_instance_t {
    emu_t        emu;
    ringbuffer_t sample_buffer;
    SDL_Thread*  thread;
    bool         running;
};

const size_t FE_MAX_INSTANCES = 16;

struct frontend_t {
    fe_emu_instance_t instances[FE_MAX_INSTANCES];
    size_t instances_in_use = 0;

    int audio_buffer_size = 0;
    int audio_page_size = 0;

    SDL_AudioDeviceID sdl_audio = 0;
};

bool FE_AllocateInstance(frontend_t& container, fe_emu_instance_t** result)
{
    if (container.instances_in_use == FE_MAX_INSTANCES)
    {
        return false;
    }

    fe_emu_instance_t& fe = container.instances[container.instances_in_use];
    memset(&fe, 0, sizeof(fe_emu_instance_t));
    ++container.instances_in_use;

    if (result)
    {
        *result = &fe;
    }

    return true;
}

void FE_SendMIDI(frontend_t& fe, size_t n, uint8_t* first, uint8_t* last)
{
    while (first != last)
    {
        MCU_PostUART(*fe.instances[n].emu.mcu, *first);
        ++first;
    }
}

void FE_BroadcastMIDI(frontend_t& fe, uint8_t* first, uint8_t* last)
{
    for (size_t i = 0; i < fe.instances_in_use; ++i)
    {
        FE_SendMIDI(fe, i, first, last);
    }
}

void FE_RouteMIDI(frontend_t& fe, uint8_t* first, uint8_t* last)
{
    if (*first < 0x80)
    {
        printf("FE_RouteMIDI received data byte %02x\n", *first);
        return;
    }

    const bool is_sysex = *first == 0xF0;
    const uint8_t channel = *first & 0x0F;

    if (is_sysex)
    {
        FE_BroadcastMIDI(fe, first, last);
    }
    else
    {
        FE_SendMIDI(fe, channel % fe.instances_in_use, first, last);
    }
}

void FE_ReceiveSample(void* userdata, int *sample)
{
    fe_emu_instance_t& fe = *(fe_emu_instance_t*)userdata;
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
    RB_Write(fe.sample_buffer, sample[0], sample[1]);
}

void FE_AudioCallback(void* userdata, Uint8* stream, int len)
{
    frontend_t& frontend = *(frontend_t*)userdata;

    const size_t num_samples = len / sizeof(int16_t);
    memset(stream, 0, len);

    size_t renderable_count = num_samples;
    for (size_t i = 0; i < frontend.instances_in_use; ++i)
    {
        renderable_count = min(
            renderable_count,
            RB_ReadableSampleCount(frontend.instances[i].sample_buffer)
        );
    }

    for (size_t i = 0; i < frontend.instances_in_use; ++i)
    {
        RB_ReadMix(frontend.instances[i].sample_buffer, (int16_t*)stream, renderable_count);
    }
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

bool FE_OpenAudio(frontend_t& fe, int deviceIndex, int pageSize, int pageNum)
{
    SDL_AudioSpec spec = {};
    SDL_AudioSpec spec_actual = {};

    fe.audio_page_size = (pageSize/2)*2; // must be even
    fe.audio_buffer_size = fe.audio_page_size*pageNum;

    // TODO: we just assume the first instance has the correct mcu type for
    // all instances, which is PROBABLY correct but maybe we want to do some
    // crazy stuff like running different mcu types concurrently in the future?
    const mcu_t& mcu = *fe.instances[0].emu.mcu;
    
    spec.format = AUDIO_S16SYS;
    spec.freq = (mcu.mcu_mk1 || mcu.mcu_jv880) ? 64000 : 66207;
    spec.channels = 2;
    spec.callback = FE_AudioCallback;
    spec.userdata = &fe;
    spec.samples = fe.audio_page_size / 4;
    
    int num = SDL_GetNumAudioDevices(0);
    if (num == 0)
    {
        printf("No audio output device found.\n");
        return false;
    }
    
    if (deviceIndex < -1 || deviceIndex >= num)
    {
        printf("Out of range audio device index is requested. Default audio output device is selected.\n");
        deviceIndex = -1;
    }
    
    const char* audioDevicename = deviceIndex == -1 ? "Default device" : SDL_GetAudioDeviceName(deviceIndex, 0);
    
    fe.sdl_audio = SDL_OpenAudioDevice(deviceIndex == -1 ? NULL : audioDevicename, 0, &spec, &spec_actual, 0);
    if (!fe.sdl_audio)
    {
        return false;
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

    SDL_PauseAudioDevice(fe.sdl_audio, 0);

    return true;
}

void FE_CloseAudio(void)
{
    SDL_CloseAudio();
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

int SDLCALL FE_RunInstance(void* userdata)
{
    fe_emu_instance_t& instance = *(fe_emu_instance_t*)userdata;

    MCU_WorkThread_Lock(*instance.emu.mcu);
    while (instance.running)
    {
        RB_SetOversamplingEnabled(instance.sample_buffer, instance.emu.pcm->config_reg_3c & 0x40);
        if (RB_IsFull(instance.sample_buffer))
        {
            MCU_WorkThread_Unlock(*instance.emu.mcu);
            while (RB_IsFull(instance.sample_buffer))
            {
                SDL_Delay(1);
            }
            MCU_WorkThread_Lock(*instance.emu.mcu);
        }

        MCU_Step(*instance.emu.mcu);
    }
    MCU_WorkThread_Unlock(*instance.emu.mcu);

    return 0;
}

void FE_Run(frontend_t& fe)
{
    bool working = true;

    for (size_t i = 0; i < fe.instances_in_use; ++i)
    {
        fe.instances[i].running = true;
        fe.instances[i].thread = SDL_CreateThread(FE_RunInstance, "work thread", &fe.instances[i]);
    }

    while (working)
    {
        for (size_t i = 0; i < fe.instances_in_use; ++i)
        {
            if (LCD_QuitRequested(*fe.instances[i].emu.lcd))
                working = false;

            LCD_Update(*fe.instances[i].emu.lcd);
        }

        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            for (size_t i = 0; i < fe.instances_in_use; ++i)
            {
                LCD_HandleEvent(*fe.instances[i].emu.lcd, sdl_event);
            }
        }

        SDL_Delay(15);
    }

    for (size_t i = 0; i < fe.instances_in_use; ++i)
    {
        fe.instances[i].running = false;
        SDL_WaitThread(fe.instances[i].thread, 0);
    }
}

bool FE_Init()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "FATAL ERROR: Failed to initialize the SDL2: %s.\n", SDL_GetError());
        fflush(stderr);
        return false;
    }

    return true;
}

bool FE_CreateInstance(frontend_t& container, const std::string& basePath, int romset)
{
    fe_emu_instance_t* fe = nullptr;

    if (!FE_AllocateInstance(container, &fe))
    {
        fprintf(stderr, "ERROR: Failed to allocate instance.\n");
        return false;
    }

    if (!EMU_Init(fe->emu))
    {
        fprintf(stderr, "ERROR: Failed to init emulator.\n");
        return false;
    }

    EMU_SetSampleCallback(fe->emu, FE_ReceiveSample, fe);

    LCD_LoadBack(*fe->emu.lcd, basePath + "/back.data");

    if (!EMU_LoadRoms(fe->emu, romset, basePath))
    {
        fprintf(stderr, "ERROR: Failed to load roms.\n");
        return false;
    }
    EMU_Reset(fe->emu);

    return true;
}

void FE_DestroyInstance(fe_emu_instance_t& fe)
{
    EMU_Free(fe.emu);
    RB_Free(fe.sample_buffer);
    fe.thread = nullptr;
    fe.running = false;
}

void FE_Quit(frontend_t& container)
{
    for (size_t i = 0; i < container.instances_in_use; ++i)
    {
        FE_DestroyInstance(container.instances[i]);
    }
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
    int instance_count = 1;

    int romset = ROM_SET_MK2;

    frontend_t frontend;

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
            else if (!strncmp(argv[i], "-instances:", 11))
            {
                instance_count = atoi(argv[i] + 11);
                instance_count = clamp<int>(instance_count, 1, FE_MAX_INSTANCES);
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
                printf("  -instances:<instance_count>    Set number of emulator instances.\n");
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
        romset = EMU_DetectRomset(basePath);
        printf("ROM set autodetect: %s\n", EMU_RomsetName(romset));
    }

    if (!FE_Init())
    {
        fprintf(stderr, "FATAL ERROR: Failed to initialize frontend\n");
        return 1;
    }

    for (int i = 0; i < instance_count; ++i)
    {
        if (!FE_CreateInstance(frontend, basePath, romset))
        {
            fprintf(stderr, "FATAL ERROR: Failed to create instance %d\n", i);
            return 1;
        }
    }

    if (!FE_OpenAudio(frontend, audioDeviceIndex, pageSize, pageNum))
    {
        fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
        fflush(stderr);
        return 1;
    }

    for (size_t i = 0; i < frontend.instances_in_use; ++i)
    {
        fe_emu_instance_t& fe = frontend.instances[i];
        if (!RB_Init(fe.sample_buffer, frontend.audio_buffer_size / 2))
        {
            fprintf(stderr, "FATAL ERROR: Failed to allocate ringbuffer %lld\n", i);
            return 1;
        }
    }

    if (!MIDI_Init(frontend, port))
    {
        fprintf(stderr, "ERROR: Failed to initialize the MIDI Input.\nWARNING: Continuing without MIDI Input...\n");
        fflush(stderr);
    }

    if (resetType != ResetType::NONE)
    {
        for (size_t i = 0; i < frontend.instances_in_use; ++i)
        {
            MIDI_Reset(*frontend.instances[i].emu.mcu, resetType);
        }
    }

    FE_Run(frontend);

    FE_Quit(frontend);

    return 0;
}
