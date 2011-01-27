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
#include "DynamicDll.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

#define LIBBLURAY_BYTESEEK 0

extern "C"
{
#include <libbluray/bluray.h>
#include <libbluray/filesystem.h>
}

class DllLibblurayInterface
{
public:
  virtual ~DllLibblurayInterface() {};
  virtual uint32_t bd_get_titles(BLURAY *bd, uint8_t flags)=0;
  virtual BLURAY_TITLE_INFO* bd_get_title_info(BLURAY *bd, uint32_t title_idx)=0;
  virtual BLURAY_TITLE_INFO* bd_get_playlist_info(BLURAY *bd, uint32_t playlist)=0;
  virtual void bd_free_title_info(BLURAY_TITLE_INFO *title_info)=0;
  virtual BLURAY *bd_open(const char* device_path, const char* keyfile_path)=0;
  virtual void bd_close(BLURAY *bd)=0;
  virtual int64_t bd_seek(BLURAY *bd, uint64_t pos)=0;
  virtual int64_t bd_seek_time(BLURAY *bd, uint64_t tick)=0;
  virtual int bd_read(BLURAY *bd, unsigned char *buf, int len)=0;
  virtual int64_t bd_seek_chapter(BLURAY *bd, unsigned chapter)=0;
  virtual int64_t bd_chapter_pos(BLURAY *bd, unsigned chapter)=0;
  virtual uint32_t bd_get_current_chapter(BLURAY *bd)=0;
  virtual int64_t bd_seek_mark(BLURAY *bd, unsigned mark)=0;
  virtual int bd_select_playlist(BLURAY *bd, uint32_t playlist)=0;
  virtual int bd_select_title(BLURAY *bd, uint32_t title)=0;
  virtual int bd_select_angle(BLURAY *bd, unsigned angle)=0;
  virtual void bd_seamless_angle_change(BLURAY *bd, unsigned angle)=0;
  virtual uint64_t bd_get_title_size(BLURAY *bd)=0;
  virtual uint32_t bd_get_current_title(BLURAY *bd)=0;
  virtual unsigned bd_get_current_angle(BLURAY *bd)=0;
  virtual uint64_t bd_tell(BLURAY *bd)=0;
  virtual uint64_t bd_tell_time(BLURAY *bd)=0;
  virtual BD_FILE_OPEN bd_register_file(BD_FILE_OPEN p)=0;
  virtual BD_DIR_OPEN bd_register_dir(BD_DIR_OPEN p)=0;
};

class DllLibbluray : public DllDynamic, DllLibblurayInterface
{
  DECLARE_DLL_WRAPPER(DllLibbluray, DLL_PATH_LIBBLURAY)

  DEFINE_METHOD2(uint32_t,            bd_get_titles,          (BLURAY *p1, uint8_t p2))
  DEFINE_METHOD2(BLURAY_TITLE_INFO*,  bd_get_title_info,      (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(BLURAY_TITLE_INFO*,  bd_get_playlist_info,   (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD1(void,                bd_free_title_info,     (BLURAY_TITLE_INFO *p1))
  DEFINE_METHOD2(BLURAY*,             bd_open,                (const char* p1, const char* p2))
  DEFINE_METHOD1(void,                bd_close,               (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek,                (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD2(int64_t,             bd_seek_time,           (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD3(int,                 bd_read,                (BLURAY *p1, unsigned char *p2, int p3))
  DEFINE_METHOD2(int64_t,             bd_seek_chapter,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int64_t,             bd_chapter_pos,         (BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint32_t,            bd_get_current_chapter, (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek_mark,           (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int,                 bd_select_playlist,     (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_title,        (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_angle,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(void,                bd_seamless_angle_change,(BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint64_t,            bd_get_title_size,      (BLURAY *p1))
  DEFINE_METHOD1(uint32_t,            bd_get_current_title,   (BLURAY *p1))
  DEFINE_METHOD1(unsigned,            bd_get_current_angle,   (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell,                (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell_time,           (BLURAY *p1))
  DEFINE_METHOD1(BD_FILE_OPEN,        bd_register_file,       (BD_FILE_OPEN p1))
  DEFINE_METHOD1(BD_DIR_OPEN,         bd_register_dir,        (BD_DIR_OPEN p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(bd_get_titles,        bd_get_titles)
    RESOLVE_METHOD_RENAME(bd_get_title_info,    bd_get_title_info)
    RESOLVE_METHOD_RENAME(bd_get_playlist_info, bd_get_playlist_info)
    RESOLVE_METHOD_RENAME(bd_free_title_info,   bd_free_title_info)
    RESOLVE_METHOD_RENAME(bd_open,              bd_open)
    RESOLVE_METHOD_RENAME(bd_close,             bd_close)
    RESOLVE_METHOD_RENAME(bd_seek,              bd_seek)
    RESOLVE_METHOD_RENAME(bd_seek_time,         bd_seek_time)
    RESOLVE_METHOD_RENAME(bd_read,              bd_read)
    RESOLVE_METHOD_RENAME(bd_seek_chapter,      bd_seek_chapter)
    RESOLVE_METHOD_RENAME(bd_chapter_pos,       bd_chapter_pos)
    RESOLVE_METHOD_RENAME(bd_get_current_chapter, bd_get_current_chapter)
    RESOLVE_METHOD_RENAME(bd_seek_mark,         bd_seek_mark)
    RESOLVE_METHOD_RENAME(bd_select_playlist,   bd_select_playlist)
    RESOLVE_METHOD_RENAME(bd_select_title,      bd_select_title)
    RESOLVE_METHOD_RENAME(bd_select_angle,      bd_select_angle)
    RESOLVE_METHOD_RENAME(bd_seamless_angle_change, bd_seamless_angle_change)
    RESOLVE_METHOD_RENAME(bd_get_title_size,    bd_get_title_size)
    RESOLVE_METHOD_RENAME(bd_get_current_title, bd_get_current_title)
    RESOLVE_METHOD_RENAME(bd_get_current_angle, bd_get_current_angle)
    RESOLVE_METHOD_RENAME(bd_tell,              bd_tell)
    RESOLVE_METHOD_RENAME(bd_tell_time,         bd_tell_time)
    RESOLVE_METHOD_RENAME(bd_register_file,     bd_register_file)
    RESOLVE_METHOD_RENAME(bd_register_dir,      bd_register_dir)
  END_METHOD_RESOLVE()
};


using namespace std;
using namespace XFILE;

static bool is_udf_iso_path(const char* filename)
{
  bool bResult = false;

  const char* ptr = strcasestr(filename, ".iso");
  if(ptr)
  {
    ptr += strlen(".iso");
    if(*ptr == '/' && strlen(++ptr) > 0)
    {
      bResult = true;
    }
  }
  return bResult;
}

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

    CStdString strFilename(filename);

    if(is_udf_iso_path(filename))
    {
      CURL::Encode(strFilename);
      strFilename.Format("udf://%s", strFilename);
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file udf iso file %s... (%p)", strFilename.c_str(), file);
    }
    else
    {
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file %s... (%p)", strFilename.c_str(), file);
    }

    file->close = file_close;
    file->seek  = file_seek;
    file->read  = file_read;
    file->write = file_write;
    file->tell  = file_tell;
    file->eof   = file_eof;

    CFile* fp = new CFile();
    if(fp->Open(strFilename,READ_CHUNKED))
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

static BD_DIR_H *dir_open(const char* dirname)
{
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", dirname);
    SDirState *st = new SDirState();

    CStdString strDirname(dirname);
    if(is_udf_iso_path(dirname))
    {
      CURL::Encode(strDirname);
      strDirname.Format("udf://%s", strDirname);
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening udf dir %s...", strDirname.c_str());
    }

    if(!CDirectory::GetDirectory(strDirname, st->list))
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

CDVDInputStreamBluray::CDVDInputStreamBluray() :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_title = NULL;
  m_bd    = NULL;
  m_dll = new DllLibbluray;
  m_dll->Load();
}

CDVDInputStreamBluray::~CDVDInputStreamBluray()
{
  Close();
  delete m_dll;
}

bool CDVDInputStreamBluray::IsEOF()
{
  return false;
}

bool CDVDInputStreamBluray::Open(const char* strFile, const std::string& content)
{
  CStdString strPath;
  URIUtils::GetDirectory(strFile,strPath);
  URIUtils::RemoveSlashAtEnd(strPath);

  if(URIUtils::GetFileName(strPath) == "PLAYLIST")
  {
    URIUtils::GetDirectory(strPath,strPath);
    URIUtils::RemoveSlashAtEnd(strPath);
  }

  if(URIUtils::GetFileName(strPath) == "BDMV")
  {
    URIUtils::GetDirectory(strPath,strPath);
    URIUtils::RemoveSlashAtEnd(strPath);
  }

  m_dll->bd_register_dir(dir_open);
  m_dll->bd_register_file(file_open);

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - opening %s", strPath.c_str());
  m_bd = m_dll->bd_open(strPath.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to open %s", strPath.c_str());
    return false;
  }

  CStdString filename = URIUtils::GetFileName(strFile);
  if(filename.Equals("index.bdmv"))
  {
    int titles = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT);

    BLURAY_TITLE_INFO *t, *s = NULL;
    for(int i=0; i < titles; i++)
    {
      t = m_dll->bd_get_title_info(m_bd, i);;
      if(!t)
      {
        CLog::Log(LOGDEBUG, "get_main_title - unable to get title %d", i);
        continue;
      }
      if(!s || s->duration < t->duration)
        std::swap(s, t);

      if(t)
        m_dll->bd_free_title_info(t);
    }
    m_title = s;
  }
  else if(URIUtils::GetExtension(filename).Equals(".mpls"))
  {
    int titles = m_dll->bd_get_titles(m_bd, TITLES_ALL);
    do
    {
      if(titles < 0)
      {
        CLog::Log(LOGERROR, "get_playlist_title - unable to get list of titles");
        m_title = NULL;
        break;
      }

      unsigned int playlist;
      if(sscanf(filename.c_str(), "%05d.mpls", &playlist) != 1)
      {
        CLog::Log(LOGERROR, "get_playlist_title - unsupported playlist file selected %s", filename.c_str());
        m_title = NULL;
        break;
      }

      BLURAY_TITLE_INFO *t;
      for(int i=0; i < titles; i++)
      {
        t = m_dll->bd_get_title_info(m_bd, i);;
        if(!t)
        {
          CLog::Log(LOGDEBUG, "get_playlist_title - unable to get title %d", i);
          continue;
        }
        if(t->playlist == playlist)
        {
          m_title = t;
          break;
        }
        m_dll->bd_free_title_info(t);
      }

      m_title = NULL;
      break;

    } while(false);
  }
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

  if(m_dll->bd_select_title(m_bd, m_title->idx) == 0 )
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
    m_dll->bd_close(m_bd);
  m_bd = NULL;
  if(m_title)
    m_dll->bd_free_title_info(m_title);
  m_title = NULL;
}

int CDVDInputStreamBluray::Read(BYTE* buf, int buf_size)
{
  return m_dll->bd_read(m_bd, buf, buf_size);
}

int CDVDInputStreamBluray::GetTotalTime()
{
  if(m_title)
    return (int)(m_title->duration / 90);
  else
    return 0;
}

int CDVDInputStreamBluray::GetTime()
{
  return (int)(m_dll->bd_tell_time(m_bd) / 90);
}

bool CDVDInputStreamBluray::SeekTime(int ms)
{
  if(m_dll->bd_seek_time(m_bd, ms * 90) < 0)
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
    return m_dll->bd_get_current_chapter(m_bd) + 1;
  else
    return 0;
}

bool CDVDInputStreamBluray::SeekChapter(int ch)
{
  if(m_title && m_dll->bd_seek_chapter(m_bd, ch-1) < 0)
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
      return m_dll->bd_tell(m_bd);
    else
      offset += bd_tell(m_bd);
  }
  else if(whence == SEEK_END)
    offset += m_dll->bd_get_title_size(m_bd);
  else if(whence != SEEK_SET)
    return -1;

  int64_t pos = m_dll->bd_seek(m_bd, offset);
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
  return m_dll->bd_get_title_size(m_bd);
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
