/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDDemux.h"
#include "DVDCodecs/DVDCodecs.h"

void CDemuxStreamTeletext::GetStreamInfo(std::string& strInfo)
{
  strInfo = "Teletext Data Stream";
}

void CDemuxStreamAudio::GetStreamType(std::string& strInfo)
{
  char sInfo[64];

  if (codec == AV_CODEC_ID_AC3) strcpy(sInfo, "AC3 ");
  else if (codec == AV_CODEC_ID_DTS)
  {
#ifdef FF_PROFILE_DTS_HD_MA
    if (profile == FF_PROFILE_DTS_HD_MA)
      strcpy(sInfo, "DTS-HD MA ");
    else if (profile == FF_PROFILE_DTS_HD_HRA)
      strcpy(sInfo, "DTS-HD HRA ");
    else
#endif
      strcpy(sInfo, "DTS ");
  }
  else if (codec == AV_CODEC_ID_MP2) strcpy(sInfo, "MP2 ");
  else if (codec == AV_CODEC_ID_TRUEHD) strcpy(sInfo, "Dolby TrueHD ");
  else strcpy(sInfo, "");

  int ilocalChannels = iChannels;
  if(bExtendedStreamInfo)
      ilocalChannels = iExtendedChannels;
  
  if (ilocalChannels == 1) strcat(sInfo, "Mono");
  else if (ilocalChannels == 2) strcat(sInfo, "Stereo");
  else if (ilocalChannels == 3 && bExtendedStreamInfo) 
  {
    if(lfe_channel == PRESENT)
      strcat(sInfo, "2.1");
    else
      strcat(sInfo, "3.0");
  }
  else if (ilocalChannels == 6) strcat(sInfo, "5.1");
  else if (ilocalChannels == 8) strcat(sInfo, "7.1");
  else if (ilocalChannels != 0)
  {
    char temp[32];
    sprintf(temp, " %d%s", iChannels, "-chs");
    strcat(sInfo, temp);
  }
  strInfo = sInfo;
}

  void CDemuxStreamAudio::GetExtendedStreamInfo(AVCodecID codectype, pFrame pframe )
  {
    if(pframe == NULL)
      return;
    
    bool bset_default_value = true;
    if(codectype == AV_CODEC_ID_DTS)
    {
      if(this->Parse_dts_audio_header(pframe))
      {
        CLog::Log(LOGINFO, "%s : Correctly detected DTS-HD MA extended info", __FUNCTION__);
        bset_default_value = false;
      }
      else
      {
        CLog::Log(LOGERROR, "%s : DTS-HD MA extended info not correctly detected", __FUNCTION__);
      }
    }
    
    if(bset_default_value)
    {
      bExtendedStreamInfo = false;
      iExtendedChannels = 0;
      lfe_channel = UNKNOW;
      iExtendedSampleRate = 0;
      iExtendedResolution = 0;
    }
  }

  bool CDemuxStreamAudio::Parse_dts_audio_header(pFrame pframe)
  {
    CLog::Log(LOGINFO, "%s : Started DTS-HD MA extended info parsing", __FUNCTION__);
    
    if(pframe == NULL) //minimum FSIZE allowded
    {
      CLog::Log(LOGERROR, "%s : pframe should not be null", __FUNCTION__);
      return false;
    }
    
    if(pframe->size < 94) //minimum FSIZE allowded
    {
      CLog::Log(LOGERROR, "%s : Frame s ize is smaller then 94 (size : %d)", __FUNCTION__, pframe->size);
      return false;
    }
    
    bits_reader_t bitstream;
    CBitstreamConverter::bits_reader_set( &bitstream, pframe->data, pframe->size );
    int ibyteread = 0;
    
    // 1) CORE STREAM
    ////////////////////////
    
    //SYNC CORE = ExtractBits(32)
    uint32_t SYNC_CORE = CBitstreamConverter::read_bits( &bitstream, 32 ); ibyteread += 32;
    if(SYNC_CORE == 0x7ffe8001)
      CLog::Log(LOGDEBUG, "%s : Correctly detected CORE Sync Word : 0x%08x ", __FUNCTION__, SYNC_CORE);
    else
    {
      CLog::Log(LOGERROR, "%s : CORE Sync Word not detected : 0x%08x ", __FUNCTION__, SYNC_CORE);
      return false;
    }
 	//FTYPE = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1 ); ibyteread += 1;
	//SHORT = ExtractBits(5);
	CBitstreamConverter::skip_bits( &bitstream, 5 ); ibyteread += 5;
	//CPF = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1 ); ibyteread += 1;
	//NBLKS = ExtractBits(7);
	CBitstreamConverter::skip_bits( &bitstream, 7); ibyteread += 7;  
	//FSIZE = ExtractBits(14);
	uint16_t FSIZE = CBitstreamConverter::read_bits( &bitstream, 14 ) + 1; ibyteread += 14;
    CLog::Log(LOGDEBUG, "%s : Core Stream size (FSIZE)  is  : %d ", __FUNCTION__, FSIZE);
	
    int bcheckextendedstream = 0;
	if(FSIZE > pframe->size)
	{
      CLog::Log(LOGERROR, "%s : Core Stream size (FSIZE)  is bigger then frame size  : %d / %d", __FUNCTION__, FSIZE, pframe->size);
      return false;
	}
	else if (FSIZE == pframe->size)
      CLog::Log(LOGDEBUG, "%s : Core Stream size (FSIZE)  is the same as the frame size  (no extension stream) : %d / %d, ", __FUNCTION__, FSIZE, pframe->size);
	else //fsize is smaller then frame size so there is certainly an extension stream
	  bcheckextendedstream = 1;  
  
 	//AMODE = ExtractBits(6);
	CBitstreamConverter::skip_bits( &bitstream, 6); ibyteread += 6;
	//SFREQ = ExtractBits(4);
	CBitstreamConverter::skip_bits( &bitstream, 4); ibyteread += 4;
	//RATE = ExtractBits(5);
	CBitstreamConverter::skip_bits( &bitstream, 5); ibyteread += 5;
	//FixedBit = ExtractBit(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1;
	//DYNF = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1;
	//TIMEF = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1;
	//AUXF = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1;
	//HDCD = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1; 
  	//EXT_AUDIO_ID = ExtractBits(3);
	uint8_t EXT_AUDIO_ID = CBitstreamConverter::read_bits( &bitstream, 3 ); ibyteread += 3;
	//EXT_AUDIO = ExtractBits(1);
	uint8_t EXT_AUDIO = CBitstreamConverter::read_bits( &bitstream, 1 );  ibyteread += 1;
	//ASPF = ExtractBits(1);
	CBitstreamConverter::skip_bits( &bitstream, 1); ibyteread += 1;
	//LFF = ExtractBits(2);
	uint8_t LFF = CBitstreamConverter::read_bits( &bitstream, 2 );  ibyteread += 2;
		
	/// Lot more ........... but we don't need anything more for the moment
		
	// 2) CORE STREAM EXTENSIONS
	////////////////////////////
  
	//REM : for the moment core streams extensions are not handled
    //TODO : found a real case and implement this or even continue .... (not found a dts-hd ma with core extension ...))
	if(EXT_AUDIO == 1)
	{
      std::string Extension_id_txt;
      if(EXT_AUDIO_ID == 0)
        Extension_id_txt =  "Channel Extension (XCh)";
      else if(EXT_AUDIO_ID == 2)
        Extension_id_txt = "Frequency Extension (X96)";
      else if(EXT_AUDIO_ID == 6)
        Extension_id_txt = "Channel Extension (XXCH)";
      else
        Extension_id_txt = "Reserved";

      CLog::Log(LOGERROR, "%s : Core stream extension have been detected, those are not yet handled by xbmc at the moment. Extension type : %s", __FUNCTION__, Extension_id_txt.c_str());
      return false;
	}  
  
	// 3) EXTENSION SUBSTREAM
    ///////////////////////////////////////////

	if(bcheckextendedstream)
	{
      uint8_t nuBitResolution  = 0;
      uint8_t nuMaxSampleRate  = 0;
      uint8_t nuTotalNumChs  = 0;
      CLog::Log(LOGDEBUG, "%s : Parsing Extended substream", __FUNCTION__);
      if( pframe->size < FSIZE + 12) //Enough to get up to the extended frame size where further check is done 
      {
        CLog::Log(LOGERROR, "%s : Frame size is not big enough to get up to the extended stream size   : %d / %d", __FUNCTION__, FSIZE + 12, pframe->size);
        return false;
      }
      
      //jump to the beginning of the extension substream
      CBitstreamConverter::skip_bits(&bitstream, (FSIZE * 8)  - ibyteread); ibyteread = FSIZE * 8;
                
      //SYNC SUBSTREAM (32)
      uint32_t SYNCEXTSSH = CBitstreamConverter::read_bits( &bitstream, 32 ); ibyteread += 32;
      if(SYNCEXTSSH == 0x64582025)
      {
        CLog::Log(LOGDEBUG, "%s : Correctly detected SUBSTREAM Sync Word : 0x%08x ", __FUNCTION__, SYNCEXTSSH);
      }
      else
      {
  		CLog::Log(LOGERROR, "%s : SUBSTREAM Sync Word not detected : 0x%08x ", __FUNCTION__, SYNCEXTSSH);
        return false;
      }
      //UserDefinedBits = ExtractBits(8)
      CBitstreamConverter::skip_bits( &bitstream, 8); ibyteread += 8;
      //nExtSSIndex = ExtractBits(2);
      uint8_t nExtSSIndex  = CBitstreamConverter::read_bits( &bitstream, 2 );  ibyteread += 2;
      CLog::Log(LOGDEBUG, "%s : nExtSSIndex is %u", __FUNCTION__, nExtSSIndex);
      //bHeaderSizeType = ExtractBits(1);
      uint8_t bHeaderSizeType  = CBitstreamConverter::read_bits( &bitstream, 1 );  ibyteread += 1;
      CLog::Log(LOGDEBUG, "%s : bHeaderSizeType is %u", __FUNCTION__, bHeaderSizeType);
      uint32_t nuBits4Header = 0;
      uint32_t nuBits4ExSSFsize = 0;
      if (bHeaderSizeType == 0)
      {
        nuBits4Header = 8;
		nuBits4ExSSFsize = 16;
      }
      else
      {
        nuBits4Header = 12;
        nuBits4ExSSFsize = 20;
      }		          	
      //nuExtSSHeaderSize = ExtractBits(nuBits4Header) + 1; //Could be used for CRC checksum ...
      uint16_t nuExtSSHeaderSize  = CBitstreamConverter::read_bits( &bitstream, nuBits4Header) + 1;  ibyteread += nuBits4Header; 
      CLog::Log(LOGDEBUG, "%s : Extended stream header size (nuExtSSHeaderSize) : %u", __FUNCTION__, nuExtSSHeaderSize);
      //nuExtSSFsize = ExtractBits(nuBits4ExSSFsize) + 1;
      uint32_t nuExtSSFsize  = CBitstreamConverter::read_bits( &bitstream, nuBits4ExSSFsize ) + 1;  ibyteread += nuBits4ExSSFsize; 
      CLog::Log(LOGDEBUG, "%s : Extended stream  size (nuExtSSFsize) : %u", __FUNCTION__, nuExtSSFsize);
      if(nuExtSSFsize + FSIZE < (uint32_t)pframe->size)
      {
        CLog::Log(LOGERROR, "%s : Core stream + Substream length is bigger then frame size; (bade frame ???) : %d / %u / %d", __FUNCTION__,FSIZE, nuExtSSFsize, pframe->size);
        return false;
      }
      //REM : for the moment multiple substreams are not handled
      // TODO : found a real case and implement this or even continue .... (will be hard to find a real case on classic media ...))
      if(nuExtSSFsize + FSIZE > (uint32_t)pframe->size)
      {
        CLog::Log(LOGERROR, "%s : Core stream + Substream length is smaller then frame size, this usually mean that other extension streams are present but this is not handled by xbmc : %d / %u / %d - StreamIndex : %u", 
                          __FUNCTION__,FSIZE, nuExtSSFsize, pframe->size, nExtSSIndex);
        return false;
      }
      
      //bStaticFieldsPresent = ExtractBits(1);
      uint8_t bStaticFieldsPresent  = CBitstreamConverter::read_bits( &bitstream, 1) ;  ibyteread += 1; 
      CLog::Log(LOGDEBUG, "%s : bStaticFieldsPresent : %u", __FUNCTION__, bStaticFieldsPresent);

      uint8_t  nuNumAudioPresnt = -1;
      uint8_t  nuNumAssets = -1;
      
      //REM : for the moment substreams without bStaticFieldsPresent are not handled
      // TODO : found a real case and implement this or even continue ....
      if(!bStaticFieldsPresent)
      {
        //nuNumAudioPresnt = 1;
        //nuNumAssets = 1;
        CLog::Log(LOGERROR, "%s : Substream without bStaticFieldsPresent flag set can not be handled for the moment", __FUNCTION__);
      }
      else
      {
        //nuRefClockCode = ExtractBits(2);
		CBitstreamConverter::skip_bits( &bitstream, 2); ibyteread += 2;
		//nuExSSFrameDurationCode = 512*(ExtractBits(3)+1);
		CBitstreamConverter::skip_bits( &bitstream, 3); ibyteread += 3;
		//bTimeStampFlag = ExtractBits(1);
		uint8_t bTimeStampFlag  = CBitstreamConverter::read_bits( &bitstream, 1) ;  ibyteread += 1;
		CLog::Log(LOGDEBUG, "%s : bTimeStampFlag : %u", __FUNCTION__, bTimeStampFlag);
		if(bTimeStampFlag)
		{
          //nuTimeStamp = ExtractBits(32);
          CBitstreamConverter::skip_bits( &bitstream, 32); ibyteread += 32;
          //nLSB = ExtractBits(4);
          CBitstreamConverter::skip_bits( &bitstream, 4); ibyteread += 4;
		}
		//nuNumAudioPresnt = ExtractBits(3)+1;
		nuNumAudioPresnt  = CBitstreamConverter::read_bits( &bitstream, 3) + 1 ;  ibyteread += 3;
        CLog::Log(LOGDEBUG, "%s : nuNumAudioPresnt : %u", __FUNCTION__, nuNumAudioPresnt);
		//nuNumAssets = ExtractBits(3)+1;
		nuNumAssets  = CBitstreamConverter::read_bits( &bitstream, 3) + 1 ;  ibyteread += 3;					
		CLog::Log(LOGDEBUG, "%s : nuNumAssets : %u", __FUNCTION__, nuNumAssets);
		//nuActiveExSSMask & nuActiveAssetMask
		uint32_t* nuActiveExSSMask = (uint32_t *) malloc(nuNumAudioPresnt * sizeof(uint32_t)); //needed temporary too skip some bits
		for (int nAuPr=0; nAuPr<nuNumAudioPresnt; nAuPr++){
          nuActiveExSSMask[nAuPr] = CBitstreamConverter::read_bits( &bitstream, nExtSSIndex + 1) ;  ibyteread += (nExtSSIndex + 1);}
		for (int nAuPr=0; nAuPr<nuNumAudioPresnt; nAuPr++)
		{
          for (int nSS=0; nSS<nExtSSIndex+1; nSS++)
          {
            if (((nuActiveExSSMask[nAuPr]>>nSS) & 0x1) == 1){
              //nuActiveAssetMask[nAuPr][nSS] = ExtractBits(8);
              CBitstreamConverter::skip_bits( &bitstream, 8); ibyteread += 8; } //else //nuActiveAssetMask[nAuPr][nSS]= 0;
          }
		}
		free(nuActiveExSSMask);

		//bMixMetadataEnbl = ExtractBits(1);
		uint8_t bMixMetadataEnbl  = CBitstreamConverter::read_bits( &bitstream, 1) ;  ibyteread += 1;
		CLog::Log(LOGDEBUG, "%s : bMixMetadataEnbl : %u", __FUNCTION__, bMixMetadataEnbl);
		if (bMixMetadataEnbl)
		{
          //nuMixMetadataAdjLevel = ExtractBits(2);
          CBitstreamConverter::skip_bits( &bitstream, 2); ibyteread += 2; 
          //nuBits4MixOutMask = (ExtractBits(2)+1)<<2;
          uint8_t nuBits4MixOutMask  = (CBitstreamConverter::read_bits( &bitstream, 2) + 1) << 2 ;  ibyteread += 2;
          //nuNumMixOutConfigs = ExtractBits(2) + 1;
          uint8_t nuNumMixOutConfigs  = CBitstreamConverter::read_bits( &bitstream, 2) + 1 ;  ibyteread += 2; 
          // Output Mixing Configuration Loop
          for (int ns=0; ns<nuNumMixOutConfigs; ns++)
          {
            //nuMixOutChMask[ns]= ExtractBits(nBits4MixOutMask);
			CBitstreamConverter::skip_bits( &bitstream, nuBits4MixOutMask); ibyteread += nuBits4MixOutMask;  //nNumMixOutCh[ns] = NumSpkrTableLookUp(nuMixOutChMask[ns]);
          }
		}
      }//bStaticFieldsPresent

      //REM : for the moment substreams with multiple audio presentation and/or audio assets are not handled  
      // TODO : found a real case and implement this or even continue .... (may be available in blurays with additional director description content ...)
      if( nuNumAudioPresnt != 1 || nuNumAssets !=1 )
      {
        CLog::Log(LOGERROR, "%s : Substream with mutiple audio presentations and/or mumltiple audio assets are not yet handled by xbmc : %u / %u", __FUNCTION__, nuNumAudioPresnt, nuNumAssets);
        return false;
      } 
     
      for (int nAst=0; nAst< nuNumAssets; nAst++){
        //nuAssetFsize[nAst] = ExtractBits(nuBits4ExSSFsize)+1;
		CBitstreamConverter::skip_bits( &bitstream, nuBits4ExSSFsize); ibyteread += nuBits4ExSSFsize;}

      for (int nAst=0; nAst< nuNumAssets; nAst++)
      {
        //nuAssetDescriptFsize = ExtractBits(9)+1;
		CBitstreamConverter::skip_bits( &bitstream, 9); ibyteread += 9;
		//nuAssetIndex = ExtractBits(3);
		CBitstreamConverter::skip_bits( &bitstream, 3); ibyteread += 3;
		if (bStaticFieldsPresent)
		{
          //bAssetTypeDescrPresent = ExtractBits(1);
          uint8_t bAssetTypeDescrPresent  = CBitstreamConverter::read_bits( &bitstream, 1)  ;  ibyteread += 1;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - bAssetTypeDescrPresent : %u", __FUNCTION__, nAst, bAssetTypeDescrPresent);
          if (bAssetTypeDescrPresent){
            //nuAssetTypeDescriptor = ExtractBits(4);
            CBitstreamConverter::skip_bits( &bitstream, 4); ibyteread += 4;}

          //bLanguageDescrPresent = ExtractBits(1);
          uint8_t bLanguageDescrPresent  = CBitstreamConverter::read_bits( &bitstream, 1)  ;  ibyteread += 1;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - bLanguageDescrPresent : %u", __FUNCTION__, nAst, bLanguageDescrPresent);
          if (bLanguageDescrPresent){
            //LanguageDescriptor = ExtractBits(24);
			//unsigned int LanguageDescriptor  = read_bits( &bitstream, 24)  ;  ibyteread += 24;//not yet found in a real case ;(
			CBitstreamConverter::skip_bits( &bitstream, 24); ibyteread += 24;}

          //bInfoTextPresent = ExtractBits(1);
          uint8_t bInfoTextPresent  = CBitstreamConverter::read_bits( &bitstream, 1)  ;  ibyteread += 1;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - bInfoTextPresent : %u", __FUNCTION__, nAst, bInfoTextPresent);
          if (bInfoTextPresent)
          {
            //nuInfoTextByteSize = ExtractBits(10)+1;
			uint16_t nuInfoTextByteSize  = CBitstreamConverter::read_bits( &bitstream, 10) + 1 ;  ibyteread += 10;
			//InfoTextString = ExtractBits(nuInfoTextByteSize*8);
			CBitstreamConverter::skip_bits( &bitstream, nuInfoTextByteSize * 8); ibyteread += (nuInfoTextByteSize * 8);
          }
          
          //REM : we do not handle Substreams EXtension and take those fields for extended infos
          //Works pretty well for DTS_HD MA :)
          
          //nuBitResolution = ExtractBits(5) + 1;
          nuBitResolution  = CBitstreamConverter::read_bits( &bitstream, 5) + 1 ;  ibyteread += 5;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - nuBitResolution : %u", __FUNCTION__, nAst, nuBitResolution);
          //nuMaxSampleRate = ExtractBits(4)
          nuMaxSampleRate  = CBitstreamConverter::read_bits( &bitstream, 4) ;  ibyteread += 4;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - nuMaxSampleRate : %u", __FUNCTION__, nAst, nuMaxSampleRate);
          //nuTotalNumChs = ExtractBits(8)+1;
          nuTotalNumChs  = CBitstreamConverter::read_bits( &bitstream, 8) + 1 ;  ibyteread += 8;
          CLog::Log(LOGDEBUG, "%s : Asset :  %u - nuTotalNumChs : %u", __FUNCTION__, nAst, nuTotalNumChs);
            
          /// Lot more ........... but we don't need anything more for the moment
        }//bStaticFieldsPresent
        /// Lot more ........... but we don't need anything more for the moment
      }//for (int nAst=0; nAst< nuNumAssets; nAst++)
      /// Lot more ........... but we don't need anything more for the moment
      
      //TODO : Move the crc verification upper ...
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      int bits_to_crc = ((FSIZE + nuExtSSHeaderSize) * 8)  - ibyteread - 16;
      CBitstreamConverter::skip_bits( &bitstream, bits_to_crc); ibyteread += bits_to_crc;
      //nCRC16ExtSSHeader = ExtractBits(16);
      uint16_t nCRC16ExtSSHeader  = CBitstreamConverter::read_bits( &bitstream, 16) ;  ibyteread += 16;
      const AVCRC *ctx;
      ctx = av_crc_get_table(AV_CRC_16_CCITT);	
      uint16_t crc =  av_crc(ctx, 0xffff, pframe->data + FSIZE + 5 ,  nuExtSSHeaderSize - 2 - 5);
      crc = ((crc << 8) & 0xFF00) + (crc >> 8); //crc = htons(crc);
      CLog::Log(LOGDEBUG, "%s : calculated CRC16 checksum : %04x", __FUNCTION__, crc);
      if( nCRC16ExtSSHeader != crc)
      {
        CLog::Log(LOGERROR, "%s : CRC16 checksum mismatch %04x / %04x", __FUNCTION__, nCRC16ExtSSHeader, crc);
        return false;
      }
      else
		CLog::Log(LOGDEBUG, "%s : Checksum verification succeded", __FUNCTION__);
      
      bExtendedStreamInfo = true;
      iExtendedChannels = nuTotalNumChs;  
      iExtendedResolution = nuBitResolution;
      iExtendedSampleRate = DTS_HD_MaxSampleRate[nuMaxSampleRate];
      
      if(LFF == 0)
        lfe_channel = NOT_PRESENT;
      else if(LFF == 1 || LFF == 2)
        lfe_channel = PRESENT;
      else
        lfe_channel = UNKNOW;
      
    }//bcheckextendedstream   

    return true;
  }
  
int CDVDDemux::GetNrOfAudioStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_AUDIO) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfVideoStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_VIDEO) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfSubtitleStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_SUBTITLE) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfTeletextStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_TELETEXT) iCounter++;
  }

  return iCounter;
}

CDemuxStreamAudio* CDVDDemux::GetStreamFromAudioId(int iAudioIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_AUDIO) counter++;
    if (iAudioIndex == counter)
      return (CDemuxStreamAudio*)pStream;
  }
  return NULL;
}

CDemuxStreamVideo* CDVDDemux::GetStreamFromVideoId(int iVideoIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_VIDEO) counter++;
    if (iVideoIndex == counter)
      return (CDemuxStreamVideo*)pStream;
  }
  return NULL;
}

CDemuxStreamSubtitle* CDVDDemux::GetStreamFromSubtitleId(int iSubtitleIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_SUBTITLE) counter++;
    if (iSubtitleIndex == counter)
      return (CDemuxStreamSubtitle*)pStream;
  }
  return NULL;
}

CDemuxStreamTeletext* CDVDDemux::GetStreamFromTeletextId(int iTeletextIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_TELETEXT) counter++;
    if (iTeletextIndex == counter)
      return (CDemuxStreamTeletext*)pStream;
  }
  return NULL;
}

void CDemuxStream::GetStreamName( std::string& strInfo )
{
  strInfo = "";
}

AVDiscard CDemuxStream::GetDiscard()
{
  return AVDISCARD_NONE;
}

void CDemuxStream::SetDiscard(AVDiscard discard)
{
  return;
}

