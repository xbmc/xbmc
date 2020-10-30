/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CSetting;
class TiXmlNode;

class ISettingCallback
{
public:
  virtual ~ISettingCallback() = default;

  /*!
   \brief The value of the given setting is being changed.

   This callback is triggered whenever the value of a setting is being
   changed. The given CSetting already contains the new value and the handler
   of the callback has the possibility to allow or revert changing the value
   of the setting. In case of a revert OnSettingChanging() is called again to
   inform all listeners that the value change has been reverted.

   \param setting The setting whose value is being changed (already containing the changed value)
   \return True if the new value is acceptable otherwise false
   */
  virtual bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) { return true; }

  /*!
   \brief The value of the given setting has changed.

   This callback is triggered whenever the value of a setting has been
   successfully changed (i.e. none of the OnSettingChanging() handlers)
   has reverted the change.

   \param setting The setting whose value has been changed
   */
  virtual void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) {}

  /*!
   \brief The given setting has been activated.

   This callback is triggered whenever the given setting has been activated.
   This callback is only fired for CSettingAction settings.

   \param setting The setting which has been activated.
   */
  virtual void OnSettingAction(const std::shared_ptr<const CSetting>& setting) {}

  /*!
   \brief The given setting needs to be updated.

   This callback is triggered when a setting needs to be updated because its
   value is outdated. This only happens when initially loading the value of a
   setting and will not be triggered afterwards.

   \param setting The setting which needs to be updated.
   \param oldSettingId The id of the previous setting.
   \param oldSettingNode The old setting node
   \return True if the setting has been successfully updated otherwise false
   */
  virtual bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                               const char* oldSettingId,
                               const TiXmlNode* oldSettingNode)
  {
    return false;
  }

  /*!
   \brief The given property of the given setting has changed

   This callback is triggered when a property (e.g. enabled or the list of
   dynamic options) has changed.

   \param setting The setting which has a changed property
   \param propertyName The string representation of the changed property
   */
  virtual void OnSettingPropertyChanged(const std::shared_ptr<const CSetting>& setting,
                                        const char* propertyName)
  {
  }
};
