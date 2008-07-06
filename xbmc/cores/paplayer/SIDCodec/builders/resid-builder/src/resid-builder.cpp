/***************************************************************************
         resid-builder.cpp - ReSID builder class for creating/controlling
                             resids.
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
 *  $Log: resid-builder.cpp,v $
 *  Revision 1.6  2002/10/17 18:45:31  s_a_white
 *  Exit unlock function early once correct sid is found.
 *
 *  Revision 1.5  2002/03/04 19:06:38  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.4  2002/01/29 21:58:28  s_a_white
 *  Moved out sid emulation to a private header file.
 *
 *  Revision 1.3  2001/12/11 19:33:18  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.2  2001/12/09 10:53:50  s_a_white
 *  Added exporting of credits.
 *
 *  Revision 1.1.1.1  2001/11/25 15:03:20  s_a_white
 *  Initial Release
 *
 ***************************************************************************/

#include <stdio.h>
#include <cstring>

#include "config.h"
#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid.h"
#include "resid-emu.h"

// Error String(s)
const char *ReSIDBuilder::ERR_FILTER_DEFINITION = "RESID ERROR: Filter definition is not valid (see docs).";

ReSIDBuilder::ReSIDBuilder (const char * const name)
:sidbuilder (name)
{
    m_error = "N/A";
}

ReSIDBuilder::~ReSIDBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint ReSIDBuilder::create (uint sids)
{
	uint   count;
    ReSID *sid = NULL;
    m_status   = true;

    // Check available devices
	count = devices (false);
	if (!m_status)
		goto ReSIDBuilder_create_error;
    if (count && (count < sids))
		sids = count;

	for (count = 0; count < sids; count++)
	{
#   ifdef HAVE_EXCEPTIONS
	    sid = new(std::nothrow) ReSID(this);
#   else
	    sid = new ReSID(this);
#   endif

        // Memory alloc failed?
        if (!sid)
		{
			sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", name ());
            m_error = m_errorBuffer;
			goto ReSIDBuilder_create_error;
		}

		// SID init failed?
		if (!*sid)
		{
			m_error = sid->error ();
			goto ReSIDBuilder_create_error;
		}
	    sidobjs.push_back (sid);
	}
    return count;

ReSIDBuilder_create_error:
    m_status = false;
    if (sid)
        delete sid;
    return count;
}

const char *ReSIDBuilder::credits ()
{
    m_status = true;

    // Available devices
    if (sidobjs.size ())
    {
        ReSID *sid = (ReSID *) sidobjs[0];
        return sid->credits ();
    }

    {   // Create an emulation to obtain credits
        ReSID sid(this);
        if (!sid)
		{
            m_status = false;
            strcpy (m_errorBuffer, sid.error ());
            return 0;
        }
        return sid.credits ();
    }
}


uint ReSIDBuilder::devices (bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
		return 0;
}

void ReSIDBuilder::filter (const sid_filter_t *filter)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        if (!sid->filter (filter))
            goto ReSIDBuilder_sidFilterDef_error;
    }
return;

ReSIDBuilder_sidFilterDef_error:
    m_error  = ERR_FILTER_DEFINITION;
    m_status = false;
}

void ReSIDBuilder::filter (bool enable)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        sid->filter (enable);
    }
}

// Find a free SID of the required specs
sidemu *ReSIDBuilder::lock (c64env *env, sid2_model_t model)
{
    int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSID *sid = (ReSID *) sidobjs[i];
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
void ReSIDBuilder::unlock (sidemu *device)
{
    int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = (ReSID *) sidobjs[i];
        if (sid == device)
		{   // Unlock it
            sid->lock (NULL);
			break;
		}
    }
}

// Remove all SID emulations.
void ReSIDBuilder::remove ()
{
    int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        delete sidobjs[i];
    sidobjs.clear();
}

void ReSIDBuilder::sampling (uint_least32_t freq)
{
    int size = sidobjs.size ();
	m_status = true;
    for (int i = 0; i < size; i++)
	{
		ReSID *sid = (ReSID *) sidobjs[i];
        sid->sampling (freq);
    }
}
