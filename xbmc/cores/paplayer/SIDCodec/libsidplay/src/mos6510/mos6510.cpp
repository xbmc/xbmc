/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Thu May 11 06:22:40 BST 2000
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
/***************************************************************************
 *  $Log: mos6510.cpp,v $
 *  Revision 1.13  2004/04/23 01:06:24  s_a_white
 *  Display correct cycle instruction starts on via dbgClk.  This is set at the
 *  start of every instruction correctly allows for cycle stealing.
 *
 *  Revision 1.12  2004/02/21 13:20:10  s_a_white
 *  Zero debug cycle count so is from start of instruction rather than after
 *  the last addressing mode cycle.
 *
 *  Revision 1.11  2004/01/13 22:36:07  s_a_white
 *  Converted some missed printfs to fprintfs
 *
 *  Revision 1.10  2003/10/28 00:22:52  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.9  2003/10/16 07:46:05  s_a_white
 *  Allow redirection of debug information of file.
 *
 *  Revision 1.8  2001/08/05 15:46:38  s_a_white
 *  No longer need to check on which cycle to print debug information.
 *
 *  Revision 1.7  2001/07/14 13:04:34  s_a_white
 *  Accumulator is now unsigned, which improves code readability.
 *
 *  Revision 1.6  2001/03/09 22:27:46  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.5  2001/02/13 23:01:10  s_a_white
 *  envReadMemDataByte now used for debugging.
 *
 *  Revision 1.4  2000/12/11 19:03:16  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sidtypes.h"
#include "sidendian.h"
#include "sidenv.h"
#include "conf6510.h"
#include "opcodes.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#ifdef MOS6510_STATE_6510
#   include "state6510.h"
#   include "state6510.cpp"
#else

#include "mos6510.h"

// Check to see what type of emulation is required
#ifdef MOS6510_CYCLE_BASED
#   include "cycle_based/mos6510c.i"

#   ifdef MOS6510_SIDPLAY
        // Compile in sidplay code
#       include "cycle_based/sid6510c.i"
#   endif // MOS6510_SIDPLAY
#else
    // Line based emulation code has not been provided
#endif // MOS6510_CYCLE_BASED

void MOS6510::DumpState (void)
{
    uint8_t        opcode, data;
    uint_least16_t operand, address;

    fprintf(m_fdbg, " PC  I  A  X  Y  SP  DR PR NV-BDIZC  Instruction (%u)\n",
            m_dbgClk);
    fprintf(m_fdbg, "%04x ",   instrStartPC);
    fprintf(m_fdbg, "%u ",     interrupts.irqs);
    fprintf(m_fdbg, "%02x ",   Register_Accumulator);
    fprintf(m_fdbg, "%02x ",   Register_X);
    fprintf(m_fdbg, "%02x ",   Register_Y);
    fprintf(m_fdbg, "01%02x ", endian_16lo8 (Register_StackPointer));
    fprintf(m_fdbg, "%02x ",   envReadMemDataByte (0));
    fprintf(m_fdbg, "%02x ",   envReadMemDataByte (1));

    if (getFlagN()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (getFlagV()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (Register_Status & (1 << SR_NOTUSED)) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (Register_Status & (1 << SR_BREAK))   fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (getFlagD()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (getFlagI()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (getFlagZ()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");
    if (getFlagC()) fprintf(m_fdbg, "1"); else fprintf(m_fdbg, "0");

    opcode  = instrOpcode;
    operand = Instr_Operand;
    data    = Cycle_Data;

    switch (opcode)
    {
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        address = (uint_least16_t) (Register_ProgramCounter + (int8_t) operand);
    break;

    default:
        address = Cycle_EffectiveAddress;
    break;
    }

    fprintf(m_fdbg, "  %02x ", opcode);

    switch(opcode)
    {
    //Accumulator or Implied addressing
    case ASLn: case LSRn: case ROLn: case RORn:
        fprintf(m_fdbg, "      ");
    break;
    //Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case NOPz_:
    case ORAz: case ROLz: case RORz: case SAXz: case SBCz: case SREz:
    case STAz: case STXz: case STYz: case SLOz: case RLAz: case RRAz:
    //ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", (uint8_t) operand);
        break;
    //Zero Page with X Offset Addressing Mode Handler
    case ADCzx:  case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx:  case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case NOPzx_: case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx:
    case SBCzx:  case SLOzx: case SREzx: case STAzx: case STYzx:
    //ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", (uint8_t) operand);
        break;
    //Zero Page with Y Offset Addressing Mode Handler
    case LDXzy: case STXzy: case SAXzy: case LAXzy:
    //AXSzx - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", endian_16lo8 (operand));
        break;
    //Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case JMPw: case JSRw: case LAXa: case LDAa: case LDXa: case LDYa:
    case LSRa: case NOPa: case ORAa: case ROLa: case RORa: case SAXa:
    case SBCa: case SLOa: case SREa: case STAa: case STXa: case STYa:
    case RLAa: case RRAa:
    //ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        fprintf(m_fdbg, "%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Absolute With X Offset Addresing Mode Handler
    case ADCax:  case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax:  case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case NOPax_: case ORAax: case RLAax: case ROLax: case RORax: case RRAax:
    case SBCax:  case SHYax: case SLOax: case SREax: case STAax:
    //ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        fprintf(m_fdbg, "%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHAay: case SHSay: case SHXay: case SLOay:
    case SREay: case STAay:
    //ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        fprintf(m_fdbg, "%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Immediate and Relative Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:

    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case NOPb_: case ORAb: case SBCb_: case SBXb:
    //OALb ALRb XAAb - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", endian_16lo8 (operand));
        break;
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        fprintf(m_fdbg, "%02x    ", endian_16lo8 (operand));
        break;
    //Indirect Addressing Mode Handler
    case JMPi:
        fprintf(m_fdbg, "%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SAXix: case SBCix: case SLOix:
    case SREix: case STAix: case RLAix: case RRAix:
    //ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", endian_16lo8 (operand));
        break;
    //Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SHAiy: case SLOiy: case SREiy: case STAiy:
    //AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        fprintf(m_fdbg, "%02x    ", endian_16lo8 (operand));
        break;
    default:
        fprintf(m_fdbg, "      ");
        break;
    }

    switch(opcode)
    {
    case ADCb: case ADCz: case ADCzx: case ADCa: case ADCax: case ADCay:
    case ADCix: case ADCiy:
        fprintf(m_fdbg, " ADC"); break;
    case ANCb_:
        fprintf(m_fdbg, "*ANC"); break;
    case ANDb: case ANDz: case ANDzx: case ANDa: case ANDax: case ANDay:
    case ANDix: case ANDiy:
        fprintf(m_fdbg, " AND"); break;
    case ANEb: //Also known as XAA
        fprintf(m_fdbg, "*ANE"); break;
    case ARRb:
        fprintf(m_fdbg, "*ARR"); break;
    case ASLn: case ASLz: case ASLzx: case ASLa: case ASLax:
        fprintf(m_fdbg, " ASL"); break;
    case ASRb: //Also known as ALR
        fprintf(m_fdbg, "*ASR"); break;
    case BCCr:
        fprintf(m_fdbg, " BCC"); break;
    case BCSr:
        fprintf(m_fdbg, " BCS"); break;
    case BEQr:
        fprintf(m_fdbg, " BEQ"); break;
    case BITz: case BITa:
        fprintf(m_fdbg, " BIT"); break;
    case BMIr:
        fprintf(m_fdbg, " BMI"); break;
    case BNEr:
        fprintf(m_fdbg, " BNE"); break;
    case BPLr:
        fprintf(m_fdbg, " BPL"); break;
    case BRKn:
        fprintf(m_fdbg, " BRK"); break;
    case BVCr:
        fprintf(m_fdbg, " BVC"); break;
    case BVSr:
        fprintf(m_fdbg, " BVS"); break;
    case CLCn:
        fprintf(m_fdbg, " CLC"); break;
    case CLDn:
        fprintf(m_fdbg, " CLD"); break;
    case CLIn:
        fprintf(m_fdbg, " CLI"); break;
    case CLVn:
        fprintf(m_fdbg, " CLV"); break;
    case CMPb: case CMPz: case CMPzx: case CMPa: case CMPax: case CMPay:
    case CMPix: case CMPiy:
        fprintf(m_fdbg, " CMP"); break;
    case CPXb: case CPXz: case CPXa:
        fprintf(m_fdbg, " CPX"); break;
    case CPYb: case CPYz: case CPYa:
        fprintf(m_fdbg, " CPY"); break;
    case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
    case DCPiy: //Also known as DCM
        fprintf(m_fdbg, "*DCP"); break;
    case DECz: case DECzx: case DECa: case DECax:
        fprintf(m_fdbg, " DEC"); break;
    case DEXn:
        fprintf(m_fdbg, " DEX"); break;
    case DEYn:
        fprintf(m_fdbg, " DEY"); break;
    case EORb: case EORz: case EORzx: case EORa: case EORax: case EORay:
    case EORix: case EORiy:
        fprintf(m_fdbg, " EOR"); break;
    case INCz: case INCzx: case INCa: case INCax:
        fprintf(m_fdbg, " INC"); break;
    case INXn:
        fprintf(m_fdbg, " INX"); break;
    case INYn:
        fprintf(m_fdbg, " INY"); break;
    case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
    case ISBiy: //Also known as INS
        fprintf(m_fdbg, "*ISB"); break;
    case JMPw: case JMPi:
        fprintf(m_fdbg, " JMP"); break;
    case JSRw:
        fprintf(m_fdbg, " JSR"); break;
    case LASay:
        fprintf(m_fdbg, "*LAS"); break;
    case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
        fprintf(m_fdbg, "*LAX"); break;
    case LDAb: case LDAz: case LDAzx: case LDAa: case LDAax: case LDAay:
    case LDAix: case LDAiy:
        fprintf(m_fdbg, " LDA"); break;
    case LDXb: case LDXz: case LDXzy: case LDXa: case LDXay:
        fprintf(m_fdbg, " LDX"); break;
    case LDYb: case LDYz: case LDYzx: case LDYa: case LDYax:
        fprintf(m_fdbg, " LDY"); break;
    case LSRz: case LSRzx: case LSRa: case LSRax: case LSRn:
        fprintf(m_fdbg, " LSR"); break;
    case NOPn_: case NOPb_: case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        if(opcode != NOPn) fprintf(m_fdbg, "*");
        else fprintf(m_fdbg, " ");
        fprintf(m_fdbg, "NOP"); break;
    case LXAb: //Also known as OAL
        fprintf(m_fdbg, "*LXA"); break;
    case ORAb: case ORAz: case ORAzx: case ORAa: case ORAax: case ORAay:
    case ORAix: case ORAiy:
        fprintf(m_fdbg, " ORA"); break;
    case PHAn:
        fprintf(m_fdbg, " PHA"); break;
    case PHPn:
        fprintf(m_fdbg, " PHP"); break;
    case PLAn:
        fprintf(m_fdbg, " PLA"); break;
    case PLPn:
        fprintf(m_fdbg, " PLP"); break;
    case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
    case RLAiy:
        fprintf(m_fdbg, "*RLA"); break;
    case ROLz: case ROLzx: case ROLa: case ROLax: case ROLn:
        fprintf(m_fdbg, " ROL"); break;
    case RORz: case RORzx: case RORa: case RORax: case RORn:
        fprintf(m_fdbg, " ROR"); break;
    case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
    case RRAiy:
        fprintf(m_fdbg, "*RRA"); break;
    case RTIn:
        fprintf(m_fdbg, " RTI"); break;
    case RTSn:
        fprintf(m_fdbg, " RTS"); break;
    case SAXz: case SAXzy: case SAXa: case SAXix: //Also known as AXS
        fprintf(m_fdbg, "*SAX"); break;
    case SBCb_:
        if(opcode != SBCb) fprintf(m_fdbg, "*");
        else fprintf(m_fdbg, " ");
        fprintf(m_fdbg, "SBC"); break;
    case SBCz: case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
    case SBCiy:
        fprintf(m_fdbg, " SBC"); break;
    case SBXb:
        fprintf(m_fdbg, "*SBX"); break;
    case SECn:
        fprintf(m_fdbg, " SEC"); break;
    case SEDn:
        fprintf(m_fdbg, " SED"); break;
    case SEIn:
        fprintf(m_fdbg, " SEI"); break;
    case SHAay: case SHAiy: //Also known as AXA
        fprintf(m_fdbg, "*SHA"); break;
    case SHSay: //Also known as TAS
        fprintf(m_fdbg, "*SHS"); break;
    case SHXay: //Also known as XAS
        fprintf(m_fdbg, "*SHX"); break;
    case SHYax: //Also known as SAY
        fprintf(m_fdbg, "*SHY"); break;
    case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
    case SLOiy: //Also known as ASO
        fprintf(m_fdbg, "*SLO"); break;
    case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
    case SREiy: //Also known as LSE
        fprintf(m_fdbg, "*SRE"); break;
    case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
    case STAiy:
        fprintf(m_fdbg, " STA"); break;
    case STXz: case STXzy: case STXa:
        fprintf(m_fdbg, " STX"); break;
    case STYz: case STYzx: case STYa:
        fprintf(m_fdbg, " STY"); break;
    case TAXn:
        fprintf(m_fdbg, " TAX"); break;
    case TAYn:
        fprintf(m_fdbg, " TAY"); break;
    case TSXn:
        fprintf(m_fdbg, " TSX"); break;
    case TXAn:
        fprintf(m_fdbg, " TXA"); break;
    case TXSn:
        fprintf(m_fdbg, " TXS"); break;
    case TYAn:
        fprintf(m_fdbg, " TYA"); break;
    default:
        fprintf(m_fdbg, "*HLT"); break;
    }

    switch(opcode)
    {
    //Accumulator or Implied addressing
    case ASLn: case LSRn: case ROLn: case RORn:
        fprintf(m_fdbg, "n  A");
    break;

    //Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case ORAz:

    case ROLz: case RORz: case SBCz: case SREz: case SLOz: case RLAz:
    case RRAz:
    //ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        fprintf(m_fdbg, "z  %02x {%02x}", (uint8_t) operand, data);
    break;
    case SAXz: case STAz: case STXz: case STYz:
#ifdef MOS6510_DEBUG
    case NOPz_:
#endif
        fprintf(m_fdbg, "z  %02x", endian_16lo8 (operand));
    break;

    //Zero Page with X Offset Addressing Mode Handler
    case ADCzx: case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx: case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx: case SBCzx:
    case SLOzx: case SREzx:
    //ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        fprintf(m_fdbg, "zx %02x,X", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case STAzx: case STYzx:
#ifdef MOS6510_DEBUG
    case NOPzx_:
#endif
        fprintf(m_fdbg, "zx %02x,X", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Zero Page with Y Offset Addressing Mode Handler
    case LAXzy: case LDXzy:
    //AXSzx - Optional Opcode Names
        fprintf(m_fdbg, "zy %02x,Y", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case STXzy: case SAXzy:
        fprintf(m_fdbg, "zy %02x,Y", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case LAXa: case LDAa: case LDXa: case LDYa: case LSRa: case ORAa:
    case ROLa: case RORa: case SBCa: case SLOa: case SREa: case RLAa:
    case RRAa:
    //ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        fprintf(m_fdbg, "a  %04x {%02x}", operand, data);
    break;
    case SAXa: case STAa: case STXa: case STYa:
#ifdef MOS6510_DEBUG
    case NOPa:
#endif
        fprintf(m_fdbg, "a  %04x", operand);
    break;
    case JMPw: case JSRw:
        fprintf(m_fdbg, "w  %04x", operand);
    break;

    //Absolute With X Offset Addresing Mode Handler
    case ADCax: case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax: case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case ORAax: case RLAax: case ROLax: case RORax: case RRAax: case SBCax:
    case SLOax: case SREax:
    //ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        fprintf(m_fdbg, "ax %04x,X", operand);
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case SHYax: case STAax:
#ifdef MOS6510_DEBUG
    case NOPax_:
#endif
        fprintf(m_fdbg, "ax %04x,X", operand);
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHSay: case SLOay: case SREay:
    //ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        fprintf(m_fdbg, "ay %04x,Y", operand);
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case SHAay: case SHXay: case STAay:
        fprintf(m_fdbg, "ay %04x,Y", operand);
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Immediate Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:
    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case ORAb: case SBCb_: case SBXb:
    //OALb ALRb XAAb - Optional Opcode Names
#ifdef MOS6510_DEBUG
    case NOPb_:
#endif
        fprintf(m_fdbg, "b  #%02x", endian_16lo8 (operand));
    break;

    //Relative Addressing Mode Handler
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        fprintf(m_fdbg, "r  #%02x", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Indirect Addressing Mode Handler
    case JMPi:
        fprintf(m_fdbg, "i  (%04x)", operand);
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SBCix: case SLOix: case SREix:
    case RLAix: case RRAix:
    //ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        fprintf(m_fdbg, "ix (%02x,X)", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case SAXix: case STAix:
        fprintf(m_fdbg, "ix (%02x,X)", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]", address);
    break;

    //Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SLOiy: case SREiy:
    //AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        fprintf(m_fdbg, "iy (%02x),Y", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]{%02x}", address, data);
    break;
    case SHAiy: case STAiy:
        fprintf(m_fdbg, "iy (%02x),Y", endian_16lo8 (operand));
        fprintf(m_fdbg, " [%04x]", address);
    break;

    default:
    break;
    }

    fprintf (m_fdbg, "\n\n");
    fflush  (m_fdbg);
}

#endif // MOS6510_STATE_6510
