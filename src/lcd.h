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
#pragma once

#include <stdint.h>
#include <string>

struct MCU;

static const int lcd_width = 741;
static const int lcd_height = 268;

struct LCD {
    MCU *mcu;

    uint32_t LCD_DL, LCD_N, LCD_F, LCD_D, LCD_C, LCD_B, LCD_ID, LCD_S;
    uint32_t LCD_DD_RAM, LCD_AC, LCD_CG_RAM;
    uint32_t LCD_RAM_MODE = 0;
    uint8_t LCD_Data[80];
    uint8_t LCD_CG[64];

    uint8_t lcd_enable = 1;
    bool lcd_quit_requested = false;

    std::string m_back_path = "back.data";
    
    uint32_t lcd_buffer[lcd_height][lcd_width];
    uint32_t lcd_background[lcd_height][lcd_width];

    void LCD_SetBackPath(const std::string &path);
    void LCD_FontRenderStandard(int32_t x, int32_t y, uint8_t ch);
    void LCD_FontRenderLevel(int32_t x, int32_t y, uint8_t ch, uint8_t width = 5);
    void LCD_Init(MCU *mcu);
    void LCD_Write(uint32_t address, uint8_t data);
    void LCD_Enable(uint32_t enable);
    bool LCD_QuitRequested();
    uint32_t* LCD_Update(void);
    void LCD_SendButton(uint8_t button, int state);
};

