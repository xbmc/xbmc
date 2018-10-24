/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonInfo.h"

#include <memory>
#include <set>

class TiXmlElement;

namespace ADDON
{
  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
  class CInstanceVisualization;
  typedef std::shared_ptr<CInstanceVisualization> VizPtr;
  class CSkinInfo;
  typedef std::shared_ptr<CSkinInfo> SkinPtr;
  class CPluginSource;
  typedef std::shared_ptr<CPluginSource> PluginPtr;

  class CAddonMgr;
  class CAddonSettings;

  class IAddon : public std::enable_shared_from_this<IAddon>
  {
  public:
    virtual ~IAddon() = default;
    virtual TYPE Type() const =0;
    virtual TYPE FullType() const =0;
    virtual bool IsType(TYPE type) const =0;
    virtual std::string ID() const =0;
    virtual std::string Name() const =0;
    virtual bool IsInUse() const =0;
    virtual AddonVersion Version() const =0;
    virtual AddonVersion MinVersion() const =0;
    virtual std::string Summary() const =0;
    virtual std::string Description() const =0;
    virtual std::string Path() const =0;
    virtual std::string Profile() const =0;
    virtual std::string LibPath() const =0;
    virtual std::string ChangeLog() const =0;
    virtual std::string FanArt() const =0;
    virtual ArtMap Art() const =0;
    virtual std::vector<std::string> Screenshots() const =0;
    virtual std::string Author() const =0;
    virtual std::string Icon() const =0;
    virtual std::string Disclaimer() const =0;
    virtual std::string Broken() const =0;
    virtual CDateTime InstallDate() const =0;
    virtual CDateTime LastUpdated() const =0;
    virtual CDateTime LastUsed() const =0;
    virtual std::string Origin() const =0;
    virtual uint64_t PackageSize() const =0;
    virtual const InfoMap &ExtraInfo() const =0;
    virtual bool HasSettings() =0;
    virtual void SaveSettings() =0;
    virtual void UpdateSetting(const std::string& key, const std::string& value) =0;
    virtual bool UpdateSettingBool(const std::string& key, bool value) = 0;
    virtual bool UpdateSettingInt(const std::string& key, int value) = 0;
    virtual bool UpdateSettingNumber(const std::string& key, double value) = 0;
    virtual bool UpdateSettingString(const std::string& key, const std::string& value) = 0;
    virtual std::string GetSetting(const std::string& key) =0;
    virtual bool GetSettingBool(const std::string& key, bool& value) = 0;
    virtual bool GetSettingInt(const std::string& key, int& value) = 0;
    virtual bool GetSettingNumber(const std::string& key, double& value) = 0;
    virtual bool GetSettingString(const std::string& key, std::string& value) = 0;
    virtual CAddonSettings* GetSettings() const =0;
    virtual const std::vector<DependencyInfo> &GetDependencies() const =0;
    virtual AddonVersion GetDependencyVersion(const std::string &dependencyID) const =0;
    virtual bool MeetsVersion(const AddonVersion &version) const =0;
    virtual bool ReloadSettings() =0;
    virtual AddonPtr GetRunningInstance() const=0;
    virtual void OnPreInstall() =0;
    virtual void OnPostInstall(bool update, bool modal) =0;
    virtual void OnPreUnInstall() =0;
    virtual void OnPostUnInstall() =0;

    // Derived property
    bool IsBroken() const { return !Broken().empty(); }
  };
};
