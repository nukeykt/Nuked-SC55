#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "lcd.h"
#include "lcd_font.h"
#include "mcu.h"
#include "submcu.h"

uint32_t LCD_DL, LCD_N, LCD_F, LCD_D, LCD_C, LCD_B, LCD_ID, LCD_S;
uint32_t LCD_DD_RAM, LCD_AC, LCD_CG_RAM;
uint32_t LCD_RAM_MODE = 0;
uint8_t LCD_Data[80];
uint8_t LCD_CG[64];

uint8_t lcd_enable;

void LCD_Enable(uint32_t enable)
{
    lcd_enable = enable;
}

void LCD_Write(uint32_t address, uint8_t data)
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

const int lcd_width = 741;
const int lcd_height = 268;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

uint32_t lcd_buffer[lcd_height][lcd_width];
uint32_t lcd_background[lcd_height][lcd_width];

uint32_t lcd_init;

int button_map[][2] = {
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
};


void LCD_Init(void)
{
    FILE *raw;

    window = SDL_CreateWindow("SC-55mkII", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, lcd_width, lcd_height, SDL_WINDOW_SHOWN);
    if (!window)
        return;

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        return;

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, lcd_width, lcd_height);

    if (!texture)
        return;

    raw = fopen("back.data", "rb");
    if (!raw)
        return;

    fread(lcd_background, 1, sizeof(lcd_background), raw);
    fclose(raw);

    lcd_init = 1;
}

uint32_t lcd_col1 = 0x000000;
uint32_t lcd_col2 = 0x0050c8;

void LCD_FontRenderStandard(int32_t x, int32_t y, uint8_t ch)
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

void LCD_FontRenderLevel(int32_t x, int32_t y, uint8_t ch, uint8_t width = 5)
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

void LCD_Update(void)
{
    if (!lcd_init)
        return;

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

    SDL_UpdateTexture(texture, NULL, lcd_buffer, lcd_width * 4);

    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);

    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                if (sdl_event.key.repeat)
                    continue;
                int mask = 0;
                for (int i = 0; i < sizeof(button_map) / sizeof(button_map[0]); i++)
                {
                    if (button_map[i][0] == sdl_event.key.keysym.scancode)
                        mask |= 1<<button_map[i][1];
                }
                if (sdl_event.type == SDL_KEYDOWN)
                    mcu_button_pressed |= mask;
                else
                    mcu_button_pressed &= ~mask;

                if (sdl_event.key.keysym.scancode >= SDL_SCANCODE_1 && sdl_event.key.keysym.scancode < SDL_SCANCODE_0)
                {
#if 0
                    int kk = sdl_event.key.keysym.scancode - SDL_SCANCODE_1;
                    if (sdl_event.type == SDL_KEYDOWN)
                    {
                        SM_PostUART(0xc0);
                        SM_PostUART(118);
                        SM_PostUART(0x90);
                        SM_PostUART(0x30 + kk);
                        SM_PostUART(0x7f);
                    }
                    else
                    {
                        SM_PostUART(0x90);
                        SM_PostUART(0x30 + kk);
                        SM_PostUART(0);
                    }
#endif
                    int kk = sdl_event.key.keysym.scancode - SDL_SCANCODE_1;
                    const int patch = 47;
                    if (sdl_event.type == SDL_KEYDOWN)
                    {
                        static int bend = 0x2000;
                        if (kk == 4)
                        {
                            SM_PostUART(0x99);
                            SM_PostUART(0x32);
                            SM_PostUART(0x7f);
                        }
                        else if (kk == 3)
                        {
                            bend += 0x100;
                            if (bend > 0x3fff)
                                bend = 0x3fff;
                            SM_PostUART(0xe1);
                            SM_PostUART(bend & 127);
                            SM_PostUART((bend >> 7) & 127);
                        }
                        else if (kk == 2)
                        {
                            bend -= 0x100;
                            if (bend < 0)
                                bend = 0;
                            SM_PostUART(0xe1);
                            SM_PostUART(bend & 127);
                            SM_PostUART((bend >> 7) & 127);
                        }
                        else if (kk)
                        {
                            SM_PostUART(0xc1);
                            SM_PostUART(patch);
                            SM_PostUART(0xe1);
                            SM_PostUART(bend & 127);
                            SM_PostUART((bend >> 7) & 127);
                            SM_PostUART(0x91);
                            SM_PostUART(0x32);
                            SM_PostUART(0x7f);
                        }
                        else if (kk == 0)
                        {
                            SM_PostUART(0xc0);
                            SM_PostUART(patch);
                            SM_PostUART(0xe0);
                            SM_PostUART(0x00);
                            SM_PostUART(0x40);
                            SM_PostUART(0x90);
                            SM_PostUART(0x34);
                            SM_PostUART(0x7f);
                        }
                    }
                    else
                    {
                        if (kk == 1)
                        {
                            SM_PostUART(0x91);
                            SM_PostUART(0x32);
                            SM_PostUART(0);
                        }
                        else if (kk == 0)
                        {
                            SM_PostUART(0x90);
                            SM_PostUART(0x34);
                            SM_PostUART(0);
                        }
                        else if (kk == 4)
                        {
                            SM_PostUART(0x99);
                            SM_PostUART(0x32);
                            SM_PostUART(0);
                        }
                    }
                }
                break;
            }
        }
    }
}

