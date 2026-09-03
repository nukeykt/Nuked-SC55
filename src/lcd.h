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
#pragma once

#include <stdint.h>
#include <string>

extern int lcd_width;
extern int lcd_height;

extern uint32_t lcd_col1;
extern uint32_t lcd_col2;

void LCD_SetBackPath(const std::string &path);
void LCD_Init(void);
void LCD_UnInit(void);
void LCD_Write(uint32_t address, uint8_t data);
void LCD_Write_7seg(uint8_t address, uint8_t data);
void LCD_Enable(uint32_t enable);
bool LCD_QuitRequested();
void LCD_Sync(void);
void LCD_Update(void);
