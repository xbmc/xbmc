/***************************************************************************
                          mos6510c.h  -  Cycle Accurate 6510 Emulation
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
/***************************************************************************
 *  $Log: mos6510c.h,v $
 *  Revision 1.25  2004/04/23 01:01:37  s_a_white
 *  Added debug clock to record cycle instructions starts on.
 *
 *  Revision 1.24  2004/03/07 22:38:57  s_a_white
 *  Rename write to nosteal since it needs setting for non memory access cycles to
 *
 *  Revision 1.23  2004/03/06 21:07:12  s_a_white
 *  Don't start a new cycle stealing sequence if one is already started!  This can
 *  happen if an interrupt occurs during an optimised sleep.
 *
 *  Revision 1.22  2004/02/29 14:33:59  s_a_white
 *  If an interrupt occurs during a branch instruction but after the decision
 *  has occured then it should not be delayed.
 *
 *  Revision 1.21  2003/10/29 22:18:03  s_a_white
 *  IRQs are now only taken in on phase 1 as previously they could be clocked
 *  in on both phases of the cycle resulting in them sometimes not being
 *  delayed long enough.
 *
 *  Revision 1.20  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.19  2003/10/16 07:48:32  s_a_white
 *  Allow redirection of debug information of file.
 *
 *  Revision 1.18  2003/01/20 23:10:48  s_a_white
 *  Fixed RMW instructions to perform memory re-write on the correct cycle.
 *
 *  Revision 1.17  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.16  2003/01/17 08:42:09  s_a_white
 *  Event scheduler phase support.  Better handling the operation of IRQs
 *  during stolen cycles.
 *
 *  Revision 1.15  2002/11/28 20:35:06  s_a_white
 *  Reduced number of thrown exceptions when dma occurs.
 *
 *  Revision 1.14  2002/11/25 20:10:55  s_a_white
 *  A bus access failure should stop the CPU dead like the cycle never started.
 *  This is currently simulated using throw (execption handling) for now.
 *
 *  Revision 1.13  2002/11/21 19:52:48  s_a_white
 *  CPU upgraded to be like other components.  Theres nolonger a clock call,
 *  instead events are registered to occur at a specific time.
 *
 *  Revision 1.12  2002/11/19 22:57:33  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.11  2002/11/01 17:35:27  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.10  2001/08/05 15:46:02  s_a_white
 *  No longer need to check on which cycle an instruction ends or when to print
 *  debug information.
 *
 *  Revision 1.9  2001/07/14 16:48:03  s_a_white
 *  cycleCount and related must roject.Syn
 *
 *  Revision 1.8  2001/07/14 13:15:30  s_a_white
 *  Accumulator is now unsigned, which improves code readability.  Emulation
 *  tested with testsuite 2.15.  Various instructions required modification.
 *
 *  Revision 1.7  2001/03/28 21:17:34  s_a_white
 *  Added support for proper RMW instructions.
 *
 *  Revision 1.6  2001/03/24 18:09:17  s_a_white
 *  On entry to interrupt routine the first instruction in the handler is now always
 *  executed before pending interrupts are re-checked.
 *
 *  Revision 1.5  2001/03/19 23:48:21  s_a_white
 *  Interrupts made virtual to allow for redefintion for Sidplay1 compatible
 *  interrupts.
 *
 *  Revision 1.4  2001/03/09 22:28:03  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.3  2001/02/13 21:03:33  s_a_white
 *  Changed inlines to non-inlines due to function bodies not being in header.
 *
 *  Revision 1.2  2000/12/11 19:04:32  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _mos6510c_h_
#define _mos6510c_h_

#include <stdio.h>
#include "sidtypes.h"
#include "sidendian.h"


class MOS6510: public C64Environment, public Event
{
protected:
    // External signals
    bool aec; /* Address Controller, blocks reads */
    bool m_blocked;
    event_clock_t m_stealingClk;
    event_clock_t m_dbgClk;
    FILE *m_fdbg;

    bool dodump;
    EventContext &eventContext;
    event_phase_t m_phase;    // Clock phase in use by the processor
    event_phase_t m_extPhase; // Clock phase when external events appear

    struct ProcessorCycle
    {
        void (MOS6510::*func)(void);
        bool nosteal;
        ProcessorCycle ()
            :func(NULL), nosteal(false) { ; }
    };

    // Declare processor operations
    struct ProcessorOperations
    {
        struct ProcessorCycle *cycle;
        uint          cycles;
        uint_least8_t opcode;
        ProcessorOperations ()
            :cycle(NULL), cycles (0) { ; }
    };

    struct ProcessorCycle       fetchCycle;
    struct ProcessorCycle      *procCycle;
    struct ProcessorOperations  instrTable[0x100];
    struct ProcessorOperations  interruptTable[3];
    struct ProcessorOperations *instrCurrent;

    uint_least16_t instrStartPC;
    uint_least8_t  instrOpcode;
    int_least8_t   lastAddrCycle;
    int_least8_t   cycleCount;

    // Pointers to the current instruction cycle
    uint_least16_t Cycle_EffectiveAddress;
    uint8_t        Cycle_Data;
    uint_least16_t Cycle_Pointer;

    uint8_t        Register_Accumulator;
    uint8_t        Register_X;
    uint8_t        Register_Y;
    uint_least32_t Register_ProgramCounter;
    uint8_t        Register_Status;
    uint_least8_t  Register_c_Flag;
    uint_least8_t  Register_n_Flag;
    uint_least8_t  Register_v_Flag;
    uint_least8_t  Register_z_Flag;
    uint_least16_t Register_StackPointer;
    uint_least16_t Instr_Operand;

    // Interrupts
    struct
    {
        uint_least8_t  pending;
        uint_least8_t  irqs;
        event_clock_t  nmiClk;
        event_clock_t  irqClk;
        bool           irqRequest;
        bool           irqLatch;
    } interrupts;

    uint8_t        Debug_Data;
    uint_least16_t Debug_EffectiveAddress;
    uint_least8_t  Debug_Opcode;
    uint_least16_t Debug_Operand;
    uint_least16_t Debug_ProgramCounter;

protected:
    void        clock            (void);
    void        event            (void);
    void        Initialise       (void);
    // Declare Interrupt Routines
    inline void RSTRequest       (void);
    inline void RST1Request      (void);
    inline void NMIRequest       (void);
    inline void NMI1Request      (void);
    inline void IRQRequest       (void);
    inline void IRQ1Request      (void);
    inline void IRQ2Request      (void);
    bool        interruptPending (void);

    // Declare Instruction Routines
    virtual void FetchOpcode         (void);
    void        NextInstr            (void);
    inline void FetchDataByte        (void);
    inline void FetchLowAddr         (void);
    inline void FetchLowAddrX        (void);
    inline void FetchLowAddrY        (void);
    inline void FetchHighAddr        (void);
    inline void FetchHighAddrX       (void);
    inline void FetchHighAddrX2      (void);
    inline void FetchHighAddrY       (void);
    inline void FetchHighAddrY2      (void);
    inline void FetchLowEffAddr      (void);
    inline void FetchHighEffAddr     (void);
    inline void FetchHighEffAddrY    (void);
    inline void FetchHighEffAddrY2   (void);
    inline void FetchLowPointer      (void);
    inline void FetchLowPointerX     (void);
    inline void FetchHighPointer     (void);
    inline void FetchEffAddrDataByte (void);
    inline void PutEffAddrDataByte   (void);
    inline void PushLowPC            (void);
    inline void PushHighPC           (void);
    inline void PushSR               (bool b_flag);
    inline void PushSR               (void);
    inline void PopLowPC             (void);
    inline void PopHighPC            (void);
    inline void PopSR                (void);
    inline void WasteCycle           (void);
    inline void DebugCycle           (void);

    // Delcare Instruction Operation Routines
    inline void adc_instr     (void);
    inline void alr_instr     (void);
    inline void anc_instr     (void);
    inline void and_instr     (void);
    inline void ane_instr     (void);
    inline void arr_instr     (void);
    inline void asl_instr     (void);
    inline void asla_instr    (void);
    inline void aso_instr     (void);
    inline void axa_instr     (void);
    inline void axs_instr     (void);
    inline void bcc_instr     (void);
    inline void bcs_instr     (void);
    inline void beq_instr     (void);
    inline void bit_instr     (void);
    inline void bmi_instr     (void);
    inline void bne_instr     (void);
    inline void branch_instr  (bool condition);
    inline void branch2_instr (void);
    inline void bpl_instr     (void);
    inline void brk_instr     (void);
    inline void bvc_instr     (void);
    inline void bvs_instr     (void);
    inline void clc_instr     (void);
    inline void cld_instr     (void);
    inline void cli_instr     (void);
    inline void clv_instr     (void);
    inline void cmp_instr     (void);
    inline void cpx_instr     (void);
    inline void cpy_instr     (void);
    inline void dcm_instr     (void);
    inline void dec_instr     (void);
    inline void dex_instr     (void);
    inline void dey_instr     (void);
    inline void eor_instr     (void);
    inline void inc_instr     (void);
    inline void ins_instr     (void);
    inline void inx_instr     (void);
    inline void iny_instr     (void);
    inline void jmp_instr     (void);
    inline void jsr_instr     (void);
    inline void las_instr     (void);
    inline void lax_instr     (void);
    inline void lda_instr     (void);
    inline void ldx_instr     (void);
    inline void ldy_instr     (void);
    inline void lse_instr     (void);
    inline void lsr_instr     (void);
    inline void lsra_instr    (void);
    inline void oal_instr     (void);
    inline void ora_instr     (void);
    inline void pha_instr     (void);
    inline void pla_instr     (void);
    inline void rla_instr     (void);
    inline void rol_instr     (void);
    inline void rola_instr    (void);
    inline void ror_instr     (void);
    inline void rora_instr    (void);
    inline void rra_instr     (void);
    inline void rti_instr     (void);
    inline void rts_instr     (void);
    inline void sbx_instr     (void);
    inline void say_instr     (void);
    inline void sbc_instr     (void);
    inline void sec_instr     (void);
    inline void sed_instr     (void);
    inline void sei_instr     (void);
    inline void shs_instr     (void);
    inline void sta_instr     (void);
    inline void stx_instr     (void);
    inline void sty_instr     (void);
    inline void tas_instr     (void);
    inline void tax_instr     (void);
    inline void tay_instr     (void);
    inline void tsx_instr     (void);
    inline void txa_instr     (void);
    inline void txs_instr     (void);
    inline void tya_instr     (void);
    inline void xas_instr     (void);
    void        illegal_instr (void);

    // Declare Arithmatic Operations
    inline void Perform_ADC   (void);
    inline void Perform_SBC   (void);

public:
    MOS6510 (EventContext *context);
    virtual ~MOS6510 ();
    virtual void reset     (void);
    virtual void credits   (char *str);
    virtual void DumpState (void);
    void         debug     (bool enable, FILE *out);
    void         aecSignal (bool state);

    // Non-standard functions
    virtual void triggerRST (void);
    virtual void triggerNMI (void);
    virtual void triggerIRQ (void);
    void         clearIRQ   (void);
};


//-------------------------------------------------------------------------//
// Emulate One Complete Cycle                                              //
inline void MOS6510::clock (void)
{
    int_least8_t i = cycleCount++;
    if (procCycle[i].nosteal || aec)
    {
        (this->*(procCycle[i].func)) ();
        return;
    }
    else if (!m_blocked)
    {
        m_blocked     = true;
        m_stealingClk = eventContext.getTime (m_phase);
    }
    cycleCount--;
    eventContext.cancel (this);
}

inline void MOS6510::event (void)
{
    eventContext.schedule (this, 1, m_phase);
    clock ();
}

#endif // _mos6510c_h_
