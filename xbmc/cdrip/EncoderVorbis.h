#ifndef _ENCODERVORBIS_H
#define _ENCODERVORBIS_H

#include "Encoder.h"
#include "..\lib\libOggVorbis\vorbisenc.h"

// merge sections from liboggvorbis and liboggvorbisd
#pragma comment(linker, "/merge:OGG_TEXT=LIBOGGVO")
#pragma comment(linker, "/merge:OGG_DATA=LIBOGGVO")
#pragma comment(linker, "/merge:OGG_BSS=LIBOGGVO")
// this section is so small, we don't bother with unloading it
// it gives us more trouble the it solves
//#pragma comment(linker, "/merge:OGG_RDAT=LIBOGGVO")
#pragma comment(linker, "/section:LIBOGGVO,RWE")

class CEncoderVorbis : public CEncoder
{
public:
	CEncoderVorbis();
	~CEncoderVorbis();
	bool               Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
	int                Encode(int nNumBytesRead, BYTE* pbtStream);
	bool               Close();
	void               AddTag(int key,const char* value);

private:

	vorbis_info        m_sVorbisInfo; /* struct that stores all the static vorbis bitstream settings */
	vorbis_dsp_state   m_sVorbisDspState; /* central working state for the packet->PCM decoder */
	vorbis_block       m_sVorbisBlock; /* local working space for packet->PCM decode */
	vorbis_comment     m_sVorbisComment;

	ogg_stream_state   m_sOggStreamState; /* take physical pages, weld into a logical stream of packets */
	ogg_page           m_sOggPage; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet         m_sOggPacket; /* one raw packet of data for decode */

	BYTE*              m_pBuffer;
};

#endif // _ENCODERVORBIS_H