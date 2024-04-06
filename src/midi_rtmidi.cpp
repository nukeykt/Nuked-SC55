#include <stdio.h>
#include "mcu.h"
#include "midi.h"
#include <RtMidi.h>

static RtMidiIn *s_midi_in = nullptr;


static void MidiOnReceive(double, std::vector<uint8_t> *message, void *)
{
    uint8_t *beg = message->data();
    uint8_t *end = message->data() + message->size();

    while(beg < end)
        MCU_PostUART(*beg++);
}

static void MidiOnError(RtMidiError::Type, const std::string &errorText, void *)
{
    fprintf(stderr, "RtMidi: Error has occured: %s\n", errorText.c_str());
    fflush(stderr);
}

int MIDI_Init(int port)
{
    if (s_midi_in)
    {
        printf("MIDI already running\n");
        return 0; // Already running
    }

    s_midi_in = new RtMidiIn(RtMidi::UNSPECIFIED, "Nuked SC55", 1024);
    s_midi_in->ignoreTypes(false, false, false); // SysEx disabled by default
    s_midi_in->setCallback(&MidiOnReceive, nullptr); // FIXME: (local bug) Fix the linking error
    s_midi_in->setErrorCallback(&MidiOnError, nullptr);

    unsigned count = s_midi_in->getPortCount();

    if (count == 0)
    {
        printf("No midi input\n");
        delete s_midi_in;
        s_midi_in = nullptr;
        return 0;
    }

    if (port < 0 || port >= count)
    {
        printf("Out of range midi port is requested. Defaulting to port 0\n");
        port = 0;
    }

    s_midi_in->openPort(port, "Nuked SC55");

    return 1;
}

void MIDI_Quit()
{
    if (s_midi_in)
    {
        s_midi_in->closePort();
        delete s_midi_in;
        s_midi_in = nullptr;
    }
}
