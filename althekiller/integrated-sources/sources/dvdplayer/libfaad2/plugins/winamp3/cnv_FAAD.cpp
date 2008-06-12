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

#include <stdlib.h>
#include "cnv_FAAD.h"
#include "faadwa3.h"
#include "CRegistry.h"
#include "Defines.h"

// *********************************************************************************************

void ReadCfgDec(faacDecConfiguration *cfg) 
{ 
CRegistry reg;

	if(reg.OpenCreate(HKEY_CURRENT_USER,REGISTRY_PROGRAM_NAME  "\\FAAD"))
	{
		cfg->defObjectType=reg.GetSetByte("Profile",LOW);
		cfg->defSampleRate=reg.GetSetDword("SampleRate",44100);
		cfg->outputFormat=reg.GetSetByte("Bps",FAAD_FMT_16BIT);
	}
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}
// -----------------------------------------------------------------------------------------------

void WriteCfgDec(faacDecConfiguration *cfg)
{ 
CRegistry reg;

	if(reg.OpenCreate(HKEY_CURRENT_USER,REGISTRY_PROGRAM_NAME  "\\FAAD"))
	{
		reg.SetByte("Profile",cfg->defObjectType); 
		reg.SetDword("SampleRate",cfg->defSampleRate); 
		reg.SetByte("Bps",cfg->outputFormat);
	}
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}

// *********************************************************************************************



static WACNAME wac;
WAComponentClient *the = &wac;

#include <studio/services/servicei.h>
static waServiceT<svc_mediaConverter, AacPcm> aacpcm;

// {3AF667AD-3CF8-459e-8C7C-BD8CD1D6F8C2}
static const GUID guid = 
{ 0x3af667ad, 0x3cf8, 0x459e, { 0x8c, 0x7c, 0xbd, 0x8c, 0xd1, 0xd6, 0xf8, 0xc2 } };

#include <attribs/attrstr.h>
#define FEEDID_PROFILE "PROFILE"
_string cfg_profile("Profile", "Low Complexity");
#define FEEDID_SAMPLERATE "SAMPLERATE"
_string cfg_samplerate("Samplerate", "44100");
#define FEEDID_BPS "BPS"
_string cfg_bps("Bps", "16");



// *********************************************************************************************

#include <studio/services/svc_textfeed.h>

class TextFeed : public svc_textFeedI
{
 public:
  TextFeed()
  {
    registerFeed(FEEDID_PROFILE);
    registerFeed(FEEDID_SAMPLERATE);
    registerFeed(FEEDID_BPS);
  }
  static const char *getServiceName() { return "FAAD TextFeed Service"; }
};

static waServiceTSingle<svc_textFeed, TextFeed> svc_feed;




// *********************************************************************************************



WACNAME::WACNAME() : WAComponentClient(FILES_SUPPORT " files support")
{
#ifdef FORTIFY
    FortifySetName("cnv_FAAD.wac");
    FortifyEnterScope();
#endif
}

WACNAME::~WACNAME()
{
#ifdef FORTIFY
    FortifyLeaveScope();
#endif
}

GUID WACNAME::getGUID()
{
    return guid;
}

void WACNAME::onRegisterServices()
{
    api->service_register(&aacpcm);
    api->core_registerExtension("*.aac;*.mp4", FILES_SUPPORT " Files");

    api->service_register(&svc_feed);
//	following line is long and causes a crash
//	svc_feed.getSingleService()->sendFeed(FEEDID_SAMPLERATE,"6000;8000;11025;16000;22050;32000;44100;48000;64000;88200;96000;192000");
	svc_feed.getSingleService()->sendFeed(FEEDID_PROFILE,"Main;Low Complexity;SSR;LTP");
	svc_feed.getSingleService()->sendFeed(FEEDID_SAMPLERATE,"8000;11025;16000;22050;32000;44100;48000;96000");
	svc_feed.getSingleService()->sendFeed(FEEDID_BPS,"16;24;32;FLOAT");
}

void WACNAME::onDestroy()
{
    api->service_deregister(&aacpcm);
    api->service_deregister(&svc_feed);

    WAComponentClient::onDestroy();

faacDecConfiguration	Cfg;
	Cfg.defObjectType=atoi(cfg_profile);
	Cfg.defSampleRate=atoi(cfg_samplerate);
	Cfg.outputFormat=atoi(cfg_bps);
	WriteCfgDec(&Cfg);
}

void WACNAME::onCreate()
{
static const GUID cfg_audio_guid = // {EDAA0599-3E43-4eb5-A65D-C0A0484240E7}
{ 0xedaa0599, 0x3e43, 0x4eb5, { 0xa6, 0x5d, 0xc0, 0xa0, 0x48, 0x42, 0x40, 0xe7 } };
// following line adds this module to the list of Audio modules
	api->preferences_registerGroup("FAAD.Options", FILES_SUPPORT " decoder", getGUID(), cfg_audio_guid);
	registerSkinFile("Wacs/xml/FAAD/FAAD_config.xml");

faacDecConfiguration	Cfg;
char					buf[50];
	ReadCfgDec(&Cfg);
	cfg_samplerate=itoa(Cfg.defSampleRate,buf,10);
	switch(Cfg.defObjectType)
	{
	case MAIN:
		cfg_profile="Main";
		break;
	case LOW:
		cfg_profile="Low Complexity";
		break;
	case SSR:
		cfg_profile="SSR";
		break;
	case LTP:
		cfg_profile="LTP";
		break;
	}
	switch(Cfg.outputFormat)
	{
	case FAAD_FMT_16BIT:
		cfg_bps="16";
		break;
	case FAAD_FMT_24BIT:
		cfg_bps="24";
		break;
	case FAAD_FMT_32BIT:
		cfg_bps="32";
		break;
	case FAAD_FMT_FLOAT:
		cfg_bps="FLOAT";
		break;
	}
	registerAttribute(&cfg_profile);
	registerAttribute(&cfg_samplerate);
	registerAttribute(&cfg_bps);
}
