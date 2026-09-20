/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsMigrationStep.h"
#include "utils/XBMCTinyXML.h"

namespace KODI::SETTINGS
{

/*!
 * \brief Return the default list of migration functions.
 * \return The migration functions list.
 */
MigrationStepList BuildMigrationSteps();

} // namespace KODI::SETTINGS
