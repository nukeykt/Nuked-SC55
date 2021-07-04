#include "mcu.h"
#include "mcu_opcodes.h"


void MCU_Opcode_00(uint8_t prefix)
{
    // nop
}

void MCU_Opcode_General(uint8_t prefix) // Immediate operation
{
    uint16_t data = MCU_ReadCodeAdvance();
    if (prefix == 0x0c)
    {
        data <<= 8;
        data |= MCU_ReadCodeAdvance();
    }
    uint8_t inst = MCU_ReadCodeAdvance();
    uint8_t hi = inst >> 4;
    uint8_t lo = inst & 0x0f;
    uint8_t rg = inst & 0x07;
    switch (hi)
    {
    case 0x8: // mov, ldc
        if (lo & 0x8)
        {

        }
        else
        {

        }
        break;
#if 0
    case 0x2: // add, adds
        break;
    case 0x3: // sub, subs
        break;
    case 0x4: // or, orc
        break;
    case 0x5: // and, andc
        break;
    case 0x6: // xor, xorc
        break;
    case 0x7: // cmp
        break;
    case 0x8: // mov, ldc
        break;
    case 0xa: // addx, mulxu
        break;
    case 0xb: // subx, divxu
        break;
#endif
    default:
        MCU_ErrorTrap();
        break;
    }
}

void MCU_Opcode_Short(uint8_t prefix)
{

}

void (*MCU_Opcode_Table[256])(uint8_t prefix) = {
    MCU_Opcode_00, // 00
    MCU_Opcode_00, // 01
    MCU_Opcode_00, // 02
    MCU_Opcode_00, // 03
    MCU_Opcode_General, // 04
    MCU_Opcode_General, // 05
    MCU_Opcode_00, // 06
    MCU_Opcode_00, // 07
    MCU_Opcode_00, // 08
    MCU_Opcode_00, // 09
    MCU_Opcode_00, // 0A
    MCU_Opcode_00, // 0B
    MCU_Opcode_General, // 0C
    MCU_Opcode_General, // 0D
    MCU_Opcode_00, // 0E
    MCU_Opcode_00, // 0F
    MCU_Opcode_00, // 10
    MCU_Opcode_00, // 11
    MCU_Opcode_00, // 12
    MCU_Opcode_00, // 13
    MCU_Opcode_00, // 14
    MCU_Opcode_General, // 15
    MCU_Opcode_00, // 16
    MCU_Opcode_00, // 17
    MCU_Opcode_00, // 18
    MCU_Opcode_00, // 19
    MCU_Opcode_00, // 1A
    MCU_Opcode_00, // 1B
    MCU_Opcode_00, // 1C
    MCU_Opcode_General, // 1D
    MCU_Opcode_00, // 1E
    MCU_Opcode_00, // 1F
    MCU_Opcode_00, // 20
    MCU_Opcode_00, // 21
    MCU_Opcode_00, // 22
    MCU_Opcode_00, // 23
    MCU_Opcode_00, // 24
    MCU_Opcode_00, // 25
    MCU_Opcode_00, // 26
    MCU_Opcode_00, // 27
    MCU_Opcode_00, // 28
    MCU_Opcode_00, // 29
    MCU_Opcode_00, // 2A
    MCU_Opcode_00, // 2B
    MCU_Opcode_00, // 2C
    MCU_Opcode_00, // 2D
    MCU_Opcode_00, // 2E
    MCU_Opcode_00, // 2F
    MCU_Opcode_00, // 30
    MCU_Opcode_00, // 31
    MCU_Opcode_00, // 32
    MCU_Opcode_00, // 33
    MCU_Opcode_00, // 34
    MCU_Opcode_00, // 35
    MCU_Opcode_00, // 36
    MCU_Opcode_00, // 37
    MCU_Opcode_00, // 38
    MCU_Opcode_00, // 39
    MCU_Opcode_00, // 3A
    MCU_Opcode_00, // 3B
    MCU_Opcode_00, // 3C
    MCU_Opcode_00, // 3D
    MCU_Opcode_00, // 3E
    MCU_Opcode_00, // 3F
    MCU_Opcode_00, // 40
    MCU_Opcode_00, // 41
    MCU_Opcode_00, // 42
    MCU_Opcode_00, // 43
    MCU_Opcode_00, // 44
    MCU_Opcode_00, // 45
    MCU_Opcode_00, // 46
    MCU_Opcode_00, // 47
    MCU_Opcode_00, // 48
    MCU_Opcode_00, // 49
    MCU_Opcode_00, // 4A
    MCU_Opcode_00, // 4B
    MCU_Opcode_00, // 4C
    MCU_Opcode_00, // 4D
    MCU_Opcode_00, // 4E
    MCU_Opcode_00, // 4F
    MCU_Opcode_Short, // 50
    MCU_Opcode_Short, // 51
    MCU_Opcode_Short, // 52
    MCU_Opcode_Short, // 53
    MCU_Opcode_Short, // 54
    MCU_Opcode_Short, // 55
    MCU_Opcode_Short, // 56
    MCU_Opcode_Short, // 57
    MCU_Opcode_Short, // 58
    MCU_Opcode_Short, // 59
    MCU_Opcode_Short, // 5A
    MCU_Opcode_Short, // 5B
    MCU_Opcode_Short, // 5C
    MCU_Opcode_Short, // 5D
    MCU_Opcode_Short, // 5E
    MCU_Opcode_Short, // 5F
    MCU_Opcode_Short, // 60
    MCU_Opcode_Short, // 61
    MCU_Opcode_Short, // 62
    MCU_Opcode_Short, // 63
    MCU_Opcode_Short, // 64
    MCU_Opcode_Short, // 65
    MCU_Opcode_Short, // 66
    MCU_Opcode_Short, // 67
    MCU_Opcode_Short, // 68
    MCU_Opcode_Short, // 69
    MCU_Opcode_Short, // 6A
    MCU_Opcode_Short, // 6B
    MCU_Opcode_Short, // 6C
    MCU_Opcode_Short, // 6D
    MCU_Opcode_Short, // 6E
    MCU_Opcode_Short, // 6F
    MCU_Opcode_Short, // 70
    MCU_Opcode_Short, // 71
    MCU_Opcode_Short, // 72
    MCU_Opcode_Short, // 73
    MCU_Opcode_Short, // 74
    MCU_Opcode_Short, // 75
    MCU_Opcode_Short, // 76
    MCU_Opcode_Short, // 77
    MCU_Opcode_Short, // 78
    MCU_Opcode_Short, // 79
    MCU_Opcode_Short, // 7A
    MCU_Opcode_Short, // 7B
    MCU_Opcode_Short, // 7C
    MCU_Opcode_Short, // 7D
    MCU_Opcode_Short, // 7E
    MCU_Opcode_Short, // 7F
    MCU_Opcode_Short, // 80
    MCU_Opcode_Short, // 81
    MCU_Opcode_Short, // 82
    MCU_Opcode_Short, // 83
    MCU_Opcode_Short, // 84
    MCU_Opcode_Short, // 85
    MCU_Opcode_Short, // 86
    MCU_Opcode_Short, // 87
    MCU_Opcode_Short, // 88
    MCU_Opcode_Short, // 89
    MCU_Opcode_Short, // 8A
    MCU_Opcode_Short, // 8B
    MCU_Opcode_Short, // 8C
    MCU_Opcode_Short, // 8D
    MCU_Opcode_Short, // 8E
    MCU_Opcode_Short, // 8F
    MCU_Opcode_Short, // 90
    MCU_Opcode_Short, // 91
    MCU_Opcode_Short, // 92
    MCU_Opcode_Short, // 93
    MCU_Opcode_Short, // 94
    MCU_Opcode_Short, // 95
    MCU_Opcode_Short, // 96
    MCU_Opcode_Short, // 97
    MCU_Opcode_Short, // 98
    MCU_Opcode_Short, // 99
    MCU_Opcode_Short, // 9A
    MCU_Opcode_Short, // 9B
    MCU_Opcode_Short, // 9C
    MCU_Opcode_Short, // 9D
    MCU_Opcode_Short, // 9E
    MCU_Opcode_Short, // 9F
    MCU_Opcode_General, // A0
    MCU_Opcode_General, // A1
    MCU_Opcode_General, // A2
    MCU_Opcode_General, // A3
    MCU_Opcode_General, // A4
    MCU_Opcode_General, // A5
    MCU_Opcode_General, // A6
    MCU_Opcode_General, // A7
    MCU_Opcode_General, // A8
    MCU_Opcode_General, // A9
    MCU_Opcode_General, // AA
    MCU_Opcode_General, // AB
    MCU_Opcode_General, // AC
    MCU_Opcode_General, // AD
    MCU_Opcode_General, // AE
    MCU_Opcode_General, // AF
    MCU_Opcode_General, // B0
    MCU_Opcode_General, // B1
    MCU_Opcode_General, // B2
    MCU_Opcode_General, // B3
    MCU_Opcode_General, // B4
    MCU_Opcode_General, // B5
    MCU_Opcode_General, // B6
    MCU_Opcode_General, // B7
    MCU_Opcode_General, // B8
    MCU_Opcode_General, // B9
    MCU_Opcode_General, // BA
    MCU_Opcode_General, // BB
    MCU_Opcode_General, // BC
    MCU_Opcode_General, // BD
    MCU_Opcode_General, // BE
    MCU_Opcode_General, // BF
    MCU_Opcode_General, // C0
    MCU_Opcode_General, // C1
    MCU_Opcode_General, // C2
    MCU_Opcode_General, // C3
    MCU_Opcode_General, // C4
    MCU_Opcode_General, // C5
    MCU_Opcode_General, // C6
    MCU_Opcode_General, // C7
    MCU_Opcode_General, // C8
    MCU_Opcode_General, // C9
    MCU_Opcode_General, // CA
    MCU_Opcode_General, // CB
    MCU_Opcode_General, // CC
    MCU_Opcode_General, // CD
    MCU_Opcode_General, // CE
    MCU_Opcode_General, // CF
    MCU_Opcode_General, // D0
    MCU_Opcode_General, // D1
    MCU_Opcode_General, // D2
    MCU_Opcode_General, // D3
    MCU_Opcode_General, // D4
    MCU_Opcode_General, // D5
    MCU_Opcode_General, // D6
    MCU_Opcode_General, // D7
    MCU_Opcode_General, // D8
    MCU_Opcode_General, // D9
    MCU_Opcode_General, // DA
    MCU_Opcode_General, // DB
    MCU_Opcode_General, // DC
    MCU_Opcode_General, // DD
    MCU_Opcode_General, // DE
    MCU_Opcode_General, // DF
    MCU_Opcode_General, // E0
    MCU_Opcode_General, // E1
    MCU_Opcode_General, // E2
    MCU_Opcode_General, // E3
    MCU_Opcode_General, // E4
    MCU_Opcode_General, // E5
    MCU_Opcode_General, // E6
    MCU_Opcode_General, // E7
    MCU_Opcode_General, // E8
    MCU_Opcode_General, // E9
    MCU_Opcode_General, // EA
    MCU_Opcode_General, // EB
    MCU_Opcode_General, // EC
    MCU_Opcode_General, // ED
    MCU_Opcode_General, // EE
    MCU_Opcode_General, // EF
    MCU_Opcode_General, // F0
    MCU_Opcode_General, // F1
    MCU_Opcode_General, // F2
    MCU_Opcode_General, // F3
    MCU_Opcode_General, // F4
    MCU_Opcode_General, // F5
    MCU_Opcode_General, // F6
    MCU_Opcode_General, // F7
    MCU_Opcode_General, // F8
    MCU_Opcode_General, // F9
    MCU_Opcode_General, // FA
    MCU_Opcode_General, // FB
    MCU_Opcode_General, // FC
    MCU_Opcode_General, // FD
    MCU_Opcode_General, // FE
    MCU_Opcode_General, // FF
};


