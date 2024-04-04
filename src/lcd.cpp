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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "SDL_mutex.h"
#include "lcd.h"
#include "lcd_font.h"
#include "mcu.h"
#include "submcu.h"
#include "utils/files.h"

void LCD::LCD_Enable(uint32_t enable)
{
    lcd_enable = enable;
}

bool LCD::LCD_QuitRequested()
{
    return lcd_quit_requested;
}

void LCD::LCD_Write(uint32_t address, uint8_t data)
{
    if (address == 0)
    {
        if ((data & 0xe0) == 0x20)
        {
            LCD_DL = (data & 0x10) != 0;
            LCD_N = (data & 0x8) != 0;
            LCD_F = (data & 0x4) != 0;
        }
        else if ((data & 0xf8) == 0x8)
        {
            LCD_D = (data & 0x4) != 0;
            LCD_C = (data & 0x2) != 0;
            LCD_B = (data & 0x1) != 0;
        }
        else if ((data & 0xff) == 0x01)
        {
            LCD_DD_RAM = 0;
            LCD_ID = 0;
            memset(LCD_Data, 0x20, sizeof(LCD_Data));
        }
        else if ((data & 0xff) == 0x02)
        {
            LCD_DD_RAM = 0;
        }
        else if ((data & 0xfc) == 0x04)
        {
            LCD_ID = (data & 0x2) != 0;
            LCD_S = (data & 0x2) != 0;
        }
        else if ((data & 0xc0) == 0x40)
        {
            LCD_CG_RAM = (data & 0x3f);
            LCD_RAM_MODE = 0;
        }
        else if ((data & 0x80) == 0x80)
        {
            LCD_DD_RAM = (data & 0x7f);
            LCD_RAM_MODE = 1;
        }
        else
        {
            address += 0;
        }
    }
    else
    {
        if (!LCD_RAM_MODE)
        {
            LCD_CG[LCD_CG_RAM] = data & 0x1f;
            if (LCD_ID)
            {
                LCD_CG_RAM++;
            }
            else
            {
                LCD_CG_RAM--;
            }
            LCD_CG_RAM &= 0x3f;
        }
        else
        {
            if (LCD_N)
            {
                if (LCD_DD_RAM & 0x40)
                {
                    if ((LCD_DD_RAM & 0x3f) < 40)
                        LCD_Data[(LCD_DD_RAM & 0x3f) + 40] = data;
                }
                else
                {
                    if ((LCD_DD_RAM & 0x3f) < 40)
                        LCD_Data[LCD_DD_RAM & 0x3f] = data;
                }
            }
            else
            {
                if (LCD_DD_RAM < 80)
                    LCD_Data[LCD_DD_RAM] = data;
            }
            if (LCD_ID)
            {
                LCD_DD_RAM++;
            }
            else
            {
                LCD_DD_RAM--;
            }
            LCD_DD_RAM &= 0x7f;
        }
    }
    //printf("%i %.2x ", address, data);
    // if (data >= 0x20 && data <= 'z')
    //     printf("%c\n", data);
    //else
    //    printf("\n");
}

const int button_map[][2] =
{
    SDL_SCANCODE_Q, MCU_BUTTON_POWER,
    SDL_SCANCODE_W, MCU_BUTTON_INST_ALL,
    SDL_SCANCODE_E, MCU_BUTTON_INST_MUTE,
    SDL_SCANCODE_R, MCU_BUTTON_PART_L,
    SDL_SCANCODE_T, MCU_BUTTON_PART_R,
    SDL_SCANCODE_Y, MCU_BUTTON_INST_L,
    SDL_SCANCODE_U, MCU_BUTTON_INST_R,
    SDL_SCANCODE_I, MCU_BUTTON_KEY_SHIFT_L,
    SDL_SCANCODE_O, MCU_BUTTON_KEY_SHIFT_R,
    SDL_SCANCODE_P, MCU_BUTTON_LEVEL_L,
    SDL_SCANCODE_LEFTBRACKET, MCU_BUTTON_LEVEL_R,
    SDL_SCANCODE_A, MCU_BUTTON_MIDI_CH_L,
    SDL_SCANCODE_S, MCU_BUTTON_MIDI_CH_R,
    SDL_SCANCODE_D, MCU_BUTTON_PAN_L,
    SDL_SCANCODE_F, MCU_BUTTON_PAN_R,
    SDL_SCANCODE_G, MCU_BUTTON_REVERB_L,
    SDL_SCANCODE_H, MCU_BUTTON_REVERB_R,
    SDL_SCANCODE_J, MCU_BUTTON_CHORUS_L,
    SDL_SCANCODE_K, MCU_BUTTON_CHORUS_R,
    SDL_SCANCODE_LEFT, MCU_BUTTON_PART_L,
    SDL_SCANCODE_RIGHT, MCU_BUTTON_PART_R
};


void LCD::LCD_SetBackPath(const std::string &path)
{
    m_back_path = path;
}

void LCD::LCD_Init(MCU *mcu)
{
    this->mcu = mcu;
    
    FILE *raw;

    raw = Files::utf8_fopen(m_back_path.c_str(), "rb");
    if (!raw)
        return;

    fread(lcd_background, 1, sizeof(lcd_background), raw);
    fclose(raw);
}

const uint32_t lcd_col1 = 0x000000;
const uint32_t lcd_col2 = 0x0050c8;

void LCD::LCD_FontRenderStandard(int32_t x, int32_t y, uint8_t ch)
{
    uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 6;
            int yy = y + j * 6;
            for (int ii = 0; ii < 5; ii++)
            {
                for (int jj = 0; jj < 5; jj++)
                {
                    lcd_buffer[xx+ii][yy+jj] = col;
                }
            }
        }
    }
}

void LCD::LCD_FontRenderLevel(int32_t x, int32_t y, uint8_t ch, uint8_t width)
{
    uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < width; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 11;
            int yy = y + j * 26;
            for (int ii = 0; ii < 9; ii++)
            {
                for (int jj = 0; jj < 24; jj++)
                {
                    lcd_buffer[xx+ii][yy+jj] = col;
                }
            }
        }
    }
}

uint32_t* LCD::LCD_Update(void)
{
    // if (!lcd_init)
    //     return;

    if (!lcd_enable)
    {
        memset(lcd_buffer, 0, sizeof(lcd_buffer));
    }
    else
    {
        memcpy(lcd_buffer, lcd_background, sizeof(lcd_buffer));

        if (0)
        {
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 20; j++)
                {
                    uint8_t ch = LCD_Data[i * 20 + j];
                    LCD_FontRenderStandard(i * 50, j * 34, ch);
                }
            }
        }
        else
        {
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[0 + i];
                LCD_FontRenderStandard(11, 34 + i * 35, ch);
            }
            for (int i = 0; i < 16; i++)
            {
                uint8_t ch = LCD_Data[3 + i];
                LCD_FontRenderStandard(11, 153 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[40 + i];
                LCD_FontRenderStandard(75, 34 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[43 + i];
                LCD_FontRenderStandard(75, 153 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[49 + i];
                LCD_FontRenderStandard(139, 34 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[46 + i];
                LCD_FontRenderStandard(139, 153 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[52 + i];
                LCD_FontRenderStandard(203, 34 + i * 35, ch);
            }
            for (int i = 0; i < 3; i++)
            {
                uint8_t ch = LCD_Data[55 + i];
                LCD_FontRenderStandard(203, 153 + i * 35, ch);
            }

            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    uint8_t ch = LCD_Data[20 + j + i * 40];
                    LCD_FontRenderLevel(71 + i * 88, 293 + j * 130, ch, j == 3 ? 1 : 5);
                }
            }
        }
    }

    return (uint32_t*)lcd_buffer;
}

void LCD::LCD_SendButton(uint8_t button, int state) {
    uint32_t button_pressed = (uint32_t)SDL_AtomicGet(&mcu->mcu_button_pressed);
    int mask = (1 << button);
    if (state) {
        button_pressed |= mask;
    } else {
        button_pressed &= ~mask;
    }
    SDL_AtomicSet(&mcu->mcu_button_pressed, (int)button_pressed);
}

