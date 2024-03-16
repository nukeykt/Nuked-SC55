#pragma once

#include <stdint.h>
#include <string>

void LCD_SetBackPath(const std::string &path);
void LCD_Init(void);
void LCD_UnInit(void);
void LCD_Write(uint32_t address, uint8_t data);
void LCD_Enable(uint32_t enable);
bool LCD_QuitRequested();
void LCD_Sync(void);
void LCD_Update(void);
