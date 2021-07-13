#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "lcd.h"
#include "lcd_font.h"

uint32_t LCD_DL, LCD_N, LCD_F, LCD_D, LCD_C, LCD_B, LCD_ID, LCD_S;
uint32_t LCD_DD_RAM, LCD_AC, LCD_CG_RAM;
uint32_t LCD_RAM_MODE = 0;
uint8_t LCD_Data[80];
uint8_t LCD_CG[64];

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

void LCD_Init(void)
{
    FILE *raw;

    window = SDL_CreateWindow("mcuemu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, lcd_width, lcd_height, SDL_WINDOW_SHOWN);
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

    raw = fopen("fonts.data", "rb");
    if (!raw)
        return;

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
            uint8_t ch = LCD_Data[0 + i];
            LCD_FontRenderStandard(75, 34 + i * 35, ch);
        }
        for (int i = 0; i < 3; i++)
        {
            uint8_t ch = LCD_Data[0 + i];
            LCD_FontRenderStandard(75, 153 + i * 35, ch);
        }
        for (int i = 0; i < 3; i++)
        {
            uint8_t ch = LCD_Data[0 + i];
            LCD_FontRenderStandard(139, 34 + i * 35, ch);
        }
        for (int i = 0; i < 3; i++)
        {
            uint8_t ch = LCD_Data[0 + i];
            LCD_FontRenderStandard(139, 153 + i * 35, ch);
        }
        for (int i = 0; i < 3; i++)
        {
            uint8_t ch = LCD_Data[0 + i];
            LCD_FontRenderStandard(203, 34 + i * 35, ch);
        }
        for (int i = 0; i < 3; i++)
        {
            uint8_t ch = LCD_Data[0 + i];
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

    SDL_UpdateTexture(texture, NULL, lcd_buffer, lcd_width * 4);

    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);

    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type)
        {
        default:
            break;
        }
    }
}

