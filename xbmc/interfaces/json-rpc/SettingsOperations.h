#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "JSONRPC.h"

class CVariant;
class ISetting;
class CSettingSection;
class CSettingCategory;
class CSettingGroup;
class CSetting;
class CSettingBool;
class CSettingInt;
class CSettingNumber;
class CSettingString;
class CSettingAction;
class CSettingList;
class CSettingPath;
class CSettingAddon;
class CSettingDate;
class CSettingTime;
class ISettingControl;

namespace JSONRPC
{
  class CSettingsOperations
  {
  public:
    static JSONRPC_STATUS GetSections(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetCategories(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSettings(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    
    static JSONRPC_STATUS GetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS ResetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

  private:
    static int ParseSettingLevel(const std::string &strLevel);

    static bool SerializeISetting(std::shared_ptr<const ISetting> setting, CVariant &obj);
    static bool SerializeSettingSection(std::shared_ptr<const CSettingSection> setting, CVariant &obj);
    static bool SerializeSettingCategory(std::shared_ptr<const CSettingCategory> setting, CVariant &obj);
    static bool SerializeSettingGroup(std::shared_ptr<const CSettingGroup> setting, CVariant &obj);
    static bool SerializeSetting(std::shared_ptr<const CSetting> setting, CVariant &obj);
    static bool SerializeSettingBool(std::shared_ptr<const CSettingBool> setting, CVariant &obj);
    static bool SerializeSettingInt(std::shared_ptr<const CSettingInt> setting, CVariant &obj);
    static bool SerializeSettingNumber(std::shared_ptr<const CSettingNumber> setting, CVariant &obj);
    static bool SerializeSettingString(std::shared_ptr<const CSettingString> setting, CVariant &obj);
    static bool SerializeSettingAction(std::shared_ptr<const CSettingAction> setting, CVariant &obj);
    static bool SerializeSettingList(std::shared_ptr<const CSettingList> setting, CVariant &obj);
    static bool SerializeSettingPath(std::shared_ptr<const CSettingPath> setting, CVariant &obj);
    static bool SerializeSettingAddon(std::shared_ptr<const CSettingAddon> setting, CVariant &obj);
    static bool SerializeSettingDate(std::shared_ptr<const CSettingDate> setting, CVariant &obj);
    static bool SerializeSettingTime(std::shared_ptr<const CSettingTime> setting, CVariant &obj);
    static bool SerializeSettingControl(std::shared_ptr<const ISettingControl> control, CVariant &obj);

    static void SerializeSettingListValues(const std::vector<CVariant> &values, CVariant &obj);
  };
}
