#include "../stdafx.h"
#include "EncoderVorbis.h"
#include "..\util.h"
#include "..\cores\DllLoader\Dll.h"
#include "EncoderDLL.h"

CEncoderVorbis::CEncoderVorbis()
{
  m_pDLLOgg = NULL;
  m_pDLLVorbis = NULL;
  m_pBuffer = NULL;
}

CEncoderVorbis::~CEncoderVorbis()
{
	FileClose();
}

DllLoader* CEncoderVorbis::LoadDLL(const char* strFile)
{
  DllLoader* pDLL = NULL;
  
  if (!strFile) return false;
  
	CLog::Log(LOGNOTICE, "CDVDPlayer::LoadDLL() Loading %s", strFile);
	pDLL = new DllLoader(strFile, false);
	if (!pDLL->Parse())
	{
		CLog::Log(LOGERROR, "CEncoderVorbis::LoadDLL() parse %s failed", strFile);
		delete pDLL;
		return NULL;
	}
	CLog::Log(LOGNOTICE, "CEncoderVorbis::LoadDLL() resolving imports for %s", strFile);
	if (!pDLL->ResolveImports())
	{
		CLog::Log(LOGERROR, "CEncoderVorbis::LoadDLL() resolving imports for vorbis.dll failed", strFile);
	}
	
	return pDLL;
}

bool CEncoderVorbis::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
	// we only accept 2 / 44100 / 16 atm
	if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

	// set input stream information and open the file
	if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits)) return false;

	float fQuality = 0.5f;
	if (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_MEDIUM) fQuality = 0.4f;
	if (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_STANDARD) fQuality = 0.5f;
	if (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_EXTREME) fQuality = 0.7f;

  // load the dll
  if (!m_pDLLOgg) m_pDLLOgg = LoadDLL("Q:\\system\\cdrip\\ogg.dll");
  if (!m_pDLLVorbis) m_pDLLVorbis = LoadDLL("Q:\\system\\cdrip\\vorbis.dll");

  if (!m_pDLLOgg || !m_pDLLVorbis || !cdripper_load_dll_ogg(*m_pDLLOgg) || !cdripper_load_dll_vorbis(*m_pDLLVorbis))
  {
    // failed loading the dll's, unload it all
    CLog::Log(LOGERROR, "CEncoderVorbis::Init() Error while loading ogg.dll and or vorbis.dll");
    if (m_pDLLOgg) delete m_pDLLOgg;
    m_pDLLOgg = NULL;
    if (m_pDLLVorbis) delete m_pDLLVorbis;
    m_pDLLVorbis = NULL;
    return false;
  }

	vorbis_info_init(&m_sVorbisInfo);
	if (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_CBR)
	{
		// not realy cbr, but abr in this case
		int iBitRate = g_guiSettings.GetInt("CDDARipper.Bitrate") * 1000;
		vorbis_encode_init(&m_sVorbisInfo, m_iInChannels, m_iInSampleRate, -1, iBitRate, -1);
	}
	else
	{
		if(vorbis_encode_init_vbr(&m_sVorbisInfo, m_iInChannels, m_iInSampleRate, fQuality)) return false;
	}

	/* add a comment */
  vorbis_comment_init(&m_sVorbisComment);
	vorbis_comment_add_tag(&m_sVorbisComment,"comment", (char*)m_strComment.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"artist", (char*)m_strArtist.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"title", (char*)m_strTitle.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"album", (char*)m_strAlbum.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"genre", (char*)m_strGenre.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"tracknumber", (char*)m_strTrack.c_str());
	vorbis_comment_add_tag(&m_sVorbisComment,"date", (char*)m_strYear.c_str());

	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&m_sVorbisDspState, &m_sVorbisInfo);

	vorbis_block_init(&m_sVorbisDspState, &m_sVorbisBlock);

	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build
	chained streams just by concatenation */
	srand(time(NULL));
	ogg_stream_init(&m_sOggStreamState, rand());

	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(&m_sVorbisDspState, &m_sVorbisComment,
				&header, &header_comm, &header_code);

		ogg_stream_packetin(&m_sOggStreamState, &header);
		ogg_stream_packetin(&m_sOggStreamState, &header_comm);
		ogg_stream_packetin(&m_sOggStreamState, &header_code);

		/* This ensures the actual
		* audio data will start on a new page, as per spec
		*/
		while(1){
			int result = ogg_stream_flush(&m_sOggStreamState, &m_sOggPage);
			if(result == 0)break;
			FileWrite(m_sOggPage.header, m_sOggPage.header_len);
			FileWrite(m_sOggPage.body, m_sOggPage.body_len);
		}
	}
	m_pBuffer = new BYTE[4096];

	return true;
}

int CEncoderVorbis::Encode(int nNumBytesRead, BYTE* pbtStream)
{
	int eos = 0;

	/* data to encode */
	LONG nBlocks = (int)(nNumBytesRead/4096);
	LONG nBytesleft = nNumBytesRead - nBlocks*4096;
	LONG block = 4096;

	for(int a = 0; a <= nBlocks; a++)
	{
		if(a == nBlocks)
		{
			// no more blocks of 4096 bytes to write, just write the last bytes
			block = nBytesleft;
		}

		/* expose the buffer to submit data */
		float **buffer = vorbis_analysis_buffer(&m_sVorbisDspState, 1024);
    
		/* uninterleave samples */
		fast_memcpy(m_pBuffer, pbtStream, block);
		pbtStream += 4096;
		LONG iSamples = block/(2*2);
		signed char* buf = (signed char*) m_pBuffer;
		for (int i = 0; i < iSamples; i++) {
			int j = i << 2; // j = i * 4
			buffer[0][i] = (((long)buf[j+1]<<8) | (0x00ff&(int)buf[j]))/32768.0f;
			buffer[1][i] = (((long)buf[j+3]<<8) | (0x00ff&(int)buf[j+2]))/32768.0f;
		}
	
		/* tell the library how much we actually submitted */
		vorbis_analysis_wrote(&m_sVorbisDspState, iSamples);

		/* vorbis does some data preanalysis, then divvies up blocks for
		more involved (potentially parallel) processing.  Get a single
		block for encoding now */
		while(vorbis_analysis_blockout(&m_sVorbisDspState, &m_sVorbisBlock) == 1)
		{
			/* analysis, assume we want to use bitrate management */
			vorbis_analysis(&m_sVorbisBlock, NULL);
			vorbis_bitrate_addblock(&m_sVorbisBlock);

			while(vorbis_bitrate_flushpacket(&m_sVorbisDspState, &m_sOggPacket))
			{
				/* weld the packet into the bitstream */
				ogg_stream_packetin(&m_sOggStreamState, &m_sOggPacket);

				/* write out pages (if any) */
				while(!eos)
				{
					int result = ogg_stream_pageout(&m_sOggStreamState, &m_sOggPage);
					if(result == 0)break;
					WriteStream(m_sOggPage.header, m_sOggPage.header_len);
					WriteStream(m_sOggPage.body, m_sOggPage.body_len);
	
					/* this could be set above, but for illustrative purposes, I do
					it here (to show that vorbis does know where the stream ends) */
					if(ogg_page_eos(&m_sOggPage)) eos = 1;
				}
			}
		}
	}

	return 1;
}

bool CEncoderVorbis::Close()
{
	int eos = 0;
	// tell vorbis we are encoding the end of the stream
	vorbis_analysis_wrote(&m_sVorbisDspState, 0);
	while(vorbis_analysis_blockout(&m_sVorbisDspState, &m_sVorbisBlock) == 1)
	{
		/* analysis, assume we want to use bitrate management */
		vorbis_analysis(&m_sVorbisBlock, NULL);
		vorbis_bitrate_addblock(&m_sVorbisBlock);

		while(vorbis_bitrate_flushpacket(&m_sVorbisDspState, &m_sOggPacket))
		{
			/* weld the packet into the bitstream */
			ogg_stream_packetin(&m_sOggStreamState, &m_sOggPacket);

			/* write out pages (if any) */
			while(!eos)
			{
				int result = ogg_stream_pageout(&m_sOggStreamState, &m_sOggPage);
				if(result==0)break;
				WriteStream(m_sOggPage.header, m_sOggPage.header_len);
				WriteStream(m_sOggPage.body, m_sOggPage.body_len);

				/* this could be set above, but for illustrative purposes, I do
				it here (to show that vorbis does know where the stream ends) */
				if(ogg_page_eos(&m_sOggPage)) eos = 1;
			}
		}
	}

  /* clean up and exit.  vorbis_info_clear() must be called last */
  ogg_stream_clear(&m_sOggStreamState);
  vorbis_block_clear(&m_sVorbisBlock);
  vorbis_dsp_clear(&m_sVorbisDspState);
  vorbis_comment_clear(&m_sVorbisComment);
  vorbis_info_clear(&m_sVorbisInfo);
  
  /* ogg_page and ogg_packet structs always point to storage in
     libvorbis.  They're never freed or manipulated directly */
	FlushStream();
	FileClose();

	delete []m_pBuffer;
	m_pBuffer = NULL;

	if (m_pDLLOgg)
	{
	  CLog::Log(LOGNOTICE, "CEncoderVorbis::Close() Unloading ogg.dll");
		delete m_pDLLOgg;
		m_pDLLOgg = NULL;
	}
	
	if (m_pDLLVorbis)
	{
	  CLog::Log(LOGNOTICE, "CEncoderVorbis::Close() Unloading vorbis.dll");
		delete m_pDLLVorbis;
		m_pDLLVorbis = NULL;
	}
	
	return true;
}
