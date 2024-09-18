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
#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"

int32_t MCU_SUB_Common(int32_t t1, int32_t t2, int32_t c_bit, uint32_t siz)
{
    int32_t st1, st2;
    int32_t N, Z, C, V = 0;
    if (siz)
    {
        st1 = (int16_t)t1;
        st2 = (int16_t)t2;
        t1 = (uint16_t)t1;
        t2 = (uint16_t)t2;
        t1 -= t2;
        t1 -= c_bit;
        C = (t1 >> 16) & 1;

        t1 &= 0xffff;
        N = (t1 & 0x8000) != 0;
        Z = t1 == 0;

        st1 -= st2;
        st1 -= c_bit;
        if (st1 < INT16_MIN || st1 > INT16_MAX)
            V = 1;
    }
    else
    {
        st1 = (int8_t)t1;
        st2 = (int8_t)t2;
        t1 = (uint8_t)t1;
        t2 = (uint8_t)t2;
        t1 -= t2;
        t1 -= c_bit;
        C = (t1 >> 8) & 1;

        t1 &= 0xff;
        N = (t1 & 0x80) != 0;
        Z = t1 == 0;

        st1 -= st2;
        st1 -= c_bit;
        if (st1 < INT8_MIN || st1 > INT8_MAX)
            V = 1;
    }
    MCU_SetStatus(N, STATUS_N);
    MCU_SetStatus(Z, STATUS_Z);
    MCU_SetStatus(C, STATUS_C);
    MCU_SetStatus(V, STATUS_V);

    return t1;
}

int32_t MCU_ADD_Common(int32_t t1, int32_t t2, int32_t c_bit, uint32_t siz)
{
    int32_t st1, st2;
    int32_t N, Z, C, V = 0;
    if (siz)
    {
        st1 = (int16_t)t1;
        st2 = (int16_t)t2;
        t1 = (uint16_t)t1;
        t2 = (uint16_t)t2;
        t1 += t2;
        t1 += c_bit;
        C = (t1 >> 16) & 1;

        t1 &= 0xffff;
        N = (t1 & 0x8000) != 0;
        Z = t1 == 0;

        st1 += st2;
        st1 += c_bit;
        if (st1 < INT16_MIN || st1 > INT16_MAX)
            V = 1;
    }
    else
    {
        st1 = (int8_t)t1;
        st2 = (int8_t)t2;
        t1 = (uint8_t)t1;
        t2 = (uint8_t)t2;
        t1 += t2;
        t1 += c_bit;
        C = (t1 >> 8) & 1;

        t1 &= 0xff;
        N = (t1 & 0x80) != 0;
        Z = t1 == 0;

        st1 += st2;
        st1 += c_bit;
        if (st1 < INT8_MIN || st1 > INT8_MAX)
            V = 1;
    }
    MCU_SetStatus(N, STATUS_N);
    MCU_SetStatus(Z, STATUS_Z);
    MCU_SetStatus(C, STATUS_C);
    MCU_SetStatus(V, STATUS_V);

    return t1;
}

void MCU_Operand_Nop(uint8_t operand)
{
}

void MCU_Operand_Sleep(uint8_t operand)
{
    mcu.sleep = 1;
}

void MCU_Operand_NotImplemented(uint8_t operand)
{
    MCU_ErrorTrap();
}

enum {
    GENERAL_DIRECT = 0,
    GENERAL_INDIRECT,
    GENERAL_ABSOLUTE,
    GENERAL_IMMEDIATE
};

enum {
    OPERAND_BYTE = 0,
    OPERAND_WORD
};

enum {
    INCREASE_NONE = 0,
    INCREASE_DECREASE,
    INCREASE_INCREASE
};

void MCU_LDM(uint8_t operand)
{
    uint8_t rlist = MCU_ReadCodeAdvance();
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        if (rlist & (1 << i))
        {
            uint16_t data = MCU_PopStack();
            if (i != 7)
                mcu.r[i] = data;
        }
    }
}

void MCU_STM(uint8_t operand)
{
    uint8_t rlist = MCU_ReadCodeAdvance();
    int32_t i;
    for (i = 7; i >= 0; i--)
    {
        if (rlist & (1 << i))
        {
            uint16_t data = mcu.r[i];
            if (i == 7)
                data -= 2;
            MCU_PushStack(data);
        }
    }
}

void MCU_TRAPA(uint8_t operand)
{
    uint32_t opcode = MCU_ReadCodeAdvance();
    if ((opcode & 0xf0) == 0x10)
    {
        MCU_Interrupt_TRAPA(opcode & 0x0f);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_LINK(uint8_t operand)
{
    if (operand == 0x17)
    {
        int16_t data = (int8_t)MCU_ReadCodeAdvance();
        MCU_PushStack(mcu.r[6]);
        mcu.r[6] = mcu.r[7];
        mcu.r[7] += data;
    }
    else if (operand == 0x1f)
    {
        uint32_t dataH = MCU_ReadCodeAdvance();
        uint32_t dataL = MCU_ReadCodeAdvance();
        MCU_ErrorTrap(); // TODO
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_UNLK(uint8_t operand)
{
    mcu.r[7] = mcu.r[6];
    mcu.r[6] = MCU_PopStack();
}

void MCU_Jump_PJSR(uint8_t operand)
{
    uint32_t ocp = mcu.cp;
    uint32_t opc = mcu.pc;
    uint8_t page = MCU_ReadCodeAdvance();
    uint16_t address;
    address = MCU_ReadCodeAdvance() << 8;
    address |= MCU_ReadCodeAdvance();
    MCU_PushStack(mcu.pc);
    MCU_PushStack(mcu.cp);
    mcu.cp = page;
    if (mcu.cp == 0x27)
        mcu.cp += 0;
    mcu.pc = address;
}

void MCU_Jump_JSR(uint8_t operand)
{
    uint16_t address;
    address = MCU_ReadCodeAdvance() << 8;
    address |= MCU_ReadCodeAdvance();
    MCU_PushStack(mcu.pc);
    mcu.pc = address;
}

void MCU_Jump_RTE(uint8_t operand)
{
    mcu.sr = MCU_PopStack();
    mcu.cp = (uint8_t)MCU_PopStack();
    mcu.pc = MCU_PopStack();
    mcu.ex_ignore = 1;
}   

void MCU_Jump_Bcc(uint8_t operand)
{
    uint16_t disp;
    uint32_t cond;
    uint32_t branch = 0;
    uint32_t N, C, Z, V;
    if (operand & 0x10)
    {
        disp = MCU_ReadCodeAdvance() << 8;
        disp |= MCU_ReadCodeAdvance();
    }
    else
    {
        disp = (int8_t)MCU_ReadCodeAdvance();
    }
    cond = operand & 0x0f;

    N = (mcu.sr & STATUS_N) != 0;
    C = (mcu.sr & STATUS_C) != 0;
    Z = (mcu.sr & STATUS_Z) != 0;
    V = (mcu.sr & STATUS_V) != 0;

    switch (cond)
    {
    case 0x0: // BRA/BT
        branch = 1;
        break;
    case 0x1: // BRN/BF
        branch = 0;
        break;
    case 0x2: // BHI
        branch = (C | Z) == 0;
        break;
    case 0x3: // BLS
        branch = (C | Z) == 1;
        break;
    case 0x4: // BCC/BHS
        branch = C == 0;
        break;
    case 0x5: // BCS/BLO
        branch = C == 1;
        break;
    case 0x6: // BNE
        branch = Z == 0;
        break;
    case 0x7: // BEQ
        branch = Z == 1;
        break;
    case 0x8: // BVC
        branch = V == 0;
        break;
    case 0x9: // BVS
        branch = V == 1;
        break;
    case 0xa: // BPL
        branch = N == 0;
        break;
    case 0xb: // BMI
        branch = N == 1;
        break;
    case 0xc: // BGE
        branch = (N ^ V) == 0;
        break;
    case 0xd: // BLT
        branch = (N ^ V) == 1;
        break;
    case 0xe: // BGT
        branch = (Z | (N ^ V)) == 0;
        break;
    case 0xf: // BLE
        branch = (Z | (N ^ V)) == 1;
        break;
    }

    if (branch)
    {
        mcu.pc += disp;
    }
}

void MCU_Jump_RTS(uint8_t operand)
{
    mcu.pc = MCU_PopStack();
}

void MCU_Jump_RTD(uint8_t operand)
{
    int16_t imm = (int8_t)MCU_ReadCodeAdvance();
    mcu.pc = MCU_PopStack();

    if (operand == 0x14)
    {
        mcu.r[7] += imm;
        if (mcu.r[7] & 1)
            MCU_ErrorTrap();
    }
    else if (operand == 0x1c)
    {
        // TODO
        MCU_ErrorTrap();
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Jump_JMP(uint8_t operand)
{
    if (operand == 0x11)
    {
        uint8_t opcode = MCU_ReadCodeAdvance();
        uint8_t opcode_h = opcode >> 3;
        uint8_t opcode_l = opcode & 0x07;
        if (opcode == 0x19)
        {
            mcu.cp = (uint8_t)MCU_PopStack();
            mcu.pc = MCU_PopStack();
        }
        else if (opcode_h == 0x18)
        {
            mcu.cp = mcu.r[opcode_l] & 0xff;
            mcu.pc = mcu.r[opcode_l + 1];
        }
        else if (opcode_h == 0x19)
        {
            MCU_PushStack(mcu.pc);
            MCU_PushStack(mcu.cp);
            opcode_l &= ~1;
            mcu.cp = mcu.r[opcode_l] & 0xff;
            mcu.pc = mcu.r[opcode_l + 1];
        }
        else if (opcode_h == 0x1a)
        {
            mcu.pc = mcu.r[opcode_l];
        }
        else if (opcode_h == 0x1b)
        {
            MCU_PushStack(mcu.pc);
            mcu.pc = mcu.r[opcode_l];
        }
        else if (opcode_h == 0x1c)
        {
            mcu.pc = mcu.r[opcode_l] + MCU_ReadCodeAdvance();
        }
        else if (opcode_h == 0x1e)
        {
            uint32_t addr;
            addr = MCU_ReadCodeAdvance() << 8;
            addr |= MCU_ReadCodeAdvance();
            mcu.pc = mcu.r[opcode_l] + addr;
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else if (operand == 0x01)
    {
        uint8_t opcode = MCU_ReadCodeAdvance();
        uint8_t reg = opcode & 0x07;
        opcode >>= 3;
        if (opcode == 0x17)
        {
            uint16_t disp = (int8_t)MCU_ReadCodeAdvance();
            mcu.r[reg]--;
            if (mcu.r[reg] != 0xffff)
            {
                mcu.pc += disp;
            }
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else if (operand == 0x10)
    {
        uint32_t addr;
        addr = MCU_ReadCodeAdvance() << 8;
        addr |= MCU_ReadCodeAdvance();
        mcu.pc = addr;
    }
    else if (operand == 0x06)
    {
        uint8_t opcode = MCU_ReadCodeAdvance();
        uint8_t reg = opcode & 0x07;
        opcode >>= 3;
        if (opcode == 0x17)
        {
            uint16_t disp = (int8_t)MCU_ReadCodeAdvance();
            uint32_t Z = (mcu.sr & STATUS_Z) != 0;
            if (Z)
            {
                mcu.r[reg]--;
                if (mcu.r[reg] != 0xffff)
                {
                    mcu.pc += disp;
                }
            }
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else if (operand == 0x07)
    {
        uint8_t opcode = MCU_ReadCodeAdvance();
        uint8_t reg = opcode & 0x07;
        opcode >>= 3;
        if (opcode == 0x17)
        {
            uint16_t disp = (int8_t)MCU_ReadCodeAdvance();
            uint32_t Z = (mcu.sr & STATUS_Z) != 0;
            if (!Z)
            {
                mcu.r[reg]--;
                if (mcu.r[reg] != 0xffff)
                {
                    mcu.pc += disp;
                }
            }
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Jump_BSR(uint8_t operand)
{
    uint16_t disp;
    if (operand == 0x0e)
    {
        disp = (int8_t)MCU_ReadCodeAdvance();
    }
    else
    {
        disp = MCU_ReadCodeAdvance() << 8;
        disp |= MCU_ReadCodeAdvance();
    }
    MCU_PushStack(mcu.pc);
    mcu.pc += disp;
}

void MCU_Jump_PJMP(uint8_t operand)
{
    uint8_t page;
    uint16_t address;
    page = MCU_ReadCodeAdvance();
    address = MCU_ReadCodeAdvance() << 8;
    address |= MCU_ReadCodeAdvance();
    mcu.cp = page;
    mcu.pc = address;
}

uint32_t operand_type;
uint16_t operand_ea;
uint8_t operand_ep;
uint8_t operand_size;
uint8_t operand_reg;
uint8_t operand_status;
uint16_t operand_data;
uint8_t opcode_extended;

uint32_t MCU_Operand_Read(void)
{
    switch (operand_type)
    {
    case GENERAL_DIRECT:
        if (operand_size)
            return mcu.r[operand_reg];
        return mcu.r[operand_reg] & 0xff;
    case GENERAL_INDIRECT:
    case GENERAL_ABSOLUTE:
        if (operand_size)
        {
            if (operand_ea & 1)
            {
                MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
            }
            return MCU_Read16(MCU_GetAddress(operand_ep, operand_ea));
        }
        return MCU_Read(MCU_GetAddress(operand_ep, operand_ea));
    case GENERAL_IMMEDIATE:
        return operand_data;
    }
    return 0;
}

void MCU_Operand_Write(uint32_t data)
{
    switch (operand_type)
    {
    case GENERAL_DIRECT:
        if (operand_size)
            mcu.r[operand_reg] = data;
        else
        {
            mcu.r[operand_reg] &= ~0xff;
            mcu.r[operand_reg] |= data & 0xff;
        }
        break;
    case GENERAL_INDIRECT:
    case GENERAL_ABSOLUTE:
        if (operand_size)
        {
            if (operand_ea & 1)
            {
                MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
            }
            MCU_Write16(MCU_GetAddress(operand_ep, operand_ea), data);
        }
        else
            MCU_Write(MCU_GetAddress(operand_ep, operand_ea), data);
        break;
    case GENERAL_IMMEDIATE:
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_INVALID_INSTRUCTION);
        break;
    }
}

void MCU_Operand_General(uint8_t operand)
{
    uint32_t type = GENERAL_DIRECT;
    uint32_t disp = 0;
    uint32_t increase = INCREASE_NONE;
    uint32_t absolute = 0;
    uint32_t reg = 0;
    uint32_t siz = OPERAND_BYTE;
    uint32_t data = 0;
    uint32_t addr = 0;
    uint32_t addrpage = 0;
    uint32_t ea = 0;
    uint32_t ep = 0;
    uint8_t opcode;
    uint8_t opcode_reg;
    if (operand & 0x08)
        siz = OPERAND_WORD;
    else
        siz = OPERAND_BYTE;
    reg = operand & 0x07;
    switch (operand & 0xf0)
    {
    case 0xa0:
        type = GENERAL_DIRECT;
        break;
    case 0xd0:
        type = GENERAL_INDIRECT;
        break;
    case 0xe0:
        type = GENERAL_INDIRECT;
        disp = (int8_t)MCU_ReadCodeAdvance();
        break;
    case 0xf0:
        type = GENERAL_INDIRECT;
        disp = MCU_ReadCodeAdvance();
        disp <<= 8;
        disp |= MCU_ReadCodeAdvance();
        break;
    case 0xb0:
        type = GENERAL_INDIRECT;
        increase = INCREASE_DECREASE;
        break;
    case 0xc0:
        type = GENERAL_INDIRECT;
        increase = INCREASE_INCREASE;
        break;
    case 0x00:
        if (reg == 5)
        {
            type = GENERAL_ABSOLUTE;
            addr = mcu.br << 8;
            addr |= MCU_ReadCodeAdvance();
            addrpage = 0;
        }
        else if (reg == 4)
        {
            type = GENERAL_IMMEDIATE;
            data = MCU_ReadCodeAdvance();
            if (siz)
            {
                data <<= 8;
                data |= MCU_ReadCodeAdvance();
            }
        }
        break;
    case 0x10:
        if (reg == 5)
        {
            type = GENERAL_ABSOLUTE;
            addr = MCU_ReadCodeAdvance() << 8;
            addr |= MCU_ReadCodeAdvance();
            addrpage = mcu.dp;
        }
        break;
    }
    if (type == GENERAL_INDIRECT)
    {
        if (increase == INCREASE_DECREASE)
        {
            if (siz || reg == 7)
            {
                mcu.r[reg] -= 2;
            }
            else
            {
                mcu.r[reg] -= 1;
            }
        }
        ea = mcu.r[reg] + disp;
        if (increase == INCREASE_INCREASE)
        {
            if (siz || reg == 7)
            {
                mcu.r[reg] += 2;
            }
            else
            {
                mcu.r[reg] += 1;
            }
        }

        ea &= 0xffff;

        ep = MCU_GetPageForRegister(reg) & 0xff;
    }
    else if (type == GENERAL_ABSOLUTE)
    {
        ea = addr & 0xffff;

        ep = addrpage & 0xff;
    }

    opcode = MCU_ReadCodeAdvance();
    opcode_extended = opcode == 0x00;
    if (opcode_extended)
    {
        opcode = MCU_ReadCodeAdvance();
    }
    opcode_reg = opcode & 0x07;
    opcode >>= 3;

    operand_type = type;
    operand_ea = ea;
    operand_ep = ep;
    operand_size = siz;
    operand_reg = reg;
    operand_data = data;
    operand_status = 0;

    MCU_Opcode_Table[opcode](opcode, opcode_reg);
}

void MCU_SetStatusCommon(uint32_t val, uint32_t siz)
{
    if (siz)
        val &= 0xffff;
    else
        val &= 0xff;
    if (siz)
        MCU_SetStatus(val & 0x8000, STATUS_N);
    else
        MCU_SetStatus(val & 0x80, STATUS_N);
    MCU_SetStatus(val == 0, STATUS_Z);
    MCU_SetStatus(0, STATUS_V);
}

void MCU_Opcode_Short_NotImplemented(uint8_t opcode)
{
    MCU_ErrorTrap();
}

void MCU_Opcode_Short_MOVE(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint8_t data = MCU_ReadCodeAdvance();
    mcu.r[reg] &= ~0xff;
    mcu.r[reg] |= data;
    MCU_SetStatusCommon(data, 0);
}

void MCU_Opcode_Short_MOVI(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint16_t data;
    data = MCU_ReadCodeAdvance() << 8;
    data |= MCU_ReadCodeAdvance();
    mcu.r[reg] = data;
    MCU_SetStatusCommon(data, 1);
}

void MCU_Opcode_Short_MOVF(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint32_t siz = (opcode & 0x08) != 0;
    int8_t disp = MCU_ReadCodeAdvance();
    uint32_t addr = (mcu.r[6] + disp) & 0xffff;
    addr |= mcu.tp << 16;
    if ((opcode & 0x10) == 0)
    {
        uint16_t data;
        if (siz)
        {
            data = MCU_Read16(addr);
            mcu.r[reg] = data;
            MCU_SetStatusCommon(data, 1);
        }
        else
        {
            data = MCU_Read(addr);
            mcu.r[reg] &= ~0xff;
            mcu.r[reg] |= data;
            MCU_SetStatusCommon(data, 0);
        }
    }
    else
    {
        uint16_t data;
        if (siz)
        {
            data = mcu.r[reg];
            MCU_Write16(addr, data);
            MCU_SetStatusCommon(data, 1);
        }
        else
        {
            data = mcu.r[reg] & 0xff;
            MCU_Write(addr, data);
            MCU_SetStatusCommon(data, 0);
        }
    }
}

void MCU_Opcode_Short_MOVL(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint32_t siz = (opcode & 0x08) != 0;
    uint16_t addr = mcu.br << 8;
    uint32_t data;
    addr |= MCU_ReadCodeAdvance();
    if (siz)
    {
        if (addr & 1)
            MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
        data = MCU_Read16(addr);
        mcu.r[reg] = data;
        MCU_SetStatusCommon(data, 1);
    }
    else
    {
        data = MCU_Read(addr);
        mcu.r[reg] &= ~0xff;
        mcu.r[reg] |= data;
        MCU_SetStatusCommon(data, 0);
    }
}

void MCU_Opcode_Short_MOVS(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint32_t siz = (opcode & 0x08) != 0;
    uint16_t addr = mcu.br << 8;
    uint32_t data;
    addr |= MCU_ReadCodeAdvance();
    if (siz)
    {
        if (addr & 1)
            MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
        data = mcu.r[reg];
        MCU_Write16(addr, data);
        MCU_SetStatusCommon(data, 1);
    }
    else
    {
        data = mcu.r[reg] & 0xff;
        MCU_Write(addr, data);
        MCU_SetStatusCommon(data, 0);
    }
}

void MCU_Opcode_Short_CMP(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint32_t siz = (opcode & 0x08) != 0;
    int32_t t1, t2;
    if (siz)
    {
        t2 = MCU_ReadCodeAdvance() << 8;
        t2 |= MCU_ReadCodeAdvance();
    }
    else
    {
        t2 = MCU_ReadCodeAdvance();
    }
    t1 = mcu.r[reg];
    MCU_SUB_Common(t1, t2, 0, siz);
}

void MCU_Opcode_NotImplemented(uint8_t opcode, uint8_t opcode_reg)
{
    MCU_ErrorTrap();
}

void MCU_Opcode_MOVG_Immediate(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data;
    if (opcode_reg == 6 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE))
    {
        data = (int8_t)MCU_ReadCodeAdvance();
        MCU_Operand_Write(data);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 7 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE))
    {
        data = MCU_ReadCodeAdvance() << 8;
        data |= MCU_ReadCodeAdvance();
        MCU_Operand_Write(data);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 4 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE) && operand_size == OPERAND_BYTE)
    {
        uint32_t t1 = MCU_Operand_Read();
        uint32_t t2 = MCU_ReadCodeAdvance();
        MCU_SUB_Common(t1, t2, 0, OPERAND_BYTE);
    }
    else if (opcode_reg == 4 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE) && operand_size == OPERAND_WORD) // FIXME
    {
        uint32_t t1 = MCU_Operand_Read();
        uint32_t t2 = (uint16_t)((int8_t)MCU_ReadCodeAdvance());
        MCU_SUB_Common(t1, t2, 0, OPERAND_WORD);
    }
    else if (opcode_reg == 5 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE) && operand_size == OPERAND_WORD)
    {
        uint32_t t1, t2;
        t1 = MCU_Operand_Read();
        t2 = MCU_ReadCodeAdvance() << 8;
        t2 |= MCU_ReadCodeAdvance();
        MCU_SUB_Common(t1, t2, 0, OPERAND_WORD);
    }
    else if (opcode_reg == 5 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE) && operand_size == OPERAND_BYTE) // FIXME
    {
        uint32_t t1, t2;
        t1 = MCU_Operand_Read();
        t2 = MCU_ReadCodeAdvance() << 8;
        t2 |= MCU_ReadCodeAdvance();
        MCU_SUB_Common(t1, t2, 0, OPERAND_BYTE);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_BSET_ORC(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type == GENERAL_IMMEDIATE) // ORC
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t val = MCU_ControlRegisterRead(opcode_reg, operand_size);
        val |= data;
        MCU_ControlRegisterWrite(opcode_reg, operand_size, val);
        if (opcode_reg >= 2)
        {
            MCU_SetStatusCommon(val, operand_size);
        }
        mcu.ex_ignore = 1;
    }
    else // BSET
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = mcu.r[opcode_reg] & 0x0f;
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
        data |= 1 << bit;
        MCU_Operand_Write(data);
    }
}

void MCU_Opcode_BCLR_ANDC(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type == GENERAL_IMMEDIATE) // ANDC
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t val = MCU_ControlRegisterRead(opcode_reg, operand_size);
        val &= data;
        MCU_ControlRegisterWrite(opcode_reg, operand_size, val);
        if (opcode_reg >= 2)
        {
            MCU_SetStatusCommon(val, operand_size);
        }
        mcu.ex_ignore = 1;
    }
    else // BCLR
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = mcu.r[opcode_reg] & 0x0f;
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
        data &= ~(1 << bit);
        MCU_Operand_Write(data);
    }
}

void MCU_Opcode_BTST(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type != GENERAL_IMMEDIATE)
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = mcu.r[opcode_reg] & 0x0f;
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_CLR(uint8_t opcode, uint8_t opcode_reg)
{
    if (opcode_reg == 3 && operand_type != GENERAL_IMMEDIATE) // CLR
    {
        MCU_Operand_Write(0);
        MCU_SetStatus(0, STATUS_N);
        MCU_SetStatus(1, STATUS_Z);
        MCU_SetStatus(0, STATUS_V);
        MCU_SetStatus(0, STATUS_C);
    }
    else if (opcode_reg == 6 && operand_type != GENERAL_IMMEDIATE) // TST
    {
        uint32_t data = MCU_Operand_Read();
        MCU_SetStatusCommon(data, operand_size);
        MCU_SetStatus(0, STATUS_C);
    }
    else if (opcode_reg == 2 && operand_type == GENERAL_DIRECT && operand_size == 0) // EXTU
    {
        uint32_t data = (uint8_t)mcu.r[operand_reg];
        mcu.r[operand_reg] = data;
        MCU_SetStatus(0, STATUS_N);
        MCU_SetStatus(data == 0, STATUS_Z);
        MCU_SetStatus(0, STATUS_V);
        MCU_SetStatus(0, STATUS_C);
    }
    else if (opcode_reg == 0 && operand_type == GENERAL_DIRECT && operand_size == 0) // SWAP
    {
        uint32_t data = mcu.r[operand_reg];
        uint32_t data_h = data >> 8;
        uint32_t data_l = data & 0xff;
        data = (data_l << 8) | data_h;
        mcu.r[operand_reg] = data;
        MCU_SetStatusCommon(data, OPERAND_WORD);
    }
    else if (opcode_reg == 5 && operand_type != GENERAL_IMMEDIATE) // NOT
    {
        uint32_t data = MCU_Operand_Read();
        data = ~data;
        MCU_Operand_Write(data);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 4 && operand_type != GENERAL_IMMEDIATE) // NEG
    {
        uint32_t data = MCU_Operand_Read();
        data = MCU_SUB_Common(0, data, 0, operand_size);
        MCU_Operand_Write(data);
    }
    else if (opcode_reg == 1 && operand_type == GENERAL_DIRECT && operand_size == 0) // EXTS
    {
        uint32_t data = mcu.r[operand_reg];
        mcu.r[operand_reg] = (int8_t)data;
        MCU_SetStatusCommon(data, OPERAND_WORD);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_LDC(uint8_t opcode, uint8_t opcode_reg)
{
    // FIXME: Check also other cases
    if (operand_reg == 7 && opcode_reg == 4)
    {
        operand_size = 1;
        uint32_t data = MCU_Operand_Read();
        MCU_ControlRegisterWrite(4, 0, data & 0xff);
        MCU_ControlRegisterWrite(5, 0, data >> 8);
    }
    else
    {
        uint32_t data = MCU_Operand_Read();
        MCU_ControlRegisterWrite(opcode_reg, operand_size, data);
    }
    mcu.ex_ignore = 1;
}

void MCU_Opcode_STC(uint8_t opcode, uint8_t opcode_reg)
{
    // FIXME: Check also other cases
    if (operand_reg == 7 && opcode_reg == 4)
    {
        operand_size = 1;
        uint32_t dataL = MCU_ControlRegisterRead(4, 0);
        uint32_t dataH = MCU_ControlRegisterRead(5, 0);
        MCU_Operand_Write(dataL | dataH << 8);
    }
    else
    {
        uint32_t data = MCU_ControlRegisterRead(opcode_reg, operand_size);
        MCU_Operand_Write(data);
    }
}

void MCU_Opcode_BSET(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type != GENERAL_IMMEDIATE)
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = opcode_reg | ((opcode & 1) << 3);
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
        data |= 1 << bit;
        MCU_Operand_Write(data);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_BCLR(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type != GENERAL_IMMEDIATE)
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = opcode_reg | ((opcode & 1) << 3);
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
        data &= ~(1 << bit);
        MCU_Operand_Write(data);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_MOVG(uint8_t opcode, uint8_t opcode_reg)
{
    if (opcode_extended)
    {
        if (opcode == 0x12)
        {
            // FIXME
            MCU_ErrorTrap();
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else
    {
        uint8_t d = (opcode & 2) != 0;
        uint32_t data;
        if (d)
        {
            if (operand_type == GENERAL_DIRECT) // XCH
            {
                if (operand_size)
                {
                    uint32_t r1 = mcu.r[opcode_reg];
                    uint32_t r2 = mcu.r[operand_reg];
                    mcu.r[opcode_reg] = r2;
                    mcu.r[operand_reg] = r1;
                }
                else
                {
                    MCU_ErrorTrap();
                }
            }
            else
            {
                data = mcu.r[opcode_reg];
                MCU_Operand_Write(data);
                MCU_SetStatusCommon(data, operand_size);
            }
        }
        else
        {
            data = MCU_Operand_Read();
            if (operand_size)
                mcu.r[opcode_reg] = data;
            else
            {
                mcu.r[opcode_reg] &= ~0xff;
                mcu.r[opcode_reg] |= data & 0xff;
            }
            MCU_SetStatusCommon(data, operand_size);
        }
    }
}

void MCU_Opcode_BTSTI(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type != GENERAL_IMMEDIATE)
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = opcode_reg | ((opcode & 1) << 3);
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_BNOTI(uint8_t opcode, uint8_t opcode_reg)
{
    if (operand_type != GENERAL_IMMEDIATE)
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = opcode_reg | ((opcode & 1) << 3);
        MCU_SetStatus((data & (1 << bit)) == 0, STATUS_Z);
        data ^= (1 << bit);
        MCU_Operand_Write(data); 
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_OR(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data = MCU_Operand_Read();
    mcu.r[opcode_reg] |= data;
    MCU_SetStatusCommon(mcu.r[opcode_reg], operand_size);
}

void MCU_Opcode_CMP(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    MCU_SUB_Common(t1, t2, 0, operand_size);
}

void MCU_Opcode_ADDQ(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = MCU_Operand_Read();
    int32_t t2 = 0;
    switch (opcode_reg)
    {
    case 0:
        t2 = 1;
        break;
    case 1:
        t2 = 2;
        break;
    case 4:
        t2 = -1;
        break;
    case 5:
        t2 = -2;
        break;
    default:
        MCU_ErrorTrap();
        break;
    }
    t1 = MCU_ADD_Common(t1, t2, 0, operand_size);
    MCU_Operand_Write(t1);
}

void MCU_Opcode_ADD(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    t1 = MCU_ADD_Common(t1, t2, 0, operand_size);
    if (operand_size)
        mcu.r[opcode_reg] = t1;
    else
    {
        mcu.r[opcode_reg] &= ~0xff;
        mcu.r[opcode_reg] |= t1 & 0xff;
    }
}

void MCU_Opcode_SUB(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    t1 = MCU_SUB_Common(t1, t2, 0, operand_size);
    if (operand_size)
        mcu.r[opcode_reg] = t1;
    else
    {
        mcu.r[opcode_reg] &= ~0xff;
        mcu.r[opcode_reg] |= t1 & 0xff;
    }
}

void MCU_Opcode_SUBS(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    if (operand_size)
        mcu.r[opcode_reg] = t1 - t2;
    else
        mcu.r[opcode_reg] = t1 - (int8_t)t2;
}

void MCU_Opcode_AND(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data = mcu.r[opcode_reg];
    data &= MCU_Operand_Read();
    if (operand_size)
        mcu.r[opcode_reg] = data;
    else
    {
        mcu.r[opcode_reg] &= ~0xff;
        mcu.r[opcode_reg] |= data & 0xff;
    }
    MCU_SetStatusCommon(mcu.r[opcode_reg], operand_size);
}

void MCU_Opcode_SHLR(uint8_t opcode, uint8_t opcode_reg)
{
    if (opcode_reg == 0x03 && operand_type != GENERAL_IMMEDIATE) // SHLR
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C = data & 1;
        data >>= 1;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x02 && operand_type != GENERAL_IMMEDIATE) // SHLL
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C;
        if (operand_size)
            C = (data & 0x8000) != 0;
        else
            C = (data & 0x80) != 0;
        data <<= 1;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x06 && operand_type != GENERAL_IMMEDIATE) // ROTXL
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = (mcu.sr & STATUS_C) != 0;
        uint32_t C;
        if (operand_size)
            C = (data & 0x8000) != 0;
        else
            C = (data & 0x80) != 0;
        data <<= 1;
        data |= bit;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x07 && operand_type != GENERAL_IMMEDIATE) // ROTXR
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t bit = (mcu.sr & STATUS_C) != 0;
        uint32_t C = data & 0x1;
        data >>= 1;
        if (operand_size)
            data |= bit << 15;
        else
            data |= bit << 7;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x04 && operand_type != GENERAL_IMMEDIATE) // ROTL
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C;
        if (operand_size)
            C = (data & 0x8000) != 0;
        else
            C = (data & 0x80) != 0;
        data <<= 1;
        data |= C;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x00 && operand_type != GENERAL_IMMEDIATE) // SHAL
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C;
        if (operand_size)
            C = (data & 0x8000) != 0;
        else
            C = (data & 0x80) != 0;
        data <<= 1;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x01 && operand_type != GENERAL_IMMEDIATE) // SHAR
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C = data & 0x1;
        uint32_t msb;
        if (operand_size)
        {
            msb = data & 0x8000;
            data &= 0x7fff;
        }
        else
        {
            msb = data & 0x80;
            data &= 0x7f;
        }
        data >>= 1;
        data |= msb;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 0x05 && operand_type != GENERAL_IMMEDIATE) // ROTR
    {
        uint32_t data = MCU_Operand_Read();
        uint32_t C = (data & 0x1) != 0;
        data >>= 1;
        if (operand_size)
            data |= C << 15;
        else
            data |= C << 7;
        MCU_Operand_Write(data);
        MCU_SetStatus(C, STATUS_C);
        MCU_SetStatusCommon(data, operand_size);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_MULXU(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t t1 = MCU_Operand_Read();
    uint32_t t2 = mcu.r[opcode_reg];
    uint32_t N, Z;
    if (!operand_size)
        t2 &= 0xff;
    t1 *= t2;

    if (operand_size)
    {
        opcode_reg &= ~1;
        mcu.r[opcode_reg | 0] = t1 >> 16;
        mcu.r[opcode_reg | 1] = t1;
        N = (t1 & 0x80000000UL) != 0; // FIXME
    }
    else
    {
        t1 &= 0xffff;
        mcu.r[opcode_reg] = t1;
        N = (t1 & 0x8000UL) != 0; // FIXME
    }
    Z = t1 == 0;
    MCU_SetStatus(N, STATUS_N);
    MCU_SetStatus(Z, STATUS_Z);
    MCU_SetStatus(0, STATUS_V);
    MCU_SetStatus(0, STATUS_C);
}

void MCU_Opcode_DIVXU(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t t1 = MCU_Operand_Read();
    uint32_t t2;
    uint32_t R, Q;

    if (!t1)
    {
        MCU_ErrorTrap(); // FIXME: implement proper exception
        MCU_SetStatus(0, STATUS_N);
        MCU_SetStatus(1, STATUS_Z);
        MCU_SetStatus(0, STATUS_V);
        MCU_SetStatus(0, STATUS_C);
        return;
    }

    if (operand_size)
    {
        opcode_reg &= ~1;
        t2 = mcu.r[opcode_reg | 0] << 16;
        t2 |= mcu.r[opcode_reg | 1];

        R = t2 % t1;
        Q = t2 / t1;

        if (Q > UINT16_MAX)
        {
            MCU_SetStatus(0, STATUS_N);
            MCU_SetStatus(0, STATUS_Z);
            MCU_SetStatus(1, STATUS_V);
            MCU_SetStatus(0, STATUS_C);
        }
        else
        {
            mcu.r[opcode_reg | 0] = R;
            mcu.r[opcode_reg | 1] = Q;
            MCU_SetStatusCommon(Q, OPERAND_WORD);
            MCU_SetStatus(0, STATUS_C);
        }
    }
    else
    {
        t2 = mcu.r[opcode_reg];

        R = t2 % t1;
        Q = t2 / t1;

        if (Q > UINT8_MAX)
        {
            MCU_SetStatus(0, STATUS_N);
            MCU_SetStatus(0, STATUS_Z);
            MCU_SetStatus(1, STATUS_V);
            MCU_SetStatus(0, STATUS_C);
        }
        else
        {
            R &= 0xff;
            Q &= 0xff;
            mcu.r[opcode_reg] = (R << 8) | Q;
            MCU_SetStatusCommon(Q, OPERAND_BYTE);
            MCU_SetStatus(0, STATUS_C);
        }
    }
}

void MCU_Opcode_ADDS(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data = MCU_Operand_Read();
    if (!operand_size)
        data = (int8_t)data;
    mcu.r[opcode_reg] += data;
}

void MCU_Opcode_XOR(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data = MCU_Operand_Read();
    mcu.r[opcode_reg] ^= data;
    MCU_SetStatusCommon(mcu.r[opcode_reg], operand_size);
}

void MCU_Opcode_ADDX(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    int32_t C = (mcu.sr & STATUS_C) != 0;
    int32_t Z = (mcu.sr & STATUS_Z) != 0;
    t1 = MCU_ADD_Common(t1, t2, C, operand_size);
    if (!Z)
        MCU_SetStatus(0, STATUS_Z);
        
    if (operand_size)
        mcu.r[opcode_reg] = t1;
    else
    {
        mcu.r[opcode_reg] &= ~0xff;
        mcu.r[opcode_reg] |= t1 & 0xff;
    }
}

void MCU_Opcode_SUBX(uint8_t opcode, uint8_t opcode_reg)
{
    int32_t t1 = mcu.r[opcode_reg];
    int32_t t2 = MCU_Operand_Read();
    int32_t C = (mcu.sr & STATUS_C) != 0;
    t1 = MCU_SUB_Common(t1, t2, C, operand_size);
    if (operand_size)
        mcu.r[opcode_reg] = t1;
    else
    {
        mcu.r[opcode_reg] &= ~0xff;
        mcu.r[opcode_reg] |= t1 & 0xff;
    }
}

void (*MCU_Operand_Table[256])(uint8_t operand) = {
    MCU_Operand_Nop, // 00
    MCU_Jump_JMP, // 01
    MCU_LDM, // 02
    MCU_Jump_PJSR, // 03
    MCU_Operand_General, // 04
    MCU_Operand_General, // 05
    MCU_Jump_JMP, // 06
    MCU_Jump_JMP, // 07
    MCU_TRAPA, // 08
    MCU_Operand_NotImplemented, // 09
    MCU_Jump_RTE, // 0A
    MCU_Operand_NotImplemented, // 0B
    MCU_Operand_General, // 0C
    MCU_Operand_General, // 0D
    MCU_Jump_BSR, // 0E
    MCU_UNLK, // 0F
    MCU_Jump_JMP, // 10
    MCU_Jump_JMP, // 11
    MCU_STM, // 12
    MCU_Jump_PJMP, // 13
    MCU_Jump_RTD, // 14
    MCU_Operand_General, // 15
    MCU_Operand_NotImplemented, // 16
    MCU_LINK, // 17
    MCU_Jump_JSR, // 18
    MCU_Jump_RTS, // 19
    MCU_Operand_Sleep, // 1A
    MCU_Operand_NotImplemented, // 1B
    MCU_Jump_RTD, // 1C
    MCU_Operand_General, // 1D
    MCU_Jump_BSR, // 1E
    MCU_LINK, // 1F
    MCU_Jump_Bcc, // 20
    MCU_Jump_Bcc, // 21
    MCU_Jump_Bcc, // 22
    MCU_Jump_Bcc, // 23
    MCU_Jump_Bcc, // 24
    MCU_Jump_Bcc, // 25
    MCU_Jump_Bcc, // 26
    MCU_Jump_Bcc, // 27
    MCU_Jump_Bcc, // 28
    MCU_Jump_Bcc, // 29
    MCU_Jump_Bcc, // 2A
    MCU_Jump_Bcc, // 2B
    MCU_Jump_Bcc, // 2C
    MCU_Jump_Bcc, // 2D
    MCU_Jump_Bcc, // 2E
    MCU_Jump_Bcc, // 2F
    MCU_Jump_Bcc, // 30
    MCU_Jump_Bcc, // 31
    MCU_Jump_Bcc, // 32
    MCU_Jump_Bcc, // 33
    MCU_Jump_Bcc, // 34
    MCU_Jump_Bcc, // 35
    MCU_Jump_Bcc, // 36
    MCU_Jump_Bcc, // 37
    MCU_Jump_Bcc, // 38
    MCU_Jump_Bcc, // 39
    MCU_Jump_Bcc, // 3A
    MCU_Jump_Bcc, // 3B
    MCU_Jump_Bcc, // 3C
    MCU_Jump_Bcc, // 3D
    MCU_Jump_Bcc, // 3E
    MCU_Jump_Bcc, // 3F
    MCU_Opcode_Short_CMP, // 40
    MCU_Opcode_Short_CMP, // 41
    MCU_Opcode_Short_CMP, // 42
    MCU_Opcode_Short_CMP, // 43
    MCU_Opcode_Short_CMP, // 44
    MCU_Opcode_Short_CMP, // 45
    MCU_Opcode_Short_CMP, // 46
    MCU_Opcode_Short_CMP, // 47
    MCU_Opcode_Short_CMP, // 48
    MCU_Opcode_Short_CMP, // 49
    MCU_Opcode_Short_CMP, // 4A
    MCU_Opcode_Short_CMP, // 4B
    MCU_Opcode_Short_CMP, // 4C
    MCU_Opcode_Short_CMP, // 4D
    MCU_Opcode_Short_CMP, // 4E
    MCU_Opcode_Short_CMP, // 4F
    MCU_Opcode_Short_MOVE, // 50
    MCU_Opcode_Short_MOVE, // 51
    MCU_Opcode_Short_MOVE, // 52
    MCU_Opcode_Short_MOVE, // 53
    MCU_Opcode_Short_MOVE, // 54
    MCU_Opcode_Short_MOVE, // 55
    MCU_Opcode_Short_MOVE, // 56
    MCU_Opcode_Short_MOVE, // 57
    MCU_Opcode_Short_MOVI, // 58
    MCU_Opcode_Short_MOVI, // 59
    MCU_Opcode_Short_MOVI, // 5A
    MCU_Opcode_Short_MOVI, // 5B
    MCU_Opcode_Short_MOVI, // 5C
    MCU_Opcode_Short_MOVI, // 5D
    MCU_Opcode_Short_MOVI, // 5E
    MCU_Opcode_Short_MOVI, // 5F
    MCU_Opcode_Short_MOVL, // 60
    MCU_Opcode_Short_MOVL, // 61
    MCU_Opcode_Short_MOVL, // 62
    MCU_Opcode_Short_MOVL, // 63
    MCU_Opcode_Short_MOVL, // 64
    MCU_Opcode_Short_MOVL, // 65
    MCU_Opcode_Short_MOVL, // 66
    MCU_Opcode_Short_MOVL, // 67
    MCU_Opcode_Short_MOVL, // 68
    MCU_Opcode_Short_MOVL, // 69
    MCU_Opcode_Short_MOVL, // 6A
    MCU_Opcode_Short_MOVL, // 6B
    MCU_Opcode_Short_MOVL, // 6C
    MCU_Opcode_Short_MOVL, // 6D
    MCU_Opcode_Short_MOVL, // 6E
    MCU_Opcode_Short_MOVL, // 6F
    MCU_Opcode_Short_MOVS, // 70
    MCU_Opcode_Short_MOVS, // 71
    MCU_Opcode_Short_MOVS, // 72
    MCU_Opcode_Short_MOVS, // 73
    MCU_Opcode_Short_MOVS, // 74
    MCU_Opcode_Short_MOVS, // 75
    MCU_Opcode_Short_MOVS, // 76
    MCU_Opcode_Short_MOVS, // 77
    MCU_Opcode_Short_MOVS, // 78
    MCU_Opcode_Short_MOVS, // 79
    MCU_Opcode_Short_MOVS, // 7A
    MCU_Opcode_Short_MOVS, // 7B
    MCU_Opcode_Short_MOVS, // 7C
    MCU_Opcode_Short_MOVS, // 7D
    MCU_Opcode_Short_MOVS, // 7E
    MCU_Opcode_Short_MOVS, // 7F
    MCU_Opcode_Short_MOVF, // 80
    MCU_Opcode_Short_MOVF, // 81
    MCU_Opcode_Short_MOVF, // 82
    MCU_Opcode_Short_MOVF, // 83
    MCU_Opcode_Short_MOVF, // 84
    MCU_Opcode_Short_MOVF, // 85
    MCU_Opcode_Short_MOVF, // 86
    MCU_Opcode_Short_MOVF, // 87
    MCU_Opcode_Short_MOVF, // 88
    MCU_Opcode_Short_MOVF, // 89
    MCU_Opcode_Short_MOVF, // 8A
    MCU_Opcode_Short_MOVF, // 8B
    MCU_Opcode_Short_MOVF, // 8C
    MCU_Opcode_Short_MOVF, // 8D
    MCU_Opcode_Short_MOVF, // 8E
    MCU_Opcode_Short_MOVF, // 8F
    MCU_Opcode_Short_MOVF, // 90
    MCU_Opcode_Short_MOVF, // 91
    MCU_Opcode_Short_MOVF, // 92
    MCU_Opcode_Short_MOVF, // 93
    MCU_Opcode_Short_MOVF, // 94
    MCU_Opcode_Short_MOVF, // 95
    MCU_Opcode_Short_MOVF, // 96
    MCU_Opcode_Short_MOVF, // 97
    MCU_Opcode_Short_MOVF, // 98
    MCU_Opcode_Short_MOVF, // 99
    MCU_Opcode_Short_MOVF, // 9A
    MCU_Opcode_Short_MOVF, // 9B
    MCU_Opcode_Short_MOVF, // 9C
    MCU_Opcode_Short_MOVF, // 9D
    MCU_Opcode_Short_MOVF, // 9E
    MCU_Opcode_Short_MOVF, // 9F
    MCU_Operand_General, // A0
    MCU_Operand_General, // A1
    MCU_Operand_General, // A2
    MCU_Operand_General, // A3
    MCU_Operand_General, // A4
    MCU_Operand_General, // A5
    MCU_Operand_General, // A6
    MCU_Operand_General, // A7
    MCU_Operand_General, // A8
    MCU_Operand_General, // A9
    MCU_Operand_General, // AA
    MCU_Operand_General, // AB
    MCU_Operand_General, // AC
    MCU_Operand_General, // AD
    MCU_Operand_General, // AE
    MCU_Operand_General, // AF
    MCU_Operand_General, // B0
    MCU_Operand_General, // B1
    MCU_Operand_General, // B2
    MCU_Operand_General, // B3
    MCU_Operand_General, // B4
    MCU_Operand_General, // B5
    MCU_Operand_General, // B6
    MCU_Operand_General, // B7
    MCU_Operand_General, // B8
    MCU_Operand_General, // B9
    MCU_Operand_General, // BA
    MCU_Operand_General, // BB
    MCU_Operand_General, // BC
    MCU_Operand_General, // BD
    MCU_Operand_General, // BE
    MCU_Operand_General, // BF
    MCU_Operand_General, // C0
    MCU_Operand_General, // C1
    MCU_Operand_General, // C2
    MCU_Operand_General, // C3
    MCU_Operand_General, // C4
    MCU_Operand_General, // C5
    MCU_Operand_General, // C6
    MCU_Operand_General, // C7
    MCU_Operand_General, // C8
    MCU_Operand_General, // C9
    MCU_Operand_General, // CA
    MCU_Operand_General, // CB
    MCU_Operand_General, // CC
    MCU_Operand_General, // CD
    MCU_Operand_General, // CE
    MCU_Operand_General, // CF
    MCU_Operand_General, // D0
    MCU_Operand_General, // D1
    MCU_Operand_General, // D2
    MCU_Operand_General, // D3
    MCU_Operand_General, // D4
    MCU_Operand_General, // D5
    MCU_Operand_General, // D6
    MCU_Operand_General, // D7
    MCU_Operand_General, // D8
    MCU_Operand_General, // D9
    MCU_Operand_General, // DA
    MCU_Operand_General, // DB
    MCU_Operand_General, // DC
    MCU_Operand_General, // DD
    MCU_Operand_General, // DE
    MCU_Operand_General, // DF
    MCU_Operand_General, // E0
    MCU_Operand_General, // E1
    MCU_Operand_General, // E2
    MCU_Operand_General, // E3
    MCU_Operand_General, // E4
    MCU_Operand_General, // E5
    MCU_Operand_General, // E6
    MCU_Operand_General, // E7
    MCU_Operand_General, // E8
    MCU_Operand_General, // E9
    MCU_Operand_General, // EA
    MCU_Operand_General, // EB
    MCU_Operand_General, // EC
    MCU_Operand_General, // ED
    MCU_Operand_General, // EE
    MCU_Operand_General, // EF
    MCU_Operand_General, // F0
    MCU_Operand_General, // F1
    MCU_Operand_General, // F2
    MCU_Operand_General, // F3
    MCU_Operand_General, // F4
    MCU_Operand_General, // F5
    MCU_Operand_General, // F6
    MCU_Operand_General, // F7
    MCU_Operand_General, // F8
    MCU_Operand_General, // F9
    MCU_Operand_General, // FA
    MCU_Operand_General, // FB
    MCU_Operand_General, // FC
    MCU_Operand_General, // FD
    MCU_Operand_General, // FE
    MCU_Operand_General, // FF
};

void (*MCU_Opcode_Table[32])(uint8_t opcode, uint8_t opcode_reg) = {
    MCU_Opcode_MOVG_Immediate, // 00
    MCU_Opcode_ADDQ, // 01
    MCU_Opcode_CLR, // 02
    MCU_Opcode_SHLR, // 03
    MCU_Opcode_ADD, // 04
    MCU_Opcode_ADDS, // 05
    MCU_Opcode_SUB, // 06
    MCU_Opcode_SUBS, // 07
    MCU_Opcode_OR, // 08
    MCU_Opcode_BSET_ORC, // 09
    MCU_Opcode_AND, // 0A
    MCU_Opcode_BCLR_ANDC, // 0B
    MCU_Opcode_XOR, // 0C
    MCU_Opcode_NotImplemented, // 0D
    MCU_Opcode_CMP, // 0E
    MCU_Opcode_BTST, // 0F
    MCU_Opcode_MOVG, // 10
    MCU_Opcode_LDC, // 11
    MCU_Opcode_MOVG, // 12
    MCU_Opcode_STC, // 13
    MCU_Opcode_ADDX, // 14
    MCU_Opcode_MULXU, // 15
    MCU_Opcode_SUBX, // 16
    MCU_Opcode_DIVXU, // 17
    MCU_Opcode_BSET, // 18
    MCU_Opcode_BSET, // 19
    MCU_Opcode_BCLR, // 1A
    MCU_Opcode_BCLR, // 1B
    MCU_Opcode_BNOTI, // 1C
    MCU_Opcode_BNOTI, // 1D
    MCU_Opcode_BTSTI, // 1E
    MCU_Opcode_BTSTI, // 1F
};
