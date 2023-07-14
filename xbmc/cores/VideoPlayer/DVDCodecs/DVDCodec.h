#pragma once

#include "cores/VideoPlayer/Process/ProcessInfo.h"

class CDVDStreamInfo;
class CDVDCodecOptions;

class CDVDCodec {
	public:
		explicit CDVDCodec(CProcessInfo& processInfo) : m_processInfo(processInfo) {}
				
		/**
		* Open the decoder, returns true on success
		* Decoders not capable of running multiple instances should return false in case
		* there is already a instance open
		*/
		virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

		/**
		* add data, decoder has to consume the entire packet
		* returns true if the packet was consumed or if resubmitting it is useless
		*/
		virtual bool AddData(const DemuxPacket& packet) = 0;
		
		 /**
		 * Reset the decoder.
		 * Should be the same as calling Dispose and Open after each other
		 */
		 virtual void Reset() = 0;

		 /**
		 * should return codecs name
		 */
         virtual const char* GetName() = 0;
	
	protected:
		CProcessInfo& m_processInfo;
};