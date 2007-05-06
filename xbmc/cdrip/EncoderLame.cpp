#include "stdafx.h"
#include "EncoderLame.h"
#include "../Id3Tag.h"

// taken from Lame from main.c
int CEncoderLame::parse_args_from_string(lame_global_flags * const gfp, const char *p,
                            char *inPath, char *outPath)
{                       /* Quick & very Dirty */
  char *q;
  char *f;
  char *r[128];
  int c = 0;
  int ret;

  CLog::Log(LOGINFO, "Encoder: Encoding with %s", p);
  if (p == NULL || *p == '\0')
    return 0;

  f = q = (char*)malloc(strlen(p) + 1);
  strcpy(q, p);

  r[c++] = "lhama";
  while (1)
  {
    r[c++] = q;
    while (*q != ' ' && *q != '\0')
      q++;
    if (*q == '\0')
      break;
    *q++ = '\0';
  }
  r[c] = NULL;

  ret = m_dll.parse_args(gfp, c, r, inPath, outPath, NULL, NULL);
  free(f);
  return ret;
}

CEncoderLame::CEncoderLame()
{
  memset(m_inPath, 0, XBMC_MAX_PATH + 1);
  memset(m_outPath, 0, XBMC_MAX_PATH + 1);
}

bool CEncoderLame::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

  // set input stream information and open the file
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits)) return false;

  // load the lame dll
  if (!m_dll.Load())
    return false;

  m_pGlobalFlags = m_dll.lame_init();
  if (!m_pGlobalFlags)
  {
    CLog::Log(LOGERROR, "Error: lame_init() failed");
    return false;
  }

  // setup parmaters, see lame.h for possibilities
  if (g_guiSettings.GetInt("cddaripper.quality") == CDDARIP_QUALITY_CBR)
  {
    // use cbr and specified bitrate from settings
    CStdString strSettings;
    strSettings.Format("%s%i", "--preset cbr ", g_guiSettings.GetInt("cddaripper.bitrate"));
    parse_args_from_string(m_pGlobalFlags, strSettings.c_str(), m_inPath, m_outPath);
    //lame_set_mode(pGlobalFlags, JOINT_STEREO);
    //lame_set_brate(pGlobalFlags, g_stSettings.m_iRipBitRate);
  }
  else
  {
    // use presets (VBR)
    CStdString strSettings;
    switch (g_guiSettings.GetInt("cddaripper.quality"))
    {
    case CDDARIP_QUALITY_MEDIUM: { strSettings = "--preset fast medium"; break;}  // 150-180kbps
    case CDDARIP_QUALITY_STANDARD: { strSettings = "--preset fast standard"; break;}  // 170-210kbps
    case CDDARIP_QUALITY_EXTREME: { strSettings = "--preset fast extreme"; break;} // 200-240kbps
    }
    parse_args_from_string(m_pGlobalFlags, strSettings.c_str(), m_inPath, m_outPath);
  }

  m_dll.lame_set_asm_optimizations(m_pGlobalFlags, MMX, 1);
  m_dll.lame_set_asm_optimizations(m_pGlobalFlags, SSE, 1);
  m_dll.lame_set_in_samplerate(m_pGlobalFlags, 44100);

  // Now that all the options are set, lame needs to analyze them and
  // set some more internal options and check for problems
  if (m_dll.lame_init_params(m_pGlobalFlags) < 0)
  {
    CLog::Log(LOGERROR, "Error: Cannot init Lame params");
    return false;
  }

  return true;
}

int CEncoderLame::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  int iBytes = m_dll.lame_encode_buffer_interleaved(m_pGlobalFlags, (short*)pbtStream, nNumBytesRead / 4, m_buffer, sizeof(m_buffer));

  if (iBytes < 0)
  {
    CLog::Log(LOGERROR, "Internal Lame error: %i", iBytes);
    return 0;
  }

  if (WriteStream(m_buffer, iBytes) != iBytes)
  {
    CLog::Log(LOGERROR, "Error writing Lame buffer to file");
    return 0;
  }

  return 1;
}

bool CEncoderLame::Close()
{
  // may return one more mp3 frames
  int iBytes = m_dll.lame_encode_flush(m_pGlobalFlags, m_buffer, sizeof(m_buffer));

  if (iBytes < 0)
  {
    CLog::Log(LOGERROR, "Internal Lame error: %i", iBytes);
    return false;
  }

  WriteStream(m_buffer, iBytes);
  FlushStream();
  FileClose();

  // open again, but now the old way, lame only accepts FILE pointers
  FILE* file = fopen(m_strFile.c_str(), "rb+");
  if (!file)
  {
    CLog::Log(LOGERROR, "Error: Cannot open file for writing tags: %s", m_strFile.c_str());
    return false;
  }

  m_dll.lame_mp3_tags_fid(m_pGlobalFlags, file); /* add VBR tags to mp3 file */
  fclose(file);

  m_dll.lame_close(m_pGlobalFlags);

  // unload the lame dll
  m_dll.Unload();

  // Store a id3 tag in the ripped file
  CID3Tag id3tag;
  CMusicInfoTag tag;
  tag.SetAlbum(m_strAlbum);
  tag.SetArtist(m_strArtist);
  tag.SetGenre(m_strGenre);
  tag.SetTitle(m_strTitle);
  tag.SetTrackNumber(atoi(m_strTrack.c_str()));
  SYSTEMTIME time;
  time.wYear=atoi(m_strYear.c_str());
  tag.SetReleaseDate(time);
  id3tag.SetMusicInfoTag(tag);
  id3tag.Write(m_strFile);

  return true;
}
