#include "emu.h"
#include "mcu.h"
#include "submcu.h"
#include "mcu_timer.h"
#include "lcd.h"
#include "pcm.h"

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
