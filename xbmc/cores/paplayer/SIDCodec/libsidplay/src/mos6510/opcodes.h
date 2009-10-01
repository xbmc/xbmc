/***************************************************************************
                          opcodes.h  -  description
                             -------------------
    begin                : Thu May 11 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _opcodes_h_
#define _opcodes_h_

#define OPCODE_MAX 0x100

/* HLT
    case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
    case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
    case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
    case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
*/

#define BRKn  0x00
#define JSRw  0x20
#define RTIn  0x40
#define RTSn  0x60
#define NOPb  0x80
#define NOPb_ NOPb: case 0x82: case 0xC2: case 0xE2: case 0x89
#define LDYb  0xA0
#define CPYb  0xC0
#define CPXb  0xE0

#define ORAix 0x01
#define ANDix 0x21
#define EORix 0x41
#define ADCix 0x61
#define STAix 0x81
#define LDAix 0xA1
#define CMPix 0xC1
#define SBCix 0xE1

#define LDXb 0xA2

#define SLOix 0x03
#define RLAix 0x23
#define SREix 0x43
#define RRAix 0x63
#define SAXix 0x83
#define LAXix 0xA3
#define DCPix 0xC3
#define ISBix 0xE3

#define NOPz  0x04
#define NOPz_ NOPz: case 0x44: case 0x64
#define BITz  0x24
#define STYz  0x84
#define LDYz  0xA4
#define CPYz  0xC4
#define CPXz  0xE4

#define ORAz 0x05
#define ANDz 0x25
#define EORz 0x45
#define ADCz 0x65
#define STAz 0x85
#define LDAz 0xA5
#define CMPz 0xC5
#define SBCz 0xE5

#define ASLz 0x06
#define ROLz 0x26
#define LSRz 0x46
#define RORz 0x66
#define STXz 0x86
#define LDXz 0xA6
#define DECz 0xC6
#define INCz 0xE6

#define SLOz 0x07
#define RLAz 0x27
#define SREz 0x47
#define RRAz 0x67
#define SAXz 0x87
#define LAXz 0xA7
#define DCPz 0xC7
#define ISBz 0xE7

#define PHPn 0x08
#define PLPn 0x28
#define PHAn 0x48
#define PLAn 0x68
#define DEYn 0x88
#define TAYn 0xA8
#define INYn 0xC8
#define INXn 0xE8

#define ORAb  0x09
#define ANDb  0x29
#define EORb  0x49
#define ADCb  0x69
#define LDAb  0xA9
#define CMPb  0xC9
#define SBCb  0xE9
#define SBCb_ SBCb: case 0XEB

#define ASLn  0x0A
#define ROLn  0x2A
#define LSRn  0x4A
#define RORn  0x6A
#define TXAn  0x8A
#define TAXn  0xAA
#define DEXn  0xCA
#define NOPn  0xEA
#define NOPn_ NOPn: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA

#define ANCb  0x0B
#define ANCb_ ANCb: case 0x2B
#define ASRb  0x4B
#define ARRb  0x6B
#define ANEb  0x8B
#define XAAb  0x8B
#define LXAb  0xAB
#define SBXb  0xCB

#define NOPa 0x0C
#define BITa 0x2C
#define JMPw 0x4C
#define JMPi 0x6C
#define STYa 0x8C
#define LDYa 0xAC
#define CPYa 0xCC
#define CPXa 0xEC

#define ORAa 0x0D
#define ANDa 0x2D
#define EORa 0x4D
#define ADCa 0x6D
#define STAa 0x8D
#define LDAa 0xAD
#define CMPa 0xCD
#define SBCa 0xED

#define ASLa 0x0E
#define ROLa 0x2E
#define LSRa 0x4E
#define RORa 0x6E
#define STXa 0x8E
#define LDXa 0xAE
#define DECa 0xCE
#define INCa 0xEE

#define SLOa 0x0F
#define RLAa 0x2F
#define SREa 0x4F
#define RRAa 0x6F
#define SAXa 0x8F
#define LAXa 0xAF
#define DCPa 0xCF
#define ISBa 0xEF

#define BPLr 0x10
#define BMIr 0x30
#define BVCr 0x50
#define BVSr 0x70
#define BCCr 0x90
#define BCSr 0xB0
#define BNEr 0xD0
#define BEQr 0xF0

#define ORAiy 0x11
#define ANDiy 0x31
#define EORiy 0x51
#define ADCiy 0x71
#define STAiy 0x91
#define LDAiy 0xB1
#define CMPiy 0xD1
#define SBCiy 0xF1

#define SLOiy 0x13
#define RLAiy 0x33
#define SREiy 0x53
#define RRAiy 0x73
#define SHAiy 0x93
#define LAXiy 0xB3
#define DCPiy 0xD3
#define ISBiy 0xF3

#define NOPzx  0x14
#define NOPzx_ NOPzx: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4
#define STYzx  0x94
#define LDYzx  0xB4

#define ORAzx 0x15
#define ANDzx 0x35
#define EORzx 0x55
#define ADCzx 0x75
#define STAzx 0x95
#define LDAzx 0xB5
#define CMPzx 0xD5
#define SBCzx 0xF5

#define ASLzx 0x16
#define ROLzx 0x36
#define LSRzx 0x56
#define RORzx 0x76
#define STXzy 0x96
#define LDXzy 0xB6
#define DECzx 0xD6
#define INCzx 0xF6

#define SLOzx 0x17
#define RLAzx 0x37
#define SREzx 0x57
#define RRAzx 0x77
#define SAXzy 0x97
#define LAXzy 0xB7
#define DCPzx 0xD7
#define ISBzx 0xF7

#define CLCn 0x18
#define SECn 0x38
#define CLIn 0x58
#define SEIn 0x78
#define TYAn 0x98
#define CLVn 0xB8
#define CLDn 0xD8
#define SEDn 0xF8

#define ORAay 0x19
#define ANDay 0x39
#define EORay 0x59
#define ADCay 0x79
#define STAay 0x99
#define LDAay 0xB9
#define CMPay 0xD9
#define SBCay 0xF9

#define TXSn 0x9A
#define TSXn 0xBA

#define SLOay 0x1B
#define RLAay 0x3B
#define SREay 0x5B
#define RRAay 0x7B
#define SHSay 0x9B
#define TASay 0x9B
#define LASay 0xBB
#define DCPay 0xDB
#define ISBay 0xFB

#define NOPax  0x1C
#define NOPax_ NOPax: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC
#define SHYax  0x9C
#define LDYax  0xBC

#define ORAax 0x1D
#define ANDax 0x3D
#define EORax 0x5D
#define ADCax 0x7D
#define STAax 0x9D
#define LDAax 0xBD
#define CMPax 0xDD
#define SBCax 0xFD

#define ASLax 0x1E
#define ROLax 0x3E
#define LSRax 0x5E
#define RORax 0x7E
#define SHXay 0x9E
#define LDXay 0xBE
#define DECax 0xDE
#define INCax 0xFE

#define SLOax 0x1F
#define RLAax 0x3F
#define SREax 0x5F
#define RRAax 0x7F
#define SHAay 0x9F
#define LAXay 0xBF
#define DCPax 0xDF
#define ISBax 0xFF

// Instruction Aliases
#define ASOix SLOix
#define LSEix SREix
#define AXSix SAXix
#define DCMix DCPix
#define INSix ISBix
#define ASOz  SLOz
#define LSEz  SREz
#define AXSz  SAXz
#define DCMz  DCPz
#define INSz  ISBz
#define ALRb  ASRb
#define OALb  LXAb
#define ASOa  SLOa
#define LSEa  SREa
#define AXSa  SAXa
#define DCMa  DCPa
#define INSa  ISBa
#define ASOiy SLOiy
#define LSEiy SREiy
#define AXAiy SHAiy
#define DCMiy DCPiy
#define INSiy ISBiy
#define ASOzx SLOzx
#define LSEzx SREzx
#define AXSzy SAXzy
#define DCMzx DCPzx
#define INSzx ISBzx
#define ASOay SLOay
#define LSEay SREay
#define DCMay DCPay
#define INSay ISBay
#define SAYax SHYax
#define XASay SHXay
#define ASOax SLOax
#define LSEax SREax
#define AXAay SHAay
#define DCMax DCPax
#define INSax ISBax
#define SKBn  NOPb
#define SKWn  NOPa

#endif // _opcodes_h_
