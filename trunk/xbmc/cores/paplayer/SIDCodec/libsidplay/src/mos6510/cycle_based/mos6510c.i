/***************************************************************************
                          mos6510.i  -  Cycle Accurate 6510 emulation
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
 *  $Log: mos6510c.i,v $
 *  Revision 1.48  2004/05/03 22:37:17  s_a_white
 *  Remove debug code.
 *
 *  Revision 1.47  2004/04/23 01:34:59  s_a_white
 *  CPU timing corrected.
 *
 *  Revision 1.46  2004/04/23 01:04:26  s_a_white
 *  Overlap FetchOpcode with last cycle of previous instruction rather than
 *  next cycle of this instruction.  Although results are almost indentical there
 *  are differences when cycle stealing is invloved.
 *
 *  Revision 1.45  2004/03/07 22:37:42  s_a_white
 *  Cycle stealing only takes effect after 3 consecutive writes or any read.
 *  However stealing does not effect internal only operations and since all writes
 *  are a maximum of 3 cycles followed by a read make just the reads stealable.
 *
 *  Revision 1.44  2004/03/06 21:05:42  s_a_white
 *  Remove debug statements.
 *
 *  Revision 1.43  2004/02/29 14:33:59  s_a_white
 *  If an interrupt occurs during a branch instruction but after the decision
 *  has occured then it should not be delayed.
 *
 *  Revision 1.42  2003/10/29 22:18:04  s_a_white
 *  IRQs are now only taken in on phase 1 as previously they could be clocked
 *  in on both phases of the cycle resulting in them sometimes not being
 *  delayed long enough.
 *
 *  Revision 1.41  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.40  2003/10/16 07:48:32  s_a_white
 *  Allow redirection of debug information of file.
 *
 *  Revision 1.39  2003/07/10 06:52:26  s_a_white
 *  Fixed two memory reads that should have been data and not code accesses
 *
 *  Revision 1.38  2003/05/16 18:28:59  s_a_white
 *  Code modifications to allowing compiling on the QNX6 platform.
 *
 *  Revision 1.37  2003/02/20 21:12:19  s_a_white
 *  Switching to NMI during BRK now occurs correctly.
 *
 *  Revision 1.36  2003/02/20 19:00:20  s_a_white
 *  Code tidy
 *
 *  Revision 1.35  2003/01/23 17:42:33  s_a_white
 *  Fix use of new when exceptions are not supported.
 *
 *  Revision 1.34  2003/01/21 19:26:16  s_a_white
 *  Pending interrupt check is one cycle to late.  Adjust the interrupt delay tests
 *  to compensate.
 *
 *  Revision 1.33  2003/01/20 23:10:48  s_a_white
 *  Fixed RMW instructions to perform memory re-write on the correct cycle.
 *
 *  Revision 1.32  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.31  2003/01/17 08:42:09  s_a_white
 *  Event scheduler phase support.  Better handling the operation of IRQs
 *  during stolen cycles.
 *
 *  Revision 1.30  2002/12/16 08:42:58  s_a_white
 *  Fixed use of nothrow to be namespaced with std::.
 *
 *  Revision 1.29  2002/11/28 20:35:06  s_a_white
 *  Reduced number of thrown exceptions when dma occurs.
 *
 *  Revision 1.28  2002/11/25 20:10:55  s_a_white
 *  A bus access failure should stop the CPU dead like the cycle never started.
 *  This is currently simulated using throw (execption handling) for now.
 *
 *  Revision 1.27  2002/11/21 19:52:48  s_a_white
 *  CPU upgraded to be like other components.  Theres nolonger a clock call,
 *  instead events are registered to occur at a specific time.
 *
 *  Revision 1.26  2002/11/19 22:57:33  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.25  2002/11/01 19:22:36  s_a_white
 *  Removed debug printf.
 *
 *  Revision 1.24  2002/11/01 17:35:27  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.23  2002/03/12 18:48:03  s_a_white
 *  Tidied illegal instruction debug print out.
 *
 *  Revision 1.22  2001/12/11 19:24:15  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.21  2001/11/16 19:21:03  s_a_white
 *  Sign fixes.
 *
 *  Revision 1.20  2001/10/28 21:31:26  s_a_white
 *  Removed kernel debuging code.
 *
 *  Revision 1.19  2001/09/03 22:21:52  s_a_white
 *  When initialising the status register and therefore unmasking the irqs,
 *  check the irq line to see if any are pending.
 *
 *  Revision 1.18  2001/08/10 20:05:50  s_a_white
 *  Fixed RMW instructions which broke due to the optimisation.
 *
 *  Revision 1.17  2001/08/05 15:46:02  s_a_white
 *  No longer need to check on which cycle an instruction ends or when to print
 *  debug information.
 *
 *  Revision 1.16  2001/07/14 13:15:30  s_a_white
 *  Accumulator is now unsigned, which improves code readability.  Emulation
 *  tested with testsuite 2.15.  Various instructions required modification.
 *
 *  Revision 1.15  2001/04/20 22:23:11  s_a_white
 *  Handling of page boundary crossing now correct for branch instructions.
 *
 *  Revision 1.14  2001/03/28 22:59:59  s_a_white
 *  Converted some bad envReadMemByte's to
 *  envReadMemDataByte
 *
 *  Revision 1.13  2001/03/28 21:17:34  s_a_white
 *  Added support for proper RMW instructions.
 *
 *  Revision 1.12  2001/03/24 18:09:17  s_a_white
 *  On entry to interrupt routine the first instruction in the handler is now always
 *  executed before pending interrupts are re-checked.
 *
 *  Revision 1.11  2001/03/22 22:40:43  s_a_white
 *  Added new header for definition of nothrow.
 *
 *  Revision 1.10  2001/03/21 22:27:18  s_a_white
 *  Change to IRQ error message.
 *
 *  Revision 1.9  2001/03/19 23:46:35  s_a_white
 *  NMI no longer sets I flag.  RTI and store instructions are no longer
 *  overlapped.
 *
 *  Revision 1.8  2001/03/09 22:28:51  s_a_white
 *  Speed optimisation update and fix for interrupt flag in PushSR call.
 *
 *  Revision 1.7  2001/02/22 08:28:57  s_a_white
 *  Interrupt masking fixed.
 *
 *  Revision 1.6  2001/02/13 23:01:44  s_a_white
 *  envReadMemDataByte now used for some memory accesses.
 *
 *  Revision 1.5  2000/12/24 00:45:38  s_a_white
 *  HAVE_EXCEPTIONS update
 *
 *  Revision 1.4  2000/12/14 23:55:07  s_a_white
 *  PushSR optimisation and PopSR code cleanup.
 *
 ***************************************************************************/

#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

//#define PC64_TESTSUITE
#ifdef PC64_TESTSUITE
static const char _sidtune_CHRtab[256] =  // CHR$ conversion table (0x01 = no output)
{
   0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0xd, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x21, 0x1,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};
static char filetmp[0x100];
static int  filepos = 0;
#endif // PC64_TESTSUITE


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Status Register Routines                                                //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Use macros to access flags.  Allows compatiblity with other versions
// of this emulation
// Set N and Z flags according to byte
#define setFlagsNZ(x) (Register_z_Flag = (Register_n_Flag = (uint_least8_t) (x)))
#define setFlagN(x)   (Register_n_Flag = (uint_least8_t) (x))
#define setFlagV(x)   (Register_v_Flag = (uint_least8_t) (x))
#define setFlagD(x)   (Register_Status = (Register_Status & ~(1 << SR_DECIMAL)) \
                                       | (((x) != 0) << SR_DECIMAL))
#define setFlagI(x)   (Register_Status = (Register_Status & ~(1 << SR_INTERRUPT)) \
                                       | (((x) != 0) << SR_INTERRUPT))
#define setFlagZ(x)   (Register_z_Flag = (uint_least8_t) (x))
#define setFlagC(x)   (Register_c_Flag = (uint_least8_t) (x))


#define getFlagN()    ((Register_n_Flag &  (1 << SR_NEGATIVE))  != 0)
#define getFlagV()    (Register_v_Flag != 0)
#define getFlagD()    ((Register_Status  & (1 << SR_DECIMAL))   != 0)
#define getFlagI()    ((Register_Status  & (1 << SR_INTERRUPT)) != 0)
#define getFlagZ()    (Register_z_Flag == 0)
#define getFlagC()    (Register_c_Flag != 0)


// Handle bus access signals
void MOS6510::aecSignal (bool state)
{
    if (aec != state)
    {
        event_clock_t clock = eventContext.getTime (m_extPhase);

        // If the cpu blocked waiting for the bus
        // then schedule a retry.
        aec = state;
        if (state && m_blocked)
        {   // Correct IRQs that appeard before the steal
            event_clock_t stolen = clock - m_stealingClk;
            interrupts.nmiClk += stolen;
            interrupts.irqClk += stolen;
            // IRQs that appeared during the steal must have
            // there clocks corrected
            if (interrupts.nmiClk > clock)
                interrupts.nmiClk = clock - 1;
            if (interrupts.irqClk > clock)
                interrupts.irqClk = clock - 1;
            m_blocked = false;
        }

        eventContext.schedule (this, eventContext.phase() == m_phase, m_phase);
    }
}


// Push P on stack, decrement S
void MOS6510::PushSR (bool b_flag)
{
    uint_least16_t addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    /* Rev 1.04 - Corrected flag mask */
    Register_Status &= ((1 << SR_NOTUSED) | (1 << SR_INTERRUPT) |
                        (1 << SR_DECIMAL) | (1 << SR_BREAK));
    Register_Status |= (getFlagN () << SR_NEGATIVE);
    Register_Status |= (getFlagV () << SR_OVERFLOW);
    Register_Status |= (getFlagZ () << SR_ZERO);
    Register_Status |= (getFlagC () << SR_CARRY);
    envWriteMemByte (addr, Register_Status & ~((!b_flag) << SR_BREAK));
    Register_StackPointer--;
}

void MOS6510::PushSR (void)
{
    PushSR (true);
}

// increment S, Pop P off stack
void MOS6510::PopSR (void)
{
    bool newFlagI, oldFlagI;
    oldFlagI = getFlagI ();

    // Get status register off stack
    Register_StackPointer++;
    {
        uint_least16_t addr = Register_StackPointer;
        endian_16hi8 (addr, SP_PAGE);
        Register_Status = envReadMemDataByte (addr);
    }
    Register_Status |= ((1 << SR_NOTUSED) | (1 << SR_BREAK));
    setFlagN (Register_Status);
    setFlagV (Register_Status   & (1 << SR_OVERFLOW));
    setFlagZ (!(Register_Status & (1 << SR_ZERO)));
    setFlagC (Register_Status   & (1 << SR_CARRY));

    // I flag change is delayed by 1 instruction
    newFlagI = getFlagI ();
    interrupts.irqLatch = oldFlagI ^ newFlagI;
    // Check to see if interrupts got re-enabled
    if (!newFlagI && interrupts.irqs)
        interrupts.irqRequest = true;
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Interrupt Routines                                                      //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
#define iIRQSMAX 3
enum
{
    oNONE = -1,
    oRST,
    oNMI,
    oIRQ
};

enum
{
    iNONE = 0,
    iRST  = 1 << oRST,
    iNMI  = 1 << oNMI,
    iIRQ  = 1 << oIRQ
};

void MOS6510::triggerRST (void)
{
    interrupts.pending |= iRST;
}

void MOS6510::triggerNMI (void)
{
    interrupts.pending |= iNMI;
    interrupts.nmiClk   = eventContext.getTime (m_extPhase);
}

// Level triggered interrupt
void MOS6510::triggerIRQ (void)
{   // IRQ Suppressed
    if (!getFlagI ())
        interrupts.irqRequest = true;
    if (!interrupts.irqs++)
        interrupts.irqClk = eventContext.getTime (m_extPhase);

    if (interrupts.irqs > iIRQSMAX)
    {
        fprintf (m_fdbg, "\nMOS6510 ERROR: An external component is not clearing down it's IRQs.\n\n");
        exit (-1);
    }
}

void MOS6510::clearIRQ (void)
{
    if (interrupts.irqs > 0)
    {   
        if (!(--interrupts.irqs))
        {   // Clear off the interrupts
            interrupts.irqRequest = false;
        }
    }
}

bool MOS6510::interruptPending (void)
{
    int_least8_t offset, pending;
    static const int_least8_t offTable[] = {oNONE, oRST, oNMI, oRST,
                                            oIRQ,  oRST, oNMI, oRST};
    // Update IRQ pending
    if (!interrupts.irqLatch)
    {
        interrupts.pending &= ~iIRQ;
        if (interrupts.irqRequest)
            interrupts.pending |= iIRQ;
    }

    pending = interrupts.pending;
MOS6510_interruptPending_check:
    // Service the highest priority interrupt
    offset = offTable[pending];
    switch (offset)
    {
    case oNONE:
        return false;

    case oNMI:
    {
        // Try to determine if we should be processing the NMI yet
        event_clock_t cycles = eventContext.getTime (interrupts.nmiClk, m_extPhase);
        if (cycles >= MOS6510_INTERRUPT_DELAY)
        {
            interrupts.pending &= ~iNMI;
            break;
        }

        // NMI delayed so check for other interrupts
        pending &= ~iNMI;
        goto MOS6510_interruptPending_check;
    }

    case oIRQ:
    {
        // Try to determine if we should be processing the IRQ yet
        event_clock_t cycles = eventContext.getTime (interrupts.irqClk, m_extPhase);
        if (cycles >= MOS6510_INTERRUPT_DELAY)
            break;

        // NMI delayed so check for other interrupts
        pending &= ~iIRQ;
        goto MOS6510_interruptPending_check;
    }

    case oRST:
        break;
    }

#ifdef MOS6510_DEBUG
    event_clock_t cycles = eventContext.getTime (m_phase);
    if (dodump)
    {
    fprintf (m_fdbg, "****************************************************\n");
    switch (offset)
    {
    case oIRQ:
        fprintf (m_fdbg, " IRQ Routine (%u)\n", cycles);
    break;
    case oNMI:
        fprintf (m_fdbg, " NMI Routine (%u)\n", cycles);
    break;
    case oRST:
        fprintf (m_fdbg, " RST Routine (%u)\n", cycles);
    break;
    }
    fprintf (m_fdbg, "****************************************************\n");
    }
#endif

    // Start the interrupt
    instrCurrent = &interruptTable[offset];
    procCycle    = instrCurrent->cycle;
    cycleCount   = 0;
    clock ();
    return true;
}

void MOS6510::RSTRequest (void)
{
    envReset ();
}

void MOS6510::NMIRequest (void)
{
    endian_16lo8 (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFA));
}

void MOS6510::NMI1Request (void)
{
    endian_16hi8  (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFB));
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
}

void MOS6510::IRQRequest (void)
{
    PushSR   (false);
    setFlagI (true);
    interrupts.irqRequest = false;
}

void MOS6510::IRQ1Request (void)
{
    endian_16lo8 (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFE));
}

void MOS6510::IRQ2Request (void)
{
    endian_16hi8  (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFF));
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
}

void MOS6510::NextInstr (void)
{
    if (!interruptPending ())
    {
        cycleCount = 0;
        procCycle  = &fetchCycle;
        clock ();
    }
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Addressing Routines                                  //
// Addressing operations as described in 64doc by John West and            //
// Marko Makela                                                            //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

// Fetch opcode, increment PC
// Addressing Modes: All
void MOS6510::FetchOpcode (void)
{   // On new instruction all interrupt delays are reset
    interrupts.irqLatch = false;

#ifdef MOS6510_DEBUG
    m_dbgClk = eventContext.getTime (m_phase);
#endif

    instrStartPC  = endian_32lo16 (Register_ProgramCounter++);
    instrOpcode   = envReadMemByte (instrStartPC);
    // Convert opcode to pointer in instruction table
    instrCurrent  = &instrTable[instrOpcode];
    Instr_Operand = 0;
    procCycle     = instrCurrent->cycle;
    cycleCount    = 0;
}

// Fetch value, increment PC
/* Addressing Modes:    Immediate
                        Relative
*/
void MOS6510::FetchDataByte (void)
{   // Get data byte from memory
    Cycle_Data = envReadMemByte (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;

    // Nextline used for Debug
    Instr_Operand = (uint_least16_t) Cycle_Data;
}

// Fetch low address byte, increment PC
/* Addressing Modes:    Stack Manipulation
                        Absolute
                        Zero Page
                        Zerp Page Indexed
                        Absolute Indexed
                        Absolute Indirect
*/                      
void MOS6510::FetchLowAddr (void)
{
    Cycle_EffectiveAddress = envReadMemByte (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;

    // Nextline used for Debug
    Instr_Operand = Cycle_EffectiveAddress;
}

// Read from address, add index register X to it
// Addressing Modes:    Zero Page Indexed
void MOS6510::FetchLowAddrX (void)
{
    FetchLowAddr ();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_X) & 0xFF;
}

// Read from address, add index register Y to it
// Addressing Modes:    Zero Page Indexed
void MOS6510::FetchLowAddrY (void)
{
    FetchLowAddr ();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_Y) & 0xFF;
}

// Fetch high address byte, increment PC (Absoulte Addressing)
// Low byte must have been obtained first!
// Addressing Modes:    Absolute
void MOS6510::FetchHighAddr (void)
{   // Get the high byte of an address from memory
    endian_16hi8 (Cycle_EffectiveAddress, envReadMemByte (endian_32lo16 (Register_ProgramCounter)));
    Register_ProgramCounter++;

    // Nextline used for Debug
    endian_16hi8 (Instr_Operand, endian_16hi8 (Cycle_EffectiveAddress));
}

// Fetch high byte of address, add index register X to low address byte,
// increment PC
// Addressing Modes:    Absolute Indexed
void MOS6510::FetchHighAddrX (void)
{
    uint8_t page;
    // Rev 1.05 (saw) - Call base Function
    FetchHighAddr ();
    page = endian_16hi8 (Cycle_EffectiveAddress);
    Cycle_EffectiveAddress += Register_X;

#ifdef MOS6510_ACCURATE_CYCLES
    // Handle page boundary crossing
    if (endian_16hi8 (Cycle_EffectiveAddress) == page)
        cycleCount++;
#endif
}

// Same as above except dosen't worry about page crossing
void MOS6510::FetchHighAddrX2 (void)
{
    FetchHighAddr ();
    Cycle_EffectiveAddress += Register_X;
}

// Fetch high byte of address, add index register Y to low address byte,
// increment PC
// Addressing Modes:    Absolute Indexed
void MOS6510::FetchHighAddrY (void)
{
    uint8_t page;
    // Rev 1.05 (saw) - Call base Function
    FetchHighAddr ();
    page = endian_16hi8 (Cycle_EffectiveAddress);
    Cycle_EffectiveAddress += Register_Y;

#ifdef MOS6510_ACCURATE_CYCLES
    // Handle page boundary crossing
    if (endian_16hi8 (Cycle_EffectiveAddress) == page)
        cycleCount++;
#endif
}

// Same as above except dosen't worry about page crossing
void MOS6510::FetchHighAddrY2 (void)
{
    FetchHighAddr ();
    Cycle_EffectiveAddress += Register_Y;
}

// Fetch pointer address low, increment PC
/* Addressing Modes:    Absolute Indirect
                        Indirect indexed (post Y)
*/
void MOS6510::FetchLowPointer (void)
{
    Cycle_Pointer = envReadMemByte (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;
    // Nextline used for Debug
    Instr_Operand = Cycle_Pointer;
}

// Read pointer from the address and add X to it
// Addressing Modes:    Indexed Indirect (pre X)
void MOS6510::FetchLowPointerX (void)
{
    endian_16hi8 (Cycle_Pointer, envReadMemDataByte (Cycle_Pointer));
    // Page boundary crossing is not handled
    Cycle_Pointer = (Cycle_Pointer + Register_X) & 0xFF;
}

// Fetch pointer address high, increment PC
// Addressing Modes:    Absolute Indirect
void MOS6510::FetchHighPointer (void)
{
    endian_16hi8 (Cycle_Pointer, envReadMemByte (endian_32lo16 (Register_ProgramCounter)));
    Register_ProgramCounter++;

    // Nextline used for Debug
    endian_16hi8 (Instr_Operand, endian_16hi8 (Cycle_Pointer));
}

// Fetch effective address low
/* Addressing Modes:    Indirect
                        Indexed Indirect (pre X)
                        Indirect indexed (post Y)
*/
void MOS6510::FetchLowEffAddr (void)
{
    Cycle_EffectiveAddress = envReadMemDataByte (Cycle_Pointer);
}

// Fetch effective address high
/* Addressing Modes:    Indirect
                        Indexed Indirect (pre X)
*/
void MOS6510::FetchHighEffAddr (void)
{   // Rev 1.03 (Mike) - Extra +1 removed
    endian_16lo8 (Cycle_Pointer, (Cycle_Pointer + 1) & 0xff);
    endian_16hi8 (Cycle_EffectiveAddress, envReadMemDataByte (Cycle_Pointer));
}

// Fetch effective address high, add Y to low byte of effective address
// Addressing Modes:    Indirect indexed (post Y)
void MOS6510::FetchHighEffAddrY (void)
{
    uint8_t page;
    // Rev 1.05 (saw) - Call base Function
    FetchHighEffAddr ();
    page = endian_16hi8 (Cycle_EffectiveAddress);
    Cycle_EffectiveAddress += Register_Y;

#ifdef MOS6510_ACCURATE_CYCLES
    // Handle page boundary crossing
    if (endian_16hi8 (Cycle_EffectiveAddress) == page)
        cycleCount++;
#endif
}

// Same as above except dosen't worry about page crossing
void MOS6510::FetchHighEffAddrY2 (void)
{
    FetchHighEffAddr ();
    Cycle_EffectiveAddress += Register_Y;
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Data Accessing Routines                                          //
// Data Accessing operations as described in 64doc by John West and        //
// Marko Makela                                                            //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::FetchEffAddrDataByte (void)
{
    Cycle_Data = envReadMemDataByte (Cycle_EffectiveAddress);
}

void MOS6510::PutEffAddrDataByte (void)
{
    envWriteMemByte (Cycle_EffectiveAddress, Cycle_Data);
}

// Push Program Counter Low Byte on stack, decrement S
void MOS6510::PushLowPC (void)
{
    uint_least16_t addr;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    envWriteMemByte (addr, endian_32lo8 (Register_ProgramCounter));
    Register_StackPointer--;
}

// Push Program Counter High Byte on stack, decrement S
void MOS6510::PushHighPC (void)
{
    uint_least16_t addr;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    envWriteMemByte (addr, endian_32hi8 (Register_ProgramCounter));
    Register_StackPointer--;
}

// Increment stack and pull program counter low byte from stack,
void MOS6510::PopLowPC (void)
{
    uint_least16_t addr;
    Register_StackPointer++;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    endian_16lo8 (Cycle_EffectiveAddress, envReadMemDataByte (addr));
}

// Increment stack and pull program counter high byte from stack,
void MOS6510::PopHighPC (void)
{
    uint_least16_t addr;
    Register_StackPointer++;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    endian_16hi8 (Cycle_EffectiveAddress, envReadMemDataByte (addr));
}

void MOS6510::WasteCycle (void)
{
#ifndef MOS6510_ACCURATE_CYCLES
    clock ();
#endif
}

void MOS6510::DebugCycle (void)
{
    if (dodump)
        DumpState ();
    clock ();
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Opcodes                                              //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::brk_instr (void)
{
    PushSR   ();
    setFlagI (true);
    interrupts.irqRequest = false;

    // Check for an NMI, and switch over if pending
    if (interrupts.pending & iNMI)
    {
        event_clock_t cycles = eventContext.getTime (interrupts.nmiClk, m_extPhase);
        if (cycles > MOS6510_INTERRUPT_DELAY)
        {
            interrupts.pending &= ~iNMI;
            instrCurrent = &interruptTable[oNMI];
            procCycle    = instrCurrent->cycle;
        }
    }
}

void MOS6510::cld_instr (void)
{
    setFlagD (false);
    clock ();
}

void MOS6510::cli_instr (void)
{
    bool oldFlagI = getFlagI ();
    setFlagI (false);
    // I flag change is delayed by 1 instruction
    interrupts.irqLatch = oldFlagI ^ getFlagI ();
    // Check to see if interrupts got re-enabled
    if (interrupts.irqs)
        interrupts.irqRequest = true;
    clock ();
}

void MOS6510::jmp_instr (void)
{
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);

#ifdef PC64_TESTSUITE
    // Hack - Output character to screen
    unsigned short pc = endian_32lo16 (Register_ProgramCounter);
    if (pc == 0xffd2)
    {
        char ch = _sidtune_CHRtab[Register_Accumulator];
        switch (ch)
        {
        case 0:
            break;
        case 1:
            printf (" ");
            fprintf (stderr, " ");
        case 0xd:
            printf ("\n");
            fprintf (stderr, "\n");
            filepos = 0;
            break;
        default:
            filetmp[filepos++] = ch;
            printf ("%c", ch);
            fprintf (stderr, "%c", ch);
        }
    }

    if (pc == 0xe16f)
    {
        filetmp[filepos] = '\0';
        envLoadFile (filetmp);
    }
#endif // PC64_TESTSUITE

    clock ();
}

void MOS6510::jsr_instr (void)
{   // JSR uses absolute addressing in this emulation,
    // hence the -1.  The real SID does not use this addressing
    // mode.
    Register_ProgramCounter--;
    PushHighPC ();
}

void MOS6510::pha_instr (void)
{
    uint_least16_t addr;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    envWriteMemByte (addr, Register_Accumulator);
    Register_StackPointer--;
}

/* RTI does not delay the IRQ I flag change as it is set 3 cycles before
 * the end of the opcode, and thus the 6510 has enough time to call the
 * interrupt routine as soon as the opcode ends, if necessary. */
void MOS6510::rti_instr (void)
{
#ifdef MOS6510_DEBUG
    if (dodump)
        fprintf (m_fdbg, "****************************************************\n\n");
#endif

    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
    interrupts.irqLatch = false;
    clock ();
}

void MOS6510::rts_instr (void)
{
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
    Register_ProgramCounter++;
}

void MOS6510::sed_instr (void)
{
    setFlagD (true);
    clock ();
}

void MOS6510::sei_instr (void)
{
    bool oldFlagI = getFlagI ();
    setFlagI (true);
    // I flag change is delayed by 1 instruction
    interrupts.irqLatch   = oldFlagI ^ getFlagI ();
    interrupts.irqRequest = false;
    clock ();
}

void MOS6510::sta_instr (void)
{
    Cycle_Data = Register_Accumulator;
    PutEffAddrDataByte ();
}

void MOS6510::stx_instr (void)
{
    Cycle_Data = Register_X;
    PutEffAddrDataByte ();
}

void MOS6510::sty_instr (void)
{
    Cycle_Data = Register_Y;
    PutEffAddrDataByte ();
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Undocumented Opcodes                                 //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

// Undocumented - This opcode stores the result of A AND X AND the high
// byte of the target address of the operand +1 in memory.
void MOS6510::axa_instr (void)
{
    Cycle_Data = Register_X & Register_Accumulator & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    PutEffAddrDataByte ();
}

// Undocumented - AXS ANDs the contents of the A and X registers (without changing the
// contents of either register) and stores the result in memory.
// AXS does not affect any flags in the processor status register.
void MOS6510::axs_instr (void)
{
    Cycle_Data = Register_Accumulator & Register_X;
    PutEffAddrDataByte ();
}

/* Not required - Operation performed By another method
// Undocumented - HLT crashes the microprocessor.  When this opcode is executed, program
// execution ceases.  No hardware interrupts will execute either.  The author
// has characterized this instruction as a halt instruction since this is the
// most straightforward explanation for this opcode's behaviour.  Only a reset
// will restart execution.  This opcode leaves no trace of any operation
// performed!  No registers affected.
void MOS6510::hlt_instr (void)
{
}
*/

/* Not required - Operation performed By another method
void MOS6510::nop_instr (void)
{
}
*/

/* Not required - Operation performed By another method
void MOS6510::php_instr (void)
{
}
*/

// Undocumented - This opcode ANDs the contents of the Y register with <ab+1> and stores the
// result in memory.
void MOS6510::say_instr (void)
{
    Cycle_Data = Register_Y & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    PutEffAddrDataByte ();
}

/* Not required - Operation performed By another method
// Undocumented - skip next byte.
void MOS6510::skb_instr (void)
{
    Register_ProgramCounter++;
}
*/

/* Not required - Operation performed By another method
// Undocumented - skip next word.
void MOS6510::skw_instr (void)
{
    Register_ProgramCounter += 2;
}
*/

// Undocumented - This opcode ANDs the contents of the X register with <ab+1> and stores the
// result in memory.
void MOS6510::xas_instr (void)
{
    Cycle_Data = Register_X & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    PutEffAddrDataByte ();
}


#ifdef X86
#include "MOS6510\CYCLE_~1\X86.CPP"
//#include "MOS6510\CYCLE_BASED\X86.CPP"
#else

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Binary Coded Decimal Correction                                 //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::Perform_ADC (void)
{
    uint C      = getFlagC ();
    uint A      = Register_Accumulator;
    uint s      = Cycle_Data;
    uint regAC2 = A + s + C;

    if (getFlagD ())
    {   // BCD mode
        uint lo = (A & 0x0f) + (s & 0x0f) + C;
        uint hi = (A & 0xf0) + (s & 0xf0);
        if (lo > 0x09) lo += 0x06;
        if (lo > 0x0f) hi += 0x10;

        setFlagZ (regAC2);
        setFlagN (hi);
        setFlagV (((hi ^ A) & 0x80) && !((A ^ s) & 0x80));
        if (hi > 0x90) hi += 0x60;

        setFlagC (hi > 0xff);
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        setFlagC   (regAC2 > 0xff);
        setFlagV   (((regAC2 ^ A) & 0x80) && !((A ^ s) & 0x80));
        setFlagsNZ (Register_Accumulator = regAC2 & 0xff);
    }
}

void MOS6510::Perform_SBC (void)
{
    uint C      = !getFlagC ();
    uint A      = Register_Accumulator;
    uint s      = Cycle_Data;
    uint regAC2 = A - s - C;

    setFlagC   (regAC2 < 0x100);
    setFlagV   (((regAC2 ^ A) & 0x80) && ((A ^ s) & 0x80));
    setFlagsNZ (regAC2);

    if (getFlagD ())
    {   // BCD mode
        uint lo = (A & 0x0f) - (s & 0x0f) - C;
        uint hi = (A & 0xf0) - (s & 0xf0);
        if (lo & 0x10)
        {
             lo -= 0x06;
             hi -= 0x10;
        }
        if (hi & 0x100) hi -= 0x60;
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        Register_Accumulator = regAC2 & 0xff;
    }
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Addressing Routines                                 //
//-------------------------------------------------------------------------/


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Opcodes                                             //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::adc_instr (void)
{
    Perform_ADC ();
    clock ();
}

void MOS6510::and_instr (void)
{
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
    clock ();
}

void MOS6510::ane_instr (void)
{
    setFlagsNZ (Register_Accumulator = (Register_Accumulator | 0xee) & Register_X & Cycle_Data);
    clock ();
}

void MOS6510::asl_instr (void)
{
    PutEffAddrDataByte ();
    setFlagC   (Cycle_Data & 0x80);
    setFlagsNZ (Cycle_Data <<= 1);
}

void MOS6510::asla_instr (void)
{
    setFlagC   (Register_Accumulator & 0x80);
    setFlagsNZ (Register_Accumulator <<= 1);
    clock ();
}

void MOS6510::branch_instr (bool condition)
{
    if (condition)
#ifdef MOS6510_ACCURATE_CYCLES
    {
        uint8_t page;
        page = endian_32hi8 (Register_ProgramCounter);
        Register_ProgramCounter += (int8_t) Cycle_Data;

        // Handle page boundary crossing
        if (endian_32hi8 (Register_ProgramCounter) != page)
            cycleCount++;
    }
    else
    {
        cycleCount += 2;
        clock ();
    }
#else
    Register_ProgramCounter += (int8_t) Cycle_Data;
#endif
}

void MOS6510::branch2_instr ()
{
    // This only gets processed when page boundary
    // is no crossed.  This causes pending interrupts
    // to be delayed by a cycle
    interrupts.irqClk++;
    interrupts.nmiClk++;
    cycleCount++;
    clock ();
}

void MOS6510::bcc_instr (void)
{
    branch_instr (!getFlagC ());
}

void MOS6510::bcs_instr (void)
{
    branch_instr (getFlagC ());
}

void MOS6510::beq_instr (void)
{
    branch_instr (getFlagZ ());
}

void MOS6510::bit_instr (void)
{
    setFlagZ (Register_Accumulator & Cycle_Data);
    setFlagN (Cycle_Data);
    setFlagV (Cycle_Data & 0x40);
    clock ();
}

void MOS6510::bmi_instr (void)
{
    branch_instr (getFlagN ());
}

void MOS6510::bne_instr (void)
{
    branch_instr (!getFlagZ ());
}

void MOS6510::bpl_instr(void)
{
    branch_instr (!getFlagN ());
}

void MOS6510::bvc_instr (void)
{
    branch_instr (!getFlagV ());
}

void MOS6510::bvs_instr (void)
{
    branch_instr (getFlagV ());
}

void MOS6510::clc_instr (void)
{
    setFlagC (false);
    clock ();
}

void MOS6510::clv_instr (void)
{
    setFlagV (false);
    clock ();
}

void MOS6510::cmp_instr (void)
{
    uint_least16_t tmp = (uint_least16_t) Register_Accumulator - Cycle_Data;
    setFlagsNZ (tmp);
    setFlagC   (tmp < 0x100);
    clock ();
}

void MOS6510::cpx_instr (void)
{
    uint_least16_t tmp = (uint_least16_t) Register_X - Cycle_Data;
    setFlagsNZ (tmp);
    setFlagC   (tmp < 0x100);
    clock ();
}

void MOS6510::cpy_instr (void)
{
    uint_least16_t tmp = (uint_least16_t) Register_Y - Cycle_Data;
    setFlagsNZ (tmp);
    setFlagC   (tmp < 0x100);
    clock ();
}

void MOS6510::dec_instr (void)
{
    PutEffAddrDataByte ();
    setFlagsNZ (--Cycle_Data);
}

void MOS6510::dex_instr (void)
{
    setFlagsNZ (--Register_X);
    clock ();
}

void MOS6510::dey_instr (void)
{
    setFlagsNZ (--Register_Y);
    clock ();
}

void MOS6510::eor_instr (void)
{
    setFlagsNZ (Register_Accumulator^= Cycle_Data);
    clock ();
}

void MOS6510::inc_instr (void)
{
    PutEffAddrDataByte ();
    setFlagsNZ (++Cycle_Data);
}

void MOS6510::inx_instr (void)
{
    setFlagsNZ (++Register_X);
    clock ();
}

void MOS6510::iny_instr (void)
{
    setFlagsNZ (++Register_Y);
    clock ();
}

void MOS6510::lda_instr (void)
{
    setFlagsNZ (Register_Accumulator = Cycle_Data);
    clock ();
}

void MOS6510::ldx_instr (void)
{
    setFlagsNZ (Register_X = Cycle_Data);
    clock ();
}

void MOS6510::ldy_instr (void)
{
    setFlagsNZ (Register_Y = Cycle_Data);
    clock ();
}

void MOS6510::lsr_instr (void)
{
    PutEffAddrDataByte ();
    setFlagC   (Cycle_Data & 0x01);
    setFlagsNZ (Cycle_Data >>= 1);
}

void MOS6510::lsra_instr (void)
{
    setFlagC   (Register_Accumulator & 0x01);
    setFlagsNZ (Register_Accumulator >>= 1);
    clock ();
}

void MOS6510::ora_instr (void)
{
    setFlagsNZ (Register_Accumulator |= Cycle_Data);
    clock ();
}

void MOS6510::pla_instr (void)
{
    uint_least16_t addr;
    Register_StackPointer++;
    addr = Register_StackPointer;
    endian_16hi8 (addr, SP_PAGE);
    setFlagsNZ (Register_Accumulator = envReadMemDataByte (addr));
}

void MOS6510::rol_instr (void)
{
    uint8_t tmp = Cycle_Data & 0x80;
    PutEffAddrDataByte ();
    Cycle_Data   <<= 1;
    if (getFlagC ()) Cycle_Data |= 0x01;
    setFlagsNZ (Cycle_Data);
    setFlagC   (tmp);
}

void MOS6510::rola_instr (void)
{
    uint8_t tmp = Register_Accumulator & 0x80;
    Register_Accumulator <<= 1;
    if (getFlagC ()) Register_Accumulator |= 0x01;
    setFlagsNZ (Register_Accumulator);
    setFlagC   (tmp);
    clock ();
}

void MOS6510::ror_instr (void)
{
    uint8_t tmp  = Cycle_Data & 0x01;
    PutEffAddrDataByte ();
    Cycle_Data >>= 1;
    if (getFlagC ()) Cycle_Data |= 0x80;
    setFlagsNZ (Cycle_Data);
    setFlagC   (tmp);
}

void MOS6510::rora_instr (void)
{
    uint8_t tmp = Register_Accumulator & 0x01;
    Register_Accumulator >>= 1;
    if (getFlagC ()) Register_Accumulator |= 0x80;
    setFlagsNZ (Register_Accumulator);
    setFlagC   (tmp);
    clock ();
}

void MOS6510::sbx_instr (void)
{
    uint tmp = (Register_X & Register_Accumulator) - Cycle_Data;
    setFlagsNZ (Register_X = tmp & 0xff);
    setFlagC   (tmp < 0x100);
    clock ();
}

void MOS6510::sbc_instr (void)
{
    Perform_SBC ();
    clock ();
}

void MOS6510::sec_instr (void)
{
    setFlagC (true);
    clock ();
}

void MOS6510::shs_instr (void)
{
    endian_16lo8 (Register_StackPointer, (Register_Accumulator & Register_X));
    Cycle_Data = (endian_16hi8 (Cycle_EffectiveAddress) + 1) & Register_StackPointer;
    PutEffAddrDataByte ();
}

void MOS6510::tax_instr (void)
{
    setFlagsNZ (Register_X = Register_Accumulator);
    clock ();
}

void MOS6510::tay_instr (void)
{
    setFlagsNZ (Register_Y = Register_Accumulator);
    clock ();
}

void MOS6510::tsx_instr (void)
{   // Rev 1.03 (saw) - Got these tsx and txs reversed
    setFlagsNZ (Register_X = endian_16lo8 (Register_StackPointer));
    clock ();
}

void MOS6510::txa_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_X);
    clock ();
}

void MOS6510::txs_instr (void)
{   // Rev 1.03 (saw) - Got these tsx and txs reversed
    endian_16lo8 (Register_StackPointer, Register_X);
    clock ();
}

void MOS6510::tya_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_Y);
    clock ();
}

void MOS6510::illegal_instr (void)
{
    fprintf (m_fdbg, "\n\nILLEGAL INSTRUCTION, resetting emulation. **************\n");
    DumpState ();
    fprintf (m_fdbg, "********************************************************\n");
    // Perform Environment Reset
    envReset ();
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Undocuemented Opcodes                               //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

// Undocumented - This opcode ANDs the contents of the A register with an immediate value and
// then LSRs the result.
void MOS6510::alr_instr (void)
{
    Register_Accumulator &= Cycle_Data;
    setFlagC   (Register_Accumulator & 0x01);
    setFlagsNZ (Register_Accumulator >>= 1);
    clock ();
}

// Undcouemented - ANC ANDs the contents of the A register with an immediate value and then
// moves bit 7 of A into the Carry flag.  This opcode works basically
// identically to AND #immed. except that the Carry flag is set to the same
// state that the Negative flag is set to.
void MOS6510::anc_instr (void)
{
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
    setFlagC   (getFlagN ());
    clock ();
}

// Undocumented - This opcode ANDs the contents of the A register with an immediate value and
// then RORs the result (Implementation based on that of Frodo C64 Emulator)
void MOS6510::arr_instr (void)
{
    uint8_t data = Cycle_Data & Register_Accumulator;
    Register_Accumulator = data >> 1;
    if (getFlagC ()) Register_Accumulator |= 0x80;

    if (getFlagD ())
    {
        setFlagN (0);
        if (getFlagC ()) setFlagN (1 << SR_NEGATIVE);
        setFlagZ (Register_Accumulator);
        setFlagV ((data ^ Register_Accumulator) & 0x40);

        if ((data & 0x0f) + (data & 0x01) > 5)
            Register_Accumulator  = Register_Accumulator & 0xf0 | (Register_Accumulator + 6) & 0x0f;
        setFlagC (((data + (data & 0x10)) & 0x1f0) > 0x50);
        if (getFlagC ())
            Register_Accumulator += 0x60;
    }
    else
    {
        setFlagsNZ (Register_Accumulator);
        setFlagC   (Register_Accumulator & 0x40);
        setFlagV  ((Register_Accumulator & 0x40) ^ ((Register_Accumulator & 0x20) << 1));
    }
    clock ();
}

// Undocumented - This opcode ASLs the contents of a memory location and then ORs the result
// with the accumulator.
void MOS6510::aso_instr (void)
{
    PutEffAddrDataByte ();
    setFlagC   (Cycle_Data & 0x80);
    Cycle_Data <<= 1;
    setFlagsNZ (Register_Accumulator |= Cycle_Data);
}

// Undocumented - This opcode DECs the contents of a memory location and then CMPs the result
// with the A register.
void MOS6510::dcm_instr (void)
{
    uint_least16_t tmp;
    PutEffAddrDataByte ();
    Cycle_Data--;
    tmp = (uint_least16_t) Register_Accumulator - Cycle_Data;
    setFlagsNZ (tmp);
    setFlagC   (tmp < 0x100);
}

// Undocumented - This opcode INCs the contents of a memory location and then SBCs the result
// from the A register.
void MOS6510::ins_instr (void)
{
    PutEffAddrDataByte ();
    Cycle_Data++;
    Perform_SBC ();
}

// Undocumented - This opcode ANDs the contents of a memory location with the contents of the
// stack pointer register and stores the result in the accumulator, the X
// register, and the stack pointer.  Affected flags: N Z.
void MOS6510::las_instr (void)
{
    setFlagsNZ (Cycle_Data &= endian_16lo8 (Register_StackPointer));
    Register_Accumulator  = Cycle_Data;
    Register_X            = Cycle_Data;
    Register_StackPointer = Cycle_Data;
    clock ();
}

// Undocumented - This opcode loads both the accumulator and the X register with the contents
// of a memory location.
void MOS6510::lax_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_X = Cycle_Data);
    clock ();
}

// Undocumented - LSE LSRs the contents of a memory location and then EORs the result with
// the accumulator.
void MOS6510::lse_instr (void)
{
    PutEffAddrDataByte ();
    setFlagC   (Cycle_Data & 0x01);
    Cycle_Data >>= 1;
    setFlagsNZ (Register_Accumulator ^= Cycle_Data);
}

// Undocumented - This opcode ORs the A register with #xx, ANDs the result with an immediate
// value, and then stores the result in both A and X.
// xx may be EE,EF,FE, OR FF, but most emulators seem to use EE
void MOS6510::oal_instr (void)
{
    setFlagsNZ (Register_X = (Register_Accumulator = (Cycle_Data & (Register_Accumulator | 0xee))));
    clock ();
}

// Undocumented - RLA ROLs the contents of a memory location and then ANDs the result with
// the accumulator.
void MOS6510::rla_instr (void)
{
    uint8_t tmp = Cycle_Data & 0x80;
    PutEffAddrDataByte ();
    Cycle_Data  = Cycle_Data << 1;
    if (getFlagC ()) Cycle_Data |= 0x01;
    setFlagC   (tmp);
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
}

// Undocumented - RRA RORs the contents of a memory location and then ADCs the result with
// the accumulator.
void MOS6510::rra_instr (void)
{
    uint8_t tmp  = Cycle_Data & 0x01;
    PutEffAddrDataByte ();
    Cycle_Data >>= 1;
    if (getFlagC ()) Cycle_Data |= 0x80;
    setFlagC (tmp);
    Perform_ADC ();
}

// Undocumented - This opcode ANDs the contents of the A and X registers (without changing
// the contents of either register) and transfers the result to the stack
// pointer.  It then ANDs that result with the contents of the high byte of
// the target address of the operand +1 and stores that final result in
// memory.
void MOS6510::tas_instr (void)
{
    endian_16lo8  (Register_StackPointer, Register_Accumulator & Register_X);
    uint_least16_t tmp = Register_StackPointer & (Cycle_EffectiveAddress + 1);
    Cycle_Data         = (signed) endian_16lo8 (tmp);
}

#endif // X86


//-------------------------------------------------------------------------//
// Initialise and create CPU Chip                                          //

//MOS6510::MOS6510 (model_t _model, const char *id)
MOS6510::MOS6510 (EventContext *context)
:eventContext(*context),
 Event("CPU"),
 m_fdbg(stdout),
 m_phase(EVENT_CLOCK_PHI2),
 m_extPhase(EVENT_CLOCK_PHI1)
{
    struct ProcessorOperations *instr;
    uint8_t legalMode  = true;
    uint8_t legalInstr = true;
    uint    i, pass;

    //----------------------------------------------------------------------
    // Build up the processor instruction table
    for (i = 0; i < 0x100; i++)
    {
#if MOS6510_DEBUG > 1
        printf ("Building Command %d[%02x]..", i, i);
#endif

        // Pass 1 allocates the memory, Pass 2 builds the instruction
        instr     = &instrTable[i];
        procCycle = 0;

        for (pass = 0; pass < 2; pass++)
        {
            enum {WRITE = 0, READ = 1};
            int access = WRITE;
            cycleCount = -1;
            legalMode  = true;
            legalInstr = true;

            switch (i)
            {
            // Accumulator or Implied addressing
            case ASLn: case CLCn: case CLDn: case CLIn: case CLVn:  case DEXn:
            case DEYn: case INXn: case INYn: case LSRn: case NOPn_: case PHAn:
            case PHPn: case PLAn: case PLPn: case ROLn: case RORn:  case RTIn:
            case RTSn: case SECn: case SEDn: case SEIn: case TAXn:  case TAYn:
            case TSXn: case TXAn: case TXSn: case TYAn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
            break;

            // Immediate and Relative Addressing Mode Handler
            case ADCb: case ANDb:  case ANCb_: case ANEb: case ASRb: case ARRb:
            case BCCr: case BCSr:  case BEQr:  case BMIr: case BNEr: case BPLr:
            case BRKn: case BVCr:  case BVSr:  case CMPb: case CPXb: case CPYb:
            case EORb: case LDAb:  case LDXb:  case LDYb: case LXAb: case NOPb_:
            case ORAb: case SBCb_: case SBXb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchDataByte;
            break;

            // Zero Page Addressing Mode Handler - Read & RMW
            case ADCz:  case ANDz: case BITz: case CMPz: case CPXz: case CPYz:
            case EORz:  case LAXz: case LDAz: case LDXz: case LDYz: case ORAz: 
            case NOPz_: case SBCz:
            case ASLz: case DCPz: case DECz: case INCz: case ISBz: case LSRz:
            case ROLz: case RORz: case SREz: case SLOz: case RLAz: case RRAz:
                access++;
            case SAXz: case STAz: case STXz: case STYz:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Zero Page with X Offset Addressing Mode Handler
            case ADCzx: case ANDzx:  case CMPzx: case EORzx: case LDAzx: case LDYzx:
            case NOPzx_: case ORAzx: case SBCzx:
            case ASLzx: case DCPzx: case DECzx: case INCzx: case ISBzx: case LSRzx:
            case RLAzx:    case ROLzx: case RORzx: case RRAzx: case SLOzx: case SREzx:
                access++;
            case STAzx: case STYzx:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddrX;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Zero Page with Y Offset Addressing Mode Handler
            case LDXzy: case LAXzy:
                access = READ;
            case STXzy: case SAXzy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddrY;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;
            
            // Absolute Addressing Mode Handler
            case ADCa: case ANDa: case BITa: case CMPa: case CPXa: case CPYa:
            case EORa: case LAXa: case LDAa: case LDXa: case LDYa: case NOPa:
            case ORAa: case SBCa:
            case ASLa: case DCPa: case DECa: case INCa: case ISBa: case LSRa: 
            case ROLa: case RORa: case SLOa: case SREa: case RLAa: case RRAa:
                access++;
            case JMPw: case JSRw: case SAXa: case STAa: case STXa: case STYa:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighAddr;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Absolute With X Offset Addressing Mode Handler (Read)
            case ADCax: case ANDax:  case CMPax: case EORax: case LDAax:
            case LDYax: case NOPax_: case ORAax: case SBCax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighAddrX;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
            break;

            // Absolute X (No page crossing handled)
            case ASLax: case DCPax: case DECax: case INCax: case ISBax:
            case LSRax: case RLAax: case ROLax: case RORax: case RRAax:
            case SLOax: case SREax:
                access = READ;
            case SHYax: case STAax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighAddrX2;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Absolute With Y Offset Addresing Mode Handler (Read)
            case ADCay: case ANDay: case CMPay: case EORay: case LASay:
            case LAXay: case LDAay: case LDXay: case ORAay: case SBCay:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighAddrY;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
            break;
            
            // Absolute Y (No page crossing handled)
            case DCPay: case ISBay: case RLAay: case RRAay: case SLOay:
            case SREay:
                access = READ;
            case SHAay: case SHSay: case SHXay: case STAay:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighAddrY2;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Absolute Indirect Addressing Mode Handler
            case JMPi:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowPointer;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighPointer;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowEffAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighEffAddr;
            break;

            // Indexed with X Preinc Addressing Mode Handler
            case ADCix: case ANDix: case CMPix: case EORix: case LAXix: case LDAix:
            case ORAix: case SBCix: 
            case DCPix: case ISBix: case SLOix: case SREix: case RLAix: case RRAix:
                access++;
            case SAXix: case STAix:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowPointer;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowPointerX;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowEffAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighEffAddr;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            // Indexed with Y Postinc Addressing Mode Handler (Read)
            case ADCiy: case ANDiy: case CMPiy: case EORiy: case LAXiy:
            case LDAiy: case ORAiy: case SBCiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowPointer;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowEffAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighEffAddrY;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
            break;
            
            // Indexed Y (No page crossing handled)
            case DCPiy: case ISBiy: case RLAiy: case RRAiy: case SLOiy:
            case SREiy:
                access = READ;
            case SHAiy: case STAiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowPointer;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchLowEffAddr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchHighEffAddrY2;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                if (access == READ) {
                    cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchEffAddrDataByte;
                }
            break;

            default:
                legalMode = false;
            break;
            }

            if (pass)
            {   // Everything upto now is reads and can therefore
                // be blocked through cycle stealing
                for (int c = -1; c < cycleCount;)
                    procCycle[++c].nosteal = false;
            }

#ifdef MOS6510_DEBUG
            if (legalMode)
            {
                cycleCount++;
                if (pass) procCycle[cycleCount].func = &MOS6510::DebugCycle;
            }
#endif // MOS6510_DEBUG

            //---------------------------------------------------------------------------------------
            // Addressing Modes Finished, other cycles are instruction dependent
            switch(i)
            {
            case ADCz:  case ADCzx: case ADCa: case ADCax: case ADCay: case ADCix:
            case ADCiy: case ADCb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::adc_instr;
            break;

            case ANCb_:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::anc_instr;
            break;

            case ANDz:  case ANDzx: case ANDa: case ANDax: case ANDay: case ANDix:
            case ANDiy: case ANDb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::and_instr;
            break;

            case ANEb: // Also known as XAA
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ane_instr;
            break;

            case ARRb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::arr_instr;
            break;

            case ASLn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::asla_instr;
            break;

            case ASLz: case ASLzx: case ASLa: case ASLax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::asl_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case ASRb: // Also known as ALR
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::alr_instr;
            break;

            case BCCr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bcc_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BCSr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bcs_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BEQr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::beq_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BITz: case BITa:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bit_instr;
            break;

            case BMIr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bmi_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BNEr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bne_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BPLr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bpl_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BRKn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushHighPC;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushLowPC;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::brk_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQ1Request;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQ2Request;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchOpcode;
            break;

            case BVCr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bvc_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case BVSr:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::bvs_instr;
#ifdef MOS6510_ACCURATE_CYCLES
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::branch2_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
#endif
            break;

            case CLCn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::clc_instr;
            break;

            case CLDn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::cld_instr;
            break;

            case CLIn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::cli_instr;
            break;

            case CLVn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::clv_instr;
            break;

            case CMPz:  case CMPzx: case CMPa: case CMPax: case CMPay: case CMPix:
            case CMPiy: case CMPb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::cmp_instr;
            break;

            case CPXz: case CPXa: case CPXb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::cpx_instr;
            break;

            case CPYz: case CPYa: case CPYb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::cpy_instr;
            break;

            case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
            case DCPiy: // Also known as DCM
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::dcm_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case DECz: case DECzx: case DECa: case DECax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::dec_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case DEXn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::dex_instr;
            break;

            case DEYn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::dey_instr;
            break;

            case EORz:  case EORzx: case EORa: case EORax: case EORay: case EORix:
            case EORiy: case EORb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::eor_instr;
            break;

/* HLT // Also known as JAM
            case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
            case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
            case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
            case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
                cycleCount++; if (pass) procCycle[cycleCount].func = hlt_instr;
            break;
*/

            case INCz: case INCzx: case INCa: case INCax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::inc_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case INXn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::inx_instr;
            break;

            case INYn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::iny_instr;
            break;

            case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
            case ISBiy: // Also known as INS
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ins_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case JSRw:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::jsr_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushLowPC;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
            case JMPw: case JMPi:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::jmp_instr;
            break;

            case LASay:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::las_instr;
            break;

            case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::lax_instr;
            break;

            case LDAz:  case LDAzx: case LDAa: case LDAax: case LDAay: case LDAix:
            case LDAiy: case LDAb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::lda_instr;
            break;

            case LDXz: case LDXzy: case LDXa: case LDXay: case LDXb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ldx_instr;
            break;

            case LDYz: case LDYzx: case LDYa: case LDYax: case LDYb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ldy_instr;
            break;

            case LSRn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::lsra_instr;
            break;

            case LSRz: case LSRzx: case LSRa: case LSRax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::lsr_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case NOPn_: case NOPb_:
            case NOPz_: case NOPzx_: case NOPa: case NOPax_:
            // NOPb NOPz NOPzx - Also known as SKBn
            // NOPa NOPax      - Also known as SKWn
            break;

            case LXAb: // Also known as OAL
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::oal_instr;
            break;

            case ORAz:  case ORAzx: case ORAa: case ORAax: case ORAay: case ORAix:
            case ORAiy: case ORAb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ora_instr;
            break;

            case PHAn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::pha_instr;
            break;

            case PHPn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushSR;
            break;

            case PLAn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::pla_instr;
                if (pass) procCycle[cycleCount].nosteal = false;
            break;

            case PLPn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopSR;
                if (pass) procCycle[cycleCount].nosteal = false;
            break;

            case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
            case RLAiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rla_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case ROLn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rola_instr;
            break;

            case ROLz: case ROLzx: case ROLa: case ROLax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rol_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case RORn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rora_instr;
            break;

            case RORz: case RORzx: case RORa: case RORax:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::ror_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
            case RRAiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rra_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case RTIn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopSR;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopLowPC;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopHighPC;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rti_instr;
            break;

            case RTSn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopLowPC;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PopHighPC;
                if (pass) procCycle[cycleCount].nosteal = false;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::rts_instr;
            break;

            case SAXz: case SAXzy: case SAXa: case SAXix: // Also known as AXS
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::axs_instr;
            break;

            case SBCz:  case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
            case SBCiy: case SBCb_:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sbc_instr;
            break;

            case SBXb:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sbx_instr;
            break;

            case SECn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sec_instr;
            break;

            case SEDn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sed_instr;
            break;

            case SEIn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sei_instr;
            break;

            case SHAay: case SHAiy: // Also known as AXA
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::axa_instr;
            break;

            case SHSay: // Also known as TAS
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::shs_instr;
            break;

            case SHXay: // Also known as XAS
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::xas_instr;
            break;

            case SHYax: // Also known as SAY
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::say_instr;
            break;

            case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
            case SLOiy: // Also known as ASO
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::aso_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
            case SREiy: // Also known as LSE
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::lse_instr;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PutEffAddrDataByte;
            break;

            case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
            case STAiy:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sta_instr;
            break;

            case STXz: case STXzy: case STXa:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::stx_instr;
            break;

            case STYz: case STYzx: case STYa:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::sty_instr;
            break;

            case TAXn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::tax_instr;
            break;

            case TAYn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::tay_instr;
            break;

            case TSXn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::tsx_instr;
            break;

            case TXAn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::txa_instr;
            break;

            case TXSn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::txs_instr;
            break;

            case TYAn:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::tya_instr;
            break;

            default:
                legalInstr = false;
            break;
            }

            if (!(legalMode || legalInstr))
            {
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::illegal_instr;
            }
            else if (!(legalMode && legalInstr))
            {
                printf ("\nInstruction 0x%x: Not built correctly.\n\n", i);
                exit(1);
            }
 
            cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::NextInstr;
            cycleCount++;
            if (!pass)
            {   // Pass 1 - Allocate Memory
                if (cycleCount)
                {
#ifdef HAVE_EXCEPTIONS
                    instr->cycle = new(std::nothrow) ProcessorCycle[cycleCount];
#else
                    instr->cycle = new ProcessorCycle[cycleCount];
#endif
                    if (!instr->cycle)
                        goto MOS6510_MemAllocFailed;
                    procCycle = instr->cycle;

                    int c = cycleCount;
                    while (c > 0)
                        procCycle[--c].nosteal = true;
                }
            }
            else
                instr->opcode = i;

#if MOS6510_DEBUG > 1
            printf (".");
#endif
        }

        instr->cycles = cycleCount;
#if MOS6510_DEBUG > 1
        printf ("Done [%d Cycles]\n", cycleCount);
#endif
    }

    //----------------------------------------------------------------------
    // Build interrupts
    for (i = 0; i < 3; i++)
    {
#if MOS6510_DEBUG > 1
        printf ("Building Interrupt %d[%02x]..", i, i);
#endif

        // Pass 1 allocates the memory, Pass 2 builds the interrupt
        instr         = &interruptTable[i];
        instr->cycle  = NULL;
        instr->opcode = 0;

        for (int pass = 0; pass < 2; pass++)
        {
            cycleCount = -1;
            if (pass) procCycle = instr->cycle;

            switch (i)
            {
            case oRST:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::RSTRequest;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchOpcode;
            break;

            case oNMI:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushHighPC;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushLowPC;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQRequest;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::NMIRequest;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::NMI1Request;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchOpcode;
            break;

            case oIRQ:
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::WasteCycle;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushHighPC;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::PushLowPC;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQRequest;
                if (pass) procCycle[cycleCount].nosteal = true;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQ1Request;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::IRQ2Request;
                cycleCount++; if (pass) procCycle[cycleCount].func = &MOS6510::FetchOpcode;
            break;
            }

            cycleCount++;
            if (!pass)
            {   // Pass 1 - Allocate Memory
                if (cycleCount)
                {
#ifdef HAVE_EXCEPTIONS
                    instr->cycle = new(std::nothrow) ProcessorCycle[cycleCount];
#else
                    instr->cycle = new ProcessorCycle[cycleCount];
#endif
                    if (!instr->cycle)
                        goto MOS6510_MemAllocFailed;
                    procCycle = instr->cycle;
                    for (int c = 0; c < cycleCount; c++)
                        procCycle[c].nosteal = false;
                }
            }

#if MOS6510_DEBUG > 1
            printf (".");
#endif
        }

        instr->cycles = cycleCount;
#if MOS6510_DEBUG > 1
        printf ("Done [%d Cycles]\n", cycleCount);
#endif
    }

    // Intialise Processor Registers
    Register_Accumulator   = 0;
    Register_X             = 0;
    Register_Y             = 0;

    Cycle_EffectiveAddress = 0;
    Cycle_Data             = 0;
    fetchCycle.func        = &MOS6510::FetchOpcode;

    dodump = false;
    Initialise ();
return;

MOS6510_MemAllocFailed:
    printf ("Unable to allocate enough memory.\n\n");
exit (-1);
}

MOS6510::~MOS6510 ()
{
    struct ProcessorOperations *instr;
    uint i;

    // Remove Opcodes
    for (i = 0; i < 0x100; i++)
    {
        instr = &instrTable[i];
        if (instr->cycle != NULL) delete [] instr->cycle;
    }

    // Remove Interrupts
    for (i = 0; i < 3; i++)
    {
        instr = &interruptTable[i];
        if (instr->cycle != NULL) delete [] instr->cycle;
    }
}


//-------------------------------------------------------------------------//
// Initialise CPU Emulation (Registers)                                    //
void MOS6510::Initialise (void)
{
    // Reset stack
    Register_StackPointer = endian_16 (SP_PAGE, 0xFF);

    // Reset Cycle Count
    cycleCount = 0;
    procCycle  = &fetchCycle;

    // Reset Status Register
    Register_Status = (1 << SR_NOTUSED) | (1 << SR_BREAK);
    // FLAGS are set from data directly and do not require
    // being calculated first before setting.  E.g. if you used
    // SetFlags (0), N flag would = 0, and Z flag would = 1.
    setFlagsNZ (1);
    setFlagC   (false);
    setFlagV   (false);

    // Set PC to some value
    Register_ProgramCounter = 0;
    // IRQs pending check
    interrupts.irqLatch   = false;
    interrupts.irqRequest = false;
    if (interrupts.irqs)
        interrupts.irqRequest = true;

    // Signals
    aec = true;

    m_blocked = false;
    eventContext.schedule (this, 0, m_phase);
}

//-------------------------------------------------------------------------//
// Reset CPU Emulation                                                     //
void MOS6510::reset (void)
{
    // Reset Interrupts
    interrupts.pending = false;
    interrupts.irqs    = 0;

    // Internal Stuff
    Initialise ();

    // Requires External Bits
    // Read from reset vector for program entry point
    endian_16lo8 (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFC));
    endian_16hi8 (Cycle_EffectiveAddress, envReadMemDataByte (0xFFFD));
    Register_ProgramCounter = Cycle_EffectiveAddress;
//    filepos = 0;
}

//-------------------------------------------------------------------------//
// Module Credits                                                          //
void MOS6510::credits (char *sbuffer)
{   // Copy credits to buffer
    sprintf (sbuffer, "%sModule     : MOS6510 Cycle Exact Emulation\n", sbuffer);
    sprintf (sbuffer, "%sWritten By : %s\n", sbuffer, MOS6510_AUTHOR);
    sprintf (sbuffer, "%sVersion    : %s\n", sbuffer, MOS6510_VERSION);
    sprintf (sbuffer, "%sReleased   : %s\n", sbuffer, MOS6510_DATE);
    sprintf (sbuffer, "%sEmail      : %s\n", sbuffer, MOS6510_EMAIL);
}

void MOS6510::debug (bool enable, FILE *out)
{
    dodump = enable;
    if (!(out && enable))
        m_fdbg = stdout;
    else
        m_fdbg = out;
}
