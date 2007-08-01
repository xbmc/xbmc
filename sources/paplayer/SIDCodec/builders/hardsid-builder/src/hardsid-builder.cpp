/***************************************************************************
         hardsid-builder.cpp - HardSID builder class for creating/controlling
                               HardSIDs.
                               -------------------
    begin                : Wed Sep 5 2001
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
/***************************************************************************
 *  $Log: hardsid-builder.cpp,v $
 *  Revision 1.9  2004/05/05 23:48:01  s_a_white
 *  Detect available sid devices on Unix system.
 *
 *  Revision 1.8  2003/10/18 13:31:58  s_a_white
 *  Improved hardsid.dll load failure error messages.
 *
 *  Revision 1.7  2003/06/27 07:07:00  s_a_white
 *  Use new hardsid.dll muting interface.
 *
 *  Revision 1.6  2002/10/17 18:37:43  s_a_white
 *  Exit unlock function early once correct sid is found.
 *
 *  Revision 1.5  2002/08/20 19:21:53  s_a_white
 *  Updated version information.
 *
 *  Revision 1.4  2002/08/09 18:11:35  s_a_white
 *  Added backwards compatibility support for older hardsid.dll.
 *
 *  Revision 1.3  2002/07/20 08:36:24  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.2  2002/03/04 19:07:07  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.1  2002/01/28 22:35:20  s_a_white
 *  Initial Release.
 *
 *
 ***************************************************************************/

#include <stdio.h>
#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "hardsid.h"
#include "hardsid-emu.h"


#ifdef HAVE_MSWINDOWS
//**************************************************************************
// Version 1 Interface
typedef BYTE (CALLBACK* HsidDLL1_InitMapper_t) (void);

HsidDLL2 hsid2 = {0};
#endif

bool HardSIDBuilder::m_initialised = false;
#ifdef HAVE_UNIX
uint HardSIDBuilder::m_count = 0;
#endif

HardSIDBuilder::HardSIDBuilder (const char * const name)
:sidbuilder (name)
{
    strcpy (m_errorBuffer, "N/A");

    if (!m_initialised)
    {   // Setup credits
        char *p = HardSID::credit;
        sprintf (p, "HardSID V%s Engine:", VERSION);
        p += strlen (p) + 1;
#ifdef HAVE_MSWINDOWS
        strcpy  (p, "\t(C) 1999-2002 Simon White <sidplay2@yahoo.com>");
#else
        strcpy  (p, "\t(C) 2001-2002 Jarno Paanenen <jpaana@s2.org>");
#endif
        p += strlen (p) + 1;
        *p = '\0';

        if (init () < 0)
            return;
        m_initialised = true;
    }
}

HardSIDBuilder::~HardSIDBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint HardSIDBuilder::create (uint sids)
{
    uint   count;
    HardSID *sid = NULL;
    m_status     = true;

    // Check available devices
    count = devices (false);
    if (!m_status)
        goto HardSIDBuilder_create_error;
    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
#   ifdef HAVE_EXCEPTIONS
        sid = new(std::nothrow) HardSID(this);
#   else
        sid = new HardSID(this);
#   endif

        // Memory alloc failed?
        if (!sid)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create HardSID object", name ());
            goto HardSIDBuilder_create_error;
        }

        // SID init failed?
        if (!*sid)
        {
            strcpy (m_errorBuffer, sid->error ());
            goto HardSIDBuilder_create_error;
        }
        sidobjs.push_back (sid);
    }
    return count;

HardSIDBuilder_create_error:
    m_status = false;
    if (sid)
        delete sid;
    return count;
}

uint HardSIDBuilder::devices (bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();

    // Available devices
    // @FIXME@ not yet supported on Linux
#ifdef HAVE_MSWINDOWS
    if (hsid2.Instance)
    {
        uint count = hsid2.Devices ();
        if (count == 0)
        {
            sprintf (m_errorBuffer, "HARDSID ERROR: No devices found (run HardSIDConfig)");
            m_status = false;
        }
        return count;
    }
#elif defined(HAVE_UNIX)
    return m_count;
#else
    return 0;
#endif
}

const char *HardSIDBuilder::credits ()
{
    m_status = true;
    return HardSID::credit;
}

void HardSIDBuilder::flush(void)
{
    int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        ((HardSID*)sidobjs[i])->flush();
}

void HardSIDBuilder::filter (bool enable)
{
    int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        HardSID *sid = (HardSID *) sidobjs[i];
        sid->filter (enable);
    }
}

// Find a free SID of the required specs
sidemu *HardSIDBuilder::lock (c64env *env, sid2_model_t model)
{
    int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        HardSID *sid = (HardSID *) sidobjs[i];
        if (sid->lock (env))
        {
            sid->model (model);
            return sid;
        }
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", name ());
    return NULL;
}

// Allow something to use this SID
void HardSIDBuilder::unlock (sidemu *device)
{
    int size = sidobjs.size ();
    // Make sure this is our SID
    for (int i = 0; i < size; i++)
    {
        HardSID *sid = (HardSID *) sidobjs[i];
        if (sid == device)
        {   // Unlock it
            sid->lock (NULL);
            break;
        }
    }
}

// Remove all SID emulations.
void HardSIDBuilder::remove ()
{
    int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        delete sidobjs[i];
    sidobjs.clear();
}

#ifdef HAVE_MSWINDOWS

// Load the library and initialise the hardsid
int HardSIDBuilder::init ()
{
    HINSTANCE dll;

    if (hsid2.Instance)
        return 0;

    m_status = false;
    dll = LoadLibrary("HARDSID.DLL");
    if (!dll)
    {
        DWORD err = GetLastError();
		if (err == ERROR_DLL_INIT_FAILED)
			sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll init failed!");
		else
			sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll not found!");
        goto HardSID_init_error;
    }

    {   // Export Needed Version 1 Interface
        HsidDLL1_InitMapper_t mapper;
        mapper = (HsidDLL1_InitMapper_t) GetProcAddress(dll, "InitHardSID_Mapper");

        if (mapper)
            mapper(); 
        else
        {
            sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll is corrupt!");
            goto HardSID_init_error;
        }
    }

    {   // Check for the Version 2 interface
        HsidDLL2_Version_t version;
        version = (HsidDLL2_Version_t) GetProcAddress(dll, "HardSID_Version");
        if (!version)
        {
            sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll not V2");
            goto HardSID_init_error;
        }
        hsid2.Version = version ();
    }

    {
        WORD version = hsid2.Version;
        if ((version >> 8) != (HSID_VERSION_MIN >> 8))
        {
            sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll not V%d", HSID_VERSION_MIN >> 8);
            goto HardSID_init_error;
        }

        if (version < HSID_VERSION_MIN)
        {
            sprintf (m_errorBuffer, "HARDSID ERROR: hardsid.dll must be V%02u.%02u or greater",
                     HSID_VERSION_MIN >> 8, HSID_VERSION_MIN & 0xff);
            goto HardSID_init_error;
        }
    }

    // Export Needed Version 2 Interface
    hsid2.Delay    = (HsidDLL2_Delay_t)   GetProcAddress(dll, "HardSID_Delay");
    hsid2.Devices  = (HsidDLL2_Devices_t) GetProcAddress(dll, "HardSID_Devices");
    hsid2.Filter   = (HsidDLL2_Filter_t)  GetProcAddress(dll, "HardSID_Filter");
    hsid2.Flush    = (HsidDLL2_Flush_t)   GetProcAddress(dll, "HardSID_Flush");
    hsid2.MuteAll  = (HsidDLL2_MuteAll_t) GetProcAddress(dll, "HardSID_MuteAll");
    hsid2.Read     = (HsidDLL2_Read_t)    GetProcAddress(dll, "HardSID_Read");
    hsid2.Sync     = (HsidDLL2_Sync_t)    GetProcAddress(dll, "HardSID_Sync");
    hsid2.Write    = (HsidDLL2_Write_t)   GetProcAddress(dll, "HardSID_Write");

    if (hsid2.Version < HSID_VERSION_204)
        hsid2.Reset  = (HsidDLL2_Reset_t)  GetProcAddress(dll, "HardSID_Reset");
    else
    {
        hsid2.Lock   = (HsidDLL2_Lock_t)   GetProcAddress(dll, "HardSID_Lock");
        hsid2.Unlock = (HsidDLL2_Unlock_t) GetProcAddress(dll, "HardSID_Unlock");
        hsid2.Reset2 = (HsidDLL2_Reset2_t) GetProcAddress(dll, "HardSID_Reset2");
    }

    if (hsid2.Version < HSID_VERSION_207)
        hsid2.Mute   = (HsidDLL2_Mute_t)   GetProcAddress(dll, "HardSID_Mute");
    else
        hsid2.Mute2  = (HsidDLL2_Mute2_t)  GetProcAddress(dll, "HardSID_Mute2");

    hsid2.Instance = dll;
    m_status       = true;
    return 0;

HardSID_init_error:
    if (dll)
        FreeLibrary (dll);
    return -1;
}

#elif defined(HAVE_UNIX)

#include <ctype.h>
#include <dirent.h>

// Find the number of sid devices.  We do not care about
// stuppid device numbering or drivers not loaded for the
// available nodes.
int HardSIDBuilder::init ()
{
    DIR    *dir;
    dirent *entry;

    m_count = 0;
    dir = opendir("/dev");
    if (!dir)
        return -1;

    while ( (entry=readdir(dir)) )
    {
        // SID device
        if (strncmp ("sid", entry->d_name, 3))
            continue;
        
        // if it is truely one of ours then it will be
        // followed by numerics only
        const char *p = entry->d_name+3;
        int index     = 0;
        while (*p)
        {
            if (!isdigit (*p))
                continue;
            index = index * 10 + (*p++ - '0');
        }
        index++;
        if (m_count < index)
            m_count = index;
    }
    closedir (dir);
}

#endif // HAVE_MSWINDOWS
