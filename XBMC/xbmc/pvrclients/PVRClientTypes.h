/*
*      Copyright (C) 2005-2009 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

/*
Common data structures shared between XBMC and PVR clients
*/

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#include <vector>

extern "C"
{
  /**
  * PVR Client Properties
  * Returned on client initialization
  */ 
  struct PVR_SERVERPROPS {
    char*  Name;
    int    HasBouquets;
    int    HasUnique;
    int    SupportEPG;
    int    SupportRecordings;
    int    SupportRadio;
    int    SupportTimers;
    char*  DefaultHostname;
    int    DefaultPort;
    char*  DefaultUser;
    char*  DefaultPassword;
  };

  /**
  * EPG Channel Definition
  */
  struct PVR_CHANNEL {
    char   Name[84];
    char   Callsign[8];
    char   IconPath[280];
    int    Number;
    int    Bouquet;
  };

  /**
  * EPG Bouquet Definition
  */
  struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  };

  /**
  * EPG Programme Definition
  * Used to signify an individual broadcast, whether it is also a recording, timer etc.
  */
  struct PVR_PROGINFO {
    int    id; // unique identifier, if supported will be used for 
    int    channum;
    int    bouquet;
    int    sourceid;
    char   title[150];
    char   subtitle[150];
    char   description[280];
    time_t starttime;
    time_t endtime;
    char   episodeid[30];
    char   seriesid[24];
    char   category[84];
    int    recording;
    int    rec_status;
    int    event_flags;
    int    startoffset;
    int    endoffset;
  };

  /**
  * The PVRSetting class for client's GUI settings.
  */
  class PVRSetting
  {
  public:
    enum SETTING_TYPE { NONE=0, CHECK, SPIN };

    PVRSetting(SETTING_TYPE t, const char *label)
    {
      name = NULL;
      if (label)
      {
        name = new char[strlen(label)+1];
        strcpy(name, label);
      }
      current = 0;
      type = t;
    }

    PVRSetting(const PVRSetting &rhs) // copy constructor
    {
      name = NULL;
      if (rhs.name)
      {
        name = new char[strlen(rhs.name)+1];
        strcpy(name, rhs.name);
      }
      current = rhs.current;
      type = rhs.type;
      for (unsigned int i = 0; i < rhs.entry.size(); i++)
      {
        char *lab = new char[strlen(rhs.entry[i]) + 1];
        strcpy(lab, rhs.entry[i]);
        entry.push_back(lab);
      }
    }

    ~PVRSetting()
    {
      if (name)
        delete[] name;
      for (unsigned int i=0; i < entry.size(); i++)
        delete[] entry[i];
    }

    void AddEntry(const char *label)
    {
      if (!label || type != SPIN) return;
      char *lab = new char[strlen(label) + 1];
      strcpy(lab, label);
      entry.push_back(lab);
    }

    // data members
    SETTING_TYPE type;
    char*        name;
    int          current;
    std::vector<const char *> entry;
  };

  struct PVRClient
  {
  public:
    void (__cdecl* GetProps)(PVR_SERVERPROPS *info);
    void (__cdecl *GetSettings)(std::vector<PVRSetting> **vecSettings);
    void (__cdecl *UpdateSetting)(int num);
  };

}

#endif //__PVRCLIENT_TYPES_H__
