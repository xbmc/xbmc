#pragma once
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
#include "boost/shared_ptr.hpp"
#include "StdString.h"
#include <set>
#include <list>

class TiXmlElement;

typedef enum
{
  CONTENT_PLUGIN          = -2,
  //TODO -1 is used as default in QueryParams.h
  CONTENT_NONE            = 0,
  CONTENT_MOVIES          = 1,
  CONTENT_TVSHOWS         = 2,
  CONTENT_MUSICVIDEOS     = 3,
  CONTENT_EPISODES        = 4,
  CONTENT_ALBUMS          = 6,
  CONTENT_ARTISTS         = 7,
  CONTENT_PROGRAMS        = 8,
  CONTENT_PVR             = 9,
  CONTENT_PICTURES        = 10,
  CONTENT_WEATHER         = 11
} CONTENT_TYPE;

namespace ADDON
{
  typedef enum
  {
    ADDON_MULTITYPE         = 0,
    ADDON_VIZ               = 1,
    ADDON_SKIN              = 2,
    ADDON_PVRDLL            = 3,
    ADDON_SCRIPT            = 4,
    ADDON_SCRAPER           = 5,
    ADDON_SCREENSAVER       = 6,
    ADDON_PLUGIN            = 7,
    ADDON_DSP_AUDIO         = 8
  } TYPE;

  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;

  class CAddonMgr;
  struct AddonProps;

  class IAddon
  {
  public:
    virtual void Set(const AddonProps &props) =0;
    virtual AddonPtr Clone() const =0;
    virtual TYPE Type() const =0;
    virtual CStdString UUID() const =0;
    virtual CStdString Parent() const =0;
    virtual CStdString Name() const =0;
    virtual bool Disabled() const =0;
    virtual CStdString Version() const =0;
    virtual CStdString Summary() const =0;
    virtual CStdString Description() const =0;
    virtual CStdString Path() const =0;
    virtual CStdString Profile() const =0;
    virtual CStdString LibName() const =0;
    virtual CStdString Author() const =0;
    virtual CStdString Icon() const =0;
    virtual int  Stars() const =0;
    virtual CStdString Disclaimer() const =0;
    virtual bool Supports(const CONTENT_TYPE &content) const =0;
    virtual bool HasSettings() =0;
    virtual bool LoadSettings() =0;
    virtual void SaveSettings() =0;
    virtual void SaveFromDefault() =0;
    virtual void UpdateSetting(const CStdString& key, const CStdString& type, const CStdString& value) =0;
    virtual CStdString GetSetting(const CStdString& key) const =0;
    virtual TiXmlElement* GetSettingsXML()=0;
    virtual CStdString GetString(uint32_t id) const =0;

  private:
    friend class CAddonMgr;
    virtual void Enable() =0;
    virtual void Disable() =0;
    virtual bool LoadStrings() =0;
    virtual void ClearStrings() =0;

  };

  struct AddonProps
  {
  public:
    AddonProps(CStdString uuid, ADDON::TYPE type) : uuid(uuid)
                                      , type(type)
    {}

    AddonProps(const AddonPtr &addon) : uuid(addon->UUID())
                                      , type(addon->Type())
                                      , parent(addon->Parent())
                                      , name(addon->Name())
                                      , icon(addon->Icon())
    {}
    const CStdString uuid;
    const ADDON::TYPE type;
    std::set<CONTENT_TYPE> contents;
    CStdString parent;
    CStdString name;
    CStdString version;
    CStdString summary;
    CStdString description;
    CStdString path;
    CStdString libname;
    CStdString author;
    CStdString icon;
    int        stars;
    CStdString disclaimer;
  };
  typedef std::list<struct AddonProps> VECADDONPROPS;
};

