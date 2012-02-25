#pragma once
/*
 *      Copyright (C) 2012 Alexander Pipelka
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

#include <list>
#include <vector>
#include <string>
#include "client.h"

using namespace ADDON;

class cXVDRConfigParameterBase
{
public:

  cXVDRConfigParameterBase();

  virtual ~cXVDRConfigParameterBase();

  virtual bool load() = 0;

  virtual bool set(const void* value) = 0;

  std::string& name();

protected:

  std::string m_setting;

  static std::list<cXVDRConfigParameterBase*>& parameters();

  friend class cXVDRSettings;

private:

  static std::list<cXVDRConfigParameterBase*> m_parameters;

};


template< class T >
class cXVDRConfigParameter : public cXVDRConfigParameterBase
{
public:

  cXVDRConfigParameter(const std::string& setting, T defaultvalue) : m_value(defaultvalue), m_default(defaultvalue)
  {
    m_setting = setting;
  }

  cXVDRConfigParameter(const std::string& setting)
  {
    m_setting = setting;
  }

  bool load()
  {
    if (XBMC->GetSetting(m_setting.c_str(), &m_value))
      return true;

    XBMC->Log(LOG_ERROR, "Couldn't get '%s' setting, falling back to default", m_setting.c_str());
    m_value = m_default;

    return true;
  }

  bool set(const void* value)
  {
    return set(*(T*)value);
  }

  bool set(T value)
  {
    if(m_value == value)
      return false;

    XBMC->Log(LOG_INFO, "Changed Setting '%s'", m_setting.c_str());
    m_value = value;
    return true;
  }

  T& operator()()
  {
    return m_value;
  }

private:

  T m_value;

  T m_default;

};

class cXVDRSettings
{
public:

  virtual ~cXVDRSettings();

  static cXVDRSettings& GetInstance();

  void load();

  bool set(const std::string& setting, const void* value);

  cXVDRConfigParameter<std::string> Hostname;
  cXVDRConfigParameter<int> ConnectTimeout;
  cXVDRConfigParameter<bool> CharsetConv;
  cXVDRConfigParameter<bool> HandleMessages;
  cXVDRConfigParameter<int> Priority;
  cXVDRConfigParameter<int> Compression;
  cXVDRConfigParameter<bool> AutoChannelGroups;
  cXVDRConfigParameter<int> AudioType;
  cXVDRConfigParameter<int> UpdateChannels;
  cXVDRConfigParameter<bool> FTAChannels;
  cXVDRConfigParameter<bool> NativeLangOnly;
  cXVDRConfigParameter<bool> EncryptedChannels;
  cXVDRConfigParameter<std::string> caids;
  std::vector<int> vcaids;

protected:

  cXVDRSettings() :
  Hostname("host", "127.0.0,1"),
  ConnectTimeout("timeout", 3),
  CharsetConv("convertchar", false),
  HandleMessages("handlemessages", true),
  Priority("priority", 50),
  Compression("compression", 2),
  AutoChannelGroups("autochannelgroups", false),
  AudioType("audiotype", 0),
  UpdateChannels("updatechannels", 3),
  FTAChannels("ftachannels", true),
  NativeLangOnly("nativelangonly", false),
  EncryptedChannels("encryptedchannels", true),
  caids("caids")
  {}

private:

  void ReadCaIDs(const char* buffer, std::vector<int>& array);

};
