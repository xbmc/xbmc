/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRFileItemListModifier.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <memory>

using namespace PVR;

namespace
{
bool NeedsResolving(const CFileItem& item)
{
  // Items created by add-ons that only know the item's path (rather than by PVR-internal code,
  // which always attaches the respective PVR tag) need to be resolved before use, e.g. to be
  // able to show proper art and info for them.
  return URIUtils::IsPVR(item.GetPath()) && !item.HasPVRChannelInfoTag() &&
         !item.HasPVRRecordingInfoTag() && !item.HasEPGInfoTag() && !item.HasPVRTimerInfoTag();
}
} // unnamed namespace

bool CPVRFileItemListModifier::CanModify(const CFileItemList& items) const
{
  return std::ranges::any_of(items, [](const auto& item) { return NeedsResolving(*item); });
}

bool CPVRFileItemListModifier::Modify(CFileItemList& items) const
{
  bool changed{false};
  for (const auto& item : items)
  {
    if (NeedsResolving(*item))
    {
      const std::shared_ptr<CFileItem> loadedItem{
          CServiceBroker::GetPVRManager().Get<PVR::GUI::Utils>().LoadItem(*item)};
      if (loadedItem)
      {
        item->UpdateInfo(*loadedItem, false /* replaceLabels */);
        changed = true;
      }
    }
  }
  return changed;
}
