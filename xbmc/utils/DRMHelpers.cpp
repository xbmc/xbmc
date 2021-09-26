/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMHelpers.h"

#include "utils/StringUtils.h"

#include <sstream>

#include <drm_fourcc.h>
#include <xf86drm.h>

namespace DRMHELPERS
{

std::string FourCCToString(uint32_t fourcc)
{
  std::stringstream ss;
  ss << static_cast<char>((fourcc & 0x000000FF));
  ss << static_cast<char>((fourcc & 0x0000FF00) >> 8);
  ss << static_cast<char>((fourcc & 0x00FF0000) >> 16);
  ss << static_cast<char>((fourcc & 0xFF000000) >> 24);

  return ss.str();
}

std::string ModifierToString(uint64_t modifier)
{
#if defined(HAVE_DRM_MODIFIER_NAME)
  std::string modifierVendorStr{"UNKNOWN_VENDOR"};

  const char* vendorName = drmGetFormatModifierVendor(modifier);
  if (vendorName)
    modifierVendorStr = std::string(vendorName);

  free(const_cast<char*>(vendorName));

  std::string modifierNameStr{"UNKNOWN_MODIFIER"};

  const char* modifierName = drmGetFormatModifierName(modifier);
  if (modifierName)
    modifierNameStr = std::string(modifierName);

  free(const_cast<char*>(modifierName));

  if (modifier == DRM_FORMAT_MOD_LINEAR)
    return modifierNameStr;

  return modifierVendorStr + "_" + modifierNameStr;
#else
  return StringUtils::Format("{:#x}", modifier);
#endif
}

} // namespace DRMHELPERS
