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
#include "Addon.h"
#include "include/xbmc_addon_dll.h"
#include "tinyXML/tinyxml.h"
#include "utils/CriticalSection.h"
#include "StdString.h"
#include "utils/Job.h"
#include "utils/Stopwatch.h"
#include <vector>
#include <map>

class DllLibCPluff;
extern "C"
{
#include "lib/cpluff/libcpluff/cpluff.h"
}

namespace ADDON
{
  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;
  typedef std::map<TYPE, VECADDONS> MAPADDONS;
  typedef std::map<TYPE, VECADDONS>::iterator IMAPADDONS;

  const CStdString ADDON_METAFILE             = "description.xml";
  const CStdString ADDON_VIS_EXT              = "*.vis";
  const CStdString ADDON_PYTHON_EXT           = "*.py";
  const CStdString ADDON_SCRAPER_EXT          = "*.xml";
  const CStdString ADDON_SCREENSAVER_EXT      = "*.xbs";
  const CStdString ADDON_DSP_AUDIO_EXT        = "*.adsp";
  const CStdString ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";

  /**
  * Class - IAddonCallback
  * This callback should be inherited by any class which manages
  * specific addon types. Could be mostly used for Dll addon types to handle
  * cleanup before restart/removal
  */
  class IAddonMgrCallback
  {
    public:
      virtual ~IAddonMgrCallback() {};
      virtual bool RequestRestart(const IAddon* addon, bool datachanged)=0;
      virtual bool RequestRemoval(const IAddon* addon)=0;
  };

  /**
  * Class - CAddonMgr
  * Holds references to all addons, enabled or
  * otherwise. Services the generic callbacks available
  * to all addon variants.
  */
  class CAddonMgr : public IJobCallback
  {
  public:
    static CAddonMgr &Get();
    bool Init();

    IAddonMgrCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonMgrCallback(TYPE type, IAddonMgrCallback* cb);
    void UnregisterAddonMgrCallback(TYPE type);

    /* Addon access */
    bool GetDefault(const TYPE &type, AddonPtr &addon, const CONTENT_TYPE &content = CONTENT_NONE);
    bool GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type = ADDON_UNKNOWN, bool enabledOnly = true);
    AddonPtr GetAddon2(const CStdString &str);
    bool HasAddons(const TYPE &type, const CONTENT_TYPE &content = CONTENT_NONE, bool enabledOnly = true);
    bool GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content = CONTENT_NONE, bool enabled = true);
    bool GetAllAddons(VECADDONS &addons, bool enabledOnly = true);
    CStdString GetString(const CStdString &id, const int number);
    
    static bool AddonFromInfoXML(const TiXmlElement *xmlDoc, AddonPtr &addon,
                                 const CStdString &strPath);
    static bool AddonFromInfoXML(const CStdString &path, AddonPtr &addon);
    static bool GetTranslatedString(const TiXmlElement *xmldoc, const char *tag, CStdString& data);
    static AddonPtr AddonFromProps(AddonProps& props);
    void UpdateRepos();
    void FindAddons();
    void LoadAddons(const CStdString &path, 
                    std::map<CStdString, AddonPtr>& unresolved);

    void OnJobComplete(unsigned int jobID, bool sucess, CJob* job);

    /* libcpluff */
    bool GetExtensions(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content);
    CStdString GetExtValue(cp_cfg_element_t *base, const char *path);
    void CPluffFatalError(const char *msg);
    void CPluffLog(cp_log_severity_t level, const char *msg, const char *apid, void *user_data);
    cp_context_t *m_cp_context;
    DllLibCPluff *m_cpluff;

    static void cp_fatalErrorHandler(const char *msg) {
      CAddonMgr::Get()->CPluffFatalError(msg);
    }
    static void cp_logger(cp_log_severity_t level, const char *msg, const char *apid, void *user_data) {
      CAddonMgr::Get()->CPluffLog(level, msg, apid, user_data);
    }

  private:
    bool DependenciesMet(AddonPtr &addon);
    bool UpdateIfKnown(AddonPtr &addon);

    // private construction, and no assignements; use the provided singleton methods
    CAddonMgr();
    CAddonMgr(const CAddonMgr&);
    CAddonMgr const& operator=(CAddonMgr const&);
    virtual ~CAddonMgr();

    static std::map<TYPE, IAddonMgrCallback*> m_managers;
    MAPADDONS m_addons;
    CStopWatch m_watch;
    std::map<CStdString, AddonPtr> m_idMap;
    CCriticalSection m_critSection;
  };

}; /* namespace ADDON */
