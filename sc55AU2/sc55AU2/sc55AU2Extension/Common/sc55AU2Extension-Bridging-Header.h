//
//  sc55AU2Extension-Bridging-Header.h
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#import "sc55AU2ExtensionAudioUnit.h"
#import "sc55AU2ExtensionParameterAddresses.h"

// void SC55_Reset();
// uint32_t* LCD_Update(void);
// void LCD_SendButton(uint8_t button, int state);

enum {
    MCU_BUTTON_POWER = 0,
    MCU_BUTTON_INST_L = 3,
    MCU_BUTTON_INST_R = 4,
    MCU_BUTTON_INST_MUTE = 5,
    MCU_BUTTON_INST_ALL = 6,
    MCU_BUTTON_MIDI_CH_L = 7,
    MCU_BUTTON_MIDI_CH_R = 8,
    MCU_BUTTON_CHORUS_L = 9,
    MCU_BUTTON_CHORUS_R = 10,
    MCU_BUTTON_PAN_L = 11,
    MCU_BUTTON_PAN_R = 12,
    MCU_BUTTON_PART_R = 13,
    MCU_BUTTON_KEY_SHIFT_L = 14,
    MCU_BUTTON_KEY_SHIFT_R = 15,
    MCU_BUTTON_REVERB_L = 16,
    MCU_BUTTON_REVERB_R = 17,
    MCU_BUTTON_LEVEL_L = 18,
    MCU_BUTTON_LEVEL_R = 19,
    MCU_BUTTON_PART_L = 20
};
