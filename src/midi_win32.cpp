/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 * This file is part of Nuked-SC55.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
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

int MIDI_Init(int port)
{
    int num = midiInGetNumDevs();

    if (num == 0)
    {
        printf("No midi input\n");
        return 0;
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
