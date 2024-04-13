#include <stdio.h>
#include "mcu.h"
#include "submcu.h"
#include "midi.h"
#include <portmidi.h>

static PmStream *midiInStream;
static bool sysExMode = false;

void MIDI_Update()
{
    PmEvent event;
    while (Pm_Read(midiInStream, &event, 1)) {
        uint8_t command = (Pm_MessageStatus(event.message) >> 4);

        SM_PostUART(Pm_MessageStatus(event.message));

        if (sysExMode && Pm_MessageStatus(event.message) == 0xF7) {
            sysExMode = false;
        } else if (sysExMode || Pm_MessageStatus(event.message) == 0xF0) {
            sysExMode = true;
            SM_PostUART(Pm_MessageData1(event.message));
            if (Pm_MessageData1(event.message) == 0xF7) {
                sysExMode = false;
                return;
            }
            SM_PostUART(Pm_MessageData2(event.message));
            if (Pm_MessageData2(event.message) == 0xF7) {
                sysExMode = false;
                return;
            }
            SM_PostUART((((event.message) >> 24) & 0xFF));
            if ((((event.message) >> 24) & 0xFF) == 0xF7) {
                sysExMode = false;
                return;
            }
        } else if (command == 0xC || command == 0xD || Pm_MessageStatus(event.message) == 0xF3) {
            SM_PostUART(Pm_MessageData1(event.message));
        } else if (command == 0x8 || command == 0x9 || command == 0xA || command == 0xB || command == 0xE){
            SM_PostUART(Pm_MessageData1(event.message));
            SM_PostUART(Pm_MessageData2(event.message));
        }
    }
}

int MIDI_Init(void)
{
    Pm_Initialize();

    int in_id = Pm_CreateVirtualInput("Virtual SC55", NULL, NULL);

    Pm_OpenInput(&midiInStream, in_id, NULL, 0, NULL, NULL);
    Pm_SetFilter(midiInStream, PM_FILT_ACTIVE | PM_FILT_CLOCK);

    // Empty the buffer, just in case anything got through
    PmEvent receiveBuffer[1];
    while (Pm_Poll(midiInStream)) {
        Pm_Read(midiInStream, receiveBuffer, 1);
    }

    return 1;
}

void MIDI_Quit()
{
    Pm_Terminate();
}
