/*
<<<<<<< HEAD
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
=======
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
>>>>>>> FETCH_HEAD
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
<<<<<<< HEAD
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
=======
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
>>>>>>> FETCH_HEAD
 *
 */
#include "system.h"
#ifdef HAVE_LIBBLURAY

#include "DVDInputStreamBluray.h"
<<<<<<< HEAD
#include "IDVDPlayer.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlayImage.h"
#include "settings/Settings.h"
#include "LangInfo.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "DllLibbluray.h"
#include "URL.h"
#include "guilib/Geometry.h"
#include "utils/StringUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "settings/DiscSettings.h"

#define LIBBLURAY_BYTESEEK 0

using namespace std;
using namespace XFILE;

void DllLibbluray::file_close(BD_FILE_H *file)
=======
#include "DynamicDll.h"
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

static void file_close(BD_FILE_H *file)
>>>>>>> FETCH_HEAD
{
  if (file)
  {
    delete static_cast<CFile*>(file->internal);
    delete file;
<<<<<<< HEAD
  }
}

int64_t DllLibbluray::file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
=======

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed file (%p)\n", file);
  }
}

static int64_t file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
>>>>>>> FETCH_HEAD
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

<<<<<<< HEAD
int64_t DllLibbluray::file_tell(BD_FILE_H *file)
=======
static int64_t file_tell(BD_FILE_H *file)
>>>>>>> FETCH_HEAD
{
  return static_cast<CFile*>(file->internal)->GetPosition();
}

<<<<<<< HEAD
int DllLibbluray::file_eof(BD_FILE_H *file)
=======
static int file_eof(BD_FILE_H *file)
>>>>>>> FETCH_HEAD
{
  if(static_cast<CFile*>(file->internal)->GetPosition() == static_cast<CFile*>(file->internal)->GetLength())
    return 1;
  else
    return 0;
}

<<<<<<< HEAD
int64_t DllLibbluray::file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<CFile*>(file->internal)->Read(buf, size); // TODO: fix size cast
}

int64_t DllLibbluray::file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
=======
static int64_t file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<CFile*>(file->internal)->Read(buf, size);
}

static int64_t file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
>>>>>>> FETCH_HEAD
{
    return -1;
}

<<<<<<< HEAD
BD_FILE_H * DllLibbluray::file_open(const char* filename, const char *mode)
{
    BD_FILE_H *file = new BD_FILE_H;

=======
static BD_FILE_H *file_open(const char* filename, const char *mode)
{
    BD_FILE_H *file = new BD_FILE_H;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening file %s... (%p)", filename, file);
>>>>>>> FETCH_HEAD
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

<<<<<<< HEAD
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%p)", file);
    
    delete fp;
    delete file;

=======
    delete fp;
    delete file;

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%p)", file);

>>>>>>> FETCH_HEAD
    return NULL;
}

struct SDirState
{
<<<<<<< HEAD
  SDirState()
=======
  SDirState() 
>>>>>>> FETCH_HEAD
    : curr(0)
  {}

  CFileItemList list;
  int           curr;
};

<<<<<<< HEAD
void DllLibbluray::dir_close(BD_DIR_H *dir)
=======
static void dir_close(BD_DIR_H *dir)
>>>>>>> FETCH_HEAD
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed dir (%p)\n", dir);
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}


<<<<<<< HEAD
int DllLibbluray::dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
=======
static int dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
>>>>>>> FETCH_HEAD
{
    SDirState* state = static_cast<SDirState*>(dir->internal);

    if(state->curr >= state->list.Size())
      return 1;

<<<<<<< HEAD
    strncpy(entry->d_name, state->list[state->curr]->GetLabel().c_str(), sizeof(entry->d_name));
=======
    strncpy(entry->d_name, state->list[state->curr]->GetLabel(), sizeof(entry->d_name));
>>>>>>> FETCH_HEAD
    entry->d_name[sizeof(entry->d_name)-1] = 0;
    state->curr++;

    return 0;
}

<<<<<<< HEAD
BD_DIR_H *DllLibbluray::dir_open(const char* dirname)
{
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", dirname);
    SDirState *st = new SDirState();

    std::string strDirname(dirname);

    if(!CDirectory::GetDirectory(strDirname, st->list))
=======
static BD_DIR_H *dir_open(const char* dirname)
{
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", dirname);


    SDirState *st = new SDirState();

    if(!CDirectory::GetDirectory(dirname, st->list))
>>>>>>> FETCH_HEAD
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

<<<<<<< HEAD
void DllLibbluray::bluray_logger(const char* msg)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Logger - %s", msg);
}


static void bluray_overlay_cb(void *this_gen, const BD_OVERLAY * ov)
{
  static_cast<CDVDInputStreamBluray*>(this_gen)->OverlayCallback(ov);
}

#ifdef HAVE_LIBBLURAY_BDJ
void  bluray_overlay_argb_cb(void *this_gen, const struct bd_argb_overlay_s * const ov)
{
  static_cast<CDVDInputStreamBluray*>(this_gen)->OverlayCallbackARGB(ov);
}
#endif

CDVDInputStreamBluray::CDVDInputStreamBluray(IDVDPlayer* player) :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_title = NULL;
  m_clip  = (uint32_t)-1;
  m_playlist = (uint32_t)-1;
  m_menu  = false;
  m_bd    = NULL;
  m_dll = new DllLibbluray;
  if (!m_dll->Load())
  {
    delete m_dll;
    m_dll = NULL;
  }
  m_content = "video/x-mpegts";
  m_player  = player;
  m_navmode = false;
  m_hold = HOLD_NONE;
  memset(&m_event, 0, sizeof(m_event));
=======
CDVDInputStreamBluray::CDVDInputStreamBluray() :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_title = NULL;
  m_bd    = NULL;
  m_dll = new DllLibbluray;
  m_dll->Load();
>>>>>>> FETCH_HEAD
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

<<<<<<< HEAD
BLURAY_TITLE_INFO* CDVDInputStreamBluray::GetTitleLongest()
{
  int titles = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT, 0);

  BLURAY_TITLE_INFO *s = NULL;
  for(int i=0; i < titles; i++)
  {
    BLURAY_TITLE_INFO *t = m_dll->bd_get_title_info(m_bd, i, 0);
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
  return s;
}

BLURAY_TITLE_INFO* CDVDInputStreamBluray::GetTitleFile(const std::string& filename)
{
  unsigned int playlist;
  if(sscanf(filename.c_str(), "%05d.mpls", &playlist) != 1)
  {
    CLog::Log(LOGERROR, "get_playlist_title - unsupported playlist file selected %s", filename.c_str());
    return NULL;
  }

  return m_dll->bd_get_playlist_info(m_bd, playlist, 0);
}


bool CDVDInputStreamBluray::Open(const char* strFile, const std::string& content)
{
  if(m_player == NULL)
    return false;

  std::string strPath(strFile);
  std::string filename;
  std::string root;

  if(URIUtils::IsProtocol(strPath, "bluray"))
  {
    CURL url(strPath);
    root     = url.GetHostName();
    filename = URIUtils::GetFileName(url.GetFileName());
  }
  else if(URIUtils::HasExtension(strPath, ".iso|.img"))
  {
    CURL url("udf://");
    url.SetHostName(strPath);
    root     = url.Get();
    filename = "index.bdmv";
  }
  else
  {
    strPath = URIUtils::GetDirectory(strPath);
    URIUtils::RemoveSlashAtEnd(strPath);

    if(URIUtils::GetFileName(strPath) == "PLAYLIST")
    {
      strPath = URIUtils::GetDirectory(strPath);
      URIUtils::RemoveSlashAtEnd(strPath);
    }

    if(URIUtils::GetFileName(strPath) == "BDMV")
    {
      strPath = URIUtils::GetDirectory(strPath);
      URIUtils::RemoveSlashAtEnd(strPath);
    }
    root     = strPath;
    filename = URIUtils::GetFileName(strFile);
  }

  // root should not have trailing slash
  URIUtils::RemoveSlashAtEnd(root);

  if (!m_dll)
    return false;

  m_dll->bd_register_dir(DllLibbluray::dir_open);
  m_dll->bd_register_file(DllLibbluray::file_open);
  m_dll->bd_set_debug_handler(DllLibbluray::bluray_logger);
  m_dll->bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - opening %s", root.c_str());
  m_bd = m_dll->bd_open(root.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to open %s", root.c_str());
    return false;
  }

  const BLURAY_DISC_INFO *disc_info;

  disc_info = m_dll->bd_get_disc_info(m_bd);

  if (!disc_info)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - bd_get_disc_info() failed");
    return false;
  }

  if (disc_info->bluray_detected)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - First Play supported: %d", disc_info->first_play_supported);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - Top menu supported  : %d", disc_info->top_menu_supported);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - HDMV titles         : %d", disc_info->num_hdmv_titles);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - BD-J titles         : %d", disc_info->num_bdj_titles);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - UNSUPPORTED titles  : %d", disc_info->num_unsupported_titles);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - AACS detected       : %d", disc_info->aacs_detected);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - libaacs detected    : %d", disc_info->libaacs_detected);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - AACS handled        : %d", disc_info->aacs_handled);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - BD+ detected        : %d", disc_info->bdplus_detected);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - libbdplus detected  : %d", disc_info->libbdplus_detected);
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - BD+ handled         : %d", disc_info->bdplus_handled);
  }
  else
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - BluRay not detected");

  if (disc_info->aacs_detected && !disc_info->aacs_handled)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - Media stream scrambled/encrypted with AACS");
    return false;
  }

  if (disc_info->bdplus_detected && !disc_info->bdplus_handled)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - Media stream scrambled/encrypted with BD+");
    return false;
  }

  int mode = CSettings::Get().GetInt("disc.playback");

  if (URIUtils::HasExtension(filename, ".mpls"))
  {
    m_navmode = false;
    m_title = GetTitleFile(filename);
  }
  else if (mode == BD_PLAYBACK_MAIN_TITLE)
  {
    m_navmode = false;
    m_title = GetTitleLongest();
  }
  else
  {
    m_navmode = true;
    if (m_navmode && !disc_info->first_play_supported) {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - Can't play disc in HDMV navigation mode - First Play title not supported");
      m_navmode = false;
    }

    if (m_navmode && disc_info->num_unsupported_titles > 0) {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - Unsupported titles found - Some titles can't be played in navigation mode");
    }

    if(!m_navmode)
      m_title = GetTitleLongest();
  }

  if(m_navmode)
  {
    int region = CSettings::Get().GetInt("dvds.playerregion");
    if(region == 0)
    {
      CLog::Log(LOGWARNING, "CDVDInputStreamBluray::Open - region dvd must be set in setting, assuming region 1");
      region = 1;
    }
    m_dll->bd_set_player_setting    (m_bd, BLURAY_PLAYER_SETTING_REGION_CODE,  region);
    m_dll->bd_set_player_setting    (m_bd, BLURAY_PLAYER_SETTING_PARENTAL,     0);
    m_dll->bd_set_player_setting    (m_bd, BLURAY_PLAYER_SETTING_PLAYER_PROFILE, 0);
    m_dll->bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_AUDIO_LANG,   g_langInfo.GetDVDAudioLanguage().c_str());
    m_dll->bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_PG_LANG,      g_langInfo.GetDVDSubtitleLanguage().c_str());
    m_dll->bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG,    g_langInfo.GetDVDMenuLanguage().c_str());
    m_dll->bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_COUNTRY_CODE, "us");
    m_dll->bd_register_overlay_proc (m_bd, this, bluray_overlay_cb);
#ifdef HAVE_LIBBLURAY_BDJ
    m_dll->bd_register_argb_overlay_proc (m_bd, this, bluray_overlay_argb_cb, NULL);
#endif

    m_dll->bd_get_event(m_bd, NULL);


    if(m_dll->bd_play(m_bd) <= 0)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed play disk %s", strPath.c_str());
      return false;
    }
    m_hold = HOLD_DATA;
  }
  else
  {
    if(!m_title)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to get title info");
      return false;
    }

    if(m_dll->bd_select_playlist(m_bd, m_title->playlist) == 0 )
    {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to select title %d", m_title->idx);
      return false;
    }
    m_clip = 0;
=======
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

  m_dll->bd_register_dir(dir_open);
  m_dll->bd_register_file(file_open);

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Open - opening %s", strPath.c_str());
  m_bd = m_dll->bd_open(strPath.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed to open %s", strPath.c_str());
    return false;
  }

  CStdString filename = CUtil::GetFileName(strFile);
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
  else if(CUtil::GetExtension(filename).Equals(".mpls"))
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
>>>>>>> FETCH_HEAD
  }

  return true;
}

// close file and reset everyting
void CDVDInputStreamBluray::Close()
{
<<<<<<< HEAD
  if (!m_dll)
    return;
  if(m_title)
    m_dll->bd_free_title_info(m_title);
  if(m_bd)
  {
    m_dll->bd_register_overlay_proc(m_bd, NULL, NULL);
    m_dll->bd_close(m_bd);
  }
  m_bd = NULL;
  m_title = NULL;
}

void CDVDInputStreamBluray::ProcessEvent() {

  int pid = -1;
  switch (m_event.event) {

  case BD_EVENT_ERROR:
    CLog::Log(LOGERROR, "CDVDInputStreamBluray - BD_EVENT_ERROR");
    break;

  case BD_EVENT_ENCRYPTED:
    CLog::Log(LOGERROR, "CDVDInputStreamBluray - BD_EVENT_ENCRYPTED");
    break;

  /* playback control */

  case BD_EVENT_SEEK:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_SEEK");
    //m_player->OnDVDNavResult(NULL, 1);
    //m_dll->bd_read_skip_still(m_bd);
    //m_hold = HOLD_HELD;
    break;

  case BD_EVENT_STILL_TIME:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_STILL_TIME %d", m_event.param);
    pid = m_event.param;
    m_player->OnDVDNavResult((void*) &pid, 5);
    m_hold = HOLD_STILL;
    break;

  case BD_EVENT_STILL:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_STILL %d",
        m_event.param);
    break;

    /* playback position */

  case BD_EVENT_ANGLE:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_ANGLE %d",
        m_event.param);
    m_angle = m_event.param;

    if (m_playlist <= MAX_PLAYLIST_ID)
    {
      if(m_title)
        m_dll->bd_free_title_info(m_title);
      m_title = m_dll->bd_get_playlist_info(m_bd, m_playlist, m_angle);
    }
    break;

  case BD_EVENT_END_OF_TITLE:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_END_OF_TITLE %d",
        m_event.param);
    /* when a title ends, playlist WILL eventually change */
    if (m_title)
      m_dll->bd_free_title_info(m_title);
    m_title = NULL;
    break;

  case BD_EVENT_TITLE:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_TITLE %d",
        m_event.param);
    break;

  case BD_EVENT_PLAYLIST:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PLAYLIST %d",
        m_event.param);
    m_playlist = m_event.param;
    if(m_title)
      m_dll->bd_free_title_info(m_title);
    m_title = m_dll->bd_get_playlist_info(m_bd, m_playlist, m_angle);
    break;

  case BD_EVENT_PLAYITEM:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PLAYITEM %d",
        m_event.param);
    m_clip    = m_event.param;
    break;

  case BD_EVENT_CHAPTER:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_CHAPTER %d",
        m_event.param);
    break;

    /* stream selection */

  case BD_EVENT_AUDIO_STREAM:
    pid = -1;
    if (m_title && m_title->clip_count > m_clip
        && m_title->clips[m_clip].audio_stream_count
            > (uint8_t) (m_event.param - 1))
      pid = m_title->clips[m_clip].audio_streams[m_event.param - 1].pid;
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_AUDIO_STREAM %d %d",
        m_event.param, pid);
    m_player->OnDVDNavResult((void*) &pid, 2);
    break;

  case BD_EVENT_PG_TEXTST:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PG_TEXTST %d",
        m_event.param);
    pid = m_event.param;
    m_player->OnDVDNavResult((void*) &pid, 4);
    break;

  case BD_EVENT_PG_TEXTST_STREAM:
    pid = -1;
    if (m_title && m_title->clip_count > m_clip
        && m_title->clips[m_clip].pg_stream_count
            > (uint8_t) (m_event.param - 1))
      pid = m_title->clips[m_clip].pg_streams[m_event.param - 1].pid;
    CLog::Log(LOGDEBUG,
        "CDVDInputStreamBluray - BD_EVENT_PG_TEXTST_STREAM %d, %d",
        m_event.param, pid);
    m_player->OnDVDNavResult((void*) &pid, 3);
    break;

#if (BLURAY_VERSION >= BLURAY_VERSION_CODE(0,2,2))
  case BD_EVENT_MENU:
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_MENU %d",
        m_event.param);
    m_menu = !!m_event.param;
    break;
#endif
#if (BLURAY_VERSION >= BLURAY_VERSION_CODE(0,3,0))
  case BD_EVENT_IDLE:
#ifdef HAVE_LIBBLURAY_BDJ
    Sleep(100);
#else
    m_hold = HOLD_ERROR;
    m_player->OnDVDNavResult(NULL, 6);
#endif
    break;
#endif

  case BD_EVENT_IG_STREAM:
  case BD_EVENT_SECONDARY_AUDIO:
  case BD_EVENT_SECONDARY_AUDIO_STREAM:
  case BD_EVENT_SECONDARY_VIDEO:
  case BD_EVENT_SECONDARY_VIDEO_SIZE:
  case BD_EVENT_SECONDARY_VIDEO_STREAM:

  case BD_EVENT_NONE:
    break;

  default:
    CLog::Log(LOGWARNING,
        "CDVDInputStreamBluray - unhandled libbluray event %d [param %d]",
        m_event.event, m_event.param);
    break;
  }

  /* event has been consumed */
  m_event.event = BD_EVENT_NONE;
}

int CDVDInputStreamBluray::Read(uint8_t* buf, int buf_size)
{
  if(m_navmode)
  {
    int result = 0;
    do {

      if(m_hold == HOLD_HELD)
        return 0;

      if(m_hold == HOLD_ERROR)
        return -1;

      result = m_dll->bd_read_ext (m_bd, buf, buf_size, &m_event);

      if(result < 0)
      {
        m_hold = HOLD_ERROR;
        return result;
      }

      /* Check for holding events */
      switch(m_event.event) {
        case BD_EVENT_SEEK:
        case BD_EVENT_TITLE:
        case BD_EVENT_ANGLE:
        case BD_EVENT_PLAYLIST:
        case BD_EVENT_PLAYITEM:
          if(m_hold != HOLD_DATA)
          {
            m_hold = HOLD_HELD;
            return result;
          }
          break;

        case BD_EVENT_STILL_TIME:
          if(m_hold == HOLD_STILL)
            m_event.event = 0; /* Consume duplicate still event */
          else
            m_hold = HOLD_HELD;
          return result;

        default:
          break;
      }

      if(result > 0)
        m_hold = HOLD_NONE;

      ProcessEvent();

    } while(result == 0);

    return result;
  }
  else
    return m_dll->bd_read(m_bd, buf, buf_size);
}

static uint8_t  clamp(double v)
{
  return (v) > 255.0 ? 255 : ((v) < 0.0 ? 0 : (uint8_t)(v+0.5f));
}

static uint32_t build_rgba(const BD_PG_PALETTE_ENTRY &e)
{
  double r = 1.164 * (e.Y - 16)                        + 1.596 * (e.Cr - 128);
  double g = 1.164 * (e.Y - 16) - 0.391 * (e.Cb - 128) - 0.813 * (e.Cr - 128);
  double b = 1.164 * (e.Y - 16) + 2.018 * (e.Cb - 128);
  return (uint32_t)e.T      << PIXEL_ASHIFT
       | (uint32_t)clamp(r) << PIXEL_RSHIFT
       | (uint32_t)clamp(g) << PIXEL_GSHIFT
       | (uint32_t)clamp(b) << PIXEL_BSHIFT;
}

void CDVDInputStreamBluray::OverlayClose()
{
#if(BD_OVERLAY_INTERFACE_VERSION >= 2)
  for(unsigned i = 0; i < 2; ++i)
    m_planes[i].o.clear();
  CDVDOverlayGroup* group   = new CDVDOverlayGroup();
  group->bForced = true;
  m_player->OnDVDNavResult(group, 0);
  group->Release();
#endif
}

void CDVDInputStreamBluray::OverlayInit(SPlane& plane, int w, int h)
{
#if(BD_OVERLAY_INTERFACE_VERSION >= 2)
  plane.o.clear();
  plane.w = w;
  plane.h = h;
#endif
}

void CDVDInputStreamBluray::OverlayClear(SPlane& plane, int x, int y, int w, int h)
{
#if(BD_OVERLAY_INTERFACE_VERSION >= 2)
  CRectInt ovr(x
          , y
          , x + w
          , y + h);

  /* fixup existing overlays */
  for(SOverlays::iterator it = plane.o.begin(); it != plane.o.end();)
  {
    CRectInt old((*it)->x
            , (*it)->y
            , (*it)->x + (*it)->width
            , (*it)->y + (*it)->height);

    vector<CRectInt> rem = old.SubtractRect(ovr);

    /* if no overlap we are done */
    if(rem.size() == 1 && !(rem[0] != old))
    {
      ++it;
      continue;
    }

    SOverlays add;
    for(vector<CRectInt>::iterator itr = rem.begin(); itr != rem.end(); ++itr)
    {
      SOverlay overlay(new CDVDOverlayImage(*(*it)
                                            , itr->x1
                                            , itr->y1
                                            , itr->Width()
                                            , itr->Height())
                    , std::ptr_fun(CDVDOverlay::Release));
      add.push_back(overlay);
    }

    it = plane.o.erase(it);
    plane.o.insert(it, add.begin(), add.end());
  }
#endif
}

void CDVDInputStreamBluray::OverlayFlush(int64_t pts)
{
#if(BD_OVERLAY_INTERFACE_VERSION >= 2)
  CDVDOverlayGroup* group   = new CDVDOverlayGroup();
  group->bForced       = true;
  group->iPTSStartTime = (double) pts;
  group->iPTSStopTime  = 0;

  for(unsigned i = 0; i < 2; ++i)
  {
    for(SOverlays::iterator it = m_planes[i].o.begin(); it != m_planes[i].o.end(); ++it)
      group->m_overlays.push_back((*it)->Acquire());
  }

  m_player->OnDVDNavResult(group, 0);
  group->Release();
#endif
}

void CDVDInputStreamBluray::OverlayCallback(const BD_OVERLAY * const ov)
{
#if(BD_OVERLAY_INTERFACE_VERSION >= 2)
  if(ov == NULL || ov->cmd == BD_OVERLAY_CLOSE)
  {
    OverlayClose();
    return;
  }

  if (ov->plane > 1)
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray - Ignoring overlay with multiple planes");
    return;
  }

  SPlane& plane(m_planes[ov->plane]);

  if (ov->cmd == BD_OVERLAY_CLEAR)
  {
    plane.o.clear();
    return;
  }

  if (ov->cmd == BD_OVERLAY_INIT)
  {
    OverlayInit(plane, ov->w, ov->h);
    return;
  }

  if (ov->cmd == BD_OVERLAY_DRAW
  ||  ov->cmd == BD_OVERLAY_WIPE)
    OverlayClear(plane, ov->x, ov->y, ov->w, ov->h);


  /* uncompress and draw bitmap */
  if (ov->img && ov->cmd == BD_OVERLAY_DRAW)
  {
    SOverlay overlay(new CDVDOverlayImage(), std::ptr_fun(CDVDOverlay::Release));

    if (ov->palette)
    {
      overlay->palette_colors = 256;
      overlay->palette        = (uint32_t*)calloc(overlay->palette_colors, 4);

      for(unsigned i = 0; i < 256; i++)
        overlay->palette[i] = build_rgba(ov->palette[i]);
    }

    const BD_PG_RLE_ELEM *rlep = ov->img;
    uint8_t *img = (uint8_t*) malloc(ov->w * ov->h);
    if (!img)
      return;
    unsigned pixels = ov->w * ov->h;

    for (unsigned i = 0; i < pixels; i += rlep->len, rlep++) {
      memset(img + i, rlep->color, rlep->len);
    }

    overlay->data     = img;
    overlay->linesize = ov->w;
    overlay->x        = ov->x;
    overlay->y        = ov->y;
    overlay->height   = ov->h;
    overlay->width    = ov->w;
    overlay->source_height = plane.h;
    overlay->source_width  = plane.w;
    plane.o.push_back(overlay);
  }

  if(ov->cmd == BD_OVERLAY_FLUSH)
    OverlayFlush(ov->pts);
#endif
}

#ifdef HAVE_LIBBLURAY_BDJ
void CDVDInputStreamBluray::OverlayCallbackARGB(const struct bd_argb_overlay_s * const ov)
{
  if(ov == NULL || ov->cmd == BD_ARGB_OVERLAY_CLOSE)
  {
    OverlayClose();
    return;
  }

  if (ov->plane > 1)
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray - Ignoring overlay with multiple planes");
    return;
  }

  SPlane& plane(m_planes[ov->plane]);

  if (ov->cmd == BD_ARGB_OVERLAY_INIT)
  {
    OverlayInit(plane, ov->w, ov->h);
    return;
  }

  if (ov->cmd == BD_ARGB_OVERLAY_DRAW)
    OverlayClear(plane, ov->x, ov->y, ov->w, ov->h);

  /* uncompress and draw bitmap */
  if (ov->argb && ov->cmd == BD_ARGB_OVERLAY_DRAW)
  {
    SOverlay overlay(new CDVDOverlayImage(), std::ptr_fun(CDVDOverlay::Release));

    overlay->palette_colors = 0;
    overlay->palette        = NULL;

    unsigned bytes = ov->stride * ov->h * 4;
    uint8_t *img = (uint8_t*) malloc(bytes);
    memcpy(img, ov->argb, bytes);

    overlay->data     = img;
    overlay->linesize = ov->stride * 4;
    overlay->x        = ov->x;
    overlay->y        = ov->y;
    overlay->height   = ov->h;
    overlay->width    = ov->w;
    overlay->source_height = plane.h;
    overlay->source_width  = plane.w;
    plane.o.push_back(overlay);
  }

  if(ov->cmd == BD_ARGB_OVERLAY_FLUSH)
    OverlayFlush(ov->pts);
}
#endif

=======
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
>>>>>>> FETCH_HEAD

int CDVDInputStreamBluray::GetTotalTime()
{
  if(m_title)
<<<<<<< HEAD
    return (int)(m_title->duration / 90);
=======
    return m_title->duration / 90;
>>>>>>> FETCH_HEAD
  else
    return 0;
}

int CDVDInputStreamBluray::GetTime()
{
<<<<<<< HEAD
  return (int)(m_dll->bd_tell_time(m_bd) / 90);
=======
  return m_dll->bd_tell_time(m_bd) / 90;
>>>>>>> FETCH_HEAD
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

<<<<<<< HEAD
int64_t CDVDInputStreamBluray::Seek(int64_t offset, int whence)
=======
__int64 CDVDInputStreamBluray::Seek(__int64 offset, int whence)
>>>>>>> FETCH_HEAD
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
<<<<<<< HEAD
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Seek - seek to %" PRId64", failed with %" PRId64, offset, pos);
=======
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Seek - seek to %"PRId64", failed with %"PRId64, offset, pos); 
>>>>>>> FETCH_HEAD
    return -1;
  }

  if(pos != offset)
<<<<<<< HEAD
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray::Seek - seek to %" PRId64", ended at %" PRId64, offset, pos);
=======
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray::Seek - seek to %"PRId64", ended at %"PRId64, offset, pos); 
>>>>>>> FETCH_HEAD

  return offset;
#else
  if(whence == SEEK_POSSIBLE)
    return 0;
  return -1;
#endif
}

<<<<<<< HEAD
int64_t CDVDInputStreamBluray::GetLength()
=======
__int64 CDVDInputStreamBluray::GetLength()
>>>>>>> FETCH_HEAD
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
<<<<<<< HEAD
  if(!m_title || m_clip >= m_title->clip_count)
    return;

  BLURAY_CLIP_INFO *clip = m_title->clips+m_clip;
=======
  if(m_title->clip_count == 0)
    return;

  BLURAY_CLIP_INFO *clip = m_title->clips;
>>>>>>> FETCH_HEAD

  if(find_stream(pid, clip->audio_streams, clip->audio_stream_count, language))
    return;
  if(find_stream(pid, clip->video_streams, clip->video_stream_count, language))
    return;
  if(find_stream(pid, clip->pg_streams, clip->pg_stream_count, language))
    return;
  if(find_stream(pid, clip->ig_streams, clip->ig_stream_count, language))
    return;
}

<<<<<<< HEAD
CDVDInputStream::ENextStream CDVDInputStreamBluray::NextStream()
{
  if(!m_navmode)
    return NEXTSTREAM_NONE;

  if (m_hold == HOLD_ERROR)
  {
#if (BLURAY_VERSION < BLURAY_VERSION_CODE(0,3,0))
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::NextStream - libbluray navigation mode read error");
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25008), g_localizeStrings.Get(25009));
#endif
    return NEXTSTREAM_NONE;
  }

  /* process any current event */
  ProcessEvent();

  /* process all queued up events */
  while(m_dll->bd_get_event(m_bd, &m_event))
    ProcessEvent();

  if(m_hold == HOLD_STILL)
    return NEXTSTREAM_RETRY;

  m_hold = HOLD_DATA;
  return NEXTSTREAM_OPEN;
}

void CDVDInputStreamBluray::UserInput(bd_vk_key_e vk)
{
  if(m_bd == NULL || !m_navmode)
    return;
  m_dll->bd_user_input(m_bd, -1, vk);
}

bool CDVDInputStreamBluray::MouseMove(const CPoint &point)
{
  if (m_bd == NULL || !m_navmode)
    return false;

  if (m_dll->bd_mouse_select(m_bd, -1, (uint16_t)point.x, (uint16_t)point.y) < 0)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::MouseMove - mouse select failed");
    return false;
  }

  return true;
}

bool CDVDInputStreamBluray::MouseClick(const CPoint &point)
{
  if (m_bd == NULL || !m_navmode)
    return false;

  if (m_dll->bd_mouse_select(m_bd, -1, (uint16_t)point.x, (uint16_t)point.y) < 0)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::MouseClick - mouse select failed");
    return false;
  }

  if (m_dll->bd_user_input(m_bd, -1, BD_VK_MOUSE_ACTIVATE) >= 0)
    return true;

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::MouseClick - mouse click (user input) failed");
  return false;
}

void CDVDInputStreamBluray::OnMenu()
{
  if(m_bd == NULL || !m_navmode)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - navigation mode not enabled");
    return;
  }

  if(m_dll->bd_user_input(m_bd, -1, BD_VK_POPUP) >= 0)
    return;
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - popup failed, trying root");

  if(m_dll->bd_user_input(m_bd, -1, BD_VK_ROOT_MENU) >= 0)
    return;

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - root failed, trying explicit");
  if(m_dll->bd_menu_call(m_bd, -1) <= 0)
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - root failed");
}

bool CDVDInputStreamBluray::IsInMenu()
{
  if(m_bd == NULL || !m_navmode)
    return false;
  if(m_menu || m_planes[BD_OVERLAY_IG].o.size() > 0)
    return true;
  return false;
}

void CDVDInputStreamBluray::SkipStill()
{
  if(m_bd == NULL || !m_navmode)
    return;

  if(m_hold == HOLD_STILL)
  {
    m_hold = HOLD_HELD;
    m_dll->bd_read_skip_still(m_bd);
  }
}

bool CDVDInputStreamBluray::HasMenu()
{
  return m_navmode;
}

=======
>>>>>>> FETCH_HEAD
#endif
