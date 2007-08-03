/***************************************************************************
                          mixer.cpp  -  Sids Mixer Routines
                             -------------------
    begin                : Sun Jul 9 2000
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
 *  $Log: mixer.cpp,v $
 *  Revision 1.11  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.10  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.9  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.8  2001/11/16 19:25:33  s_a_white
 *  Removed m_context as where getting mixed with parent class.
 *
 *  Revision 1.7  2001/10/02 18:29:32  s_a_white
 *  Corrected fixed point maths overflow caused by fastforwarding.
 *
 *  Revision 1.6  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.5  2001/07/25 17:02:37  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.4  2001/07/14 12:47:39  s_a_white
 *  Mixer routines simplified.  Added new and more efficient method of
 *  determining when an output samples is required.
 *
 *  Revision 1.3  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.2  2000/12/12 22:50:15  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include "player.h"
#include "sidendian.h"

const int_least32_t VOLUME_MAX = 255;

SIDPLAY2_NAMESPACE_START

void Player::mixerReset (void)
{   // Fixed point 16.16
    m_sampleClock  = m_samplePeriod & 0x0FFFF;
    // Schedule next sample event
    (context ()).schedule (&mixerEvent,
        m_samplePeriod >> 24, EVENT_CLOCK_PHI1);
}

void Player::mixer (void)
{   // Fixed point 16.16
    event_clock_t cycles;
    char   *buf    = m_sampleBuffer + m_sampleIndex;
    m_sampleClock += m_samplePeriod;
    cycles         = m_sampleClock >> 16;
    m_sampleClock &= 0x0FFFF;
    m_sampleIndex += (this->*output) (buf);
 
    // Schedule next sample event
    (context ()).schedule (&mixerEvent, cycles, EVENT_CLOCK_PHI1);

    // Filled buffer
    if (m_sampleIndex >= m_sampleCount)
        m_running = false;
}


//-------------------------------------------------------------------------
// Generic sound output generation routines
//-------------------------------------------------------------------------
inline
int_least32_t Player::monoOutGenericLeftIn (uint_least8_t bits)
{
    return sid[0]->output (bits) * m_leftVolume / VOLUME_MAX;
}

inline
int_least32_t Player::monoOutGenericStereoIn (uint_least8_t bits)
{
    // Convert to mono
    return ((sid[0]->output (bits) * m_leftVolume) +
        (sid[1]->output (bits) * m_rightVolume)) / (VOLUME_MAX * 2);
}

inline
int_least32_t Player::monoOutGenericRightIn (uint_least8_t bits)
{
    return sid[1]->output (bits) * m_rightVolume / VOLUME_MAX;
}


//-------------------------------------------------------------------------
// 8 bit sound output generation routines
//-------------------------------------------------------------------------
uint_least32_t Player::monoOut8MonoIn (char *buffer)
{
    *buffer = (char) monoOutGenericLeftIn (8) ^ '\x80';
    return sizeof (char);
}

uint_least32_t Player::monoOut8StereoIn (char *buffer)
{
    *buffer = (char) monoOutGenericStereoIn (8) ^ '\x80';
    return sizeof (char);
}

uint_least32_t Player::monoOut8StereoRIn (char *buffer)
{
    *buffer = (char) monoOutGenericRightIn (8) ^ '\x80';
    return sizeof (char);
}

uint_least32_t Player::stereoOut8MonoIn (char *buffer)
{
    char sample = (char) monoOutGenericLeftIn (8) ^ '\x80';
    buffer[0] = sample; 
    buffer[1] = sample; 
    return (2 * sizeof (char));
}

uint_least32_t Player::stereoOut8StereoIn (char *buffer)
{
    buffer[0] = (char) monoOutGenericLeftIn  (8) ^ '\x80';
    buffer[1] = (char) monoOutGenericRightIn (8) ^ '\x80';
    return (2 * sizeof (char));
}

//-------------------------------------------------------------------------
// 16 bit sound output generation routines
//-------------------------------------------------------------------------
uint_least32_t Player::monoOut16MonoIn (char *buffer)
{
    endian_16 (buffer, (uint_least16_t) monoOutGenericLeftIn (16));
    return sizeof (uint_least16_t);
}

uint_least32_t Player::monoOut16StereoIn (char *buffer)
{
    endian_16 (buffer, (uint_least16_t) monoOutGenericStereoIn (16));
    return sizeof (uint_least16_t);
}

uint_least32_t Player::monoOut16StereoRIn (char *buffer)
{
    endian_16 (buffer, (uint_least16_t) monoOutGenericRightIn (16));
    return sizeof (uint_least16_t);
}

uint_least32_t Player::stereoOut16MonoIn (char *buffer)
{
    uint_least16_t sample = (uint_least16_t) monoOutGenericLeftIn (16);
    endian_16 (buffer, sample);
    endian_16 (buffer + sizeof(uint_least16_t), sample);
    return (2 * sizeof (uint_least16_t));
}

uint_least32_t Player::stereoOut16StereoIn (char *buffer)
{
    endian_16 (buffer, (uint_least16_t) monoOutGenericLeftIn  (16));
    endian_16 (buffer + sizeof(uint_least16_t),
               (uint_least16_t) monoOutGenericRightIn (16));
    return (2 * sizeof (uint_least16_t));
}

SIDPLAY2_NAMESPACE_STOP
