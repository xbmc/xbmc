/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __RTPHINT_INCLUDED__
#define __RTPHINT_INCLUDED__

// forward declarations
class MP4RtpHintTrack;
class MP4RtpHint;
class MP4RtpPacket;

class MP4RtpData : public MP4Container {
public:
	MP4RtpData(MP4RtpPacket* pPacket);

	MP4RtpPacket* GetPacket() {
		return m_pPacket;
	}

	virtual u_int16_t GetDataSize() = 0;
	virtual void GetData(u_int8_t* pDest) = 0;

	MP4Track* FindTrackFromRefIndex(u_int8_t refIndex);

	virtual void WriteEmbeddedData(MP4File* pFile, u_int64_t startPos) {
		// default is no-op
	}

protected:
	MP4RtpPacket* m_pPacket;
};

MP4ARRAY_DECL(MP4RtpData, MP4RtpData*)

class MP4RtpNullData : public MP4RtpData {
public:
	MP4RtpNullData(MP4RtpPacket* pPacket);

	u_int16_t GetDataSize() {
		return 0;
	}

	void GetData(u_int8_t* pDest) {
		// no-op
	}
};

class MP4RtpImmediateData : public MP4RtpData {
public:
	MP4RtpImmediateData(MP4RtpPacket* pPacket);

	void Set(const u_int8_t* pBytes, u_int8_t numBytes);

	u_int16_t GetDataSize();

	void GetData(u_int8_t* pDest);
};

class MP4RtpSampleData : public MP4RtpData {
public:
	MP4RtpSampleData(MP4RtpPacket* pPacket);

	void SetEmbeddedImmediate(
		MP4SampleId sampleId, 
		u_int8_t* pData, u_int16_t dataLength);

	void SetReferenceSample(
		MP4SampleId refSampleId, u_int32_t refSampleOffset, 
		u_int16_t sampleLength);

	void SetEmbeddedSample(
		MP4SampleId sampleId, MP4Track* pRefTrack, 
		MP4SampleId refSampleId, u_int32_t refSampleOffset, 
		u_int16_t sampleLength);

	u_int16_t GetDataSize();

	void GetData(u_int8_t* pDest);

	void WriteEmbeddedData(MP4File* pFile, u_int64_t startPos);

protected:
	u_int8_t*		m_pRefData;

	MP4Track*		m_pRefTrack;
	MP4SampleId		m_refSampleId;
	u_int32_t		m_refSampleOffset;
};

class MP4RtpSampleDescriptionData : public MP4RtpData {
public:
	MP4RtpSampleDescriptionData(MP4RtpPacket* pPacket);

	void Set(u_int32_t sampleDescrIndex,
		u_int32_t offset, u_int16_t length);

	u_int16_t GetDataSize();

	void GetData(u_int8_t* pDest);
};

class MP4RtpPacket : public MP4Container {
public:
	MP4RtpPacket(MP4RtpHint* pHint);

	~MP4RtpPacket();

	void AddExtraProperties();

	MP4RtpHint* GetHint() {
		return m_pHint;
	}

	void Set(u_int8_t payloadNumber, u_int32_t packetId, bool setMbit);

	int32_t GetTransmitOffset();

	bool GetPBit();

	bool GetXBit();

	bool GetMBit();

	u_int8_t GetPayload();

	u_int16_t GetSequenceNumber();

	void SetTransmitOffset(int32_t transmitOffset);

	bool IsBFrame();

	void SetBFrame(bool isBFrame);

	void SetTimestampOffset(u_int32_t timestampOffset);

	void AddData(MP4RtpData* pData);

	u_int32_t GetDataSize();

	void GetData(u_int8_t* pDest);

	void Read(MP4File* pFile);

	void ReadExtra(MP4File* pFile);

	void Write(MP4File* pFile);

	void WriteEmbeddedData(MP4File* pFile, u_int64_t startPos);

	void Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits);

protected:
	MP4RtpHint*			m_pHint;
	MP4RtpDataArray		m_rtpData;
};

MP4ARRAY_DECL(MP4RtpPacket, MP4RtpPacket*)

class MP4RtpHint : public MP4Container {
public:
	MP4RtpHint(MP4RtpHintTrack* pTrack);

	~MP4RtpHint();

	MP4RtpHintTrack* GetTrack() {
		return m_pTrack;
	}

	u_int16_t GetNumberOfPackets() {
		return m_rtpPackets.Size();
	}

	bool IsBFrame() {
		return m_isBFrame;
	}
	void SetBFrame(bool isBFrame) {
		m_isBFrame = isBFrame;
	}

	u_int32_t GetTimestampOffset() {
		return m_timestampOffset;
	}
	void SetTimestampOffset(u_int32_t timestampOffset) {
		m_timestampOffset = timestampOffset;
	}

	MP4RtpPacket* AddPacket();

	MP4RtpPacket* GetPacket(u_int16_t index) {
		return m_rtpPackets[index];
	}

	MP4RtpPacket* GetCurrentPacket() {
		if (m_rtpPackets.Size() == 0) {
			return NULL;
		}
		return m_rtpPackets[m_rtpPackets.Size() - 1];
	}

	void Read(MP4File* pFile);

	void Write(MP4File* pFile);

	void Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits);

protected:
	MP4RtpHintTrack*	m_pTrack;
	MP4RtpPacketArray	m_rtpPackets;

	// values when adding packets to a hint (write mode)
	bool 				m_isBFrame;
	u_int32_t 			m_timestampOffset;
};

class MP4RtpHintTrack : public MP4Track {
public:
	MP4RtpHintTrack(MP4File* pFile, MP4Atom* pTrakAtom);

	~MP4RtpHintTrack();

	void InitRefTrack();

	void InitPayload();

	void InitRtpStart();

	void InitStats();

	MP4Track* GetRefTrack() {
		InitRefTrack();
		return m_pRefTrack;
	}

	void GetPayload(
		char** ppPayloadName = NULL,
		u_int8_t* pPayloadNumber = NULL,
		u_int16_t* pMaxPayloadSize = NULL,
		char **ppEncodingParams = NULL);

	void SetPayload(
		const char* payloadName,
		u_int8_t payloadNumber,
		u_int16_t maxPayloadSize,
		const char *encoding_parms,
		bool add_rtpmap,
		bool add_mpeg4_esid);

	void ReadHint(
		MP4SampleId hintSampleId,
		u_int16_t* pNumPackets = NULL);

	u_int16_t GetHintNumberOfPackets();

	bool GetPacketBFrame(u_int16_t packetIndex);

	u_int16_t GetPacketTransmitOffset(u_int16_t packetIndex);

	void ReadPacket(
		u_int16_t packetIndex,
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes,
		u_int32_t ssrc,
		bool includeHeader = true,
		bool includePayload = true);

	MP4Timestamp GetRtpTimestampStart();

	void SetRtpTimestampStart(MP4Timestamp start);

	void AddHint(bool isBFrame, u_int32_t timestampOffset);

	void AddPacket(bool setMbit, int32_t transmitOffset = 0);

	void AddImmediateData(const u_int8_t* pBytes, u_int32_t numBytes);

	void AddSampleData(MP4SampleId sampleId,
		 u_int32_t dataOffset, u_int32_t dataLength);

	void AddESConfigurationPacket();

	void WriteHint(MP4Duration duration, bool isSyncSample);

	void FinishWrite();

protected:
	MP4Track*	m_pRefTrack;

	MP4StringProperty*		m_pRtpMapProperty;
	MP4Integer32Property*	m_pPayloadNumberProperty;
	MP4Integer32Property*	m_pMaxPacketSizeProperty;
	MP4Integer32Property*	m_pSnroProperty;
	MP4Integer32Property*	m_pTsroProperty;
	u_int32_t				m_rtpSequenceStart;
	u_int32_t				m_rtpTimestampStart;

	// reading
	MP4RtpHint*	m_pReadHint;
	u_int8_t*	m_pReadHintSample;
	u_int32_t	m_readHintSampleSize;
	MP4Timestamp m_readHintTimestamp;

	// writing
	MP4RtpHint*	m_pWriteHint;
	MP4SampleId	m_writeHintId;
	u_int32_t	m_writePacketId;

	// statistics
	// in trak.udta.hinf
	MP4Integer64Property*	m_pTrpy;
	MP4Integer64Property*	m_pNump;
	MP4Integer64Property*	m_pTpyl;
	MP4Integer32Property*	m_pMaxr;
	MP4Integer64Property*	m_pDmed;
	MP4Integer64Property*	m_pDimm;
	MP4Integer32Property*	m_pPmax;
	MP4Integer32Property*	m_pDmax;

	// in trak.mdia.minf.hmhd
	MP4Integer16Property*	m_pMaxPdu;
	MP4Integer16Property*	m_pAvgPdu;
	MP4Integer32Property*	m_pMaxBitRate;
	MP4Integer32Property*	m_pAvgBitRate;

	MP4Timestamp			m_thisSec;
	u_int32_t				m_bytesThisSec;
	u_int32_t				m_bytesThisHint;
	u_int32_t				m_bytesThisPacket;
};

#endif /* __RTPHINT_INCLUDED__ */
