/***************************************************************************
                          SidUsage.h  -  sidusage file support
                             -------------------
    begin                : Tues Nov 19 2002
    copyright            : (C) 2002 by Simon White
    email                : sidplay2@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SidUsage_h_
#define _SidUsage_h_

#include <stdio.h>
#include <sidplay/sidusage.h>
#include <sidplay/utils/SidTuneMod.h>

struct SidTuneInfo;

// Extended usuage information
struct sid2_usage_t: public sid_usage_t
{
    uint_least16_t start; // Load image start address
    uint_least16_t end;   // Load image end address
    char           md5[SIDTUNE_MD5_LENGTH + 1]; // Tunes MD5 key
    uint_least16_t length;  // usage scan length

    // Copy common parts of basic usage to extended usage.
    sid2_usage_t &sid2_usage_t::operator= (const sid_usage_t &usage)
    {
        *((sid_usage_t *) this) = usage;
        return *this;
    }
};

class SID_EXTERN SidUsage
{
private:
    char m_decodeMAP[0x100][3];
    // Ignore errors
    uint_least8_t m_filterMAP[0x10000];

protected:
    bool  m_status;
    const char *m_errorString;

private:
    // Old obsolete MM file format
    bool           readMM     (FILE *file, sid2_usage_t &usage, const char *ext);
    // Sid Memory Map (MM file)
    bool           readSMM    (FILE *file, sid2_usage_t &usage, const char *ext);
    void           writeSMM   (FILE *file, const sid2_usage_t &usage);
    void           writeMAP   (FILE *file, const sid2_usage_t &usage);
    void           filterMAP  (int from, int to, uint_least8_t mask);

public:
    SidUsage ();

    // @FIXME@ add ext to these
    void           read       (const char *filename, sid2_usage_t &usage);
    void           write      (const char *filename, const sid2_usage_t &usage);
    const char *   error      (void) { return m_errorString; }

    operator bool () { return m_status; }
};

#endif // _SidUsage_h_
