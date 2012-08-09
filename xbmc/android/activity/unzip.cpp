/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <zip.h>
#include "XBMCApp.h"

typedef struct stat Stat;

static int do_mkdir(const char *path)
{
  Stat            st;
  int             status = 0;

  if (stat(path, &st) != 0)
  {
    /* Directory does not exist */
   if (mkdir(path, 0755) != 0)
     status = -1;
  }
  else if (!S_ISDIR(st.st_mode))
  {
    errno = ENOTDIR;
    status = -1;
  }

  return(status);
}

int mkpath(const char *path)
{
  char           *pp;
  char           *sp;
  int             status;
  char           *copypath = strdup(path);

  status = 0;
  pp = copypath;
  while (status == 0 && (sp = strchr(pp, '/')) != 0)
  {
    if (sp != pp)
    {
      /* Neither root nor double slash in path */
      *sp = '\0';
      status = do_mkdir(copypath);
      *sp = '/';
    }
    pp = sp + 1;
  }
  if (status == 0)
    status = do_mkdir(path);
  free(copypath);
  return (status);
}

int extract_to_cache(const char *archive, const char *cache_path)
{
  struct zip *ziparchive;
  struct zip_file *zipfile;
  struct zip_stat zipstat;
  char buf[4096];
  int err;
  int len;
  int fd;
  int dirname_len;
  uint64_t sum;
  char *full_path;
  char *dir_name;
  Stat localfile;
  utimbuf modified;
  CXBMCApp::android_printf("unzip: Preparing to cache. This could take a while...");

  if ((ziparchive = zip_open(archive, 0, &err)) == NULL)
  {
    zip_error_to_str(buf, sizeof(buf), err, errno);
    CXBMCApp::android_printf("unzip error: can't open archive %s/n",archive);
    return 1;
  }

  mkpath(cache_path);

  for (size_t i = 0; i < zip_get_num_entries(ziparchive, 0); i++)
  {
    if (zip_stat_index(ziparchive, i, 0, &zipstat) != 0)
    {
      CXBMCApp::android_printf("unzip error: can't open entry: %i/n",i);
      continue;
    }

    if(strncmp (zipstat.name,"assets/",7) != 0)
      continue;

    if(strncmp (zipstat.name,"assets/python2.6",16) == 0)
      continue;

    modified.modtime = modified.actime = zipstat.mtime;

    full_path = (char*)malloc( sizeof(char*) * (strlen(cache_path) + 1 + strlen(zipstat.name) + 1));
    sprintf(full_path, "%s/%s",cache_path,zipstat.name);

    if (zipstat.name[strlen(zipstat.name) - 1] == '/')
    {
      mkpath(full_path);
      free(full_path);
      continue;
    }

    zipfile = zip_fopen_index(ziparchive, i, 0);
    if (!zipfile)
    {
      CXBMCApp::android_printf("unzip error: can't open index");
      free(full_path);
      continue;
    }

    if (stat(full_path, &localfile) != 0)
    {
      dirname_len = strrchr(zipstat.name,'/') - zipstat.name;
      dir_name = (char*)malloc( sizeof(char*) * (strlen(cache_path) + 1 + dirname_len + 1));
      strncpy(dir_name, full_path, strlen(cache_path) + dirname_len + 1);
      dir_name[strlen(cache_path) + dirname_len + 1] = '\0';
      mkpath(dir_name);
      free(dir_name);
    }
    // watch this compare, zipstat.mtime is time_t which is a signed long
    else if (localfile.st_mtime == (unsigned long)zipstat.mtime)
    {
      free(full_path);
      continue;
    }

    fd = open(full_path, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if(fd < 0)
    {
      CXBMCApp::android_printf("unzip error: could not open %s",full_path);
      free(full_path);
      continue;
    }

    sum = 0;
    while (sum != zipstat.size)
    {
      len = zip_fread(zipfile, buf, 4096);
      if (len < 0)
      {
        CXBMCApp::android_printf("unzip error: no data in %s",full_path);
        free(full_path);
        continue;
      }
      write(fd, buf, len);
      sum += len;
    }
    close(fd);
    zip_fclose(zipfile);

    if (stat(full_path, &localfile) == 0)
    {
      // save the zip time. this way we know for certain if we need to refresh.
      utime(full_path, &modified);
    }
    else
    {
      CXBMCApp::android_printf("unzip error: failed to extract %s",full_path);
    }

    free(full_path);
  }

  if (zip_close(ziparchive) == -1)
    CXBMCApp::android_printf("unzip error: can't close zip archive `%s'/n", archive);

  return 0;
}
