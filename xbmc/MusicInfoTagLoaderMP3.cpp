
#include "stdafx.h"
#include "musicinfotagloadermp3.h"
#include "stdstring.h"
#include "sectionloader.h"
#include "Util.h"
#include "picture.h"
#include "utils/log.h"
#include "autoptrhandle.h"

using namespace AUTOPTR;

const uchar* ID3_GetPictureBufferOfPicType(ID3_Tag* tag, ID3_PictureType pictype, size_t* pBufSize )
{
  if (NULL == tag)
    return NULL;
  else
  {
    ID3_Frame* frame = NULL;
    ID3_Tag::Iterator* iter = tag->CreateIterator();

    while (NULL != (frame = iter->GetNext() ))
    {
      if(frame->GetID() == ID3FID_PICTURE)
      {
        if(frame->GetField(ID3FN_PICTURETYPE)->Get() == (uint32)pictype)
          break;
      }
    }
    delete iter;

    if (frame != NULL)
    {
      ID3_Field* myField = frame->GetField(ID3FN_DATA);
      if (myField != NULL)
      {
				*pBufSize=myField->Size();
        return myField->GetRawBinary();
      }
      else return NULL;
    }
    else return NULL;
  }
}

#define BYTES2INT(b1,b2,b3,b4) (((b1 & 0xFF) << (3*8)) | \
                                ((b2 & 0xFF) << (2*8)) | \
                                ((b3 & 0xFF) << (1*8)) | \
                                ((b4 & 0xFF) << (0*8)))

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) | \
                             ((b2 & 0x7F) << (2*7)) | \
                             ((b3 & 0x7F) << (1*7)) | \
                             ((b4 & 0x7F) << (0*7)))

#define MPEG_VERSION2_5 0
#define MPEG_VERSION1   1
#define MPEG_VERSION2   2

/* Xing header information */
#define VBR_FRAMES_FLAG 0x01
#define VBR_BYTES_FLAG  0x02
#define VBR_TOC_FLAG    0x04

// mp3 header flags
#define SYNC_MASK (0x7ff << 21)
#define VERSION_MASK (3 << 19)
#define LAYER_MASK (3 << 17)
#define PROTECTION_MASK (1 << 16)
#define BITRATE_MASK (0xf << 12)
#define SAMPLERATE_MASK (3 << 10)
#define PADDING_MASK (1 << 9)
#define PRIVATE_MASK (1 << 8)
#define CHANNELMODE_MASK (3 << 6)
#define MODE_EXT_MASK (3 << 4)
#define COPYRIGHT_MASK (1 << 3)
#define ORIGINAL_MASK (1 << 2)
#define EMPHASIS_MASK 3

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{
}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

bool CMusicInfoTagLoaderMP3::ReadTag( ID3_Tag& id3tag, CMusicInfoTag& tag )
{
	bool bResult= false;

	SYSTEMTIME dateTime;
	auto_aptr<char>pYear  (ID3_GetYear( &id3tag  ));
	auto_aptr<char>pTitle (ID3_GetTitle( &id3tag ));
	auto_aptr<char>pArtist(ID3_GetArtist( &id3tag));
	auto_aptr<char>pAlbum (ID3_GetAlbum( &id3tag ));
	auto_aptr<char>pGenre (ID3_GetGenre( &id3tag ));
	int nTrackNum=ID3_GetTrackNum( &id3tag );

	tag.SetTrackNumber(nTrackNum);

	if (NULL != pGenre.get())
	{
		tag.SetGenre(ParseMP3Genre(pGenre.get()));
	}
	if (NULL != pTitle.get())
	{
		bResult = true;
		tag.SetLoaded(true);
		tag.SetTitle(pTitle.get());
	}
	if (NULL != pArtist.get())
	{
		tag.SetArtist(pArtist.get());
	}
	if (NULL != pAlbum.get())
	{
		tag.SetAlbum(pAlbum.get());
	}
	if (NULL != pYear.get())
	{
		dateTime.wYear=atoi(pYear.get());
		tag.SetReleaseDate(dateTime);
	}

	//	extract Cover Art and save as album thumb
	if (ID3_HasPicture(&id3tag))
	{
		ID3_PictureType nPicTyp=ID3PT_COVERFRONT;
		CStdString strExtension;
		bool bFound=false;
		auto_aptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
		if (pMimeTyp.get() == NULL)
		{
			nPicTyp=ID3PT_OTHER;
			auto_aptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
			if (pMimeTyp.get() != NULL)
			{
				strExtension=pMimeTyp.get();
				bFound=true;
			}
		}
		else
		{
			strExtension=pMimeTyp.get();
			bFound=true;
		}

		CStdString strCoverArt, strPath, strFileName;
		CUtil::Split(tag.GetURL(), strPath, strFileName);
		CUtil::GetAlbumThumb(tag.GetAlbum()+strPath, strCoverArt,true);
		if (bFound)
		{
			if (!CUtil::ThumbExists(strCoverArt))
			{
				int nPos=strExtension.Find('/');
				if (nPos>-1)
					strExtension.Delete(0, nPos+1);

				size_t nBufSize=0;
				const BYTE* pPic=ID3_GetPictureBufferOfPicType(&id3tag, nPicTyp, &nBufSize );

				if (pPic!=NULL && nBufSize>0)
				{
					CPicture pic;
					if (pic.CreateAlbumThumbnailFromMemory(pPic, nBufSize, strExtension, strCoverArt))
					{
						CUtil::ThumbCacheAdd(strCoverArt, true);
					}
					else
					{
						CUtil::ThumbCacheAdd(strCoverArt, false);
						CLog::Log(LOGERROR, "Tag loader mp3: Unable to create album art for %s (extension=%s, size=%d)", tag.GetURL().c_str(), strExtension.c_str(), nBufSize);
					}
				}
			}
		}
		else
		{
			//	id3 has no cover, so add to cache 
			//	that it does not exist
			CUtil::ThumbCacheAdd(strCoverArt, false);
		}
	}

	return bResult;
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	try
	{
		// retrieve the ID3 Tag info from strFileName
		// and put it in tag
		bool bResult=false;
	//	CSectionLoader::Load("LIBID3");
		tag.SetURL(strFileName);
		CFile file;
		if ( file.Open( strFileName.c_str() ) ) 
		{
			//	Do not use ID3TT_ALL, because
			//	id3lib reads the ID3V1 tag first
			//	then ID3V2 tag is blocked.
			ID3_XIStreamReader reader( file );
			ID3_Tag myTag;
			if ( myTag.Link(reader, ID3TT_ID3V2) >= 0)
			{
				if ( !(bResult = ReadTag( myTag, tag )) ) 
				{
					myTag.Clear();
					if ( myTag.Link(reader, ID3TT_ID3V1 ) >= 0 ) 
					{
						bResult = ReadTag( myTag, tag );
					}
				}
//				if (bResult)
					tag.SetDuration(ReadDuration(file, myTag));
			}
			file.Close();
		}

		//	CSectionLoader::Unload("LIBID3");
		return bResult;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Tag loader mp3: exception in file %s", strFileName.c_str());
	}

	tag.SetLoaded(false);
	return false;
}

/* check if 'head' is a valid mp3 frame header */
bool CMusicInfoTagLoaderMP3::IsMp3FrameHeader(unsigned long head)
{
    if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
        return false;
    if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
        return false;
    if (!(head & LAYER_MASK)) /* no layer? */
        return false;
    if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
        return false;
    if (!(head & BITRATE_MASK)) /* no bitrate? */
        return false;
    if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
        return false;
    if (((head >> 19) & 1) == 1 &&
        ((head >> 17) & 3) == 3 &&
        ((head >> 16) & 1) == 1)
        return false;
    if ((head & 0xffff0000) == 0xfffe0000)
        return false;
    
    return true;
}

//	Inspired by http://rockbox.haxx.se/ and http://www.xs4all.nl/~rwvtveer/scilla 
int CMusicInfoTagLoaderMP3::ReadDuration(CFile& file, const ID3_Tag& id3tag)
{
	int nDuration=0;
	int nPrependedBytes=0;
	unsigned char* xing;
	unsigned char* vbri;
	unsigned char buffer[8193];

	/* Make sure file has a ID3v2 tag */
	file.Seek(0, SEEK_SET);
	file.Read(buffer, 6);

	if (buffer[0]=='I' && 
			buffer[1]=='D' && 
			buffer[2]=='3')
	{
		/* Now check what the ID3v2 size field says */
		file.Read(buffer, 4);
		nPrependedBytes = UNSYNC(buffer[0], buffer[1], buffer[2], buffer[3]) + 10;
	}

	//raw mp3Data = FileSize - ID3v1 tag - ID3v2 tag
	int nMp3DataSize=id3tag.GetFileSize()-id3tag.GetAppendedBytes()-nPrependedBytes;

	const int freqtab[][4] =
	{
			{11025, 12000, 8000, 0},  /* MPEG version 2.5 */
			{44100, 48000, 32000, 0}, /* MPEG Version 1 */
			{22050, 24000, 16000, 0}, /* MPEG version 2 */
	};

	// Skip ID3V2 tag when reading mp3 data
	file.Seek(nPrependedBytes, SEEK_SET);
	file.Read(buffer, 8192);

	int frequency=0, bitrate=0, bittable=0;
	int frame_count=0;
	double tpf=0.0, bpf=0.0;
	for (int i=0; i<8192; i++)
	{
		unsigned long mpegheader=(unsigned long)(
																	( (buffer[i] & 255) << 24) |
																	( (buffer[i+1] & 255) << 16) |
																	( (buffer[i+2] & 255) <<  8) |
																	( (buffer[i+3] & 255)      )
																); 

		//	Do we have a Xing header before the first mpeg frame?
		if (buffer[i  ]=='X' &&
				buffer[i+1]=='i' &&
				buffer[i+2]=='n' &&
				buffer[i+3]=='g')
		{
			if(buffer[i+7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
			{
					frame_count = BYTES2INT(buffer[i+8], buffer[i+8+1], buffer[i+8+2], buffer[i+8+3]);
			}
		}

		if (IsMp3FrameHeader(mpegheader))
		{
			//	skip mpeg header
			i+=4;
			int version=0;
			/* MPEG Audio Version */
			switch(mpegheader & VERSION_MASK) {
			case 0:
					/* MPEG version 2.5 is not an official standard */
					version = MPEG_VERSION2_5;
					bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
					break;
		      
			case (1 << 19):
					return 0;
		      
			case (2 << 19):
					/* MPEG version 2 (ISO/IEC 13818-3) */
					version = MPEG_VERSION2;
					bittable = MPEG_VERSION2 - 1;
					break;
		      
			case (3 << 19):
					/* MPEG version 1 (ISO/IEC 11172-3) */
					version = MPEG_VERSION1;
					bittable = MPEG_VERSION1 - 1;
					break;
			}

			int layer=0;
			switch(mpegheader & LAYER_MASK)
			{
			case (3 << 17):	//	LAYER_I
				layer=1;
				break;
			case (2 << 17):	//	LAYER_II
				layer=2;
				break;
			case (1 << 17):	//	LAYER_III
				layer=3;
				break;
			}

			/* Table of bitrates for MP3 files, all values in kilo.
			* Indexed by version, layer and value of bit 15-12 in header.
			*/
			const int bitrate_table[2][4][16] =
			{
					{
							{0},
							{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
							{0,32,48,56, 64,80, 96, 112,128,160,192,224,256,320,384,0},
							{0,32,40,48, 56,64, 80, 96, 112,128,160,192,224,256,320,0}
					},
					{
							{0},
							{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
							{0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0},
							{0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0}
					}
			};

			/* Bitrate */
			int bitindex = (mpegheader & 0xf000) >> 12;
			int freqindex = (mpegheader & 0x0C00) >> 10;
			bitrate = bitrate_table[bittable][layer][bitindex];
  
			/* Calculate bytes per frame, calculation depends on layer */
			switch(layer) {
			case 1:
					bpf = bitrate;
					bpf *= 48000;
					bpf /= freqtab[version][freqindex] << (version-1);
					break;
			case 2:
			case 3:
					bpf = bitrate;
					bpf *= 144000;
					bpf /= freqtab[version][freqindex] << (version-1);
					break;
			default:
					bpf = 1;
			}
			double tpfbs[] = { 0, 384.0f, 1152.0f, 1152.0f };
			frequency = freqtab[version][freqindex];
			tpf=tpfbs[layer] / (double) frequency;
			if (version==MPEG_VERSION2_5 && version==MPEG_VERSION2)
				tpf/=2;

			if(frequency == 0)
				return 0;

			/* Channel mode (stereo/mono) */
			int chmode = (mpegheader & 0xc0) >> 6;
			/* calculate position of Xing VBR header */
			if (version == MPEG_VERSION1) {
				if (chmode == 3) /* mono */
					xing = buffer + i + 17;
				else
					xing = buffer + i + 32;
			}
			else {
				if (chmode == 3) /* mono */
					xing = buffer + i + 9;
				else
					xing = buffer + i + 17;
			}

			/* calculate position of VBRI header */
			vbri = buffer + i + 32;

			//	Do we have a Xing header
			if (xing[0]=='X' &&
					xing[1]=='i' &&
					xing[2]=='n' &&
					xing[3]=='g')
			{
				if(xing[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
				{
						frame_count = BYTES2INT(xing[8], xing[8+1], xing[8+2], xing[8+3]);
				}
			}
			if (vbri[0]=='V' &&
					vbri[1]=='B' &&
					vbri[2]=='R' &&
					vbri[3]=='I')
			{
						frame_count = BYTES2INT(vbri[14], vbri[14+1],
																		vbri[14+2], vbri[14+3]);
			}
			//	We are done!
			break;
		}
	}

	//	Calculate duration if we have a Xing/VBRI VBR file
	if (frame_count > 0)
	{
		double d=tpf * frame_count;
		return (int)d;
	}

	//	Normal mp3 with constant bitrate duration
	//	Now song length is (filesize without id3v1/v2 tag)/((bitrate)/(8)) 
	double d=(double)(nMp3DataSize / ((bitrate*1000) / 8));
	return (int)d;
}

CStdString CMusicInfoTagLoaderMP3::ParseMP3Genre (const CStdString& str)
{
	CStdString strTemp = str;
	vector<CStdString> vecGenres;

	while (! strTemp.IsEmpty())
	{
		// remove any leading spaces
		int i = strTemp.find_first_not_of(" ");
		if (i > 0) strTemp.erase(0,i);

		// pull off the first character
		char p = strTemp[0];

		// start off looking for (something)
		if (p == '(')
		{
			strTemp.erase(0,1);

			// now look for ((something))
			p = strTemp[0];
			if (p == '(')
			{
				// remove ((something))
				i = strTemp.find_first_of("))");
				strTemp.erase(0,i+2);
			}
		}

		// no parens, so we have a start of a string
		// push chars into temp string until valid terminator found
		// valid terminators are ( or , or ;
		else
		{
			CStdString t;
			while ((p != ')') && (p != ',') && (p != ';') && (! strTemp.IsEmpty()))
			{
				strTemp.erase(0,1);
				t.push_back(p);
				p = strTemp[0];
			}
			// loop exits when terminator is found
			// be sure to remove the terminator
			strTemp.erase(0,1);

			// remove any trailing space from temp string
			p = t[(t.size()-1)];
			while (p == ' ')
			{
				t.erase((t.size()-1),1);
				p = t[(t.size()-1)];
			}

			// if the temp string is natural number try to convert it to a genre string
			if (CUtil::IsNaturalNumber(t))
			{
				char * pEnd;
				long l = strtol(t.c_str(),&pEnd,0);
				if (l < ID3_NR_OF_V1_GENRES)
				{
					// convert to genre string
					t = ID3_v1_genre_description[l];
				}
			}

			// convert RX to REMIX as per ID3 V2 spec
			else if ((t == "RX") || (t == "Rx") || (t == "rX") || (t == "rx"))
			{
				t = "REMIX";
			}

			// convert CR to COVER as per ID3 V2 spec
			else if ((t == "CR") || (t == "Cr") || (t == "cR") || (t == "cr"))
			{
				t = "COVER";
			}

			// check for duplicates in the genre vector
			// if no duplicates, push current temp string into vector
			bool bDuplicate = false;
			vector<CStdString>::iterator it;
			for (it = vecGenres.begin(); it < vecGenres.end(); it++)
			{
				CStdString strGenre = *it;
				if (strGenre == t)
				{
					bDuplicate = true;
					it = vecGenres.end();
				}
			}
			if (! bDuplicate) vecGenres.push_back(t);
		}

	}

	// finally return the / seperated string
	CStdString strGenre;
	vector<CStdString>::iterator it;
	for(it = vecGenres.begin(); it < vecGenres.end(); it++)
	{
		CStdString strTemp = *it;
		strGenre += strTemp + "/";
	}
	strGenre.TrimRight("/");
	return strGenre;
}
