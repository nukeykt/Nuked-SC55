#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"

void MCU_Operand_Nop(uint8_t operand)
{
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
                MCU_Interrupt_Request(INTERRUPT_SOURCE_ADDRESS_ERROR);
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
                MCU_Interrupt_Request(INTERRUPT_SOURCE_ADDRESS_ERROR);
            }
            MCU_Write16(MCU_GetAddress(operand_ep, operand_ea), data);
        }
        else
            MCU_Write(MCU_GetAddress(operand_ep, operand_ea), data);
        break;
    case GENERAL_IMMEDIATE:
        MCU_Interrupt_Request(INTERRUPT_SOURCE_INVALID_INSTRUCTION);
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

void MCU_Opcode_Short_MOVI(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint16_t data;
    data = MCU_ReadCodeAdvance() << 8;
    data |= MCU_ReadCodeAdvance();
    mcu.r[reg] = data;
    MCU_SetStatusCommon(data, 1);
}

void MCU_Opcode_Short_MOVL(uint8_t opcode)
{
    uint32_t reg = opcode & 0x07;
    uint32_t siz = (opcode & 0x08) != 0;
    uint16_t addr = mcu.br << 8;
    addr |= MCU_ReadCodeAdvance();
    if (siz)
    {
        if (addr & 1)
            MCU_Interrupt_Request(INTERRUPT_SOURCE_ADDRESS_ERROR);
        mcu.r[reg] = MCU_Read16(addr);
    }
    else
    {
        mcu.r[reg] &= ~0xff;
        mcu.r[reg] |= MCU_Read(addr);
    }
}

void MCU_Opcode_NotImplemented(uint8_t opcode, uint8_t opcode_reg)
{
    MCU_ErrorTrap();
}

void MCU_Opcode_MOVG_Immediate(uint8_t opcode, uint8_t opcode_reg)
{
    if (opcode_reg == 6 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE))
    {
        uint32_t data;
        data = (int8_t)MCU_ReadCodeAdvance();
        MCU_Operand_Write(data);
        MCU_SetStatusCommon(data, operand_size);
    }
    else if (opcode_reg == 7 && (operand_type == GENERAL_INDIRECT || operand_type == GENERAL_ABSOLUTE))
    {
        uint32_t data;
        data = MCU_ReadCodeAdvance() << 8;
        data |= MCU_ReadCodeAdvance();
        MCU_Operand_Write(data);
        MCU_SetStatusCommon(data, operand_size);
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
        if (!mcu.exmode)
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
        if (!mcu.exmode)
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

void MCU_Opcode_CLR(uint8_t opcode, uint8_t opcode_reg)
{
    if (opcode_reg == 3 && operand_type != GENERAL_IMMEDIATE)
    {
        MCU_Operand_Write(0);
        MCU_SetStatus(0, STATUS_N);
        MCU_SetStatus(1, STATUS_Z);
        MCU_SetStatus(0, STATUS_V);
        MCU_SetStatus(0, STATUS_C);
    }
    else
    {
        MCU_ErrorTrap();
    }
}

void MCU_Opcode_LDC(uint8_t opcode, uint8_t opcode_reg)
{
    uint32_t data = MCU_Operand_Read();
    MCU_ControlRegisterWrite(opcode_reg, operand_size, data);
    if (!mcu.exmode)
        mcu.ex_ignore = 1;
}

void (*MCU_Operand_Table[256])(uint8_t operand) = {
    MCU_Operand_Nop, // 00
    MCU_Operand_NotImplemented, // 01
    MCU_Operand_NotImplemented, // 02
    MCU_Operand_NotImplemented, // 03
    MCU_Operand_General, // 04
    MCU_Operand_General, // 05
    MCU_Operand_NotImplemented, // 06
    MCU_Operand_NotImplemented, // 07
    MCU_Operand_NotImplemented, // 08
    MCU_Operand_NotImplemented, // 09
    MCU_Operand_NotImplemented, // 0A
    MCU_Operand_NotImplemented, // 0B
    MCU_Operand_General, // 0C
    MCU_Operand_General, // 0D
    MCU_Operand_NotImplemented, // 0E
    MCU_Operand_NotImplemented, // 0F
    MCU_Operand_NotImplemented, // 10
    MCU_Operand_NotImplemented, // 11
    MCU_Operand_NotImplemented, // 12
    MCU_Operand_NotImplemented, // 13
    MCU_Operand_NotImplemented, // 14
    MCU_Operand_General, // 15
    MCU_Operand_NotImplemented, // 16
    MCU_Operand_NotImplemented, // 17
    MCU_Operand_NotImplemented, // 18
    MCU_Operand_NotImplemented, // 19
    MCU_Operand_NotImplemented, // 1A
    MCU_Operand_NotImplemented, // 1B
    MCU_Operand_NotImplemented, // 1C
    MCU_Operand_General, // 1D
    MCU_Operand_NotImplemented, // 1E
    MCU_Operand_NotImplemented, // 1F
    MCU_Operand_NotImplemented, // 20
    MCU_Operand_NotImplemented, // 21
    MCU_Operand_NotImplemented, // 22
    MCU_Operand_NotImplemented, // 23
    MCU_Operand_NotImplemented, // 24
    MCU_Operand_NotImplemented, // 25
    MCU_Operand_NotImplemented, // 26
    MCU_Operand_NotImplemented, // 27
    MCU_Operand_NotImplemented, // 28
    MCU_Operand_NotImplemented, // 29
    MCU_Operand_NotImplemented, // 2A
    MCU_Operand_NotImplemented, // 2B
    MCU_Operand_NotImplemented, // 2C
    MCU_Operand_NotImplemented, // 2D
    MCU_Operand_NotImplemented, // 2E
    MCU_Operand_NotImplemented, // 2F
    MCU_Operand_NotImplemented, // 30
    MCU_Operand_NotImplemented, // 31
    MCU_Operand_NotImplemented, // 32
    MCU_Operand_NotImplemented, // 33
    MCU_Operand_NotImplemented, // 34
    MCU_Operand_NotImplemented, // 35
    MCU_Operand_NotImplemented, // 36
    MCU_Operand_NotImplemented, // 37
    MCU_Operand_NotImplemented, // 38
    MCU_Operand_NotImplemented, // 39
    MCU_Operand_NotImplemented, // 3A
    MCU_Operand_NotImplemented, // 3B
    MCU_Operand_NotImplemented, // 3C
    MCU_Operand_NotImplemented, // 3D
    MCU_Operand_NotImplemented, // 3E
    MCU_Operand_NotImplemented, // 3F
    MCU_Operand_NotImplemented, // 40
    MCU_Operand_NotImplemented, // 41
    MCU_Operand_NotImplemented, // 42
    MCU_Operand_NotImplemented, // 43
    MCU_Operand_NotImplemented, // 44
    MCU_Operand_NotImplemented, // 45
    MCU_Operand_NotImplemented, // 46
    MCU_Operand_NotImplemented, // 47
    MCU_Operand_NotImplemented, // 48
    MCU_Operand_NotImplemented, // 49
    MCU_Operand_NotImplemented, // 4A
    MCU_Operand_NotImplemented, // 4B
    MCU_Operand_NotImplemented, // 4C
    MCU_Operand_NotImplemented, // 4D
    MCU_Operand_NotImplemented, // 4E
    MCU_Operand_NotImplemented, // 4F
    MCU_Opcode_Short_NotImplemented, // 50
    MCU_Opcode_Short_NotImplemented, // 51
    MCU_Opcode_Short_NotImplemented, // 52
    MCU_Opcode_Short_NotImplemented, // 53
    MCU_Opcode_Short_NotImplemented, // 54
    MCU_Opcode_Short_NotImplemented, // 55
    MCU_Opcode_Short_NotImplemented, // 56
    MCU_Opcode_Short_NotImplemented, // 57
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
    MCU_Opcode_Short_NotImplemented, // 70
    MCU_Opcode_Short_NotImplemented, // 71
    MCU_Opcode_Short_NotImplemented, // 72
    MCU_Opcode_Short_NotImplemented, // 73
    MCU_Opcode_Short_NotImplemented, // 74
    MCU_Opcode_Short_NotImplemented, // 75
    MCU_Opcode_Short_NotImplemented, // 76
    MCU_Opcode_Short_NotImplemented, // 77
    MCU_Opcode_Short_NotImplemented, // 78
    MCU_Opcode_Short_NotImplemented, // 79
    MCU_Opcode_Short_NotImplemented, // 7A
    MCU_Opcode_Short_NotImplemented, // 7B
    MCU_Opcode_Short_NotImplemented, // 7C
    MCU_Opcode_Short_NotImplemented, // 7D
    MCU_Opcode_Short_NotImplemented, // 7E
    MCU_Opcode_Short_NotImplemented, // 7F
    MCU_Opcode_Short_NotImplemented, // 80
    MCU_Opcode_Short_NotImplemented, // 81
    MCU_Opcode_Short_NotImplemented, // 82
    MCU_Opcode_Short_NotImplemented, // 83
    MCU_Opcode_Short_NotImplemented, // 84
    MCU_Opcode_Short_NotImplemented, // 85
    MCU_Opcode_Short_NotImplemented, // 86
    MCU_Opcode_Short_NotImplemented, // 87
    MCU_Opcode_Short_NotImplemented, // 88
    MCU_Opcode_Short_NotImplemented, // 89
    MCU_Opcode_Short_NotImplemented, // 8A
    MCU_Opcode_Short_NotImplemented, // 8B
    MCU_Opcode_Short_NotImplemented, // 8C
    MCU_Opcode_Short_NotImplemented, // 8D
    MCU_Opcode_Short_NotImplemented, // 8E
    MCU_Opcode_Short_NotImplemented, // 8F
    MCU_Opcode_Short_NotImplemented, // 90
    MCU_Opcode_Short_NotImplemented, // 91
    MCU_Opcode_Short_NotImplemented, // 92
    MCU_Opcode_Short_NotImplemented, // 93
    MCU_Opcode_Short_NotImplemented, // 94
    MCU_Opcode_Short_NotImplemented, // 95
    MCU_Opcode_Short_NotImplemented, // 96
    MCU_Opcode_Short_NotImplemented, // 97
    MCU_Opcode_Short_NotImplemented, // 98
    MCU_Opcode_Short_NotImplemented, // 99
    MCU_Opcode_Short_NotImplemented, // 9A
    MCU_Opcode_Short_NotImplemented, // 9B
    MCU_Opcode_Short_NotImplemented, // 9C
    MCU_Opcode_Short_NotImplemented, // 9D
    MCU_Opcode_Short_NotImplemented, // 9E
    MCU_Opcode_Short_NotImplemented, // 9F
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
    MCU_Opcode_NotImplemented, // 01
    MCU_Opcode_CLR, // 02
    MCU_Opcode_NotImplemented, // 03
    MCU_Opcode_NotImplemented, // 04
    MCU_Opcode_NotImplemented, // 05
    MCU_Opcode_NotImplemented, // 06
    MCU_Opcode_NotImplemented, // 07
    MCU_Opcode_NotImplemented, // 08
    MCU_Opcode_BSET_ORC, // 09
    MCU_Opcode_NotImplemented, // 0A
    MCU_Opcode_BCLR_ANDC, // 0B
    MCU_Opcode_NotImplemented, // 0C
    MCU_Opcode_NotImplemented, // 0D
    MCU_Opcode_NotImplemented, // 0E
    MCU_Opcode_NotImplemented, // 0F
    MCU_Opcode_NotImplemented, // 10
    MCU_Opcode_LDC, // 11
    MCU_Opcode_NotImplemented, // 12
    MCU_Opcode_NotImplemented, // 13
    MCU_Opcode_NotImplemented, // 14
    MCU_Opcode_NotImplemented, // 15
    MCU_Opcode_NotImplemented, // 16
    MCU_Opcode_NotImplemented, // 17
    MCU_Opcode_NotImplemented, // 18
    MCU_Opcode_NotImplemented, // 19
    MCU_Opcode_NotImplemented, // 1A
    MCU_Opcode_NotImplemented, // 1B
    MCU_Opcode_NotImplemented, // 1C
    MCU_Opcode_NotImplemented, // 1D
    MCU_Opcode_NotImplemented, // 1E
    MCU_Opcode_NotImplemented, // 1F
};

