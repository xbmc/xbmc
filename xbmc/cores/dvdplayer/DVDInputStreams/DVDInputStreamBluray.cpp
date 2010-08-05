/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "system.h"
#ifdef HAVE_LIBBLURAY

#include "DVDInputStreamBluray.h"
#include "Util.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"

#define LIBBLURAY_BYTESEEK 0

extern "C"
{
#include <libbluray/bluray.h>
#include <libbluray/filesystem.h>
}


using namespace std;
using namespace XFILE;

static void file_close(BD_FILE_H *file)
{
  if (file)
  {
    delete static_cast<CFile*>(file->internal);
    delete file;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed file (%p)\n", file);
  }
}

static int64_t file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

static int64_t file_tell(BD_FILE_H *file)
{
  return static_cast<CFile*>(file->internal)->GetPosition();
}

static int file_eof(BD_FILE_H *file)
{
  if(static_cast<CFile*>(file->internal)->GetPosition() == static_cast<CFile*>(file->internal)->GetLength())
    return 1;
  else
    return 0;
}

static int64_t file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<CFile*>(file->internal)->Read(buf, size);
}

static int64_t file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
{
    return -1;
}

static BD_FILE_H *file_open(const char* filename, const char *mode)
{
    BD_FILE_H *file = new BD_FILE_H;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file %s... (%p)", filename, file);
    file->close = file_close;
    file->seek  = file_seek;
    file->read  = file_read;
    file->write = file_write;
    file->tell  = file_tell;
    file->eof   = file_eof;

    CFile* fp = new CFile();
    if(fp->Open(filename))
    {
      file->internal = (void*)fp;
      return file;
    }

    delete fp;
    delete file;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%p)", file);

    return NULL;
}

struct SDirState
{
  SDirState() 
    : curr(0)
  {}

  CFileItemList list;
  int           curr;
};

static void dir_close(BD_DIR_H *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed dir (%p)\n", dir);
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}


static int dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
{
    SDirState* state = static_cast<SDirState*>(dir->internal);

    if(state->curr >= state->list.Size())
      return 1;

    strncpy(entry->d_name, state->list[state->curr]->GetLabel(), sizeof(entry->d_name));
    entry->d_name[sizeof(entry->d_name)-1] = 0;
    state->curr++;

    return 0;
}

BD_DIR_H *dir_open(const char* dirname)
{
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", dirname);


    SDirState *st = new SDirState();

    if(!CDirectory::GetDirectory(dirname, st->list))
    {
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening dir! (%s)\n", dirname);
      delete st;
      return NULL;
    }

    BD_DIR_H *dir = new BD_DIR_H;
    dir->close    = dir_close;
    dir->read     = dir_read;
    dir->internal = (void*)st;

    return dir;
}

static BLURAY_TITLE_INFO* get_longest_title(BLURAY* bd)
{
  int titles = bd_get_titles(bd, TITLES_RELEVANT);

  BLURAY_TITLE_INFO *t, *s = NULL;
  for(int i=0; i < titles; i++)
  {
    t = bd_get_title_info(bd, i);;
    if(!t)
    {
      CLog::Log(LOGDEBUG, "get_main_title - unable to get title %d", i);
      continue;
    }
    if(!s || s->duration < t->duration)
      std::swap(s, t);

    if(t)
      bd_free_title_info(t);
  }
  return s;
}

static BLURAY_TITLE_INFO* get_playlist_title(BLURAY* bd, const CStdString& filename)
{
  int titles = bd_get_titles(bd, TITLES_ALL);
  if(titles < 0)
  {
    CLog::Log(LOGERROR, "get_playlist_title - unable to get list of titles");
    return false;
  }

  int playlist;
  if(sscanf(filename.c_str(), "%05d.mpls", &playlist) != 1)
  {
    CLog::Log(LOGERROR, "get_playlist_title - unsupported playlist file selected %s", filename.c_str());
    return false;
  }

  BLURAY_TITLE_INFO *t;
  for(int i=0; i < titles; i++)
  {
    t = bd_get_title_info(bd, i);;
    if(!t)
    {
      CLog::Log(LOGDEBUG, "get_playlist_title - unable to get title %d", i);        
      continue;
    }
    if(t->playlist == playlist)
      return t;
    bd_free_title_info(t);
  }
  return NULL;
}


CDVDInputStreamBluray::CDVDInputStreamBluray() :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_title = NULL;
  m_bd    = NULL;
}

CDVDInputStreamBluray::~CDVDInputStreamBluray()
{
  Close();
}

bool CDVDInputStreamBluray::IsEOF()
{
  return false;
}

bool CDVDInputStreamBluray::Open(const char* strFile, const std::string& content)
{
  CStdString strPath;
  CUtil::GetDirectory(strFile,strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  if(CUtil::GetFileName(strPath) == "PLAYLIST")
  {
    CUtil::GetDirectory(strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);
  }

  if(CUtil::GetFileName(strPath) == "BDMV")
  {
    CUtil::GetDirectory(strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);
  }

  bd_register_dir(dir_open);
  bd_register_file(file_open);

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - opening %s", strPath.c_str());
  m_bd = bd_open(strPath.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to open %s", strPath.c_str());
    return false;
  }

  CStdString filename = CUtil::GetFileName(strFile);
  if(filename.Equals("index.bdmv"))
    m_title = get_longest_title(m_bd);
  else if(CUtil::GetExtension(filename).Equals(".mpls"))
    m_title = get_playlist_title(m_bd, filename);
  else
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - unsupported bluray file selected %s", strPath.c_str());
    return false;
  }

  if(!m_title)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to get title info");
    return false;
  }

  if(bd_select_title(m_bd, m_title->idx) == 0 )
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to select title %d", m_title->idx);
    return false;
  }

  return true;
}

// close file and reset everyting
void CDVDInputStreamBluray::Close()
{
  if(m_bd)
    bd_close(m_bd);
  m_bd = NULL;
  if(m_title)
    bd_free_title_info(m_title);
  m_title = NULL;
}

int CDVDInputStreamBluray::Read(BYTE* buf, int buf_size)
{
  return bd_read(m_bd, buf, buf_size);
}

int CDVDInputStreamBluray::GetTotalTime()
{
  if(m_title)
    return m_title->duration / 90;
  else
    return 0;
}

int CDVDInputStreamBluray::GetTime()
{
  return bd_tell_time(m_bd) / 90;
}

bool CDVDInputStreamBluray::SeekTime(int ms)
{
  if(bd_seek_time(m_bd, ms * 90) < 0)
    return false;
  else
    return true;
}

int CDVDInputStreamBluray::GetChapterCount()
{
  if(m_title)
    return m_title->chapter_count;
  else
    return 0;
}

int CDVDInputStreamBluray::GetChapter()
{
  if(m_title)
    return bd_get_current_chapter(m_bd) + 1;
  else
    return 0;
}

bool CDVDInputStreamBluray::SeekChapter(int ch)
{
  if(m_title && bd_seek_chapter(m_bd, ch-1) < 0)
    return false;
  else
    return true;
}

__int64 CDVDInputStreamBluray::Seek(__int64 offset, int whence)
{
#if LIBBLURAY_BYTESEEK
  if(whence == SEEK_POSSIBLE)
    return 1;
  else if(whence == SEEK_CUR)
  {
    if(offset == 0)
      return bd_tell(m_bd);
    else
      offset += bd_tell(m_bd);
  }
  else if(whence == SEEK_END)
    offset += bd_get_title_size(m_bd);
  else if(whence != SEEK_SET)
    return -1;

  int64_t pos = bd_seek(m_bd, offset);
  if(pos < 0)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Seek - seek to %"PRId64", failed with %"PRId64, offset, pos); 
    return -1;
  }

  if(pos != offset)
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray::Seek - seek to %"PRId64", ended at %"PRId64, offset, pos); 

  return offset;
#else
  if(whence == SEEK_POSSIBLE)
    return 0;
  return -1;
#endif
}

__int64 CDVDInputStreamBluray::GetLength()
{
  return bd_get_title_size(m_bd);
}

static bool find_stream(int pid, BLURAY_STREAM_INFO *info, int count, char* language)
{
  int i=0;
  for(;i<count;i++,info++)
  {
    if(info->pid == pid)
      break;
  }
  if(i==count)
    return false;
  memcpy(language, info->lang, 4);
  return true;
}

void CDVDInputStreamBluray::GetStreamInfo(int pid, char* language)
{
  if(m_title->clip_count == 0)
    return;

  BLURAY_CLIP_INFO *clip = m_title->clips;

  if(find_stream(pid, clip->audio_streams, clip->audio_stream_count, language))
    return;
  if(find_stream(pid, clip->video_streams, clip->video_stream_count, language))
    return;
  if(find_stream(pid, clip->pg_streams, clip->pg_stream_count, language))
    return;
  if(find_stream(pid, clip->ig_streams, clip->ig_stream_count, language))
    return;
}

#endif
