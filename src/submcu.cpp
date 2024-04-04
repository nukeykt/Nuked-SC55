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
#include <stdio.h>
#include <string.h>
// #include "SDL_audio.h"
#include "mcu.h"
#include "submcu.h"

enum {
    SM_VECTOR_UART3_TX = 0,
    SM_VECTOR_UART2_TX,
    SM_VECTOR_UART1_TX,
    SM_VECTOR_COLLISION,
    SM_VECTOR_TIMER_X,
    SM_VECTOR_IPCM0,
    SM_VECTOR_UART3_RX,
    SM_VECTOR_UART2_RX,
    SM_VECTOR_UART1_RX,
    SM_VECTOR_RESET
};

enum {
    SM_DEV_P1_DATA = 0x00,
    SM_DEV_P1_DIR = 0x01,
    SM_DEV_RAM_DIR = 0x02,
    SM_DEV_UART1_MODE_STATUS = 0x05,
    SM_DEV_UART1_CTRL = 0x06,
    SM_DEV_UART2_DATA = 0x08,
    SM_DEV_UART2_MODE_STATUS = 0x09,
    SM_DEV_UART2_CTRL = 0x0a,
    SM_DEV_UART3_MODE_STATUS = 0x0d,
    SM_DEV_UART3_CTRL = 0x0e,
    SM_DEV_IPCM0 = 0x10,
    SM_DEV_IPCM1 = 0x11,
    SM_DEV_IPCM2 = 0x12,
    SM_DEV_IPCM3 = 0x13,
    SM_DEV_IPCE0 = 0x14,
    SM_DEV_IPCE1 = 0x15,
    SM_DEV_IPCE2 = 0x16,
    SM_DEV_IPCE3 = 0x17,
    SM_DEV_SEMAPHORE = 0x19,
    SM_DEV_COLLISION = 0x1a,
    SM_DEV_INT_ENABLE = 0x1b,
    SM_DEV_INT_REQUEST = 0x1c,
    SM_DEV_PRESCALER = 0x1d,
    SM_DEV_TIMER = 0x1e,
    SM_DEV_TIMER_CTRL = 0x1f
};

void SubMcu::SM_ErrorTrap(void)
{
    printf("smtrap %.4x\n", sm.pc);
}

uint8_t SubMcu::SM_Read(uint16_t address)
{
    address &= 0x1fff;
    if (address & 0x1000)
    {
        return sm_rom[address & 0xfff];
    }
    else if (address < 0x80)
    {
        return sm_ram[address];
    }
    else if (address >= 0xc0 && address < 0xd8)
    {
        return sm_access[address & 0x1f];
    }
    else if (address >= 0xe0 && address < 0x100)
    {
        address &= 0x1f;
        switch (address)
        {
            case SM_DEV_UART2_DATA:
            {
                uart_rx_gotbyte = 0;
                return uart_rx_byte;
            }
            case SM_DEV_UART1_MODE_STATUS:
            {
                uint8_t ret = 0;
                ret |= 5;
                return ret;
            }
            case SM_DEV_UART2_MODE_STATUS:
            {
                uint8_t ret = uart_rx_gotbyte << 1;
                ret |= 5;
                return ret;
            }
            case SM_DEV_UART3_MODE_STATUS:
            {
                uint8_t ret = 0;
                ret |= 5;
                return ret;
            }
            case SM_DEV_P1_DATA:
                return mcu->MCU_ReadP1();
            case SM_DEV_P1_DIR:
                return sm_p1_dir;
            case SM_DEV_PRESCALER:
                return sm_timer_prescaler;
            case SM_DEV_TIMER:
                return sm_timer_counter;
        }
        return sm_device_mode[address];
    }
    else if (address >= 0x200 && address < 0x2c0)
    {
        address &= 0xff;
        if (sm_device_mode[SM_DEV_RAM_DIR] & (1<<(address>>5)))
            sm_access[address>>3] &= ~(1<<(address&7));
        return sm_shared_ram[address];
    }
    else
    {
        printf("sm: unknown read %x\n", address);
        return 0;
    }
}

void SubMcu::SM_Write(uint16_t address, uint8_t data)
{
    address &= 0x1fff;
    if (address < 0x80)
    {
        sm_ram[address] = data;
    }
    else if (address >= 0xe0 && address < 0x100)
    {
        address &= 0x1f;
        switch (address)
        {
            case SM_DEV_P1_DATA:
                mcu->MCU_WriteP1(data);
                break;
            case SM_DEV_P1_DIR:
                sm_p1_dir = data;
                break;
            case SM_DEV_IPCM0:
            case SM_DEV_IPCM1:
            case SM_DEV_IPCM2:
            case SM_DEV_IPCM3:
                sm_device_mode[address] = data;
                break;
            case SM_DEV_IPCE0:
            case SM_DEV_IPCE1:
            case SM_DEV_IPCE2:
            case SM_DEV_IPCE3:
                sm_device_mode[address] = data;
                break;
            case SM_DEV_INT_REQUEST:
                sm_device_mode[SM_DEV_INT_REQUEST] &= data;
                break;
            case SM_DEV_COLLISION:
                sm_device_mode[SM_DEV_COLLISION] &= ~0x7f;
                sm_device_mode[SM_DEV_COLLISION] |= data & 0x7f;
                if ((data & 0x80) == 0)
                    sm_device_mode[SM_DEV_COLLISION] &= ~0x80;
                break;
            default:
                sm_device_mode[address] = data;
                break;
        }
        if (address == SM_DEV_UART3_MODE_STATUS || address == SM_DEV_UART3_CTRL)
            mcu->MCU_GA_SetGAInt(5, (sm_device_mode[SM_DEV_UART3_MODE_STATUS] & 0x80) != 0
                && (sm_device_mode[SM_DEV_UART3_CTRL] & 0x20) == 0);
    }
    else if (address >= 0x200 && address < 0x2c0)
    {
        address &= 0xff;
        sm_access[address>>3] |= 1<<(address&7);
        sm_shared_ram[address] = data;
    }
    else
    {
        printf("sm: unknown write %x %x\n", address, data);
    }
}

void SubMcu::SM_SysWrite(uint32_t address, uint8_t data)
{
    address &= 0xff;
    if (address < 0xc0)
    {
        address &= 0xff;
        sm_access[address>>3] |= 1<<(address&7);
        sm_shared_ram[address] = data;
    }
    else if (address >= 0xf8 && address < 0xfc)
    {
        sm_device_mode[SM_DEV_IPCM0 + (address & 3)] = data;
        if ((address & 3) == 0) 
        {
            sm_device_mode[SM_DEV_INT_REQUEST] |= 0x10;
            sm_device_mode[SM_DEV_SEMAPHORE] &= ~0x80;
        }
    }
    else if (address == 0xff)
    {
        sm_device_mode[SM_DEV_SEMAPHORE] &= ~0x1f;
        sm_device_mode[SM_DEV_SEMAPHORE] |= data & 0x1f;
    }
    else if (address == 0xf5)
    {
        mcu->MCU_WriteP1(data);
    }
    else if (address == 0xf6)
    {
        mcu->MCU_WriteP0(data);
    }
    else if (address == 0xf7)
    {
        sm_p0_dir = data;
    }
    else
    {
        printf("sm: unknown sys write %x %x\n", address, data);
    }
}

uint8_t SubMcu::SM_SysRead(uint32_t address)
{
    address &= 0xff;
    if (address < 0xc0)
    {
        if ((sm_device_mode[SM_DEV_RAM_DIR] & (1<<(address>>5))) == 0)
            sm_access[address>>3] &= ~(1<<(address&7));
        return sm_shared_ram[address];
    }
    else if (address >= 0xf8 && address < 0xfc)
    {
        if ((address & 3) == 0)
        {
            sm_device_mode[SM_DEV_INT_REQUEST] |= 0x10;
        }
        uint8_t val = sm_device_mode[SM_DEV_IPCE0 + (address & 3)];
        sm_device_mode[SM_DEV_IPCE0 + (address & 3)] = 0; // FIXME
        return val;
    }
    else if (address == 0xff)
    {
        return sm_device_mode[SM_DEV_SEMAPHORE];
    }
    else if (address == 0xf5)
    {
        return mcu->MCU_ReadP1();
    }
    else if (address == 0xf6)
    {
        return mcu->MCU_ReadP0();
    }
    else if (address == 0xf7)
    {
        return sm_p0_dir;
    }
    else
    {
        printf("sm: unknown sys read %x\n", address);
        return 0;
    }
}

uint16_t SubMcu::SM_GetVectorAddress(uint32_t vector)
{
    uint16_t pc = SM_Read(0x1fec + vector * 2);
    pc |= SM_Read(0x1fec + vector * 2 + 1) << 8;
    return pc;
}

void SubMcu::SM_SetStatus(uint32_t condition, uint32_t mask)
{
    if (condition)
        sm.sr |= mask;
    else
        sm.sr &= ~mask;
}

void SubMcu::SM_Reset(void)
{
    memset(&sm, 0, sizeof(sm));
    this->sm.pc = SM_GetVectorAddress(SM_VECTOR_RESET);
}

uint8_t SubMcu::SM_ReadAdvance(void)
{
    uint8_t byte = SM_Read(this->sm.pc);
    this->sm.pc++;
    return byte;
}

uint16_t SubMcu::SM_ReadAdvance16(void)
{
    uint16_t word = SM_ReadAdvance();
    word |= SM_ReadAdvance() << 8;
    return word;
}

uint16_t SubMcu::SM_Read16(uint16_t address)
{
    uint16_t word = SM_Read(address);
    word |= SM_Read(address) << 8;
    return word;
}

void SubMcu::SM_Update_NZ(uint8_t val)
{
    SM_SetStatus(val == 0, SM_STATUS_Z);
    SM_SetStatus(val & 0x80, SM_STATUS_N);
}

void SubMcu::SM_PushStack(uint8_t data)
{
    SM_Write(sm.s, data);
    sm.s--;
}

uint8_t SubMcu::SM_PopStack(void)
{
    sm.s++;
    return SM_Read(sm.s);
}

void SM_Opcode_NotImplemented(SubMcu* _this, uint8_t opcode)
{
    _this->SM_ErrorTrap();
}

void SM_Opcode_SEI(SubMcu* _this, uint8_t opcode) // 78
{
    _this->SM_SetStatus(1, SM_STATUS_I);
}

void SM_Opcode_CLD(SubMcu* _this, uint8_t opcode) // d8
{
    _this->SM_SetStatus(0, SM_STATUS_D);
}

void SM_Opcode_CLT(SubMcu* _this, uint8_t opcode) // 12
{
    _this->SM_SetStatus(0, SM_STATUS_T);
}

void SM_Opcode_LDX(SubMcu* _this, uint8_t opcode) // a2, a6, ae, b6, be
{
    uint8_t val = 0;
    switch (opcode)
    {
        case 0xa2:
            val = _this->SM_ReadAdvance();
            break;
        case 0xa6:
            val = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0xb6:
            val = _this->SM_Read((_this->SM_ReadAdvance() + _this->sm.y) & 0xff);
            break;
        case 0xae:
            val = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
        case 0xbe:
            val = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.y);
            break;
    }
    _this->sm.x = val;
    _this->SM_Update_NZ(_this->sm.x);
}

void SM_Opcode_TXS(SubMcu* _this, uint8_t opcode) // 9a
{
    _this->sm.s = _this->sm.x;
}

void SM_Opcode_TXA(SubMcu* _this, uint8_t opcode) // 8a
{
    _this->sm.a = _this->sm.x;
    _this->SM_Update_NZ(_this->sm.a);
}

void SM_Opcode_STA(SubMcu* _this, uint8_t opcode) // 85, 95, 8d, 9d, 99, 81, 91
{
    uint16_t dest = 0;
    switch (opcode)
    {
        case 0x85:
            dest = _this->SM_ReadAdvance();
            break;
        case 0x95:
            dest = _this->SM_ReadAdvance() + _this->sm.x;
            break;
        case 0x8d:
            dest = _this->SM_ReadAdvance16();
            break;
        case 0x9d:
            dest = _this->SM_ReadAdvance16() + _this->sm.x;
            break;
        case 0x99:
            dest = _this->SM_ReadAdvance16() + _this->sm.y;
            break;
        case 0x81:
            dest = _this->SM_Read16((_this->SM_ReadAdvance() + _this->sm.x) & 0xff);
            break;
        case 0x91:
            dest = _this->SM_Read16(_this->SM_ReadAdvance()) + _this->sm.y;
            break;
    }

    _this->SM_Write(dest, _this->sm.a);
}

void SM_Opcode_INX(SubMcu* _this, uint8_t opcode) // e8
{
    _this->sm.x++;
    _this->SM_Update_NZ(_this->sm.x);
}

void SM_Opcode_BBC_BBS(SubMcu* _this, uint8_t opcode)
{
    int32_t zp = (opcode & 4) != 0;
    int32_t bit = (opcode >> 5) & 7;
    int32_t type = (opcode >> 4) & 1;
    uint8_t val = 0;

    if (!zp)
    {
        val = _this->sm.a;
    }
    else
    {
        val = _this->SM_Read(_this->SM_ReadAdvance());
    }

    int8_t diff = _this->SM_ReadAdvance();

    int32_t set = (val >> bit) & 1;
    
    if (set != type)
        _this->sm.pc += diff;
}

void SM_Opcode_CPX(SubMcu* _this, uint8_t opcode) // e0, e4, ec
{
    uint8_t operand = 0;
    switch (opcode)
    {
        case 0xe0:
            operand = _this->SM_ReadAdvance();
            break;
        case 0xe4:
            operand = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0xec:
            operand = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
    }
    int diff = _this->sm.x - operand;
    _this->SM_SetStatus((diff & 0x100) == 0, SM_STATUS_C);
    _this->SM_Update_NZ(diff & 0xff);
}

void SM_Opcode_BEQ(SubMcu* _this, uint8_t opcode) // f0
{
    int8_t diff = _this->SM_ReadAdvance();
    if ((_this->sm.sr & SM_STATUS_Z) != 0)
        _this->sm.pc += diff;
}

void SM_Opcode_BCC(SubMcu* _this, uint8_t opcode) // 90
{
    int8_t diff = _this->SM_ReadAdvance();
    if ((_this->sm.sr & SM_STATUS_C) == 0)
        _this->sm.pc += diff;
}

void SM_Opcode_BCS(SubMcu* _this, uint8_t opcode) // b0
{
    int8_t diff = _this->SM_ReadAdvance();
    if ((_this->sm.sr & SM_STATUS_C) != 0)
        _this->sm.pc += diff;
}

void SM_Opcode_LDM(SubMcu* _this, uint8_t opcode) // 3c
{
    uint8_t val = _this->SM_ReadAdvance();
    _this->SM_Write(_this->SM_ReadAdvance(), val);
}

void SM_Opcode_LDA(SubMcu* _this, uint8_t opcode) // a9, a5, b5, ad, bd, b9, a1, b1
{
    uint8_t val = 0;
    switch (opcode)
    {
        case 0xa9:
            val = _this->SM_ReadAdvance();
            break;
        case 0xa5:
            val = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0xb5:
            val = _this->SM_Read((_this->SM_ReadAdvance() + _this->sm.x) & 0xff);
            break;
        case 0xad:
            val = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
        case 0xbd:
            val = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.x);
            break;
        case 0xb9:
            val = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.y);
            break;
        case 0xa1:
            val = _this->SM_Read(_this->SM_Read16((_this->SM_ReadAdvance() + _this->sm.x) & 0xff));
            break;
        case 0xb1:
            val = _this->SM_Read(_this->SM_Read16(_this->SM_ReadAdvance()) + _this->sm.y);
            break;
    }

    if ((_this->sm.sr & SM_STATUS_T) == 0)
    {
        _this->sm.a = val;
        _this->SM_Update_NZ(val);
    }
    else
    {
        // FIXME
        _this->SM_Write(_this->sm.x, val);
    }
}

void SM_Opcode_CLI(SubMcu* _this, uint8_t opcode) // 58
{
    _this->SM_SetStatus(0, SM_STATUS_I);
}

void SM_Opcode_STP(SubMcu* _this, uint8_t opcode) // 42
{
    _this->sm.sleep = 1;
}

void SM_Opcode_PHA(SubMcu* _this, uint8_t opcode) // 48
{
    _this->SM_PushStack(_this->sm.a);
}

void SM_Opcode_SEB_CLB(SubMcu* _this, uint8_t opcode)
{
    int32_t zp = (opcode & 4) != 0;
    int32_t bit = (opcode >> 5) & 7;
    int32_t type = (opcode >> 4) & 1;
    uint8_t val = 0;
    uint8_t dest = 0;

    if (!zp)
    {
        val = _this->sm.a;
    }
    else
    {
        dest = _this->SM_ReadAdvance();
        val = _this->SM_Read(dest);
    }

    if (type)
        val &= ~(1 << bit);
    else
        val |= 1 << bit;

    if (!zp)
    {
        _this->sm.a = val;
    }
    else
    {
        _this->SM_Write(dest, val);
    }
}

void SM_Opcode_RTI(SubMcu* _this, uint8_t opcode) // 40
{
    _this->sm.sr = _this->SM_PopStack();
    _this->sm.pc = _this->SM_PopStack();
    _this->sm.pc |= _this->SM_PopStack() << 8;
}

void SM_Opcode_PLA(SubMcu* _this, uint8_t opcode) // 68
{
    _this->sm.a = _this->SM_PopStack();
    _this->SM_Update_NZ(_this->sm.a);
}

void SM_Opcode_BRA(SubMcu* _this, uint8_t opcode) // 80
{
    int8_t disp = _this->SM_ReadAdvance();
    _this->sm.pc += disp;
}

void SM_Opcode_JSR(SubMcu* _this, uint8_t opcode) // 20, 02, 22
{
    uint16_t newpc = 0;
    switch (opcode)
    {
        case 0x20:
            newpc = _this->SM_ReadAdvance16();
            break;
        case 0x02:
            newpc = _this->SM_Read16(_this->SM_ReadAdvance());
            break;
        case 0x22:
            newpc = 0xff00 | _this->SM_ReadAdvance();
            break;
    }

    _this->SM_PushStack(_this->sm.pc >> 8);
    _this->SM_PushStack(_this->sm.pc & 0xff);
    _this->sm.pc = newpc;
}

void SM_Opcode_CMP(SubMcu* _this, uint8_t opcode) // c9, c5, d5, cd, dd, d9, c1, d1
{
    uint8_t operand = 0;
    switch (opcode)
    {
        case 0xc9:
            operand = _this->SM_ReadAdvance();
            break;
        case 0xc5:
            operand = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0xd5:
            operand = _this->SM_Read((_this->SM_ReadAdvance()+_this->sm.x)&0xff);
            break;
        case 0xcd:
            operand = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
        case 0xdd:
            operand = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.x);
            break;
        case 0xd9:
            operand = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.y);
            break;
        case 0xc1:
            operand = _this->SM_Read(_this->SM_Read16((_this->SM_ReadAdvance() + _this->sm.x) & 0xff));
            break;
        case 0xd1:
            operand = _this->SM_Read(_this->SM_Read16(_this->SM_ReadAdvance()) + _this->sm.y);
            break;
    }
    int diff = _this->sm.a - operand;
    _this->SM_SetStatus((diff & 0x100) == 0, SM_STATUS_C);
    _this->SM_Update_NZ(diff & 0xff);
}

void SM_Opcode_BNE(SubMcu* _this, uint8_t opcode) // d0
{
    int8_t diff = _this->SM_ReadAdvance();
    if ((_this->sm.sr & SM_STATUS_Z) == 0)
        _this->sm.pc += diff;
}

void SM_Opcode_RTS(SubMcu* _this, uint8_t opcode) // 60
{
    _this->sm.pc = _this->SM_PopStack();
    _this->sm.pc |= _this->SM_PopStack() << 8;
}

void SM_Opcode_JMP(SubMcu* _this, uint8_t opcode) // 4c, 6c, b2
{
    switch (opcode)
    {
        case 0x4c:
            _this->sm.pc = _this->SM_ReadAdvance16();
            break;
        case 0x6c:
            _this->sm.pc = _this->SM_Read16(_this->SM_ReadAdvance16());
            break;
        case 0xb2:
            _this->sm.pc = _this->SM_Read16(_this->SM_ReadAdvance());
            break;
    }
}

void SM_Opcode_ORA(SubMcu* _this, uint8_t opcode) // 09, 05, 15, 0d, 1d, 01, 11
{
    uint8_t val = 0;
    uint8_t val2 = 0;

    if ((_this->sm.sr & SM_STATUS_T) == 0)
    {
        val = _this->sm.a;
    }
    else
    {
        // FIXME
        val = _this->SM_Read(_this->sm.x);
    }

    switch (opcode)
    {
        case 0x09:
            val2 = _this->SM_ReadAdvance();
            break;
        case 0x05:
            val2 = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0x15:
            val2 = _this->SM_Read((_this->SM_ReadAdvance() + _this->sm.x) & 0xff);
            break;
        case 0x0d:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
        case 0x1d:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.x);
            break;
        case 0x19:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.y);
            break;
        case 0x01:
            val2 = _this->SM_Read(_this->SM_Read16((_this->SM_ReadAdvance() + _this->sm.x) & 0xff));
            break;
        case 0x11:
            val2 = _this->SM_Read(_this->SM_Read16(_this->SM_ReadAdvance()) + _this->sm.y);
            break;
    }

    val |= val2;

    if ((_this->sm.sr & SM_STATUS_T) == 0)
    {
        _this->sm.a = val;

        _this->SM_Update_NZ(val);
    }
    else
    {
        // FIXME
        _this->SM_Write(_this->sm.x, val);
    }
}

void SM_Opcode_DEC(SubMcu* _this, uint8_t opcode) // 1a, c6, d6, ce, de
{
    uint8_t val = 0;
    uint16_t dest = 0;
    switch (opcode)
    {
        case 0x1a:
            _this->sm.a--;
            _this->SM_Update_NZ(_this->sm.a);
            return;
        case 0xc6:
            dest = _this->SM_ReadAdvance();
            break;
        case 0xd6:
            dest = (_this->SM_ReadAdvance() + _this->sm.x) & 0xff;
            break;
        case 0xce:
            dest = _this->SM_ReadAdvance16();
            break;
        case 0xde:
            dest = _this->SM_ReadAdvance16() + _this->sm.x;
            break;
    }
    val = _this->SM_Read(dest);
    val--;
    _this->SM_Write(dest, val);
    _this->SM_Update_NZ(val);
}

void SM_Opcode_TAX(SubMcu* _this, uint8_t opcode) // aa
{
    _this->sm.x = _this->sm.a;
    _this->SM_Update_NZ(_this->sm.x);
}

void SM_Opcode_STX(SubMcu* _this, uint8_t opcode) // 86 96 8e
{
    uint16_t dest = 0;
    switch (opcode)
    {
        case 0x86:
            dest = _this->SM_ReadAdvance();
            break;
        case 0x96:
            dest = _this->SM_ReadAdvance() + _this->sm.x;
            break;
        case 0x8e:
            dest = _this->SM_ReadAdvance16();
            break;
    }

    _this->SM_Write(dest, _this->sm.x);
}

void SM_Opcode_SEC(SubMcu* _this, uint8_t opcode) // 38
{
    _this->SM_SetStatus(1, SM_STATUS_C);
}

void SM_Opcode_NOP(SubMcu* _this, uint8_t opcode) // EA
{
}

void SM_Opcode_BPL(SubMcu* _this, uint8_t opcode) // 10
{
    int8_t diff = _this->SM_ReadAdvance();
    if ((_this->sm.sr & SM_STATUS_N) == 0)
        _this->sm.pc += diff;
}

void SM_Opcode_CLC(SubMcu* _this, uint8_t opcode) // 18
{
    _this->SM_SetStatus(0, SM_STATUS_C);
}

void SM_Opcode_AND(SubMcu* _this, uint8_t opcode) // 29, 25, 35, 2d, 3d, 21, 31
{
    uint8_t val = 0;
    uint8_t val2 = 0;

    if ((_this->sm.sr & SM_STATUS_T) == 0)
    {
        val = _this->sm.a;
    }
    else
    {
        // FIXME
        val = _this->SM_Read(_this->sm.x);
    }

    switch (opcode)
    {
        case 0x29:
            val2 = _this->SM_ReadAdvance();
            break;
        case 0x25:
            val2 = _this->SM_Read(_this->SM_ReadAdvance());
            break;
        case 0x35:
            val2 = _this->SM_Read((_this->SM_ReadAdvance() + _this->sm.x) & 0xff);
            break;
        case 0x2d:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16());
            break;
        case 0x3d:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.x);
            break;
        case 0x39:
            val2 = _this->SM_Read(_this->SM_ReadAdvance16() + _this->sm.y);
            break;
        case 0x21:
            val2 = _this->SM_Read(_this->SM_Read16((_this->SM_ReadAdvance() + _this->sm.x) & 0xff));
            break;
        case 0x31:
            val2 = _this->SM_Read(_this->SM_Read16(_this->SM_ReadAdvance()) + _this->sm.y);
            break;
    }

    val &= val2;

    if ((_this->sm.sr & SM_STATUS_T) == 0)
    {
        _this->sm.a = val;

        _this->SM_Update_NZ(val);
    }
    else
    {
        // FIXME
        _this->SM_Write(_this->sm.x, val);
    }
}

void SM_Opcode_INC(SubMcu* _this, uint8_t opcode) // 3a, e6, f6, ee, fe
{
    uint8_t val = 0;
    uint16_t dest = 0;
    switch (opcode)
    {
        case 0x3a:
            _this->sm.a++;
            _this->SM_Update_NZ(_this->sm.a);
            return;
        case 0xe6:
            dest = _this->SM_ReadAdvance();
            break;
        case 0xf6:
            dest = (_this->SM_ReadAdvance() + _this->sm.x) & 0xff;
            break;
        case 0xee:
            dest = _this->SM_ReadAdvance16();
            break;
        case 0xfe:
            dest = _this->SM_ReadAdvance16() + _this->sm.x;
            break;
    }
    val = _this->SM_Read(dest);
    val++;
    _this->SM_Write(dest, val);
    _this->SM_Update_NZ(val);
}

void (*SM_Opcode_Table[256])(SubMcu *_this, uint8_t opcode)
{
    SM_Opcode_NotImplemented, // 00
    SM_Opcode_ORA, // 01
    SM_Opcode_JSR, // 02
    SM_Opcode_BBC_BBS, // 03
    SM_Opcode_NotImplemented, // 04
    SM_Opcode_ORA, // 05
    SM_Opcode_NotImplemented, // 06
    SM_Opcode_BBC_BBS, // 07
    SM_Opcode_NotImplemented, // 08
    SM_Opcode_ORA, // 09
    SM_Opcode_NotImplemented, // 0a
    SM_Opcode_SEB_CLB, // 0b
    SM_Opcode_NotImplemented, // 0c
    SM_Opcode_ORA, // 0d
    SM_Opcode_NotImplemented, // 0e
    SM_Opcode_SEB_CLB, // 0f
    SM_Opcode_BPL, // 10
    SM_Opcode_ORA, // 11
    SM_Opcode_CLT, // 12
    SM_Opcode_BBC_BBS, // 13
    SM_Opcode_NotImplemented, // 14
    SM_Opcode_ORA, // 15
    SM_Opcode_NotImplemented, // 16
    SM_Opcode_BBC_BBS, // 17
    SM_Opcode_CLC, // 18
    SM_Opcode_ORA, // 19
    SM_Opcode_DEC, // 1a
    SM_Opcode_SEB_CLB, // 1b
    SM_Opcode_NotImplemented, // 1c
    SM_Opcode_ORA, // 1d
    SM_Opcode_NotImplemented, // 1e
    SM_Opcode_SEB_CLB, // 1f
    SM_Opcode_JSR, // 20
    SM_Opcode_AND, // 21
    SM_Opcode_JSR, // 22
    SM_Opcode_BBC_BBS, // 23
    SM_Opcode_NotImplemented, // 24
    SM_Opcode_AND, // 25
    SM_Opcode_NotImplemented, // 26
    SM_Opcode_BBC_BBS, // 27
    SM_Opcode_NotImplemented, // 28
    SM_Opcode_AND, // 29
    SM_Opcode_NotImplemented, // 2a
    SM_Opcode_SEB_CLB, // 2b
    SM_Opcode_NotImplemented, // 2c
    SM_Opcode_AND, // 2d
    SM_Opcode_NotImplemented, // 2e
    SM_Opcode_SEB_CLB, // 2f
    SM_Opcode_NotImplemented, // 30
    SM_Opcode_AND, // 31
    SM_Opcode_NotImplemented, // 32
    SM_Opcode_BBC_BBS, // 33
    SM_Opcode_NotImplemented, // 34
    SM_Opcode_AND, // 35
    SM_Opcode_NotImplemented, // 36
    SM_Opcode_BBC_BBS, // 37
    SM_Opcode_SEC, // 38
    SM_Opcode_AND, // 39
    SM_Opcode_INC, // 3a
    SM_Opcode_SEB_CLB, // 3b
    SM_Opcode_LDM, // 3c
    SM_Opcode_AND, // 3d
    SM_Opcode_NotImplemented, // 3e
    SM_Opcode_SEB_CLB, // 3f
    SM_Opcode_RTI, // 40
    SM_Opcode_NotImplemented, // 41
    SM_Opcode_STP, // 42
    SM_Opcode_BBC_BBS, // 43
    SM_Opcode_NotImplemented, // 44
    SM_Opcode_NotImplemented, // 45
    SM_Opcode_NotImplemented, // 46
    SM_Opcode_BBC_BBS, // 47
    SM_Opcode_PHA, // 48
    SM_Opcode_NotImplemented, // 49
    SM_Opcode_NotImplemented, // 4a
    SM_Opcode_SEB_CLB, // 4b
    SM_Opcode_JMP, // 4c
    SM_Opcode_NotImplemented, // 4d
    SM_Opcode_NotImplemented, // 4e
    SM_Opcode_SEB_CLB, // 4f
    SM_Opcode_NotImplemented, // 50
    SM_Opcode_NotImplemented, // 51
    SM_Opcode_NotImplemented, // 52
    SM_Opcode_BBC_BBS, // 53
    SM_Opcode_NotImplemented, // 54
    SM_Opcode_NotImplemented, // 55
    SM_Opcode_NotImplemented, // 56
    SM_Opcode_BBC_BBS, // 57
    SM_Opcode_CLI, // 58
    SM_Opcode_NotImplemented, // 59
    SM_Opcode_NotImplemented, // 5a
    SM_Opcode_SEB_CLB, // 5b
    SM_Opcode_NotImplemented, // 5c
    SM_Opcode_NotImplemented, // 5d
    SM_Opcode_NotImplemented, // 5e
    SM_Opcode_SEB_CLB, // 5f
    SM_Opcode_RTS, // 60
    SM_Opcode_NotImplemented, // 61
    SM_Opcode_NotImplemented, // 62
    SM_Opcode_BBC_BBS, // 63
    SM_Opcode_NotImplemented, // 64
    SM_Opcode_NotImplemented, // 65
    SM_Opcode_NotImplemented, // 66
    SM_Opcode_BBC_BBS, // 67
    SM_Opcode_PLA, // 68
    SM_Opcode_NotImplemented, // 69
    SM_Opcode_NotImplemented, // 6a
    SM_Opcode_SEB_CLB, // 6b
    SM_Opcode_JMP, // 6c
    SM_Opcode_NotImplemented, // 6d
    SM_Opcode_NotImplemented, // 6e
    SM_Opcode_SEB_CLB, // 6f
    SM_Opcode_NotImplemented, // 70
    SM_Opcode_NotImplemented, // 71
    SM_Opcode_NotImplemented, // 72
    SM_Opcode_BBC_BBS, // 73
    SM_Opcode_NotImplemented, // 74
    SM_Opcode_NotImplemented, // 75
    SM_Opcode_NotImplemented, // 76
    SM_Opcode_BBC_BBS, // 77
    SM_Opcode_SEI, // 78
    SM_Opcode_NotImplemented, // 79
    SM_Opcode_NotImplemented, // 7a
    SM_Opcode_SEB_CLB, // 7b
    SM_Opcode_NotImplemented, // 7c
    SM_Opcode_NotImplemented, // 7d
    SM_Opcode_NotImplemented, // 7e
    SM_Opcode_SEB_CLB, // 7f
    SM_Opcode_BRA, // 80
    SM_Opcode_STA, // 81
    SM_Opcode_NotImplemented, // 82
    SM_Opcode_BBC_BBS, // 83
    SM_Opcode_NotImplemented, // 84
    SM_Opcode_STA, // 85
    SM_Opcode_STX, // 86
    SM_Opcode_BBC_BBS, // 87
    SM_Opcode_NotImplemented, // 88
    SM_Opcode_NotImplemented, // 89
    SM_Opcode_TXA, // 8a
    SM_Opcode_SEB_CLB, // 8b
    SM_Opcode_NotImplemented, // 8c
    SM_Opcode_STA, // 8d
    SM_Opcode_STX, // 8e
    SM_Opcode_SEB_CLB, // 8f
    SM_Opcode_BCC, // 90
    SM_Opcode_STA, // 91
    SM_Opcode_NotImplemented, // 92
    SM_Opcode_BBC_BBS, // 93
    SM_Opcode_NotImplemented, // 94
    SM_Opcode_STA, // 95
    SM_Opcode_STX, // 96
    SM_Opcode_BBC_BBS, // 97
    SM_Opcode_NotImplemented, // 98
    SM_Opcode_STA, // 99
    SM_Opcode_TXS, // 9a
    SM_Opcode_SEB_CLB, // 9b
    SM_Opcode_NotImplemented, // 9c
    SM_Opcode_STA, // 9d
    SM_Opcode_NotImplemented, // 9e
    SM_Opcode_SEB_CLB, // 9f
    SM_Opcode_NotImplemented, // a0
    SM_Opcode_LDA, // a1
    SM_Opcode_LDX, // a2
    SM_Opcode_BBC_BBS, // a3
    SM_Opcode_NotImplemented, // a4
    SM_Opcode_LDA, // a5
    SM_Opcode_LDX, // a6
    SM_Opcode_BBC_BBS, // a7
    SM_Opcode_NotImplemented, // a8
    SM_Opcode_LDA, // a9
    SM_Opcode_TAX, // aa
    SM_Opcode_SEB_CLB, // ab
    SM_Opcode_NotImplemented, // ac
    SM_Opcode_LDA, // ad
    SM_Opcode_LDX, // ae
    SM_Opcode_SEB_CLB, // af
    SM_Opcode_BCS, // b0
    SM_Opcode_LDA, // b1
    SM_Opcode_JMP, // b2
    SM_Opcode_BBC_BBS, // b3
    SM_Opcode_NotImplemented, // b4
    SM_Opcode_LDA, // b5
    SM_Opcode_LDX, // b6
    SM_Opcode_BBC_BBS, // b7
    SM_Opcode_NotImplemented, // b8
    SM_Opcode_LDA, // b9
    SM_Opcode_NotImplemented, // ba
    SM_Opcode_SEB_CLB, // bb
    SM_Opcode_NotImplemented, // bc
    SM_Opcode_LDA, // bd
    SM_Opcode_LDX, // be
    SM_Opcode_SEB_CLB, // bf
    SM_Opcode_NotImplemented, // c0
    SM_Opcode_CMP, // c1
    SM_Opcode_NotImplemented, // c2
    SM_Opcode_BBC_BBS, // c3
    SM_Opcode_NotImplemented, // c4
    SM_Opcode_CMP, // c5
    SM_Opcode_DEC, // c6
    SM_Opcode_BBC_BBS, // c7
    SM_Opcode_NotImplemented, // c8
    SM_Opcode_CMP, // c9
    SM_Opcode_NotImplemented, // ca
    SM_Opcode_SEB_CLB, // cb
    SM_Opcode_NotImplemented, // cc
    SM_Opcode_CMP, // cd
    SM_Opcode_DEC, // ce
    SM_Opcode_SEB_CLB, // cf
    SM_Opcode_BNE, // d0
    SM_Opcode_CMP, // d1
    SM_Opcode_NotImplemented, // d2
    SM_Opcode_BBC_BBS, // d3
    SM_Opcode_NotImplemented, // d4
    SM_Opcode_CMP, // d5
    SM_Opcode_DEC, // d6
    SM_Opcode_BBC_BBS, // d7
    SM_Opcode_CLD, // d8
    SM_Opcode_CMP, // d9
    SM_Opcode_NotImplemented, // da
    SM_Opcode_SEB_CLB, // db
    SM_Opcode_NotImplemented, // dc
    SM_Opcode_CMP, // dd
    SM_Opcode_DEC, // de
    SM_Opcode_SEB_CLB, // df
    SM_Opcode_CPX, // e0
    SM_Opcode_NotImplemented, // e1
    SM_Opcode_NotImplemented, // e2
    SM_Opcode_BBC_BBS, // e3
    SM_Opcode_CPX, // e4
    SM_Opcode_NotImplemented, // e5
    SM_Opcode_INC, // e6
    SM_Opcode_BBC_BBS, // e7
    SM_Opcode_INX, // e8
    SM_Opcode_NotImplemented, // e9
    SM_Opcode_NOP, // ea
    SM_Opcode_SEB_CLB, // eb
    SM_Opcode_CPX, // ec
    SM_Opcode_NotImplemented, // ed
    SM_Opcode_INC, // ee
    SM_Opcode_SEB_CLB, // ef
    SM_Opcode_BEQ, // f0
    SM_Opcode_NotImplemented, // f1
    SM_Opcode_NotImplemented, // f2
    SM_Opcode_BBC_BBS, // f3
    SM_Opcode_NotImplemented, // f4
    SM_Opcode_NotImplemented, // f5
    SM_Opcode_INC, // f6
    SM_Opcode_BBC_BBS, // f7
    SM_Opcode_NotImplemented, // f8
    SM_Opcode_NotImplemented, // f9
    SM_Opcode_NotImplemented, // fa
    SM_Opcode_SEB_CLB, // fb
    SM_Opcode_NotImplemented, // fc
    SM_Opcode_NotImplemented, // fd
    SM_Opcode_INC, // fe
    SM_Opcode_SEB_CLB, // ff
};

void SubMcu::SM_StartVector(uint32_t vector)
{
    SM_PushStack(sm.pc >> 8);
    SM_PushStack(sm.pc & 0xff);
    SM_PushStack(this->sm.sr);

    this->sm.sr |= SM_STATUS_I;
    this->sm.sleep = 0;

    sm.pc = SM_GetVectorAddress(vector);
}

void SubMcu::SM_HandleInterrupt(void)
{
    if (this->sm.sr & SM_STATUS_I)
        return;
    
    if ((sm_device_mode[SM_DEV_UART1_CTRL] & 0x8) != 0
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x80) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x80) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x80;
        SM_StartVector(SM_VECTOR_UART1_RX);
        return;
    }
    if ((sm_device_mode[SM_DEV_UART2_CTRL] & 0x8) != 0
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x40) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x40) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x40;
        SM_StartVector(SM_VECTOR_UART2_RX);
        return;
    }
    if ((sm_device_mode[SM_DEV_UART3_CTRL] & 0x8) != 0
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x20) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x20) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x20;
        SM_StartVector(SM_VECTOR_UART3_RX);
        return;
    }
    if ((sm_device_mode[SM_DEV_TIMER_CTRL] & 0x80) != 0
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x10) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x10) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x10;
        SM_StartVector(SM_VECTOR_IPCM0);
        return;
    }
    if ((sm_device_mode[SM_DEV_TIMER_CTRL] & 0x40) != 0
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x8) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x8) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x8;
        SM_StartVector(SM_VECTOR_TIMER_X);
        return;
    }
    if ((sm_device_mode[SM_DEV_COLLISION] & 0xc0) == 0xc0)
    {
        sm_device_mode[SM_DEV_COLLISION] &= ~0x80;
        SM_StartVector(SM_VECTOR_COLLISION);
        return;
    }
    if (((sm_device_mode[SM_DEV_UART1_CTRL] & 0x10) == 0
        || (sm_cts & 1) != 0)
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x4) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x4) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x4;
        SM_StartVector(SM_VECTOR_UART1_TX);
        return;
    }
    if (((sm_device_mode[SM_DEV_UART2_CTRL] & 0x10) == 0
        || (sm_cts & 2) != 0)
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x2) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x2) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x2;
        SM_StartVector(SM_VECTOR_UART2_TX);
        return;
    }
    if (((sm_device_mode[SM_DEV_UART3_CTRL] & 0x10) == 0
        || (sm_cts & 4) != 0)
        && (sm_device_mode[SM_DEV_INT_ENABLE] & 0x1) != 0
        && (sm_device_mode[SM_DEV_INT_REQUEST] & 0x1) != 0)
    {
        sm_device_mode[SM_DEV_INT_REQUEST] &= ~0x1;
        SM_StartVector(SM_VECTOR_UART3_TX);
        return;
    }
}

void SubMcu::SM_UpdateTimer(void)
{
    while (sm_timer_cycles < sm.cycles)
    {
        if ((sm_device_mode[SM_DEV_TIMER_CTRL] & 0x20) == 0 && !this->sm.sleep)
        {
            if (sm_timer_prescaler == 0)
            {
                sm_timer_prescaler = sm_device_mode[SM_DEV_PRESCALER];

                if (sm_timer_counter == 0)
                {
                    sm_timer_counter = sm_device_mode[SM_DEV_TIMER];
                    sm_device_mode[SM_DEV_INT_REQUEST] |= 0x8;
                }
                else
                    sm_timer_counter--;
            }
            else
                sm_timer_prescaler--;
        }
        sm_timer_cycles += 16;
    }
}

void SubMcu::SM_PostUART(uint8_t data)
{
    //MCU_WorkThread_Lock();
    uart_buffer[uart_write_ptr] = data;
    uart_write_ptr = (uart_write_ptr + 1) % uart_buffer_size;
    //MCU_WorkThread_Unlock();
}

void SubMcu::SM_UpdateUART(void)
{
    if ((sm_device_mode[SM_DEV_UART1_CTRL] & 4) == 0) // RX disabled
        return;
    if (uart_write_ptr == uart_read_ptr) // no byte
        return;

    if (uart_rx_gotbyte)
        return;

    if (sm.cycles < uart_rx_delay)
        return;

    uart_rx_byte = uart_buffer[uart_read_ptr];
    uart_read_ptr = (uart_read_ptr + 1) % uart_buffer_size;
    uart_rx_gotbyte = 1;
    sm_device_mode[SM_DEV_INT_REQUEST] |= 0x40;

    uart_rx_delay = sm.cycles + 3000 * 4;
}

void SubMcu::SM_Update(uint64_t cycles)
{
    while (sm.cycles < cycles * 5)
    {
        SM_HandleInterrupt();

        if (!this->sm.sleep)
        {
            uint8_t opcode = SM_ReadAdvance();

            SM_Opcode_Table[opcode](this, opcode);
        }

        sm.cycles += 12 * 4; // FIXME
        
        SM_UpdateTimer();
        SM_UpdateUART();
    }
}
