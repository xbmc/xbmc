/***************************************************************************
                          c64sid.h  -  ReSid Wrapper for redefining the
                                       filter
                             -------------------
    begin                : Fri Apr 4 2001
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

#include <cstring>
#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid.h"
#include "resid-emu.h"


char ReSID::m_credit[];

ReSID::ReSID (sidbuilder *builder)
:sidemu(builder),
 m_context(NULL),
 m_phase(EVENT_CLOCK_PHI1),
#ifdef HAVE_EXCEPTIONS
 m_sid(*(new(std::nothrow) RESID::SID)),
#else
 m_sid(*(new RESID::SID)),
#endif
 m_gain(100),
 m_status(true),
 m_locked(false),
 m_optimisation(0)
{
    char *p = m_credit;
    m_error = "N/A";

    // Setup credits
    //sprintf (p, "ReSID V%s Engine:", VERSION);
    sprintf (p, "ReSID V%s Engine:", "foo");
    p += strlen (p) + 1;
    strcpy  (p, "\t(C) 1999-2002 Simon White <sidplay2@yahoo.com>");
    p += strlen (p) + 1;
    sprintf (p, "MOS6581 (SID) Emulation (ReSID V%s):", RESID::resid_version_string);
    p += strlen (p) + 1;
    sprintf (p, "\t(C) 1999-2002 Dag Lem <resid@nimrod.no>");
    p += strlen (p) + 1;
    *p = '\0';

    if (!&m_sid)
    {
        m_error  = "RESID ERROR: Unable to create sid object";
        m_status = false;
        return;
    }
    reset (0);
}

ReSID::~ReSID ()
{
    if (&m_sid)
        delete &m_sid;
}

bool ReSID::filter (const sid_filter_t *filter)
{
    RESID::fc_point fc[0x802];
    const RESID::fc_point *f0 = fc;
    int   points = 0;

    if (filter == NULL)
    {   // Select default filter
        m_sid.fc_default (f0, points);
    }
    else
    {   // Make sure there are enough filter points and they are legal
        points = filter->points;
        if ((points < 2) || (points > 0x800))
            return false;

        {
            const sid_fc_t  fstart = {-1, 0};
            const sid_fc_t *fprev  = &fstart, *fin = filter->cutoff;
            RESID::fc_point *fout = fc;
            // Last check, make sure they are list in numerical order
            // for both axis
            while (points-- > 0)
            {
                if ((*fprev)[0] >= (*fin)[0])
                    return false;
                fout++;
                (*fout)[0] = (RESID::sound_sample) (*fin)[0];
                (*fout)[1] = (RESID::sound_sample) (*fin)[1];
                fprev      = fin++;
            }
            // Updated ReSID interpolate requires we
            // repeat the end points
            (*(fout+1))[0] = (*fout)[0];
            (*(fout+1))[1] = (*fout)[1];
            fc[0][0] = fc[1][0];
            fc[0][1] = fc[1][1];
            points   = filter->points + 2;
        }
    }

    // function from reSID
    points--;
    RESID::interpolate (f0, f0 + points, m_sid.fc_plotter (), 1.0);
    return true;
}

// Standard component options
void ReSID::reset (uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset ();
    m_sid.write (0x18, volume);
}

uint8_t ReSID::read (uint_least8_t addr)
{
    event_clock_t cycles = m_context->getTime (m_accessClk, m_phase);
    m_accessClk += cycles;
    if (m_optimisation)
    {
        if (cycles)
            m_sid.clock (cycles);
    }
    else
    {
        while(cycles--)
            m_sid.clock ();
         
    }
    return m_sid.read (addr);
}

void ReSID::write (uint_least8_t addr, uint8_t data)
{
    event_clock_t cycles = m_context->getTime (m_accessClk, m_phase);
    m_accessClk += cycles;
    if (m_optimisation)
    {
        if (cycles)
            m_sid.clock (cycles);
    }
    else
    {
        while(cycles--)
            m_sid.clock ();
         
    }
    m_sid.write (addr, data);
}

int_least32_t ReSID::output (uint_least8_t bits)
{
    event_clock_t cycles = m_context->getTime (m_accessClk, m_phase);
    m_accessClk += cycles;
    if (m_optimisation)
    {
        if (cycles)
            m_sid.clock (cycles);
    }
    else
    {
        while(cycles--)
            m_sid.clock ();
         
    }
    return m_sid.output (bits) * m_gain / 100;
}

void ReSID::filter (bool enable)
{
    m_sid.enable_filter (enable);
}

void ReSID::voice (uint_least8_t num, uint_least8_t volume,
                   bool mute)
{   // At this time only mute is supported
    m_sid.mute (num, mute);
}
    
void ReSID::gain (int_least8_t percent)
{
    // 0 to 99 is loss, 101 - 200 is gain
    m_gain  = percent;
    m_gain += 100;
    if (m_gain > 200)
        m_gain = 200;
}

void ReSID::sampling (uint_least32_t freq)
{
    m_sid.set_sampling_parameters (1000000, RESID::SAMPLE_FAST, freq);
}

// Set execution environment and lock sid to it
bool ReSID::lock (c64env *env)
{
    if (env == NULL)
    {
        if (!m_locked)
            return false;
        m_locked  = false;
        m_context = NULL;
    }
    else
    {
        if (m_locked)
            return false;
        m_locked  = true;
        m_context = &env->context ();
    }
    return true;
}

// Set the emulated SID model
void ReSID::model (sid2_model_t model)
{
    if (model == SID2_MOS8580)
        m_sid.set_chip_model (RESID::MOS8580);
    else
        m_sid.set_chip_model (RESID::MOS6581);
}

// Set optimisation level
void ReSID::optimisation (uint_least8_t level)
{
    m_optimisation = level;
}
