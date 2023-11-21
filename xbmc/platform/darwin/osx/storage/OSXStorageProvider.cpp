/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>
#include "OSXStorageProvider.h"
#include "utils/RegExp.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"

#include <sys/mount.h>
#include <DiskArbitration/DiskArbitration.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include "platform/darwin/osx/CocoaInterface.h"

std::vector<std::pair<std::string, std::string>> COSXStorageProvider::m_mountsToNotify;
std::vector<std::pair<std::string, std::string>> COSXStorageProvider::m_unmountsToNotify;

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<COSXStorageProvider>();
}

COSXStorageProvider::COSXStorageProvider()
{
  PumpDriveChangeEvents(NULL);
}

void COSXStorageProvider::GetLocalDrives(VECSOURCES& localDrives)
{
  CMediaSource share;

  // User home folder
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  localDrives.push_back(share);

  // User desktop folder
  share.strPath = getenv("HOME");
  share.strPath += "/Desktop";
  share.strName = "Desktop";
  share.m_ignore = true;
  localDrives.push_back(share);

  // Volumes (all mounts are present here)
  share.strPath = "/Volumes";
  share.strName = "Volumes";
  share.m_ignore = true;
  localDrives.push_back(share);

  // This will pick up all local non-removable disks including the Root Disk.
  DASessionRef session = DASessionCreate(kCFAllocatorDefault);
  if (session)
  {
    unsigned i, count = 0;
    struct statfs *buf = NULL;
    std::string mountpoint, devicepath;

    count = getmntinfo(&buf, 0);
    for (i=0; i<count; i++)
    {
      mountpoint = buf[i].f_mntonname;
      devicepath = buf[i].f_mntfromname;

      DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, devicepath.c_str());
      if (disk)
      {
        CFDictionaryRef details = DADiskCopyDescription(disk);
        if (details)
        {
          if (kCFBooleanFalse == CFDictionaryGetValue(details, kDADiskDescriptionMediaRemovableKey))
          {
            CMediaSource sharesrc;

            sharesrc.strPath = mountpoint;
            Cocoa_GetVolumeNameFromMountPoint(mountpoint, sharesrc.strName);
            sharesrc.m_ignore = true;
            localDrives.push_back(sharesrc);
          }
          CFRelease(details);
        }
        CFRelease(disk);
      }
    }

    CFRelease(session);
  }
}

void COSXStorageProvider::GetRemovableDrives(VECSOURCES& removableDrives)
{
  DASessionRef session = DASessionCreate(kCFAllocatorDefault);
  if (session)
  {
    unsigned i, count = 0;
    struct statfs *buf = NULL;
    std::string mountpoint, devicepath;

    count = getmntinfo(&buf, 0);
    for (i=0; i<count; i++)
    {
      mountpoint = buf[i].f_mntonname;
      devicepath = buf[i].f_mntfromname;

      DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, devicepath.c_str());
      if (disk)
      {
        CFDictionaryRef details = DADiskCopyDescription(disk);
        if (details)
        {
          if (kCFBooleanTrue == CFDictionaryGetValue(details, kDADiskDescriptionMediaRemovableKey))
          {
            CMediaSource share;

            share.strPath = mountpoint;
            share.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
            Cocoa_GetVolumeNameFromMountPoint(mountpoint, share.strName);
            share.m_ignore = true;
            // detect if its a cd or dvd
            // needs to be ejectable
            if (kCFBooleanTrue == CFDictionaryGetValue(details, kDADiskDescriptionMediaEjectableKey))
            {
              CFStringRef mediaKind = (CFStringRef)CFDictionaryGetValue(details, kDADiskDescriptionMediaKindKey);
              // and either cd or dvd kind of media in it
              if (mediaKind != NULL &&
                  (CFStringCompare(mediaKind, CFSTR(kIOCDMediaClass), 0) == kCFCompareEqualTo ||
                  CFStringCompare(mediaKind, CFSTR(kIODVDMediaClass), 0) == kCFCompareEqualTo))
                share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
            }
            removableDrives.push_back(share);
          }
          CFRelease(details);
        }
        CFRelease(disk);
      }
    }

    CFRelease(session);
  }
}

std::vector<std::string> COSXStorageProvider::GetDiskUsage()
{
  std::vector<std::string> result;

  FILE* pipe = popen("df -HT ufs,cd9660,hfs,apfs,udf", "r");
  if (pipe)
  {
    char line[1024];
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      result.emplace_back(line);
    }
    pclose(pipe);
  }

  return result;
}

namespace
{
  class DAOperationContext
  {
  public:
    explicit DAOperationContext(const std::string& mountpath);
    ~DAOperationContext();

    DADiskRef GetDisk() const { return m_disk; }

    void Reset();
    bool WaitForCompletion(CFTimeInterval timeout);
    void Completed(bool success);

    static void CompletionCallback(DADiskRef disk, DADissenterRef dissenter, void* context);

  private:
    DAOperationContext() = delete;

    static void RunloopPerformCallback(void* info) {}
    CFRunLoopSourceContext m_runLoopSourceContext = { .perform = RunloopPerformCallback };

    bool m_success;
    bool m_completed;
    const DASessionRef m_session;
    const CFRunLoopRef m_runloop;
    const CFRunLoopSourceRef m_runloopSource;
    DADiskRef m_disk;
  };

  DAOperationContext::DAOperationContext(const std::string& mountpath)
  : m_success(true),
    m_completed(false),
    m_session(DASessionCreate(kCFAllocatorDefault)),
    m_runloop(CFRunLoopGetCurrent()), // not owner!
    m_runloopSource(CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &m_runLoopSourceContext))
  {
    if (m_session && m_runloop && m_runloopSource)
    {
      CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)mountpath.c_str(), mountpath.size(), TRUE);
      if (url)
      {
        m_disk = DADiskCreateFromVolumePath(kCFAllocatorDefault, m_session, url);
        CFRelease(url);
      }

      DASessionScheduleWithRunLoop(m_session, m_runloop, kCFRunLoopDefaultMode);
      CFRunLoopAddSource(m_runloop, m_runloopSource, kCFRunLoopDefaultMode);
    }
  }

  DAOperationContext::~DAOperationContext()
  {
    if (m_session && m_runloop && m_runloopSource)
    {
      CFRunLoopRemoveSource(m_runloop, m_runloopSource, kCFRunLoopDefaultMode);
      DASessionUnscheduleFromRunLoop(m_session, m_runloop, kCFRunLoopDefaultMode);
      CFRunLoopSourceInvalidate(m_runloopSource);
    }

    if (m_disk)
      CFRelease(m_disk);
    if (m_runloopSource)
      CFRelease(m_runloopSource);
    if (m_session)
      CFRelease(m_session);
  }

  bool DAOperationContext::WaitForCompletion(CFTimeInterval timeout)
  {
    while (!m_completed)
    {
      if (CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeout, TRUE) == kCFRunLoopRunTimedOut)
        break;
    }
    return m_success;
  }

  void DAOperationContext::Completed(bool success)
  {
    m_success = success;
    m_completed = true;
    CFRunLoopSourceSignal(m_runloopSource);
    CFRunLoopWakeUp(m_runloop);
  }

  void DAOperationContext::Reset()
  {
    m_success = true;
    m_completed = false;
  }

  void DAOperationContext::CompletionCallback(DADiskRef disk, DADissenterRef dissenter, void* context)
  {
    DAOperationContext* dacontext = static_cast<DAOperationContext*>(context);

    bool success = true;
    if (dissenter)
    {
      DAReturn status = DADissenterGetStatus(dissenter);
      success = (status == kDAReturnSuccess || status == kDAReturnUnsupported);
    }

    dacontext->Completed(success);
  }

} // unnamed namespace

bool COSXStorageProvider::Eject(const std::string& mountpath)
{
  if (mountpath.empty())
    return false;

  DAOperationContext ctx(mountpath);
  DADiskRef disk = ctx.GetDisk();

  if (!disk)
    return false;

  bool success = false;

  CFDictionaryRef details = DADiskCopyDescription(disk);
  if (details)
  {
    // Does the device need to be unmounted first?
    if (CFDictionaryGetValueIfPresent(details, kDADiskDescriptionVolumePathKey, NULL))
    {
      DADiskUnmount(disk, kDADiskUnmountOptionDefault, DAOperationContext::CompletionCallback, &ctx);
      success = ctx.WaitForCompletion(30.0); // timeout after 30 secs
    }

    if (success)
    {
      ctx.Reset();
      DADiskEject(disk, kDADiskEjectOptionDefault, DAOperationContext::CompletionCallback, &ctx);
      success = ctx.WaitForCompletion(30.0); // timeout after 30 secs
    }

    CFRelease(details);
  }

  return success;
}

bool COSXStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback* callback)
{
  // Note: If we find a way to only notify kodi user initiated mounts/unmounts we
  //       could do this here, but currently we can't distinguish this and popups
  //       for system initiated mounts/unmounts (like done by Time Machine automatic
  //       backups) are very confusing and annoying.
  bool bChanged = !m_mountsToNotify.empty() || !m_unmountsToNotify.empty();
  if (bChanged)
  {
    m_mountsToNotify.clear();
    m_unmountsToNotify.clear();
  }
  return bChanged;
}

void COSXStorageProvider::VolumeMountNotification(const char* label, const char* mountpoint)
{
  if (label && mountpoint)
    m_mountsToNotify.emplace_back(label, mountpoint);
}

void COSXStorageProvider::VolumeUnmountNotification(const char* label, const char* mountpoint)
{
  if (label && mountpoint)
    m_unmountsToNotify.emplace_back(label, mountpoint);
}
