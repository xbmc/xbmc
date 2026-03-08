/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscs.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscTransport.h"
#include "games/addons/disc/GameClientDiscXML.h"

#include <algorithm>
#include <vector>

using namespace KODI;
using namespace GAME;

CGameClientDiscs::CGameClientDiscs(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess),
    m_transport(std::make_unique<CGameClientDiscTransport>(gameClient, addonStruct, clientAccess)),
    m_discXml(std::make_unique<CGameClientDiscXML>()),
    m_discM3u(std::make_unique<CGameClientDiscM3U>()),
    m_discModel(std::make_unique<CGameClientDiscModel>())
{
}

CGameClientDiscs::~CGameClientDiscs() = default;

bool CGameClientDiscs::SupportsDiskControl() const
{
  return m_gameClient.SupportsDiscControl();
}

void CGameClientDiscs::Initialize(const std::string& gamePath)
{
  CGameClientDiscModel restoredModel;
  if (m_discXml->Load(gamePath, restoredModel) && !restoredModel.Empty())
  {
    // Load successful
    *m_discModel = restoredModel;
    m_isEjected = m_discModel->IsEjected();

    // Prune removed discs when the core is newly loaded
    PruneRemovedDiscs(*m_discModel);

    // Set the initial disc index, if supported by libretro
    const std::optional<size_t> selectedIndex = m_discModel->GetSelectedDiscIndex();
    if (selectedIndex.has_value())
    {
      const std::string startupPath = m_discModel->GetPathByIndex(*selectedIndex);
      if (!startupPath.empty())
        m_transport->SetInitialImage(static_cast<unsigned int>(*selectedIndex), startupPath);
    }
  }

  // Avoid launching with an empty disc list
  if (m_discModel->Empty())
    m_discModel->AddDisc(gamePath);
}

void CGameClientDiscs::RestoreDiscList()
{
  if (m_discModel->Empty())
    return;

  if (!m_transport->GetEjectState())
  {
    if (!m_transport->SetEjectState(true))
      return;

    if (!m_transport->GetEjectState())
      return;
  }

  unsigned int imageCount = m_transport->GetImageCount();

  // Restore disc list
  for (size_t i = 0; i < m_discModel->Size(); ++i)
  {
    if (m_discModel->IsRemovedSlotByIndex(i))
      continue;

    const std::string imagePath = m_discModel->GetPathByIndex(i);
    if (imagePath.empty())
      continue;

    while (i >= imageCount)
    {
      if (!m_transport->AddImageIndex())
        return;

      const unsigned int previousCount = imageCount;
      imageCount = m_transport->GetImageCount();

      if (imageCount <= previousCount)
        return; // Prevent infinite loop if core doesn't increment

      if (i >= imageCount && imageCount == 0)
        return;
    }

    if (!m_transport->ReplaceImageIndex(static_cast<unsigned int>(i), imagePath))
      return;
  }

  // Restore selected index
  const std::optional<size_t> selectedIndex = m_discModel->GetSelectedDiscIndex();
  if (selectedIndex.has_value())
    m_transport->SetImageIndex(static_cast<unsigned int>(*selectedIndex));
  else
    m_transport->SetImageIndex(m_transport->GetImageCount());

  // Restore ejected state
  const bool finalEjected = m_discModel->IsEjected();
  m_transport->SetEjectState(finalEjected);
  m_isEjected = m_transport->GetEjectState();
}

void CGameClientDiscs::RefreshDiscState()
{
  // Get fresh core state
  CGameClientDiscModel coreModel;
  LoadModelFromCore(coreModel);

  // Reconcile frontend state with the fresh core model
  *m_discModel = CGameClientDiscMergeUtils::ReconcileModels(*m_discModel, coreModel);
  m_isEjected = m_discModel->IsEjected();

  SaveDiscState();
}

bool CGameClientDiscs::SetEjected(bool ejected)
{
  if (!m_transport->SetEjectState(ejected))
    return false;

  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::AddDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  // Skip duplicates in the frontend model
  if (m_discModel->GetDiscIndexByPath(filePath).has_value())
    return true;

  // Libretro only allows changing the inserted image while ejected
  if (!m_isEjected)
    return true;

  // Prefer reusing the first removed slot before growing the core list
  std::optional<size_t> removedIndex;
  for (size_t i = 0; i < m_discModel->Size(); ++i)
  {
    if (m_discModel->IsRemovedSlotByIndex(i))
    {
      removedIndex = i;
      break;
    }
  }

  if (removedIndex.has_value())
  {
    if (!m_transport->ReplaceImageIndex(static_cast<unsigned int>(*removedIndex), filePath))
      return false;
  }
  else
  {
    const unsigned int currentImageCount = m_transport->GetImageCount();

    // No removed slot available, so add a new slot in the core
    if (!m_transport->AddImageIndex())
      return false;

    const unsigned int newImageCount = m_transport->GetImageCount();

    if (newImageCount != currentImageCount + 1 || newImageCount < currentImageCount)
      return false;

    // New slot is the last one
    const unsigned int newIndex = newImageCount - 1;

    // Populate the new slot
    if (!m_transport->ReplaceImageIndex(newIndex, filePath))
    {
      // Best effort rollback in the core
      m_transport->RemoveImageIndex(newIndex);
      return false;
    }

    // Add the image to the model. This can be overwritten when we refresh
    // the disc state, but will serve as a fallback in case the libretro
    // extended disc control interface (which allows retrieving path and label)
    // isn't supported.
    m_discModel->AddDisc(filePath);
  }

  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::RemoveDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return true;

  return RemoveDiscByIndex(*discIndex);
}

bool CGameClientDiscs::RemoveDiscByIndex(size_t index)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  const std::optional<size_t> selectedIndex = m_discModel->GetSelectedDiscIndex();
  const bool wasSelected = selectedIndex.has_value() && *selectedIndex == index;

  // Remove from the core by current index
  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(index)))
    return false;

  // If the removed slot was currently selected, force "No disc" before refresh.
  // This makes UI behavior deterministic even if the core leaves a zombie slot
  // behind or reports stale selection state briefly.
  if (wasSelected)
  {
    const unsigned int noDiscIndex = m_transport->GetImageCount();
    m_transport->SetImageIndex(noDiscIndex);
  }

  m_discModel->MarkRemovedByIndex(index);

  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::InsertDisc(const std::string& filePath)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (filePath.empty())
  {
    const unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel
    if (!m_transport->SetImageIndex(imageIndex))
      return false;

    RefreshDiscState();
  }
  else
  {
    const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
    if (discIndex.has_value())
    {
      if (!InsertDiscByIndex(*discIndex))
        return false;
    }
  }

  return true;
}

bool CGameClientDiscs::InsertDiscByIndex(size_t index)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  if (!m_transport->SetImageIndex(static_cast<unsigned int>(index)))
    return false;

  RefreshDiscState();

  return true;
}

void CGameClientDiscs::LoadModelFromCore(CGameClientDiscModel& model) const
{
  model.Clear();

  // Load ejected state
  model.SetEjected(m_transport->GetEjectState());

  const unsigned int imageCount = m_transport->GetImageCount();
  const unsigned int imageIndex = m_transport->GetImageIndex();
  std::optional<size_t> selectedModelIndex;

  for (unsigned int i = 0; i < imageCount; ++i)
  {
    std::string imagePath = m_transport->GetImagePath(i);
    std::string imageLabel = m_transport->GetImageLabel(i);

    const size_t modelIndex = model.Size();
    model.AddDisc(imagePath, imageLabel);

    if (i == imageIndex)
      selectedModelIndex = modelIndex;
  }

  if (model.Empty())
    return;

  if (selectedModelIndex.has_value())
    model.SetSelectedDiscByIndex(*selectedModelIndex);
  else
    model.SetSelectedNoDisc();
}

void CGameClientDiscs::SaveDiscState() const
{
  if (!m_gameClient.GetGamePath().empty())
  {
    m_discXml->Save(m_gameClient.GetGamePath(), *m_discModel);
    m_discM3u->Save(m_gameClient.GetGamePath(), *m_discModel);
  }
}

void CGameClientDiscs::PruneRemovedDiscs(CGameClientDiscModel& model)
{
  for (size_t i = 0; i < model.Size();)
  {
    if (!model.IsRemovedSlotByIndex(i))
    {
      ++i;
      continue;
    }

    model.EraseDiscByIndex(i);
  }
}
