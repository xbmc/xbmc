/*!
\file GUIMultiImage.h
\brief
*/

#ifndef GUILIB_GUIMULTIIMAGECONTROL_H
#define GUILIB_GUIMULTIIMAGECONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIImage.h"
#include "utils/Stopwatch.h"
#include "utils/Job.h"
#include "threads/CriticalSection.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIMultiImage : public CGUIControl, public IJobCallback
{
public:
  CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd);
  CGUIMultiImage(const CGUIMultiImage &from);
  virtual ~CGUIMultiImage(void);
  virtual CGUIMultiImage *Clone() const { return new CGUIMultiImage(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual void SetInvalid();
  virtual bool CanFocus() const;
  virtual std::string GetDescription() const;

  void SetInfo(const CGUIInfoLabel &info);
  void SetAspectRatio(const CAspectRatio &ratio);

protected:
  void LoadDirectory();
  void OnDirectoryLoaded();
  void CancelLoading();

  enum DIRECTORY_STATUS { UNLOADED = 0, LOADING, LOADED, READY };
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  class CMultiImageJob : public CJob
  {
  public:
    CMultiImageJob(const std::string &path);
    virtual bool DoWork();
    virtual const char *GetType() const { return "multiimage"; };

    std::vector<std::string> m_files;
    std::string              m_path;
  };

  CGUIInfoLabel m_texturePath;
  std::string m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  unsigned int m_timePerImage;
  unsigned int m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;

  bool m_bDynamicResourceAlloc;
  std::vector<std::string> m_files;

  CGUIImage m_image;

  CCriticalSection m_section;
  DIRECTORY_STATUS m_directoryStatus;
  unsigned int m_jobID;
};
#endif
