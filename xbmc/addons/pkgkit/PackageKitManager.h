#pragma once
/*
 *      Copyright (C) 2011 Tobias Arrskog
 *      https://github.com/topfs2/packagekit-glib-wrapper
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define I_KNOW_THE_PACKAGEKIT_GLIB2_API_IS_SUBJECT_TO_CHANGE
#undef byte
#include <packagekit-glib2/packagekit.h>
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <iostream>
#include "threads/Thread.h"
#include <vector>

struct Package
{
  std::string ID;
/*
  CVariant metadata
*/
};

class IPackageRetrieveCallback
{
public:
  virtual void Progress(unsigned int progress, unsigned int total) = 0;
  virtual void PushPackages(const std::vector<Package> &packages) = 0;
};

class IInstallCallback
{
public:
  virtual void DownloadProgress(unsigned int progress, unsigned int total) = 0;
  virtual void InstallProgress(unsigned int progress, unsigned int total) = 0;
  virtual void Done(bool success) = 0;
};

class CPackageKitManager : public CThread
{
public:
  static CPackageKitManager &Get();

  void getPackages(IPackageRetrieveCallback *callback);
  GCancellable *transaction(const std::string &ID, 
                            const std::string& version,
                            IInstallCallback *callback,
                            bool install);
  void UpdateRepos(IInstallCallback *callback);

  void Cancel(GCancellable *job);
protected:
  virtual void Process();
  virtual void StopThread(bool bWait=true);

private:
  CPackageKitManager();
  virtual ~CPackageKitManager();
  static void InstallProgressCallback(PkProgress *progress, 
                                      PkProgressType type, gpointer user_data);
  static void InstallFinishCallback(GObject *object,
                                    GAsyncResult *res, gpointer user_data);

  static void GetPackagesProgressCallback(PkProgress *progress,
                                          PkProgressType type,
                                          gpointer user_data);
  static void GetPackagesFinishCallback(GObject *object,
                                        GAsyncResult *res, gpointer user_data);

  static void SearchFinishCallback(GObject *object,
                                   GAsyncResult *res, gpointer user_data);

  GMainLoop *m_loop;

  PkClient *m_client;

  // When this is true packagekit have callbacked with information enough so other async calls are possible.
};
