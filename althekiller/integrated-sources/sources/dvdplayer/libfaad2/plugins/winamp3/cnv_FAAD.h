/*
cnv_FAAD - MP4-AAC decoder plugin for Winamp3
Copyright (C) 2002 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
kreel@tiscali.it
*/

#ifndef _CNV_FAAD_H
#define _CNV_FAAD_H

#include <studio/wac.h>
#include <common/rootcomp.h>
#include <attribs/cfgitemi.h>
#include <attribs/attrint.h>
#include "Defines.h"

#define WACNAME WACcnv_FAAD



#include <attribs/attrstr.h>
extern _string cfg_samplerate;
extern _string cfg_profile;
extern _string cfg_bps;


class WACNAME : public WAComponentClient
{
public:
    WACNAME();
    virtual ~WACNAME();

    virtual const char *getName() { return FILES_SUPPORT " to PCM converter"; };
    virtual GUID getGUID();

	virtual void onRegisterServices();
    virtual void onDestroy();
	virtual void onCreate();
    
    virtual int getDisplayComponent() { return FALSE; };

    virtual CfgItem *getCfgInterface(int n) { return this; }
};

#endif
