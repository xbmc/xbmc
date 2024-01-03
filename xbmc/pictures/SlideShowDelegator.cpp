/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SlideShowDelegator.h"

void CSlideShowDelegator::SetDelegate(ISlideShowDelegate* delegate)
{
  m_delegate = delegate;
}

void CSlideShowDelegator::ResetDelegate()
{
  m_delegate = nullptr;
}

void CSlideShowDelegator::Add(const CFileItem* picture)
{
  if (m_delegate)
  {
    m_delegate->Add(picture);
  }
}

bool CSlideShowDelegator::IsPlaying() const
{
  if (m_delegate)
  {
    return m_delegate->IsPlaying();
  }
  return false;
}

void CSlideShowDelegator::Select(const std::string& picture)
{
  if (m_delegate)
  {
    m_delegate->Select(picture);
  }
}

void CSlideShowDelegator::GetSlideShowContents(CFileItemList& list)
{
  if (m_delegate)
  {
    m_delegate->GetSlideShowContents(list);
  }
}

std::shared_ptr<const CFileItem> CSlideShowDelegator::GetCurrentSlide()
{
  if (m_delegate)
  {
    return m_delegate->GetCurrentSlide();
  }
  return nullptr;
}

void CSlideShowDelegator::StartSlideShow()
{
  if (m_delegate)
  {
    m_delegate->StartSlideShow();
  }
}

void CSlideShowDelegator::PlayPicture()
{
  if (m_delegate)
  {
    m_delegate->PlayPicture();
  }
}

bool CSlideShowDelegator::InSlideShow() const
{
  if (m_delegate)
  {
    return m_delegate->InSlideShow();
  }
  return false;
}

int CSlideShowDelegator::NumSlides() const
{
  if (m_delegate)
  {
    return m_delegate->NumSlides();
  }
  return -1;
}

int CSlideShowDelegator::CurrentSlide() const
{
  if (m_delegate)
  {
    return m_delegate->CurrentSlide();
  }
  return -1;
}

bool CSlideShowDelegator::IsPaused() const
{
  if (m_delegate)
  {
    return m_delegate->IsPaused();
  }
  return false;
}

bool CSlideShowDelegator::IsShuffled() const
{
  if (m_delegate)
  {
    return m_delegate->IsShuffled();
  }
  return false;
}

void CSlideShowDelegator::Reset()
{
  if (m_delegate)
  {
    m_delegate->Reset();
  }
}

void CSlideShowDelegator::Shuffle()
{
  if (m_delegate)
  {
    m_delegate->Shuffle();
  }
}

int CSlideShowDelegator::GetDirection() const
{
  if (m_delegate)
  {
    return m_delegate->GetDirection();
  }
  return -1;
}

void CSlideShowDelegator::RunSlideShow(const std::string& strPath,
                                       bool bRecursive,
                                       bool bRandom,
                                       bool bNotRandom,
                                       const std::string& beginSlidePath,
                                       bool startSlideShow,
                                       SortBy method,
                                       SortOrder order,
                                       SortAttribute sortAttributes,
                                       const std::string& strExtensions)
{
  if (m_delegate)
  {
    m_delegate->RunSlideShow(strPath, bRecursive, bRandom, bNotRandom, beginSlidePath,
                             startSlideShow, method, order, sortAttributes, strExtensions);
  }
}

void CSlideShowDelegator::AddFromPath(const std::string& strPath,
                                      bool bRecursive,
                                      SortBy method,
                                      SortOrder order,
                                      SortAttribute sortAttributes,
                                      const std::string& strExtensions)
{
  if (m_delegate)
  {
    m_delegate->AddFromPath(strPath, bRecursive, method, order, sortAttributes, strExtensions);
  }
}
