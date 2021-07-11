#include "mcu.h"
#include "mcu_uart.h"

uint8_t UART_Read(uint32_t address)
{
    if (address == 0xff)
    {
        return 0xff;
    }
    return 0x00;
}

void UART_Write(uint32_t address, uint8_t data)
{
}

