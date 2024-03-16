#include <stdio.h>
#include "mcu.h"
#include "submcu.h"
#include "midi.h"
#include <RtMidi.h>

static RtMidiIn *s_midi_in = nullptr;
static char midi_in_buffer[1024];

static void MidiOnReceive(double timeStamp, std::vector<uint8_t> *message, void *userData)
{

}

static void MidiOnError(RtMidiError::Type type, const std::string &errorText, void *userData)
{
    fprintf(stderr, "RtMidi: Error has occured: %s\n", errorText.c_str());
    fflush(stderr);
}

int MIDI_Init(void)
{
    if (s_midi_in)
    {
        printf("MIDI already running\n");
        return 0; // Already running
    }

    s_midi_in = new RtMidiIn(RtMidi::LINUX_ALSA, "Virtual SC55", 1024);
    // s_midi_in->setCallback(&MidiOnReceive, nullptr); // FIXME: (local bug) Fix the linking error
    s_midi_in->setErrorCallback(&MidiOnError, nullptr);

    unsigned count = s_midi_in->getPortCount();

    if (count == 0)
    {
        printf("No midi input\n");
        delete s_midi_in;
        s_midi_in = nullptr;
        return 0;
    }

    s_midi_in->openPort(0, "Virtual SC55");

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
