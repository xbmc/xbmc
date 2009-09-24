/***************************************************************************
                          SidFilter.cpp  -  filter type decoding support
                             -------------------
    begin                : Sun Mar 11 2001
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

#include <sidplay/sidtypes.h>
#include "libini.h"

// For compatibilty with libsidplay2-0.7.
#ifndef sid_filter_t
typedef int sid_fc_t[2];
typedef struct
{
    sid_fc_t       cutoff[0x800];
    uint_least16_t points;
} sid_filter_t;
#define sid_filter_t sid_filter_t
#endif

#ifndef SIDPLAY1_EMUCFG_H
// For compatibilty with libsidplay1
// If you use this and libsidplay1 headers, make sure
// those are included first
// Default filter parameters.
const float SIDEMU_DEFAULTFILTERFS = (float) 400.0;
const float SIDEMU_DEFAULTFILTERFM = (float) 60.0;
const float SIDEMU_DEFAULTFILTERFT = (float) 0.05;
#endif // SIDPLAY1_EMUCFG_H


class SID_EXTERN SidFilter
{
protected:
    bool  m_status;
    char *m_errorString;
    sid_filter_t m_filter;

protected:
    void readType1 (ini_fd_t ini);
    void readType2 (ini_fd_t ini);
    void clear ();

public:
    SidFilter ();
    ~SidFilter ();

    void                read      (const char *filename);
    void                read      (ini_fd_t ini, const char *heading);
    void                calcType2 (double fs, double fm, double ft);
    const char*         error     (void) { return m_errorString; }
    const sid_filter_t* provide   () const;

    operator bool () { return m_status; }
    const SidFilter&    operator= (const SidFilter    &filter);
    const sid_filter_t &operator= (const sid_filter_t &filter);
    const sid_filter_t *operator= (const sid_filter_t *filter);
};
