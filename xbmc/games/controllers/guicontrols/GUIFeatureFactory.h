/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControlTypes.h"

class CGUIButtonControl;

namespace KODI
{
namespace GAME
{
class CPhysicalFeature;
class IConfigurationWizard;

/*!
 * \ingroup games
 */
class CGUIFeatureFactory
{
public:
  /*!
   * \brief Create a button of the specified type
   * \param type The type of button control being created
   * \return A button control, or nullptr if type is invalid
   */
  static CGUIButtonControl* CreateButton(BUTTON_TYPE type,
                                         const CGUIButtonControl& buttonTemplate,
                                         IConfigurationWizard* wizard,
                                         const CPhysicalFeature& feature,
                                         unsigned int index);
};
} // namespace GAME
} // namespace KODI
