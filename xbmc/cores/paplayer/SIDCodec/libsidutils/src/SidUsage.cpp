/***************************************************************************
                          SidUsage.cpp  -  sidusage file support
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

#include <stdio.h>
#include <sidplay/sidendian.h>
#include <sidplay/SidTune.h>
#include "SidUsage.h"
#include "smm0.h"

static const char *txt_na        = "SID Usage: N/A";
static const char *txt_file      = "SID Usage: Unable to open file";
static const char *txt_corrupt   = "SID Usage: File corrupt";
static const char *txt_invalid   = "SID Usage: Invalid file format";
static const char *txt_missing   = "SID Usage: Mandatory chunks missing";
static const char *txt_supported = "SID Usage: File type not supported";
static const char *txt_reading   = "SID Usage: Error reading file";
static const char *txt_writing   = "SID Usage: Error writing file";


SidUsage::SidUsage ()
:m_status(false)
{
    m_errorString = txt_na;

    // Probably a better way to do this
    // Detup decode table to convert usage flags to text characters
    for (int i = 0; i < SID_LOAD_IMAGE; i++)
    {        
        m_decodeMAP[i][2] = '\0';
        switch (i & (SID_EXECUTE | SID_STACK | SID_SAMPLE))
        {
        case 0:
            switch (i & (SID_READ | SID_WRITE))
            {
            case 0: // Not used
                m_decodeMAP[i][0] = '.';
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = ',';
                break;
            case SID_READ:
                m_decodeMAP[i][0] = 'r';
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'R';
                break;
            case SID_WRITE:
                m_decodeMAP[i][0] = 'w';
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'W';
                break;
            case SID_WRITE | SID_READ:
                m_decodeMAP[i][0] = 'x';
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'X';
                break;
            }
            break;
        case SID_EXECUTE:
            m_decodeMAP[i][0] = 'p';
            if (i & SID_WRITE)
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'M';
            else
                m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'P';
            break;
        case SID_STACK:
            m_decodeMAP[i][0] = 's';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'S';
            break;
        case SID_STACK | SID_EXECUTE:
            m_decodeMAP[i][0] = '$';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = '&';
            break;
        case SID_SAMPLE:
            m_decodeMAP[i][0] = 'd';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'D';
            break;
        case SID_SAMPLE | SID_EXECUTE:
            m_decodeMAP[i][0] = 'e';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'E';
            break;
        case SID_SAMPLE | SID_STACK:
            m_decodeMAP[i][0] = 'z';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = 'Z';
            break;
        case SID_SAMPLE | SID_STACK | SID_EXECUTE:
            m_decodeMAP[i][0] = '+';
            m_decodeMAP[SID_LOAD_IMAGE | i][0] = '*';
            break;
        }

        switch (i & (SID_BAD_EXECUTE | SID_BAD_READ))
        {
        case SID_BAD_READ:
            m_decodeMAP[i][1] = '?';
            // Fine when is load image
            m_decodeMAP[SID_LOAD_IMAGE | i][1] = ' ';
            break;
        case SID_BAD_EXECUTE:
        case SID_BAD_EXECUTE | SID_BAD_READ:
            m_decodeMAP[i][1] = '!';
            // Fine when is load image
            m_decodeMAP[SID_LOAD_IMAGE | i][1] = ' ';
            break;
        default:
            m_decodeMAP[i][1] = ' ';
            // We wrote first, meaning location need
            // not be in load image.
            m_decodeMAP[SID_LOAD_IMAGE | i][1] = '-';
        }
    }

    // Initialise post filter
    memset (m_filterMAP, ~0, sizeof (m_filterMAP));
    // We are filtering off bad reads on these standard known locations
    filterMAP (0x0000, 0x0001, SID_BAD_READ); /* Bank regs */
    filterMAP (0x00a5, 0x00ac, SID_BAD_READ); /* Bug in tons of SIDs */
    filterMAP (0x00fb, 0x00ff, SID_BAD_READ); /* Bug in tons of SIDs */
    filterMAP (0x02a6, 0x02a6, SID_BAD_READ); /* PAL/NTSC flag */
    filterMAP (0x02a7, 0x02ff, SID_BAD_READ); /* Bug in tons of sids */
    filterMAP (0x0314, 0x0319, SID_BAD_READ); /* Interrupt vectors */
    filterMAP (0x07e8, 0x07f7, SID_BAD_READ); /* Bug in tons of sids */
}

void SidUsage::read (const char *filename, sid2_usage_t &usage)
{
    FILE *file;
    size_t i = strlen (filename);
    const char *ext = NULL;

    m_status = false;
    file = fopen (filename, "rb");
    if (file == NULL)
    {
        m_errorString = txt_file;
        return;
    }

    // Find start of extension
    while (i > 0)
    {
        if (filename[--i] == '.')
        {
            ext = &filename[i + 1];
            break;
        }
    }

    // Initialise optional fields
    usage.flags  = 0;
    usage.md5[0] = '\0';
    usage.length = 0;

    if (readSMM (file, usage, ext)) ;
    else if (readMM (file, usage, ext)) ;
    else m_errorString = txt_invalid;
    fclose (file);
}


void SidUsage::write (const char *filename, const sid2_usage_t &usage)
{
    FILE *file;
    size_t i = strlen (filename);
    const char *ext = NULL;

    m_status = false;
    file = fopen (filename, "wb");
    if (file == NULL)
    {
        m_errorString = txt_file;
        return;
    }

    // Find start of extension
    while (i > 0)
    {
        if (filename[--i] == '.')
        {
            ext = &filename[i + 1];
            break;
        }
    }

    // Make sure we have a valid extension to check for the
    // required output format
    if (!ext)
        m_errorString = txt_invalid;
    else if (!strcmp (ext, "mm"))
        writeSMM (file, usage);
    else if (!strcmp (ext, "map"))
        writeMAP (file, usage);
    else
        m_errorString = txt_invalid;
    fclose (file);
}


bool SidUsage::readMM (FILE *file, sid2_usage_t &usage, const char *ext)
{
    // Need to check extension
    if (!ext || strcmp (ext, "mm"))
        return false;

    {   // Read header
        char version;
        unsigned short flags;
        fread (&version, sizeof (version), 1, file);
        if (version != 0)
        {
            m_errorString = txt_supported;
            return true;
        }
        fread (&flags, sizeof (flags), 1, file);
        usage.flags = flags;
    }

    {   // Read load image details
        int length;
        fread (&usage.start, sizeof (usage.start), 1, file);
        fread (&usage.end, sizeof (usage.end), 1, file);
        length = (int) usage.start - (int) usage.end + 1;
        if (length < 0)
        {
            m_errorString = txt_corrupt;
            return true;
        }
        memset (&usage.memory[usage.start], SID_LOAD_IMAGE, sizeof (char) * length);
    }

    {   // Read usage
        int ret = fgetc (file);
        while (ret != EOF)
        {   // Read usage
            if (fread (&usage.memory[ret << 8], sizeof (char) * 0x100, 1, file) != 1)
            {
                m_errorString = txt_reading;
                return true;
            }   
            ret = fgetc (file);
        }
    }
    m_status = true;
    return true;
}


bool SidUsage::readSMM (FILE *file, sid2_usage_t &usage, const char *)
{
    uint8_t tmp[4] = {0};
    uint_least32_t length, id;

    // Read header chunk
    fread (&tmp, sizeof (tmp), 1, file);
    if (endian_big32 (tmp) != FORM_ID)
        return false;

    // Read file length
    if (!fread (&tmp, sizeof (tmp), 1, file))
        return false;
    length = endian_big32 (tmp);
    if (length < (sizeof(uint8_t) * 4))
        return false;
    length -= (sizeof(uint8_t) * 4);

    // Read smm version
    if (!fread (&tmp, sizeof (tmp), 1, file))
        return false;
    id = endian_big32 (tmp);

    // Determine SMM version
    switch (id)
    {
    case SMM0_ID:
    {
        Smm_v0 smm(0);
        m_status = smm.read (file, usage, length);
        if (!m_status)
            m_errorString = txt_corrupt;
        break;
    }

    case SMM1_ID:
    case SMM2_ID:
    case SMM3_ID:
    case SMM4_ID:
    case SMM5_ID:
    case SMM6_ID:
    case SMM7_ID:
    case SMM8_ID:
    case SMM9_ID:
    default:
        m_errorString = txt_supported;
        break;
    }
    return true;
}


void SidUsage::writeSMM (FILE *file, const sid2_usage_t &usage)
{
    Smm_v0 smm(0);
    uint8_t tmp[4] = {0};
    uint_least32_t length = 4;
    fpos_t  pos;

    // Write out header
    endian_big32  (tmp, FORM_ID);
    if (!fwrite (&tmp, sizeof (tmp), 1, file))
        goto writeSMM_error;
    fgetpos (file, &pos);
    endian_big32  (tmp, 0);
    if (!fwrite (&tmp, sizeof (tmp), 1, file))
        goto writeSMM_error;
    endian_big32  (tmp, SMM0_ID);
    if (!fwrite (&tmp, sizeof (tmp), 1, file))
        goto writeSMM_error;

    // Write file
    if (!smm.write (file, usage, length))
        goto writeSMM_error;

    // Write final length
    fsetpos (file, &pos);
    endian_big32  (tmp, length);
    if (!fwrite (&tmp, sizeof (tmp), 1, file))
        goto writeSMM_error;

    m_status = true;
    return;

writeSMM_error:
    m_errorString = txt_writing;
}


// Add filtering to the specified memory locations
void SidUsage::filterMAP (int from, int to, uint_least8_t mask)
{
    for (int i = from; i <= to; i++)
        m_filterMAP[i] = ~mask;
}


void SidUsage::writeMAP (FILE *file, const sid2_usage_t &usage)
{
    bool err = false;
    // Find out end unused regions which can be removed from
    // load image
    uint_least16_t faddr = usage.start;
    uint_least16_t laddr = usage.end;

    // Trim ends unused off load image
    for (; faddr < laddr; faddr++)
    {
//        if (usage.memory[faddr] & (SID_BAD_READ | SID_BAD_EXECUTE))
        if (usage.memory[faddr] & ~SID_LOAD_IMAGE)
            break;
    }

    for (; laddr > faddr; laddr--)
    {
//        if (usage.memory[laddr] & (SID_BAD_READ | SID_BAD_EXECUTE))
        if (usage.memory[laddr] & ~SID_LOAD_IMAGE)
            break;
    }

    for (int page = 0; page < 0x100; page++)
    {
        bool used = false;
        for (int offset = 0; offset < 0x100; offset++)
            used |= (usage.memory[(page << 8) | offset] != 0);
               
        if (used)
        {
            for (int i = 0; i < 4; i++)
            {
                fprintf (file, "%02X%02X=", page, i << 6);
                for (int j = 0; j < 64; j++)
                {
                    int addr = (page << 8) | (i << 6) | j;
                    uint_least8_t u = (uint_least8_t) usage.memory[addr] & 0xff;
                    // The addresses which don't need to be in the load image have now been
                    // trimmed off.  Anything between faddr and laddr needs to be kept
                    if ((addr >= faddr) && (addr <= laddr))
                        u |= (SID_BAD_READ | SID_BAD_EXECUTE);
                    // Apply usage filter for this memory location
                    u &= m_filterMAP[addr];
                    err |= fprintf (file, "%s", m_decodeMAP[u]) < 0;
                    if ((j & 7) == 7)
                        err |= fprintf (file, " ") < 0;
                }
                err |= fprintf (file, "\n") < 0;
            }
        }
    }

    if (err)
        m_errorString = txt_writing;
    else
        m_status = true;
}
