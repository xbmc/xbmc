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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAVE_LIBARCHIVE

#include "ArchiveManager.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "File.h"

#define LIBARCHIVE_BLOCK_SIZE 10240

extern "C"
{
/* Struct holding xbmc data used in libarchive */
struct xbmc_client_data
{
  CStdString *filename;
  XFILE::CFile *file;
  void *buffer;
};

/* Function pointers used to interface libarchive with XBMC VFS */
static ssize_t __xbmc_file_read(struct archive *, void *, const void **);
static int64_t __xbmc_file_skip(struct archive *, void *, int64_t);
static int64_t __xbmc_file_seek(struct archive *, void *, int64_t, int);
static int __xbmc_file_open(struct archive *, void *);
static int __xbmc_file_close(struct archive *, void *);
static int __xbmc_file_switch(struct archive *, void *, void *);
};

CArchiveManager::~CArchiveManager()
{
  Unload();
}

struct archive *
CArchiveManager::CreateArchive(int format, int filter,
                               std::vector<CStdString> filePaths)
{
  struct archive *a;
  struct xbmc_client_data *data;
  int r = 0;

  if (!Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", __FUNCTION__);
    return NULL;
  }

  if ((a = m_dllLibArchive.archive_read_new()) == NULL)
  {
    CLog::Log(LOGERROR, "%s: Out of memory", __FUNCTION__);
    return NULL;
  }
  if (format < 0)
    m_dllLibArchive.archive_read_support_format_all(a);
  else
    m_dllLibArchive.archive_read_set_format(a, format);
  if (filter < 0)
    m_dllLibArchive.archive_read_support_filter_all(a);
  else
    m_dllLibArchive.archive_read_append_filter(a, filter);

  for (unsigned int i = 0; i < filePaths.size(); i++)
  {
    data =
      (struct xbmc_client_data *)calloc(1, sizeof(struct xbmc_client_data));
    if (!data)
    {
      CLog::Log(LOGERROR, "%s: Out of memory", __FUNCTION__);
      m_dllLibArchive.archive_read_free(a);
      return NULL;
    }
    data->filename = new CStdString(filePaths[i]);
    r |= m_dllLibArchive.archive_read_append_callback_data(a, data);
  }

  r |= m_dllLibArchive.archive_read_set_open_callback(a, __xbmc_file_open);
  r |= m_dllLibArchive.archive_read_set_read_callback(a, __xbmc_file_read);
  r |= m_dllLibArchive.archive_read_set_skip_callback(a, __xbmc_file_skip);
  r |= m_dllLibArchive.archive_read_set_close_callback(a, __xbmc_file_close);
  r |= m_dllLibArchive.archive_read_set_switch_callback(a, __xbmc_file_switch);
  r |= m_dllLibArchive.archive_read_set_seek_callback(a, __xbmc_file_seek);

  if (r != (ARCHIVE_OK))
    return NULL;
  return a;
}

bool CArchiveManager::ExtractArchive(CStdString const& strArchive,
                                     CStdString const& strPath,
                                     CStdString const& strPathInArchive,
                                     int format,
                                     int filter)
{
  struct archive *a;
  struct archive *ext;
  struct archive_entry *entry;
  int r;
  const void *buff;
  size_t size;
  off_t offset;

  int flags = ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME;

  if (!Load())
  {
    CLog::Log(LOGERROR, "%s: Unable to load DllLibArchive.", __FUNCTION__);
    return false;
  }
  a = m_dllLibArchive.archive_read_new();
  if (format < 0)
    m_dllLibArchive.archive_read_support_format_all(a);
  else
    m_dllLibArchive.archive_read_set_format(a, format);
  if (filter < 0)
    m_dllLibArchive.archive_read_support_filter_all(a);
  else
    m_dllLibArchive.archive_read_append_filter(a, filter);
  ext = m_dllLibArchive.archive_write_disk_new();
  m_dllLibArchive.archive_write_disk_set_options(ext, flags);
  m_dllLibArchive.archive_write_disk_set_standard_lookup(ext);

  struct xbmc_client_data *data =
    (struct xbmc_client_data *)calloc(1, sizeof(struct xbmc_client_data));
  if (!data)
  {
    CLog::Log(LOGERROR, "%s: Out of memory", __FUNCTION__);
    return false;
  }
  data->filename = new CStdString(strArchive);
  m_dllLibArchive.archive_read_append_callback_data(a, data);
  m_dllLibArchive.archive_read_set_open_callback(a, __xbmc_file_open);
  m_dllLibArchive.archive_read_set_read_callback(a, __xbmc_file_read);
  m_dllLibArchive.archive_read_set_skip_callback(a, __xbmc_file_skip);
  m_dllLibArchive.archive_read_set_close_callback(a, __xbmc_file_close);
  m_dllLibArchive.archive_read_set_switch_callback(a, __xbmc_file_switch);
  m_dllLibArchive.archive_read_set_seek_callback(a, __xbmc_file_seek);

  r = m_dllLibArchive.archive_read_open1(a);
  if (r != ARCHIVE_OK)
  {
    CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(a));
    return false;
  }

  while (1)
  {
    r = m_dllLibArchive.archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;

    /* Extract single file specified by strPathInArchive if defined */
    if (strPathInArchive.length() > 0 &&
      strPathInArchive.compare(m_dllLibArchive.archive_entry_pathname(entry))
        != 0)
        continue;

    /* Extract to path if strPath specified, otherwise extract to current
     * directory.
     */
    if (strPath.length() > 0 && strPath.compare(".") != 0)
    {
      CStdString destpath(strPath);
      URIUtils::AddSlashAtEnd(destpath);
      destpath.append(m_dllLibArchive.archive_entry_pathname(entry));
      m_dllLibArchive.archive_entry_set_pathname(entry, destpath);
    }
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(a));
    if (r < ARCHIVE_WARN)
      return false;
    r = m_dllLibArchive.archive_write_header(ext, entry);
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
    else if (m_dllLibArchive.archive_entry_size(entry) > 0) {
      while (1) {
        r = m_dllLibArchive.archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;
        r = m_dllLibArchive.archive_write_data_block(ext, buff, size, offset);
        if (r != ARCHIVE_OK) {
          CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
          break;
        }
      }
      if (r < ARCHIVE_WARN)
        return false;
    }
    r = m_dllLibArchive.archive_write_finish_entry(ext);
    if (r != ARCHIVE_OK)
      CLog::Log(LOGERROR,"%s", m_dllLibArchive.archive_error_string(ext));
    if (r < ARCHIVE_WARN)
      return false;
    if (strPathInArchive.length() > 0)
      break;
  }

  m_dllLibArchive.archive_read_close(a);
  m_dllLibArchive.archive_read_free(a);
  m_dllLibArchive.archive_write_close(ext);
  m_dllLibArchive.archive_write_free(ext);
  return true;
}

bool CArchiveManager::Load()
{
  CSingleLock lock(m_critSection);
  if (!m_dllLibArchive.IsLoaded())
    return m_dllLibArchive.Load();
  return true;
}

bool CArchiveManager::Unload()
{
  CSingleLock lock(m_critSection);
  if (m_dllLibArchive.IsLoaded())
    m_dllLibArchive.Unload();
  return true;
}

DllLibArchive const& CArchiveManager::getDllLibArchive() const
{
  return m_dllLibArchive;
}

static ssize_t
__xbmc_file_read(struct archive *a, void *client_data, const void **buff)
{
  struct xbmc_client_data *data = (struct xbmc_client_data*)client_data;
  (void)a;
  *buff = data->buffer;
  return data->file->Read(data->buffer, LIBARCHIVE_BLOCK_SIZE);
}

static int64_t
__xbmc_file_skip(struct archive *a, void *client_data, int64_t request)
{
  struct xbmc_client_data *data = (struct xbmc_client_data*)client_data;
  (void)a;
  int64_t result = data->file->Seek(request, SEEK_CUR);
  if (result >= 0)
    return result;

  /* Let libarchive recover with read+discard. */
  if (errno == ESPIPE)
    return 0;
  CLog::Log(LOGERROR, "CArchiveManager - %s: Error skipping in '%s'",
            __FUNCTION__, data->filename->c_str());
  return -1;
}

static int64_t
__xbmc_file_seek(struct archive *a, void *client_data, int64_t offset,
                 int whence)
{
  struct xbmc_client_data *data = (struct xbmc_client_data*)client_data;
  (void)a;
  int64_t result = data->file->Seek(offset, whence);
  if (result >= 0)
    return result;
  return (ARCHIVE_FATAL);
}

static int
__xbmc_file_open(struct archive *a, void *client_data)
{
  struct xbmc_client_data *data = (struct xbmc_client_data*)client_data;
  (void)a;
  if (!data->file)
  {
    data->file = new XFILE::CFile();
    if (data->file && data->file->Open(*data->filename))
    {
      if ((data->buffer = (void*)calloc(1, LIBARCHIVE_BLOCK_SIZE)) == NULL)
        return (ARCHIVE_FAILED);
    }
    else
    {
      CLog::Log(LOGERROR, "CArchiveManager - %s: Error opening file '%s'",
                __FUNCTION__, data->filename->c_str());
      delete data->file;
      data->file = NULL;
      return (ARCHIVE_FAILED);
    }
  }
  return (ARCHIVE_OK);
}

static int
__xbmc_file_close(struct archive *a, void *client_data)
{
  struct xbmc_client_data *data = (struct xbmc_client_data*)client_data;
  (void)a;
  if (data == NULL)
    return (ARCHIVE_FATAL);
  __xbmc_file_switch(a, data, NULL);
  delete data->filename;
  data->filename = NULL;
  free(data);
  return (ARCHIVE_OK);
}

static int
__xbmc_file_switch(struct archive *a, void *client_data1, void *client_data2)
{
  struct xbmc_client_data *data1 = (struct xbmc_client_data*)client_data1;
  struct xbmc_client_data *data2 = (struct xbmc_client_data*)client_data2;
  int r = (ARCHIVE_OK);
  (void)a;

  if (data1)
  {
    if (data1->file)
    {
      data1->file->Close();
      delete data1->file;
      data1->file = NULL;
    }
    free(data1->buffer);
    data1->buffer = NULL;
  }
  if (data2)
  {
    r = __xbmc_file_open(a, data2);
  }
  return (r);
}
#endif // HAVE_LIBARCHIVE
