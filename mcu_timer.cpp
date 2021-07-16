#include <stdint.h>
#include "mcu.h"
#include "mcu_timer.h"

uint64_t timer_cycles;

frt_t frt[3];

void TIMER_Write(uint32_t address, uint8_t data)
{

}

uint8_t TIMER_Read(uint32_t address)
{
	return 0;
}

void TIMER_Clock(uint64_t cycles)
{
	uint32_t i;
	while (timer_cycles < cycles)
	{
		for (i = 0; i < 3; i++)
		{
			uint32_t offset = 0x10 * i;
			uint16_t value = (dev_register[DEV_FRT1_FRCH+offset] << 8) + dev_register[DEV_FRT1_FRCL+offset];
			uint8_t cr = dev_register[DEV_FRT1_TCR+offset];
			switch (cr & 3)
			{
			case 0: // o / 4
				if (timer_cycles & 3)
					continue;
				break;
			case 1: // o / 8
				if (timer_cycles & 7)
					continue;
				break;
			case 2: // o / 32
				if (timer_cycles & 31)
					continue;
				break;
			case 3: // ext (o / 2)
				if (timer_cycles & 1)
					continue;
				break;
			}

			value++;
		}

		timer_cycles++;
	}
}
