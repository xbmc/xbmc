/*
 *  Copyright (C) 2021 Team CoreELEC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "LanguageHook.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"

#include <stdlib.h>
#include <vector>
#include <fstream>

namespace XBMCAddon
{
  namespace xbmcvfs
  {
    //
    /// \defgroup python_xbmcvfs configini
    /// \ingroup python_xbmcvfs
    /// @{
    /// @brief **Get/Set CoreELEC config.ini value.**
    ///
    /// \python_class{ xbmcvfs.configini.get(key) }
    /// \python_class{ xbmcvfs.configini.set(key, value) }
    ///
    /// ------------------------------------------------------------------------
    //
    class configini : public AddonClass
    {
      const std::string configini_path = "/flash/config.ini";

      // open config.ini for read
      inline bool read(std::vector<std::string> &lines) const {
        // does config.ini exist
        if (XFILE::CFile::Exists(configini_path.c_str(), true))
        {
          std::ifstream config_ini(configini_path.c_str());
          std::string line;

          lines.clear();

          while (std::getline(config_ini, line))
              lines.push_back(line);

          config_ini.close();
        }

        return (lines.size() > 0);
      }

      // open config.ini for write
      inline bool write(std::vector<std::string> lines) const {
        system("mount -o remount,rw /flash");

        std::ofstream config_ini(configini_path.c_str());
        std::copy(lines.begin(), lines.end(),
          std::ostream_iterator<std::string>(config_ini, "\n"));
        config_ini.close();

        system("sync");
        system("mount -o remount,ro /flash");

        return true;
      }

    public:
      inline configini()
      {
        DelayedCallGuard dg(languageHook);
      }

#if !defined(DOXYGEN_SHOULD_USE_THIS)
      inline configini* __enter__() { return this; };
      inline void __exit__() { };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_configini
      /// @brief \python_func{ get(key, def_no_value) }
      /// To get value for key in config.ini.
      ///
      /// @return                        value
      ///
      get(...);
#else
      inline std::string get(std::string key, std::string def_no_value = std::string())
      {
        DelayedCallGuard dg(languageHook);
        std::string ret = def_no_value;
        std::vector<std::string> flines;

        // read config.ini
        if (read(flines))
        {
          // search active line and get value
          for (int i = flines.size() - 1; i >= 0; i--)
          {
            if (StringUtils::StartsWith(flines[i], StringUtils::Format("{}=", key.c_str())))
            {
              std::vector<std::string> val = StringUtils::Split(flines[i], "=", 2);
              if (val.size() == 2)
              {
                ret = val[1];
                // Remove all double-quote characters
                ret.erase(remove(ret.begin(), ret.end(), '\"' ), ret.end());
                // Remove all single-quote characters
                ret.erase(remove(ret.begin(), ret.end(), '\'' ), ret.end());
                break;
              }
            }
          }
        }

        return ret;
      }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_configini
      /// @brief \python_func{ set(key, val) }
      /// To set value for key in config.ini.
      ///
      set(...);
#else
      inline void set(std::string key, std::string val = std::string())
      {
        DelayedCallGuard dg(languageHook);

        std::vector<std::string> flines;

        // read config.ini
        if (read(flines))
        {
          bool found = false;
          std::string strKey = StringUtils::Format("{}=", key.c_str());

          // Remove all double-quote characters
          val.erase(remove(val.begin(), val.end(), '\"' ), val.end());
          // Remove all single-quote characters
          val.erase(remove(val.begin(), val.end(), '\'' ), val.end());

          // search active line and update value
          for (int i = flines.size() - 1; i >= 0; i--)
          {
            if (flines[i].find(strKey) != std::string::npos)
            {
              if (StringUtils::StartsWith(flines[i], StringUtils::Format("{}=", key.c_str())))
              {
                flines[i] = StringUtils::Format("{}='{}'", key.c_str(), val.c_str());
                found = true;
                break;
              }
            }
          }

          // search last not active line
          if (!found)
          {
            for (int i = flines.size() - 1; i >= 0; i--)
            {
              if (flines[i].find(strKey) != std::string::npos)
              {
                if (StringUtils::StartsWith(flines[i], "#"))
                {
                  flines[i] = StringUtils::Format("{}='{}'", key.c_str(), val.c_str());
                  found = true;
                  break;
                }
              }
            }
          }

          // still not found, append on end
          if (!found)
            flines.push_back(StringUtils::Format("{}='{}'", key.c_str(), val.c_str()));

          // write contents to config.ini
          write(flines);
        }
      }
#endif
    };
  }
}
