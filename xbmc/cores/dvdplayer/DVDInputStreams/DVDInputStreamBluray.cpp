/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "system.h"
#ifdef HAVE_LIBBLURAY

#include "DVDInputStreamBluray.h"
#include "IDVDPlayer.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlayImage.h"
#include "settings/GUISettings.h"
#include "LangInfo.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "DllLibbluray.h"
#include "URL.h"

#define LIBBLURAY_BYTESEEK 0

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

void DllLibbluray::file_close(BD_FILE_H *file)
{
  if (file)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed file (%p)\n", file);
    
    delete static_cast<CFile*>(file->internal);
    delete file;
  }
}

int64_t DllLibbluray::file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

int64_t DllLibbluray::file_tell(BD_FILE_H *file)
{
  return static_cast<CFile*>(file->internal)->GetPosition();
}

int DllLibbluray::file_eof(BD_FILE_H *file)
{
  if(static_cast<CFile*>(file->internal)->GetPosition() == static_cast<CFile*>(file->internal)->GetLength())
    return 1;
  else
    return 0;
}

int64_t DllLibbluray::file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<CFile*>(file->internal)->Read(buf, size);
}

int64_t DllLibbluray::file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
{
    return -1;
}

BD_FILE_H * DllLibbluray::file_open(const char* filename, const char *mode)
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
    if(fp->Open(strFilename))
    {
      file->internal = (void*)fp;
      return file;
    }

    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%p)", file);
    
    delete fp;
    delete file;

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

void DllLibbluray::dir_close(BD_DIR_H *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed dir (%p)\n", dir);
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}


int DllLibbluray::dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
{
    SDirState* state = static_cast<SDirState*>(dir->internal);

    if(state->curr >= state->list.Size())
      return 1;

    strncpy(entry->d_name, state->list[state->curr]->GetLabel(), sizeof(entry->d_name));
    entry->d_name[sizeof(entry->d_name)-1] = 0;
    state->curr++;

    return 0;
}

BD_DIR_H *DllLibbluray::dir_open(const char* dirname)
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

void DllLibbluray::bluray_logger(const char* msg)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Logger - %s", msg);
}


static void bluray_overlay_cb(void *this_gen, const BD_OVERLAY * ov)
{
  static_cast<CDVDInputStreamBluray*>(this_gen)->OverlayCallback(ov);
}

CDVDInputStreamBluray::CDVDInputStreamBluray(IDVDPlayer* player) :
  CDVDInputStream(DVDSTREAM_TYPE_BLURAY)
{
  m_title = NULL;
  m_clip  = 0;
  m_bd    = NULL;
  m_dll = new DllLibbluray;
  if (!m_dll->Load())
  {
    delete m_dll;
    m_dll = NULL;
  }
  m_content = "video/x-mpegts";
  m_player  = player;
  m_title_playing = false;
  m_navmode = false;
  m_hold = HOLD_NONE;
  memset(&m_event, 0, sizeof(m_event));
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

BLURAY_TITLE_INFO* CDVDInputStreamBluray::GetTitleLongest()
{
  int titles = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT, 0);

  BLURAY_TITLE_INFO *t, *s = NULL;
  for(int i=0; i < titles; i++)
  {
    t = m_dll->bd_get_title_info(m_bd, i, 0);
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

  CStdString strPath(strFile);
  CStdString filename;
  CStdString root;

  if(strPath.Left(7).Equals("bluray:"))
  {
    CURL url(strPath);
    root     = url.GetHostName();
    filename = URIUtils::GetFileName(url.GetFileName());
  }
  else
  {
    URIUtils::GetDirectory(strPath,strPath);
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
    root     = strPath;
    filename = URIUtils::GetFileName(strFile);
  }

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

  if(filename.Equals("index.bdmv"))
  {
    m_navmode = false;
    m_title = GetTitleLongest();
  }
  else if(URIUtils::GetExtension(filename).Equals(".mpls"))
  {
    m_navmode = false;
    m_title = GetTitleFile(filename);
  }
  else if(filename.Equals("MovieObject.bdmv"))
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
  else
  {
    CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - unsupported bluray file selected %s", strPath.c_str());
    return false;
  }

  if(m_navmode)
  {
    int region = g_guiSettings.GetInt("dvds.playerregion");
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

    m_dll->bd_get_event(m_bd, NULL);


    if(m_dll->bd_play(m_bd) <= 0)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray::Open - failed play disk %s", strPath.c_str());
      return false;
    }
    m_hold = HOLD_DATA;
    m_title_playing = false;
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
  }

  return true;
}

// close file and reset everyting
void CDVDInputStreamBluray::Close()
{
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

int CDVDInputStreamBluray::Read(BYTE* buf, int buf_size)
{
  if(m_navmode)
  {
    int result = 0;
    do {

      if(m_hold == HOLD_HELD)
        return 0;

      if(m_hold == HOLD_SKIP)
      {
        /* m_event already holds data */
        m_hold = HOLD_DATA;
        result = 0;
      }
      else
      {
        result = m_dll->bd_read_ext (m_bd, buf, buf_size, &m_event);

        if(m_hold == HOLD_NONE)
        {
          /* Check for holding events */
          switch(m_event.event) {
            case BD_EVENT_SEEK:
            case BD_EVENT_TITLE:
              if(m_title_playing)
                m_player->OnDVDNavResult(NULL, 1);
              m_hold = HOLD_HELD;
              return result;

            case BD_EVENT_PLAYLIST:
            case BD_EVENT_PLAYITEM:
              m_hold = HOLD_HELD;
              return result;
            default:
              break;
          }
        }
        if(result > 0)
          m_hold = HOLD_NONE;
      }
      int pid = -1;
      switch (m_event.event) {

        case BD_EVENT_ERROR:
          CLog::Log(LOGERROR, "CDVDInputStreamBluray - BD_EVENT_ERROR");
          return -1;

        case BD_EVENT_ENCRYPTED:
          CLog::Log(LOGERROR, "CDVDInputStreamBluray - BD_EVENT_ENCRYPTED");
          return -1;

        /* playback control */

        case BD_EVENT_SEEK:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_SEEK");
          break;

        case BD_EVENT_STILL_TIME:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_STILL_TIME %d", m_event.param);
          return 0;

        case BD_EVENT_STILL:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_STILL %d", m_event.param);
          break;

        /* playback position */

        case BD_EVENT_ANGLE:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_ANGLE %d", m_event.param);
          break;

        case BD_EVENT_END_OF_TITLE:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_END_OF_TITLE %d", m_event.param);
          m_title_playing = false;
          break;

        case BD_EVENT_TITLE:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_TITLE %d", m_event.param);
          if(m_title)
            m_dll->bd_free_title_info(m_title);
          m_title = m_dll->bd_get_title_info(m_bd, m_event.param, 0);
          m_title_playing = true;
          break;

        case BD_EVENT_PLAYLIST:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PLAYLIST %d", m_event.param);
          if(m_title)
            m_dll->bd_free_title_info(m_title);
          m_title = m_dll->bd_get_playlist_info(m_bd, m_event.param, 0);
          break;

        case BD_EVENT_PLAYITEM:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PLAYITEM %d", m_event.param);
          m_clip = m_event.param;
          break;

        case BD_EVENT_CHAPTER:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_CHAPTER %d", m_event.param);
          break;

        /* stream selection */

        case BD_EVENT_AUDIO_STREAM:
          pid = -1;
          if(m_title
          && m_title->clip_count > m_clip
          && m_title->clips[m_clip].audio_stream_count > (uint8_t)(m_event.param - 1))
            pid = m_title->clips[m_clip].audio_streams[m_event.param-1].pid;
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_AUDIO_STREAM %d %d", m_event.param, pid);
          m_player->OnDVDNavResult((void*)&pid, 2);
          break;

        case BD_EVENT_PG_TEXTST:
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PG_TEXTST %d", m_event.param);
          pid = m_event.param;
          m_player->OnDVDNavResult((void*)&pid, 4);
          break;

        case BD_EVENT_PG_TEXTST_STREAM:
          pid = -1;
          if(m_title
          && m_title->clip_count > m_clip
          && m_title->clips[m_clip].pg_stream_count > (uint8_t)(m_event.param - 1))
            pid = m_title->clips[m_clip].pg_streams[m_event.param-1].pid;
          CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - BD_EVENT_PG_TEXTST_STREAM %d, %d", m_event.param, pid);
          m_player->OnDVDNavResult((void*)&pid, 3);
          break;

        case BD_EVENT_IG_STREAM:
        case BD_EVENT_SECONDARY_AUDIO:
        case BD_EVENT_SECONDARY_AUDIO_STREAM:
        case BD_EVENT_SECONDARY_VIDEO:
        case BD_EVENT_SECONDARY_VIDEO_SIZE:
        case BD_EVENT_SECONDARY_VIDEO_STREAM:

        case BD_EVENT_NONE:
          break;

        default:
          CLog::Log(LOGWARNING, "CDVDInputStreamBluray - unhandled libbluray event %d [param %d]", m_event.event, m_event.param);
          break;
      }

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

void CDVDInputStreamBluray::OverlayCallback(const BD_OVERLAY * const ov)
{

  CDVDOverlayGroup* group   = new CDVDOverlayGroup();
  group->bForced = true;

  if(ov == NULL)
  {
    for(unsigned i = 0; i < 2; ++i)
    {
      for(std::vector<CDVDOverlayImage*>::iterator it = m_overlays[i].begin(); it != m_overlays[i].end(); ++it)
        (*it)->Release();
      m_overlays[i].clear();
    }

    m_player->OnDVDNavResult(group, 0);
    return;
  }

  group->iPTSStartTime = (double) ov->pts;
  group->iPTSStopTime  = 0;

  if (ov->plane > 1)
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamBluray - Ignoring overlay with multiple planes");
    group->Release();
    return;
  }

  std::vector<CDVDOverlayImage*>& plane(m_overlays[ov->plane]);

  /* fixup existing overlays */
  for(std::vector<CDVDOverlayImage*>::iterator it = plane.begin(); it != plane.end();)
  {
    /* if it's fully outside we are done */
    if(ov->x + ov->w <= (*it)->x
    || ov->x         >= (*it)->x + (*it)->width
    || ov->y + ov->h <= (*it)->y
    || ov->y         >= (*it)->y + (*it)->height)
    {
      ++it;
      continue;
    }

    int y1 = std::max<int>((*it)->y                , ov->y);
    int y2 = std::min<int>((*it)->y + (*it)->height, ov->y + ov->h);
    int x1 = std::max<int>((*it)->x                , ov->x);
    int x2 = std::min<int>((*it)->x + (*it)->width , ov->x + ov->w);

    /* if all should be cleared, delete */
    if(x1 == (*it)->x
    && x2 == (*it)->x + (*it)->width
    && y1 == (*it)->y
    && y2 == (*it)->y + (*it)->height)
    {
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Delete(%d) %d-%dx%d-%d", ov->plane, x1, x2, y1, y2);
      it = plane.erase(it);
      continue;
    }
#if(1)
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Clearing(%d) %d-%dx%d-%d", ov->plane, x1, x2, y1, y2);

    /* replace overlay with a new copy*/
    CDVDOverlayImage* overlay = new CDVDOverlayImage(*(*it));
    (*it)->Release();
    (*it) = overlay;

    /* any old hw overlay must be released */
    SAFE_RELEASE(overlay->m_overlay);

    /* clear out overlap */
    y1 -= overlay->y;
    y2 -= overlay->y;
    x1 -= overlay->x;
    x2 -= overlay->x;

    /* find fully transparent */
    int transp = 0;
    for(; transp < overlay->palette_colors; ++transp)
    {
      if(((overlay->palette[transp] >> PIXEL_ASHIFT) & 0xff) == 0)
        break;
    }

    if(transp == overlay->palette_colors)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamBluray - failed to find transparent color");
      continue;
    }

    for(int y = y1; y < y2; ++y)
    {
      BYTE* line = overlay->data + y * overlay->linesize;
      for(int x = x1; x < x2; ++x)
        line[x] = transp;
    }
    ++it;
#endif
  }


  /* uncompress and draw bitmap */
  if (ov->img)
  {
    CDVDOverlayImage* overlay = new CDVDOverlayImage();

    if (ov->palette)
    {
      overlay->palette_colors = 256;
      overlay->palette        = (uint32_t*)calloc(overlay->palette_colors, 4);

      for(unsigned i = 0; i < 256; i++)
        overlay->palette[i] = build_rgba(ov->palette[i]);
    }

    const BD_PG_RLE_ELEM *rlep = ov->img;
    uint8_t *img = (uint8_t*) malloc(ov->w * ov->h);
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
    plane.push_back(overlay);
  }

  for(unsigned i = 0; i < 2; ++i)
  {
    for(std::vector<CDVDOverlayImage*>::iterator it = m_overlays[i].begin(); it != m_overlays[i].end(); ++it)
      group->m_overlays.push_back((*it)->Acquire());
  }
  m_player->OnDVDNavResult(group, 0);
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

int64_t CDVDInputStreamBluray::Seek(int64_t offset, int whence)
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

int64_t CDVDInputStreamBluray::GetLength()
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
  if(!m_title || m_clip >= m_title->clip_count)
    return;

  BLURAY_CLIP_INFO *clip = m_title->clips+m_clip;

  if(find_stream(pid, clip->audio_streams, clip->audio_stream_count, language))
    return;
  if(find_stream(pid, clip->video_streams, clip->video_stream_count, language))
    return;
  if(find_stream(pid, clip->pg_streams, clip->pg_stream_count, language))
    return;
  if(find_stream(pid, clip->ig_streams, clip->ig_stream_count, language))
    return;
}

CDVDInputStream::ENextStream CDVDInputStreamBluray::NextStream()
{
  if(!m_navmode)
    return NEXTSTREAM_NONE;

  if(m_hold == HOLD_HELD)
  {
    m_hold = HOLD_SKIP;
    return NEXTSTREAM_OPEN;
  }
  if(m_hold == HOLD_NONE)
  {
    m_hold = HOLD_DATA;
    m_dll->bd_read_skip_still(m_bd);
  }
  return NEXTSTREAM_RETRY;
}

void CDVDInputStreamBluray::UserInput(bd_vk_key_e vk)
{
  if(m_bd == NULL || !m_navmode)
    return;
  m_dll->bd_user_input(m_bd, -1, vk);
}

void CDVDInputStreamBluray::OnMenu()
{
  if(m_bd == NULL || !m_navmode)
    return;

  if(m_dll->bd_user_input(m_bd, -1, BD_VK_POPUP) >= 0)
    return;
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - popup failed, trying root");

  if(m_dll->bd_user_input(m_bd, -1, BD_VK_ROOT_MENU) >= 0)
    return;

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - root failed, trying explicit");
  if(m_dll->bd_menu_call(m_bd, -1) >= 0)
    return;
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::OnMenu - root failed");
}

bool CDVDInputStreamBluray::IsInMenu()
{
  if(m_bd == NULL || !m_navmode)
    return false;
  if(m_overlays[BD_OVERLAY_IG].size() > 0)
    return true;
  return false;
}

#endif
