#include <xtl.h>
#include "EncoderLame.h"
#include "..\utils\log.h"
#include "..\sectionLoader.h"
#include "..\settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "..\lib\liblame\parse.h"

// taken from Lame from main.c
int parse_args_from_string(lame_global_flags * const gfp, const char *p,
                       char *inPath, char *outPath)
{                       /* Quick & very Dirty */
    char   *q;
    char   *f;
    char   *r[128];
    int     c = 0;
    int     ret;

		CLog::Log("Encoder: Encoding with %s", p);
    if (p == NULL || *p == '\0')
        return 0;

    f = q = (char*)malloc(strlen(p) + 1);
    strcpy(q, p);

    r[c++] = "lhama";
    while (1) {
        r[c++] = q;
        while (*q != ' ' && *q != '\0')
            q++;
        if (*q == '\0')
            break;
        *q++ = '\0';
    }
    r[c] = NULL;

    ret = parse_args(gfp, c, r, inPath, outPath, NULL, NULL);
    free(f);
    return ret;
}

#ifdef __cplusplus
}
#endif

CEncoderLame::CEncoderLame()
{
	memset(m_inPath, 0, MAX_PATH + 1);
	memset(m_outPath, 0, MAX_PATH + 1);
	m_pFile = NULL;
}

bool CEncoderLame::Init(const char* strFile)
{
	// load lame section
	g_sectionLoader.Load("LIBLAME");

	m_pGlobalFlags = lame_init();
	if (!m_pGlobalFlags)
	{
		CLog::Log("Error: lame_init() failed");
		return false;
	}

	m_pFile = fopen(strFile, "wb+");
	if(!m_pFile)
	{
		CLog::Log("Error: Cannot open file: %s", strFile);
		return false;
	}

	// setup parmaters, see lame.h for possibilities
	if (g_stSettings.m_iRipQuality == CDDARIP_QUALITY_CBR)
	{
		// use cbr and specified bitrate from settings
		CStdString strSettings;
		strSettings.Format("%s%i", "--preset cbr ", g_stSettings.m_iRipBitRate);
		parse_args_from_string(m_pGlobalFlags, strSettings.c_str(), m_inPath, m_outPath);
		//lame_set_mode(pGlobalFlags, JOINT_STEREO);
		//lame_set_brate(pGlobalFlags, g_stSettings.m_iRipBitRate);
	}
	else
	{
		// use presets (VBR)
		CStdString strSettings;
		switch (g_stSettings.m_iRipQuality)
		{
		case CDDARIP_QUALITY_MEDIUM: { strSettings = "--preset medium"; break;}  // 150-180kbps
		case CDDARIP_QUALITY_STANDARD: { strSettings = "--preset standard"; break;}  // 170-210kbps
		case CDDARIP_QUALITY_EXTREME: { strSettings = "--preset extreme"; break;} // 200-240kbps
		}
		parse_args_from_string(m_pGlobalFlags, strSettings.c_str(), m_inPath, m_outPath);
	}

	lame_set_asm_optimizations(m_pGlobalFlags, MMX, 1);
	lame_set_asm_optimizations(m_pGlobalFlags, SSE, 1);
	lame_set_in_samplerate(m_pGlobalFlags, 44100);

	// add id3v2 tags
	// id3tag_add_v2(pGlobalFlags);

	// Now that all the options are set, lame needs to analyze them and
	// set some more internal options and check for problems
  if (lame_init_params(m_pGlobalFlags) < 0)
	{
		CLog::Log("Error: Cannot init Lame params");
		return false;
  }

	return true;
}

int CEncoderLame::Encode(int nNumBytesRead, BYTE* pbtStream)
{
	int iBytes = lame_encode_buffer_interleaved(m_pGlobalFlags, (short*)pbtStream, nNumBytesRead/4, m_buffer, sizeof(m_buffer));

	if (iBytes < 0)
	{
		CLog::Log("Internal Lame error: %i", iBytes);
		return 0;
	}

	if (fwrite(m_buffer, 1, iBytes, m_pFile) != iBytes)
	{ 
		CLog::Log("Error writing Lame buffer to file");
		return 0;
	}

	return 1;
}

bool CEncoderLame::Close()
{
	// may return one more mp3 frames
	int iBytes = lame_encode_flush(m_pGlobalFlags, m_buffer, sizeof(m_buffer));

	if (iBytes < 0) {
		CLog::Log("Internal Lame error: %i", iBytes);
		return false;
	}

	fwrite(m_buffer, 1, iBytes, m_pFile);

	lame_mp3_tags_fid(m_pGlobalFlags, m_pFile); /* add VBR tags to mp3 file */
	lame_close(m_pGlobalFlags);

	fclose(m_pFile);

	// unload lame section
	g_sectionLoader.Unload("LIBLAME");
	return true;
}

void CEncoderLame::AddTag(int key,const char* value)
{
	switch(key)
	{
	case ENC_ARTIST:
		id3tag_set_artist(m_pGlobalFlags, value);
		break;
	case ENC_TITLE:
		id3tag_set_title(m_pGlobalFlags, value);
		break;
	case ENC_ALBUM:
		id3tag_set_album(m_pGlobalFlags, value);
		break;
	case ENC_YEAR:
		id3tag_set_year(m_pGlobalFlags, value);
		break;
	case ENC_COMMENT:
		id3tag_set_comment(m_pGlobalFlags, value);
		break;
	case ENC_TRACK:
		id3tag_set_track(m_pGlobalFlags, value);
		break;
	case ENC_GENRE:
		id3tag_set_genre(m_pGlobalFlags, value);
		break;
	default:
		break;
	}
}