#pragma once

#include <stdint.h>

void LCD_Init(void);
void LCD_Write(uint32_t address, uint8_t data);
void LCD_Update(void);
