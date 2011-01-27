/***************************************************************************
                          mos656x.h  -  Minimal VIC emulation
                             -------------------
    begin                : Wed May 21 2001
    copyright            : (C) 2001 by Simon White
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

#ifndef _mos656x_h_
#define _mos656x_h_

#include "component.h"
#include "event.h"

typedef enum
{
    MOS6567R56A, /* OLD NTSC CHIP */
    MOS6567R8,   /* NTSC */
    MOS6569      /* PAL */
} mos656x_model_t;


class MOS656X: public component, public Event
{
private:
    static const char *credit;

protected:
    uint8_t        regs[0x40];
    uint8_t        icr, idr, ctrl1;
    uint_least16_t yrasters, xrasters, raster_irq;
    uint_least16_t raster_x, raster_y;
    uint_least16_t first_dma_line, last_dma_line, y_scroll;
    bool           bad_lines_enabled, bad_line;
    bool           vblanking;
    bool           lp_triggered;
    uint8_t        lpx, lpy;
    uint8_t       &sprite_enable, &sprite_y_expansion;
    uint8_t        sprite_dma, sprite_expand_y;
    uint8_t        sprite_mc_base[8];

    event_clock_t m_rasterClk;
    EventContext &event_context;
    event_phase_t m_phase;

protected:
    MOS656X (EventContext *context);
    void    event       (void);
    void    trigger     (int irq);

    // Environment Interface
    virtual void interrupt (bool state) = 0;
    virtual void addrctrl  (bool state) = 0;

public:
    void    chip  (mos656x_model_t model);
    void    lightpen ();

    // Component Standard Calls
    void    reset (void);
    uint8_t read  (uint_least8_t addr);
    void    write (uint_least8_t addr, uint8_t data);
    const   char *credits (void) {return credit;}
};


/***************************************************************************
 * Inline functions
 **************************************************************************/

enum
{
    MOS656X_INTERRUPT_RST     = 1 << 0,
    MOS656X_INTERRUPT_LP      = 1 << 3,
    MOS656X_INTERRUPT_REQUEST = 1 << 7
};

#endif // _mos656x_h_
