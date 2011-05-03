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

#include "PackageKitManager.h"

using namespace std;

CPackageKitManager::CPackageKitManager() : CThread()
{
  if (!g_thread_supported())
    g_thread_init (NULL);
  dbus_g_thread_init();
  g_type_init();
  m_loop = g_main_loop_new(NULL, FALSE);

  m_client = pk_client_new();
  Create();
}

CPackageKitManager::~CPackageKitManager()
{
  StopThread();
}

CPackageKitManager &CPackageKitManager::Get()
{
  static CPackageKitManager sMgr;
  return sMgr;
}

struct PackageFetchUserData
{
  IPackageRetrieveCallback *callback;
  vector<Package> packages;
};

struct PackageInstallUserData
{
  IInstallCallback *callback;
  string version;
  bool install;
  GCancellable* cancel;
};

void CPackageKitManager::getPackages(IPackageRetrieveCallback *callback)
{
  PackageFetchUserData *user_data = new PackageFetchUserData;
  user_data->callback = callback;

  pk_client_get_packages_async(m_client,
                               PK_FILTER_ENUM_GUI,
                               NULL,
                               CPackageKitManager::GetPackagesProgressCallback,
                               user_data,
                               CPackageKitManager::GetPackagesFinishCallback,
                               user_data);
}

GCancellable *CPackageKitManager::transaction(const string &ID, 
                                              const string &version,
                                              IInstallCallback *callback,
                                              bool install)
{
  PackageInstallUserData *user_data = new PackageInstallUserData;
  user_data->callback = callback;
  user_data->version = version;
  user_data->install = install;
  user_data->cancel = g_cancellable_new();

  gchar *packages[2];
  packages[0] = g_strdup_printf("%s", ID.c_str());
  packages[1] = NULL;

  pk_client_search_names_async(m_client,0,
                               packages,
                               user_data->cancel,
                               CPackageKitManager::InstallProgressCallback,
                               user_data,
                               CPackageKitManager::SearchFinishCallback,
                               user_data);

  g_free(packages[0]);

  return user_data->cancel;
}

void CPackageKitManager::UpdateRepos(IInstallCallback *callback)
{
  PackageInstallUserData *user_data = new PackageInstallUserData;
  user_data->callback = callback;

  pk_client_refresh_cache_async(m_client,
                                TRUE,
                                NULL,
                                CPackageKitManager::InstallProgressCallback,
                                user_data,
                                CPackageKitManager::InstallFinishCallback,
                                user_data);
}

void CPackageKitManager::Cancel(GCancellable *job)
{
  g_cancellable_cancel(job);
}

void CPackageKitManager::Process()
{
  g_main_loop_run(m_loop);
}

void CPackageKitManager::StopThread(bool bWait)
{
  if (m_loop && g_main_loop_is_running(m_loop))
    g_main_loop_quit(m_loop);
  CThread::StopThread(bWait);
}

void CPackageKitManager::InstallProgressCallback(PkProgress *progress,
                                                 PkProgressType type,
                                                 gpointer user_data)
{
  PackageInstallUserData *data = (PackageInstallUserData *)user_data;
  gint percentage;

  switch (type)
  {
    case PK_PROGRESS_TYPE_PERCENTAGE:
      g_object_get (progress, "percentage", &percentage, NULL);
      if (percentage >= 0 && percentage <= 100)
        data->callback->InstallProgress(percentage, 100);
      break;

    default:
      break;
  }
}

void CPackageKitManager::InstallFinishCallback(GObject *object, 
                                               GAsyncResult *res,
                                               gpointer user_data)
{
  PackageInstallUserData *data = (PackageInstallUserData *)user_data;

  data->callback->Done(true);

  delete data;
}

void CPackageKitManager::GetPackagesProgressCallback(PkProgress *progress, 
                                                     PkProgressType type, 
                                                     gpointer user_data)
{
  PackageFetchUserData *data = (PackageFetchUserData *)user_data;

  static gchar *package_id = NULL;
  gint percentage;

  switch (type)
  {
    case PK_PROGRESS_TYPE_PACKAGE_ID:
      g_object_get (progress, "package-id", &package_id, NULL);
      data->packages.push_back((Package){ package_id });
      break;
      break;
    case PK_PROGRESS_TYPE_PERCENTAGE:
      g_object_get (progress, "percentage", &percentage, NULL);
      if (percentage > 0 && percentage < 100)
        data->callback->Progress(percentage, 100);
      break;
    default:
      break;
  }
}

void CPackageKitManager::GetPackagesFinishCallback(GObject *object,
                                                   GAsyncResult *res,
                                                   gpointer user_data)
{
  PackageFetchUserData *data = (PackageFetchUserData *)user_data;

  data->callback->PushPackages(data->packages);

  delete data;
}

void CPackageKitManager::SearchFinishCallback(GObject *object,
                                              GAsyncResult *res,
                                              gpointer user_data)
{
  PackageInstallUserData *data = (PackageInstallUserData *)user_data;

  PkClient* client = PK_CLIENT(object);
  GError* error = NULL;
  GPtrArray* array = NULL;
  PkError* pkerror = NULL;
  PkPackage* pkg;

  PkResults* results = pk_client_generic_finish(client,res,&error);
  if (!results)
    goto out;

  pkerror = pk_results_get_error_code(results);
  if (pkerror)
    goto out;

  array = pk_results_get_package_array(results);
  if (!array || !array->len)
    goto out;

  pkg = (PkPackage*)g_ptr_array_index(array,0);
  if (pkg)
  {
    gchar** split = g_strsplit_set(pk_package_get_id(pkg),";",-1);
    bool matches = split[1] && g_ascii_strcasecmp(split[1],
                                                    data->version.c_str()) == 0;
    g_strfreev(split);
    if (matches)
    {
      gchar* packages[2];
      packages[0] = g_strdup(pk_package_get_id(pkg));
      packages[1] = NULL;
      if (data->install)
      {
        pk_client_install_packages_async(client,
                                         TRUE,
                                         packages,
                                         data->cancel,
                                         CPackageKitManager::InstallProgressCallback,
                                         user_data,
                                         CPackageKitManager::InstallFinishCallback,
                                         user_data);
      }
      else
      {
        pk_client_remove_packages_async(client,
                                        packages,
                                        TRUE,
                                        TRUE,
                                        data->cancel,
                                        CPackageKitManager::InstallProgressCallback,
                                        user_data,
                                        CPackageKitManager::InstallFinishCallback,
                                        user_data);
      }

      g_free(packages[0]);
    }
  }
out:
  if (results)
    g_object_unref(results);
  if (array)
    g_ptr_array_unref(array);
  if (error)
    g_object_unref(error);

  if (!(results && array && array->len > 0 && !error))
  {
    data->callback->Done(false);
    delete data;
  }
}
