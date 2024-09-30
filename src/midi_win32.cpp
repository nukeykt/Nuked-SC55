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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include "mcu.h"
#include "midi.h"

static HMIDIIN midi_handle;
static MIDIHDR midi_buffer;

static char midi_in_buffer[1024];

void CALLBACK MIDI_Callback(
    HMIDIIN   hMidiIn,
    UINT      wMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    switch (wMsg)
    {
        case MIM_OPEN:
            break;
        case MIM_DATA:
        {
            int b1 = dwParam1 & 0xff;
            switch (b1 & 0xf0)
            {
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xe0:
                    MCU_PostUART(b1);
                    MCU_PostUART((dwParam1 >> 8) & 0xff);
                    MCU_PostUART((dwParam1 >> 16) & 0xff);
                    break;
                case 0xc0:
                case 0xd0:
                    MCU_PostUART(b1);
                    MCU_PostUART((dwParam1 >> 8) & 0xff);
                    break;
            }
            break;
        }
        case MIM_LONGDATA:
        case MIM_LONGERROR:
        {
            midiInUnprepareHeader(midi_handle, &midi_buffer, sizeof(MIDIHDR));

            if (wMsg == MIM_LONGDATA)
            {
                for (int i = 0; i < midi_buffer.dwBytesRecorded; i++)
                {
                    MCU_PostUART(midi_in_buffer[i]);
                }
            }

            midiInPrepareHeader(midi_handle, &midi_buffer, sizeof(MIDIHDR));
            midiInAddBuffer(midi_handle, &midi_buffer, sizeof(MIDIHDR));

            break;
        }
        default:
            printf("hmm");
            break;
    }
}

int MIDI_Init(int port, bool listMidi)
{
    int num = midiInGetNumDevs();

    if (num == 0)
    {
        printf("No midi input\n");
        return 0;
    }

    if (listMidi) {              
        for (int i = 0; i < num; i++) {
            MIDIINCAPSA caps;
            midiInGetDevCapsA(i, &caps, sizeof(MIDIINCAPSA));
            printf("Port[%d]: %s\n", i, caps.szPname);
        }
    }

    if (port < 0 || port >= num)
    {
        printf("Out of range midi port is requested. Defaulting to port 0\n");
        port = 0;
    }

    MIDIINCAPSA caps;

    midiInGetDevCapsA(port, &caps, sizeof(MIDIINCAPSA));

    auto res = midiInOpen(&midi_handle, port, (DWORD_PTR)MIDI_Callback, 0, CALLBACK_FUNCTION);

    if (res != MMSYSERR_NOERROR)
    {
        printf("Can't open midi input\n");
        return 0;
    }

    printf("Opened midi port: %s\n", caps.szPname);

    midi_buffer.lpData = midi_in_buffer;
    midi_buffer.dwBufferLength = sizeof(midi_in_buffer);

    auto r1 = midiInPrepareHeader(midi_handle, &midi_buffer, sizeof(MIDIHDR));
    auto r2 = midiInAddBuffer(midi_handle, &midi_buffer, sizeof(MIDIHDR));
    auto r3 = midiInStart(midi_handle);

    return 1;
}

void MIDI_Quit()
{
    if (midi_handle)
    {
        midiInStop(midi_handle);
        midiInClose(midi_handle);
        midi_handle = 0;
    }
}
