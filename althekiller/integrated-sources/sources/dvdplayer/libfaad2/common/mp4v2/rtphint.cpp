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

#include "mp4common.h"

/* rtp hint track operations */

MP4RtpHintTrack::MP4RtpHintTrack(MP4File* pFile, MP4Atom* pTrakAtom)
	: MP4Track(pFile, pTrakAtom)
{
	m_pRefTrack = NULL;

	m_pRtpMapProperty = NULL;
	m_pPayloadNumberProperty = NULL;
	m_pMaxPacketSizeProperty = NULL;
	m_pSnroProperty = NULL;
	m_pTsroProperty = NULL;

	m_pReadHint = NULL;
	m_pReadHintSample = NULL;
	m_readHintSampleSize = 0;

	m_pWriteHint = NULL;
	m_writeHintId = MP4_INVALID_SAMPLE_ID;
	m_writePacketId = 0;

	m_pTrpy = NULL;
	m_pNump = NULL;
	m_pTpyl = NULL;
	m_pMaxr = NULL;
	m_pDmed = NULL;
	m_pDimm = NULL;
	m_pPmax = NULL;
	m_pDmax = NULL;

	m_pMaxPdu = NULL;
	m_pAvgPdu = NULL;
	m_pMaxBitRate = NULL;
	m_pAvgBitRate = NULL;

	m_thisSec = 0;
	m_bytesThisSec = 0;
	m_bytesThisHint = 0;
	m_bytesThisPacket = 0;
}

MP4RtpHintTrack::~MP4RtpHintTrack()
{
	delete m_pReadHint;
	delete m_pReadHintSample;
	delete m_pWriteHint;
}

void MP4RtpHintTrack::InitRefTrack()
{
	if (m_pRefTrack == NULL) {
		MP4Integer32Property* pRefTrackIdProperty = NULL;
		m_pTrakAtom->FindProperty(
			"trak.tref.hint.entries[0].trackId",
			(MP4Property**)&pRefTrackIdProperty);
		ASSERT(pRefTrackIdProperty);

		m_pRefTrack = m_pFile->GetTrack(pRefTrackIdProperty->GetValue());
	}
}

void MP4RtpHintTrack::InitRtpStart() 
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srandom((tv.tv_usec << 12) | (tv.tv_sec & 0xFFF));

	ASSERT(m_pTrakAtom);

	m_pTrakAtom->FindProperty(
		"trak.udta.hnti.rtp .snro.offset",
		(MP4Property**)&m_pSnroProperty);

	if (m_pSnroProperty) {
		m_rtpSequenceStart = m_pSnroProperty->GetValue();
	} else {
		m_rtpSequenceStart = random();
	}

	m_pTrakAtom->FindProperty(
		"trak.udta.hnti.rtp .tsro.offset",
		(MP4Property**)&m_pTsroProperty);

	if (m_pTsroProperty) {
		m_rtpTimestampStart = m_pTsroProperty->GetValue();
	} else {
		m_rtpTimestampStart = random();
	}
}

void MP4RtpHintTrack::ReadHint(
	MP4SampleId hintSampleId,
	u_int16_t* pNumPackets)
{
	if (m_pRefTrack == NULL) {
		InitRefTrack();
		InitRtpStart();
	}

	// dispose of any old hint
	delete m_pReadHint;
	m_pReadHint = NULL;
	delete m_pReadHintSample;
	m_pReadHintSample = NULL;
	m_readHintSampleSize = 0;

	// read the desired hint sample into memory
	ReadSample(
		hintSampleId, 
		&m_pReadHintSample, 
		&m_readHintSampleSize,
		&m_readHintTimestamp);

	m_pFile->EnableMemoryBuffer(m_pReadHintSample, m_readHintSampleSize);

	m_pReadHint = new MP4RtpHint(this);
	m_pReadHint->Read(m_pFile);

	m_pFile->DisableMemoryBuffer();

	if (pNumPackets) {
		*pNumPackets = GetHintNumberOfPackets();
	}
}

u_int16_t MP4RtpHintTrack::GetHintNumberOfPackets()
{
	if (m_pReadHint == NULL) {
		throw new MP4Error("no hint has been read",
			"MP4GetRtpHintNumberOfPackets");
	}
	return m_pReadHint->GetNumberOfPackets();
}

bool MP4RtpHintTrack::GetPacketBFrame(u_int16_t packetIndex)
{
	if (m_pReadHint == NULL) {
		throw new MP4Error("no hint has been read",
			"MP4GetRtpPacketBFrame");
	}
	MP4RtpPacket* pPacket =
		m_pReadHint->GetPacket(packetIndex);

	return pPacket->IsBFrame();
}

u_int16_t MP4RtpHintTrack::GetPacketTransmitOffset(u_int16_t packetIndex)
{
	if (m_pReadHint == NULL) {
		throw new MP4Error("no hint has been read",
			"MP4GetRtpPacketTransmitOffset");
	}

	MP4RtpPacket* pPacket =
		m_pReadHint->GetPacket(packetIndex);

	return pPacket->GetTransmitOffset();
}

void MP4RtpHintTrack::ReadPacket(
	u_int16_t packetIndex,
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes,
	u_int32_t ssrc,
	bool addHeader,
	bool addPayload)
{
	if (m_pReadHint == NULL) {
		throw new MP4Error("no hint has been read",
			"MP4ReadRtpPacket");
	}
	if (!addHeader && !addPayload) {
		throw new MP4Error("no data requested",
			"MP4ReadRtpPacket");
	}

	MP4RtpPacket* pPacket =
		m_pReadHint->GetPacket(packetIndex);

	*pNumBytes = 0;
	if (addHeader) {
		*pNumBytes += 12;
	}
	if (addPayload) {
		*pNumBytes += pPacket->GetDataSize();
	}

	// if needed, allocate the packet memory
	bool buffer_malloc = false;

	if (*ppBytes == NULL) {
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		buffer_malloc = true;
	}

	try {
		u_int8_t* pDest = *ppBytes;

		if (addHeader) {
			*pDest++ =
				0x80 | (pPacket->GetPBit() << 5) | (pPacket->GetXBit() << 4);

			*pDest++ =
				(pPacket->GetMBit() << 7) | pPacket->GetPayload();

			*((u_int16_t*)pDest) = 
				htons(m_rtpSequenceStart + pPacket->GetSequenceNumber());
			pDest += 2; 

			*((u_int32_t*)pDest) = 
				htonl(m_rtpTimestampStart + (u_int32_t)m_readHintTimestamp);
			pDest += 4; 

			*((u_int32_t*)pDest) = 
				htonl(ssrc);
			pDest += 4;
		}

		if (addPayload) {
			pPacket->GetData(pDest);
		}
	}
	catch (MP4Error* e) {
		if (buffer_malloc) {
			MP4Free(*ppBytes);
			*ppBytes = NULL;
		}
		throw e;
	}

	VERBOSE_READ_HINT(m_pFile->GetVerbosity(),
		printf("ReadPacket: %u ", packetIndex);
		MP4HexDump(*ppBytes, *pNumBytes););
}

MP4Timestamp MP4RtpHintTrack::GetRtpTimestampStart()
{
	if (m_pRefTrack == NULL) {
		InitRefTrack();
		InitRtpStart();
	}

	return m_rtpTimestampStart;
}

void MP4RtpHintTrack::SetRtpTimestampStart(MP4Timestamp start)
{
	if (!m_pTsroProperty) {
		MP4Atom* pTsroAtom =
			m_pFile->AddDescendantAtoms(m_pTrakAtom, "udta.hnti.rtp .tsro");

		ASSERT(pTsroAtom);

		pTsroAtom->FindProperty("offset",
			(MP4Property**)&m_pTsroProperty);

		ASSERT(m_pTsroProperty);
	}

	m_pTsroProperty->SetValue(start);
	m_rtpTimestampStart = start;
}

void MP4RtpHintTrack::InitPayload()
{
	ASSERT(m_pTrakAtom);

	if (m_pRtpMapProperty == NULL) {
		m_pTrakAtom->FindProperty(
			"trak.udta.hinf.payt.rtpMap",
			(MP4Property**)&m_pRtpMapProperty);
	}

	if (m_pPayloadNumberProperty == NULL) {
		m_pTrakAtom->FindProperty(
			"trak.udta.hinf.payt.payloadNumber",
			(MP4Property**)&m_pPayloadNumberProperty);
	}

	if (m_pMaxPacketSizeProperty == NULL) {
		m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stsd.rtp .maxPacketSize",
			(MP4Property**)&m_pMaxPacketSizeProperty);
	}
}

void MP4RtpHintTrack::GetPayload(
	char** ppPayloadName,
	u_int8_t* pPayloadNumber,
	u_int16_t* pMaxPayloadSize,
	char **ppEncodingParams)
{
	InitPayload();

	if (ppPayloadName || ppEncodingParams) {
	  if (ppPayloadName) 
	    *ppPayloadName = NULL;
	  if (ppEncodingParams)
	    *ppEncodingParams = NULL;
		if (m_pRtpMapProperty) {
			const char* pRtpMap = m_pRtpMapProperty->GetValue();
			char* pSlash = strchr(pRtpMap, '/');

			u_int32_t length;
			if (pSlash) {
				length = pSlash - pRtpMap;
			} else {
				length = strlen(pRtpMap);
			}

			if (ppPayloadName) {
			  *ppPayloadName = (char*)MP4Calloc(length + 1);
			  strncpy(*ppPayloadName, pRtpMap, length); 
			}
			if (pSlash && ppEncodingParams) {
			  pSlash = strchr(pSlash, '/');
			  if (pSlash != NULL) {
			    pSlash++;
			    if (pSlash != '\0') {
			      length = strlen(pRtpMap) - (pSlash - pRtpMap);
			      *ppEncodingParams = (char *)MP4Calloc(length + 1);
			      strncpy(*ppEncodingParams, pSlash, length);
			    }
			  }
			}
		} 
	}

	if (pPayloadNumber) {
		if (m_pPayloadNumberProperty) {
			*pPayloadNumber = m_pPayloadNumberProperty->GetValue();
		} else {
			*pPayloadNumber = 0;
		}
	}

	if (pMaxPayloadSize) {
		if (m_pMaxPacketSizeProperty) {
			*pMaxPayloadSize = m_pMaxPacketSizeProperty->GetValue();
		} else {
			*pMaxPayloadSize = 0;
		}
	}
}

void MP4RtpHintTrack::SetPayload(
	const char* payloadName,
	u_int8_t payloadNumber,
	u_int16_t maxPayloadSize, 
	const char *encoding_parms,
	bool include_rtp_map,
	bool include_mpeg4_esid)
{
	InitRefTrack();
	InitPayload();

	ASSERT(m_pRtpMapProperty);
	ASSERT(m_pPayloadNumberProperty);
	ASSERT(m_pMaxPacketSizeProperty);
	
	size_t len = strlen(payloadName) + 16;
	if (encoding_parms != NULL) {
	  size_t temp = strlen(encoding_parms);
	  if (temp == 0) {
	    encoding_parms = NULL;
	  } else {
	    len += temp;
	  }
	}

	char* rtpMapBuf = (char*)MP4Malloc(len);
	sprintf(rtpMapBuf, "%s/%u%c%s", 
		payloadName, 
		GetTimeScale(),
		encoding_parms != NULL ? '/' : '\0',
		encoding_parms == NULL ? "" : encoding_parms);
	m_pRtpMapProperty->SetValue(rtpMapBuf);
	
	m_pPayloadNumberProperty->SetValue(payloadNumber);

	if (maxPayloadSize == 0) {
		maxPayloadSize = 1460;
	} 
	m_pMaxPacketSizeProperty->SetValue(maxPayloadSize);

	// set sdp media type
	const char* sdpMediaType;
	if (!strcmp(m_pRefTrack->GetType(), MP4_AUDIO_TRACK_TYPE)) {
		sdpMediaType = "audio";
	} else if (!strcmp(m_pRefTrack->GetType(), MP4_VIDEO_TRACK_TYPE)) {
		sdpMediaType = "video";
	} else {
		sdpMediaType = "application";
	}

	char* sdpBuf = (char*)MP4Malloc(
		strlen(sdpMediaType) + strlen(rtpMapBuf) + 256);
	uint32_t buflen;
	buflen = sprintf(sdpBuf, 
			 "m=%s 0 RTP/AVP %u\015\012"
			 "a=control:trackID=%u\015\012",
			 sdpMediaType, payloadNumber,
			 m_trackId);
	if (include_rtp_map) {
	  buflen += sprintf(sdpBuf + buflen, 
			    "a=rtpmap:%u %s\015\012",
			    payloadNumber, rtpMapBuf);
	}
	if (include_mpeg4_esid) {
	  sprintf(sdpBuf + buflen, 
		  "a=mpeg4-esid:%u\015\012",
		  m_pRefTrack->GetId());
	}

	MP4StringProperty* pSdpProperty = NULL;
	m_pTrakAtom->FindProperty("trak.udta.hnti.sdp .sdpText",
		(MP4Property**)&pSdpProperty);
	ASSERT(pSdpProperty);
	pSdpProperty->SetValue(sdpBuf);

	// cleanup
	MP4Free(rtpMapBuf);
	MP4Free(sdpBuf);
}

void MP4RtpHintTrack::AddHint(bool isBFrame, u_int32_t timestampOffset)
{
	// on first hint, need to lookup the reference track
	if (m_writeHintId == MP4_INVALID_SAMPLE_ID) {
		InitRefTrack();
		InitStats();
	}

	if (m_pWriteHint) {
		throw new MP4Error("unwritten hint is still pending", "MP4AddRtpHint");
	}

	m_pWriteHint = new MP4RtpHint(this);
	m_pWriteHint->SetBFrame(isBFrame);
	m_pWriteHint->SetTimestampOffset(timestampOffset);

	m_bytesThisHint = 0;
	m_writeHintId++;
}

void MP4RtpHintTrack::AddPacket(bool setMbit, int32_t transmitOffset)
{
	if (m_pWriteHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddPacket");
	}

	MP4RtpPacket* pPacket = m_pWriteHint->AddPacket();

	ASSERT(m_pPayloadNumberProperty);

	pPacket->Set(
		m_pPayloadNumberProperty->GetValue(), 
		m_writePacketId++, 
		setMbit);
	pPacket->SetTransmitOffset(transmitOffset);

	m_bytesThisHint += 12;
	if (m_bytesThisPacket > m_pPmax->GetValue()) {
		m_pPmax->SetValue(m_bytesThisPacket);
	}
	m_bytesThisPacket = 12;
	m_pNump->IncrementValue();
	m_pTrpy->IncrementValue(12); // RTP packet header size
}

void MP4RtpHintTrack::AddImmediateData(
	const u_int8_t* pBytes,
	u_int32_t numBytes)
{
	if (m_pWriteHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddImmediateData");
	}

	MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
	if (pPacket == NULL) {
		throw new MP4Error("no packet pending", "MP4RtpAddImmediateData");
	}

	if (pBytes == NULL || numBytes == 0) {
		throw new MP4Error("no data",
			"AddImmediateData");
	}
	if (numBytes > 14) {
		throw new MP4Error("data size is larger than 14 bytes",
			"AddImmediateData");
	}

	MP4RtpImmediateData* pData = new MP4RtpImmediateData(pPacket);
	pData->Set(pBytes, numBytes);

	pPacket->AddData(pData);

	m_bytesThisHint += numBytes;
	m_bytesThisPacket += numBytes;
	m_pDimm->IncrementValue(numBytes);
	m_pTpyl->IncrementValue(numBytes);
	m_pTrpy->IncrementValue(numBytes);
}

void MP4RtpHintTrack::AddSampleData(
	MP4SampleId sampleId,
	u_int32_t dataOffset,
	u_int32_t dataLength)
{
	if (m_pWriteHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddSampleData");
	}

	MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
	if (pPacket == NULL) {
		throw new MP4Error("no packet pending", "MP4RtpAddSampleData");
	}

	MP4RtpSampleData* pData = new MP4RtpSampleData(pPacket);

	pData->SetReferenceSample(sampleId, dataOffset, dataLength);

	pPacket->AddData(pData);

	m_bytesThisHint += dataLength;
	m_bytesThisPacket += dataLength;
	m_pDmed->IncrementValue(dataLength);
	m_pTpyl->IncrementValue(dataLength);
	m_pTrpy->IncrementValue(dataLength);
}

void MP4RtpHintTrack::AddESConfigurationPacket()
{
	if (m_pWriteHint == NULL) {
		throw new MP4Error("no hint pending", 
			"MP4RtpAddESConfigurationPacket");
	}

	u_int8_t* pConfig = NULL;
	u_int32_t configSize = 0;

	m_pFile->GetTrackESConfiguration(m_pRefTrack->GetId(),
		&pConfig, &configSize);

	if (pConfig == NULL) {
		return;
	}

	ASSERT(m_pMaxPacketSizeProperty);

	if (configSize > m_pMaxPacketSizeProperty->GetValue()) {
		throw new MP4Error("ES configuration is too large for RTP payload",
			"MP4RtpAddESConfigurationPacket");
	}

	AddPacket(false);

	MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
	ASSERT(pPacket);
	
	// This is ugly!
	// To get the ES configuration data somewhere known
	// we create a sample data reference that points to 
	// this hint track (not the media track)
	// and this sample of the hint track 
	// the offset into this sample is filled in during the write process
	MP4RtpSampleData* pData = new MP4RtpSampleData(pPacket);

	pData->SetEmbeddedImmediate(m_writeSampleId, pConfig, configSize);

	pPacket->AddData(pData);

	m_bytesThisHint += configSize;
	m_bytesThisPacket += configSize;
	m_pTpyl->IncrementValue(configSize);
	m_pTrpy->IncrementValue(configSize);
}

void MP4RtpHintTrack::WriteHint(MP4Duration duration, bool isSyncSample)
{
	if (m_pWriteHint == NULL) {
		throw new MP4Error("no hint pending", "MP4WriteRtpHint");
	}

	u_int8_t* pBytes;
	u_int64_t numBytes;

	m_pFile->EnableMemoryBuffer();

	m_pWriteHint->Write(m_pFile);

	m_pFile->DisableMemoryBuffer(&pBytes, &numBytes);

	WriteSample(pBytes, numBytes, duration, 0, isSyncSample);

	MP4Free(pBytes);

	// update statistics
	if (m_bytesThisPacket > m_pPmax->GetValue()) {
		m_pPmax->SetValue(m_bytesThisPacket);
	}

	if (duration > m_pDmax->GetValue()) {
		m_pDmax->SetValue(duration);
	}

	MP4Timestamp startTime;

	GetSampleTimes(m_writeHintId, &startTime, NULL);

	if (startTime < m_thisSec + GetTimeScale()) {
		m_bytesThisSec += m_bytesThisHint;
	} else {
		if (m_bytesThisSec > m_pMaxr->GetValue()) {
			m_pMaxr->SetValue(m_bytesThisSec);
		}
		m_thisSec = startTime - (startTime % GetTimeScale());
		m_bytesThisSec = m_bytesThisHint;
	}

	// cleanup
	delete m_pWriteHint;
	m_pWriteHint = NULL;
}

void MP4RtpHintTrack::FinishWrite()
{
	if (m_writeHintId != MP4_INVALID_SAMPLE_ID) {
		m_pMaxPdu->SetValue(m_pPmax->GetValue());
		if (m_pNump->GetValue()) {
			m_pAvgPdu->SetValue(m_pTrpy->GetValue() / m_pNump->GetValue());
		}

		m_pMaxBitRate->SetValue(m_pMaxr->GetValue() * 8);
		if (GetDuration()) {
			m_pAvgBitRate->SetValue(
				m_pTrpy->GetValue() * 8 * GetTimeScale() / GetDuration());
		}
	}

	MP4Track::FinishWrite();
}

void MP4RtpHintTrack::InitStats()
{
	MP4Atom* pHinfAtom = m_pTrakAtom->FindAtom("trak.udta.hinf");

	ASSERT(pHinfAtom);

	pHinfAtom->FindProperty("hinf.trpy.bytes", (MP4Property**)&m_pTrpy); 
	pHinfAtom->FindProperty("hinf.nump.packets", (MP4Property**)&m_pNump); 
	pHinfAtom->FindProperty("hinf.tpyl.bytes", (MP4Property**)&m_pTpyl); 
	pHinfAtom->FindProperty("hinf.maxr.bytes", (MP4Property**)&m_pMaxr); 
	pHinfAtom->FindProperty("hinf.dmed.bytes", (MP4Property**)&m_pDmed); 
	pHinfAtom->FindProperty("hinf.dimm.bytes", (MP4Property**)&m_pDimm); 
	pHinfAtom->FindProperty("hinf.pmax.bytes", (MP4Property**)&m_pPmax); 
	pHinfAtom->FindProperty("hinf.dmax.milliSecs", (MP4Property**)&m_pDmax); 

	MP4Atom* pHmhdAtom = m_pTrakAtom->FindAtom("trak.mdia.minf.hmhd");

	ASSERT(pHmhdAtom);

	pHmhdAtom->FindProperty("hmhd.maxPduSize", (MP4Property**)&m_pMaxPdu); 
	pHmhdAtom->FindProperty("hmhd.avgPduSize", (MP4Property**)&m_pAvgPdu); 
	pHmhdAtom->FindProperty("hmhd.maxBitRate", (MP4Property**)&m_pMaxBitRate); 
	pHmhdAtom->FindProperty("hmhd.avgBitRate", (MP4Property**)&m_pAvgBitRate); 

	MP4Integer32Property* pMaxrPeriod = NULL;
	pHinfAtom->FindProperty("hinf.maxr.granularity",
		 (MP4Property**)&pMaxrPeriod); 
	if (pMaxrPeriod) {
		pMaxrPeriod->SetValue(1000);	// 1 second
	}
}


MP4RtpHint::MP4RtpHint(MP4RtpHintTrack* pTrack)
{
	m_pTrack = pTrack;

	AddProperty( /* 0 */
		new MP4Integer16Property("packetCount"));
	AddProperty( /* 1 */
		new MP4Integer16Property("reserved"));
}

MP4RtpHint::~MP4RtpHint()
{
	for (u_int32_t i = 0; i < m_rtpPackets.Size(); i++) {
		delete m_rtpPackets[i];
	}
}

MP4RtpPacket* MP4RtpHint::AddPacket() 
{
	MP4RtpPacket* pPacket = new MP4RtpPacket(this);
	m_rtpPackets.Add(pPacket);

	// packetCount property
	((MP4Integer16Property*)m_pProperties[0])->IncrementValue();

	pPacket->SetBFrame(m_isBFrame);
	pPacket->SetTimestampOffset(m_timestampOffset);

	return pPacket;
}

void MP4RtpHint::Read(MP4File* pFile)
{
	// call base class Read for required properties
	MP4Container::Read(pFile);

	u_int16_t numPackets =
		((MP4Integer16Property*)m_pProperties[0])->GetValue();

	for (u_int16_t i = 0; i < numPackets; i++) {
		MP4RtpPacket* pPacket = new MP4RtpPacket(this);

		m_rtpPackets.Add(pPacket);

		pPacket->Read(pFile);
	}

	VERBOSE_READ_HINT(pFile->GetVerbosity(),
		printf("ReadHint:\n"); Dump(stdout, 10, false););
}

void MP4RtpHint::Write(MP4File* pFile)
{
	u_int64_t hintStartPos = pFile->GetPosition();

	MP4Container::Write(pFile);

	u_int64_t packetStartPos = pFile->GetPosition();

	u_int32_t i;

	// first write out packet (and data) entries
	for (i = 0; i < m_rtpPackets.Size(); i++) {
		m_rtpPackets[i]->Write(pFile);
	}

	// now let packets write their extra data into the hint sample
	for (i = 0; i < m_rtpPackets.Size(); i++) {
		m_rtpPackets[i]->WriteEmbeddedData(pFile, hintStartPos);
	}

	u_int64_t endPos = pFile->GetPosition();

	pFile->SetPosition(packetStartPos);

	// finally rewrite the packet and data entries
	// which now contain the correct offsets for the embedded data
	for (i = 0; i < m_rtpPackets.Size(); i++) {
		m_rtpPackets[i]->Write(pFile);
	}

	pFile->SetPosition(endPos);

	VERBOSE_WRITE_HINT(pFile->GetVerbosity(),
		printf("WriteRtpHint:\n"); Dump(stdout, 14, false));
}

void MP4RtpHint::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	MP4Container::Dump(pFile, indent, dumpImplicits);

	for (u_int32_t i = 0; i < m_rtpPackets.Size(); i++) {
		Indent(pFile, indent);
		fprintf(pFile, "RtpPacket: %u\n", i);
		m_rtpPackets[i]->Dump(pFile, indent + 1, dumpImplicits);
	}
}

MP4RtpPacket::MP4RtpPacket(MP4RtpHint* pHint)
{
	m_pHint = pHint;

	AddProperty( /* 0 */
		new MP4Integer32Property("relativeXmitTime"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("reserved1", 2));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("Pbit", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("Xbit", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("reserved2", 4));
	AddProperty( /* 5 */
		new MP4BitfieldProperty("Mbit", 1));
	AddProperty( /* 6 */
		new MP4BitfieldProperty("payloadType", 7));
	AddProperty( /* 7  */
		new MP4Integer16Property("sequenceNumber"));
	AddProperty( /* 8 */
		new MP4BitfieldProperty("reserved3", 13));
	AddProperty( /* 9 */
		new MP4BitfieldProperty("extraFlag", 1));
	AddProperty( /* 10 */
		new MP4BitfieldProperty("bFrameFlag", 1));
	AddProperty( /* 11 */
		new MP4BitfieldProperty("repeatFlag", 1));
	AddProperty( /* 12 */
		new MP4Integer16Property("entryCount"));
}
 
MP4RtpPacket::~MP4RtpPacket()
{
	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		delete m_rtpData[i];
	}
}

void MP4RtpPacket::AddExtraProperties()
{
	AddProperty( /* 13 */
		new MP4Integer32Property("extraInformationLength"));

	// This is a bit of a hack, since the tlv entries are really defined 
	// as atoms but there is only one type defined now, rtpo, and getting 
	// our atom code hooked up here would be a major pain with little gain

	AddProperty( /* 14 */
		new MP4Integer32Property("tlvLength"));
	AddProperty( /* 15 */
		new MP4StringProperty("tlvType"));
	AddProperty( /* 16 */
		new MP4Integer32Property("timestampOffset"));

	((MP4Integer32Property*)m_pProperties[13])->SetValue(16);
	((MP4Integer32Property*)m_pProperties[14])->SetValue(12);
	((MP4StringProperty*)m_pProperties[15])->SetFixedLength(4);
	((MP4StringProperty*)m_pProperties[15])->SetValue("rtpo");
}

void MP4RtpPacket::Read(MP4File* pFile)
{
	// call base class Read for required properties
	MP4Container::Read(pFile);

	// read extra info if present
	// we only support the rtpo field!
	if (((MP4BitfieldProperty*)m_pProperties[9])->GetValue() == 1) {
		ReadExtra(pFile);
	}

	u_int16_t numDataEntries =
		((MP4Integer16Property*)m_pProperties[12])->GetValue();

	// read data entries
	for (u_int16_t i = 0; i < numDataEntries; i++) {
		u_int8_t dataType;
		pFile->PeekBytes(&dataType, 1);

		MP4RtpData* pData;

		switch (dataType) {
		case 0:
			pData = new MP4RtpNullData(this);
			break;
		case 1:
			pData = new MP4RtpImmediateData(this);
			break;
		case 2:
			pData = new MP4RtpSampleData(this);
			break;
		case 3:
			pData = new MP4RtpSampleDescriptionData(this);
			break;
		default:
			throw new MP4Error("unknown packet data entry type",
				"MP4ReadHint");
		}

		m_rtpData.Add(pData);

		// read data entry's properties
		pData->Read(pFile);
	}
}

void MP4RtpPacket::ReadExtra(MP4File* pFile)
{
	AddExtraProperties();

	int32_t extraLength = (int32_t)pFile->ReadUInt32();

	if (extraLength < 4) {
		throw new MP4Error("bad packet extra info length",
			"MP4RtpPacket::ReadExtra");
	}
	extraLength -= 4;

	while (extraLength > 0) {
		u_int32_t entryLength = pFile->ReadUInt32();
		u_int32_t entryTag = pFile->ReadUInt32();

		if (entryLength < 8) {
			throw new MP4Error("bad packet extra info entry length",
				"MP4RtpPacket::ReadExtra");
		}

		if (entryTag == STRTOINT32("rtpo") && entryLength == 12) {
			// read the rtp timestamp offset
			m_pProperties[16]->Read(pFile);
		} else {
			// ignore it, LATER carry it along
			pFile->SetPosition(pFile->GetPosition() + entryLength - 8);
		}

		extraLength -= entryLength;
	}

	if (extraLength < 0) {
		throw new MP4Error("invalid packet extra info length",
			"MP4RtpPacket::ReadExtra");
	}
}

void MP4RtpPacket::Set(u_int8_t payloadNumber, 
	u_int32_t packetId, bool setMbit)
{
	((MP4BitfieldProperty*)m_pProperties[5])->SetValue(setMbit);
	((MP4BitfieldProperty*)m_pProperties[6])->SetValue(payloadNumber);
	((MP4Integer16Property*)m_pProperties[7])->SetValue(packetId);
}

int32_t MP4RtpPacket::GetTransmitOffset()
{
	return ((MP4Integer32Property*)m_pProperties[0])->GetValue();
}

void MP4RtpPacket::SetTransmitOffset(int32_t transmitOffset)
{
	((MP4Integer32Property*)m_pProperties[0])->SetValue(transmitOffset);
}

bool MP4RtpPacket::GetPBit()
{
	return ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
}

bool MP4RtpPacket::GetXBit()
{
	return ((MP4BitfieldProperty*)m_pProperties[3])->GetValue();
}

bool MP4RtpPacket::GetMBit()
{
	return ((MP4BitfieldProperty*)m_pProperties[5])->GetValue();
}

u_int8_t MP4RtpPacket::GetPayload()
{
	return ((MP4BitfieldProperty*)m_pProperties[6])->GetValue();
}

u_int16_t MP4RtpPacket::GetSequenceNumber()
{
	return ((MP4Integer16Property*)m_pProperties[7])->GetValue();
}

bool MP4RtpPacket::IsBFrame()
{
	return ((MP4BitfieldProperty*)m_pProperties[10])->GetValue();
}

void MP4RtpPacket::SetBFrame(bool isBFrame)
{
	((MP4BitfieldProperty*)m_pProperties[10])->SetValue(isBFrame);
}

void MP4RtpPacket::SetTimestampOffset(u_int32_t timestampOffset)
{
	if (timestampOffset == 0) {
		return;
	}

	ASSERT(((MP4BitfieldProperty*)m_pProperties[9])->GetValue() == 0);

	// set X bit
	((MP4BitfieldProperty*)m_pProperties[9])->SetValue(1);

	AddExtraProperties();

	((MP4Integer32Property*)m_pProperties[16])->SetValue(timestampOffset);
}

void MP4RtpPacket::AddData(MP4RtpData* pData)
{
	m_rtpData.Add(pData);

	// increment entry count property
	((MP4Integer16Property*)m_pProperties[12])->IncrementValue();
}

u_int32_t MP4RtpPacket::GetDataSize()
{
	u_int32_t totalDataSize = 0;

	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		totalDataSize += m_rtpData[i]->GetDataSize();
	}

	return totalDataSize;
}

void MP4RtpPacket::GetData(u_int8_t* pDest)
{
	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		m_rtpData[i]->GetData(pDest);
		pDest += m_rtpData[i]->GetDataSize();
	}
}

void MP4RtpPacket::Write(MP4File* pFile)
{
	MP4Container::Write(pFile);

	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		m_rtpData[i]->Write(pFile);
	}
}

void MP4RtpPacket::WriteEmbeddedData(MP4File* pFile, u_int64_t startPos)
{
	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		m_rtpData[i]->WriteEmbeddedData(pFile, startPos);
	}
}

void MP4RtpPacket::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	MP4Container::Dump(pFile, indent, dumpImplicits);

	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		Indent(pFile, indent);
		fprintf(pFile, "RtpData: %u\n", i);
		m_rtpData[i]->Dump(pFile, indent + 1, dumpImplicits);
	}
}

MP4RtpData::MP4RtpData(MP4RtpPacket* pPacket)
{
	m_pPacket = pPacket;

	AddProperty( /* 0 */
		new MP4Integer8Property("type"));
}

MP4Track* MP4RtpData::FindTrackFromRefIndex(u_int8_t refIndex)
{
	MP4Track* pTrack;

	if (refIndex == (u_int8_t)-1) {
		// ourselves
		pTrack = GetPacket()->GetHint()->GetTrack();
	} else if (refIndex == 0) {
		// our reference track
		pTrack = GetPacket()->GetHint()->GetTrack()->GetRefTrack();
	} else {
		// some other track
		MP4RtpHintTrack* pHintTrack =
			GetPacket()->GetHint()->GetTrack();

		MP4Atom* pTrakAtom = pHintTrack->GetTrakAtom();
		ASSERT(pTrakAtom);

		MP4Integer32Property* pTrackIdProperty = NULL;
		pTrakAtom->FindProperty(
			"trak.tref.hint.entries",
			(MP4Property**)&pTrackIdProperty);
		ASSERT(pTrackIdProperty);

		u_int32_t refTrackId = 
			pTrackIdProperty->GetValue(refIndex - 1);

		pTrack = pHintTrack->GetFile()->GetTrack(refTrackId); 
	}

	return pTrack;
}

MP4RtpNullData::MP4RtpNullData(MP4RtpPacket* pPacket)
	: MP4RtpData(pPacket)
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(0);

	AddProperty( /* 1 */
		new MP4BytesProperty("pad", 15));

	((MP4BytesProperty*)m_pProperties[1])->SetFixedSize(15);
}

MP4RtpImmediateData::MP4RtpImmediateData(MP4RtpPacket* pPacket)
	: MP4RtpData(pPacket)
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(1);

	AddProperty( /* 1 */
		new MP4Integer8Property("count"));
	AddProperty( /* 2 */
		new MP4BytesProperty("data", 14));

	((MP4BytesProperty*)m_pProperties[2])->SetFixedSize(14);
}

void MP4RtpImmediateData::Set(const u_int8_t* pBytes, u_int8_t numBytes)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue(numBytes);
	((MP4BytesProperty*)m_pProperties[2])->SetValue(pBytes, numBytes);
}

u_int16_t MP4RtpImmediateData::GetDataSize()
{
	return ((MP4Integer8Property*)m_pProperties[1])->GetValue();
}

void MP4RtpImmediateData::GetData(u_int8_t* pDest)
{
	u_int8_t* pValue;
	u_int32_t valueSize;
	((MP4BytesProperty*)m_pProperties[2])->GetValue(&pValue, &valueSize);

	memcpy(pDest, pValue, GetDataSize());
	MP4Free(pValue);
}

MP4RtpSampleData::MP4RtpSampleData(MP4RtpPacket* pPacket)
	: MP4RtpData(pPacket)
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(2);

	AddProperty( /* 1 */
		new MP4Integer8Property("trackRefIndex"));
	AddProperty( /* 2 */
		new MP4Integer16Property("length"));
	AddProperty( /* 3 */
		new MP4Integer32Property("sampleNumber"));
	AddProperty( /* 4 */
		new MP4Integer32Property("sampleOffset"));
	AddProperty( /* 5 */
		new MP4Integer16Property("bytesPerBlock"));
	AddProperty( /* 6 */
		new MP4Integer16Property("samplesPerBlock"));

	((MP4Integer16Property*)m_pProperties[5])->SetValue(1);
	((MP4Integer16Property*)m_pProperties[6])->SetValue(1);

	m_pRefData = NULL;
	m_pRefTrack = NULL;
	m_refSampleId = MP4_INVALID_SAMPLE_ID;
	m_refSampleOffset = 0;
}

void MP4RtpSampleData::SetEmbeddedImmediate(MP4SampleId sampleId, 
	u_int8_t* pData, u_int16_t dataLength)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue((u_int8_t)-1);
	((MP4Integer16Property*)m_pProperties[2])->SetValue(dataLength);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleId);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(0); 
	m_pRefData = pData;
}

void MP4RtpSampleData::SetReferenceSample(
	MP4SampleId refSampleId, u_int32_t refSampleOffset, 
	u_int16_t sampleLength)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue(0);
	((MP4Integer16Property*)m_pProperties[2])->SetValue(sampleLength);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(refSampleId);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(refSampleOffset);
}

void MP4RtpSampleData::SetEmbeddedSample(
	MP4SampleId sampleId, MP4Track* pRefTrack,
	MP4SampleId refSampleId, u_int32_t refSampleOffset, 
	u_int16_t sampleLength)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue((u_int8_t)-1);
	((MP4Integer16Property*)m_pProperties[2])->SetValue(sampleLength);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleId);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(0);
	m_pRefTrack = pRefTrack;
	m_refSampleId = refSampleId;
	m_refSampleOffset = refSampleOffset;
}

u_int16_t MP4RtpSampleData::GetDataSize()
{
	return ((MP4Integer16Property*)m_pProperties[2])->GetValue();
}

void MP4RtpSampleData::GetData(u_int8_t* pDest)
{
	u_int8_t trackRefIndex = 
		((MP4Integer8Property*)m_pProperties[1])->GetValue();

	MP4Track* pSampleTrack =
		FindTrackFromRefIndex(trackRefIndex);

	pSampleTrack->ReadSampleFragment(
		((MP4Integer32Property*)m_pProperties[3])->GetValue(),	// sampleId 
		((MP4Integer32Property*)m_pProperties[4])->GetValue(),	// sampleOffset
		((MP4Integer16Property*)m_pProperties[2])->GetValue(),	// sampleLength
		pDest);
}

void MP4RtpSampleData::WriteEmbeddedData(MP4File* pFile, u_int64_t startPos)
{
	// if not using embedded data, nothing to do
	if (((MP4Integer8Property*)m_pProperties[1])->GetValue() != (u_int8_t)-1) {
		return;
	}

	// figure out the offset within this hint sample for this embedded data
	u_int64_t offset = pFile->GetPosition() - startPos;
	ASSERT(offset <= 0xFFFFFFFF);	
	((MP4Integer32Property*)m_pProperties[4])->SetValue((u_int32_t)offset);

	u_int16_t length = ((MP4Integer16Property*)m_pProperties[2])->GetValue();

	if (m_pRefData) {
		pFile->WriteBytes(m_pRefData, length);
		return;
	} 

	if (m_refSampleId != MP4_INVALID_SAMPLE_ID) {
		u_int8_t* pSample = NULL;
		u_int32_t sampleSize = 0;

		ASSERT(m_pRefTrack);
		m_pRefTrack->ReadSample(m_refSampleId, &pSample, &sampleSize);

		ASSERT(m_refSampleOffset + length <= sampleSize);

		pFile->WriteBytes(&pSample[m_refSampleOffset], length);

		MP4Free(pSample);
		return;
	}
}

MP4RtpSampleDescriptionData::MP4RtpSampleDescriptionData(MP4RtpPacket* pPacket)
	: MP4RtpData(pPacket)
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(3);

	AddProperty( /* 1 */
		new MP4Integer8Property("trackRefIndex"));
	AddProperty( /* 2 */
		new MP4Integer16Property("length"));
	AddProperty( /* 3 */
		new MP4Integer32Property("sampleDescriptionIndex"));
	AddProperty( /* 4 */
		new MP4Integer32Property("sampleDescriptionOffset"));
	AddProperty( /* 5 */
		new MP4Integer32Property("reserved"));
}

void MP4RtpSampleDescriptionData::Set(u_int32_t sampleDescrIndex,
	u_int32_t offset, u_int16_t length)
{
	((MP4Integer16Property*)m_pProperties[2])->SetValue(length);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleDescrIndex);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(offset);
}

u_int16_t MP4RtpSampleDescriptionData::GetDataSize()
{
	return ((MP4Integer16Property*)m_pProperties[2])->GetValue();
}

void MP4RtpSampleDescriptionData::GetData(u_int8_t* pDest)
{
	// we start with the index into our track references
	u_int8_t trackRefIndex = 
		((MP4Integer8Property*)m_pProperties[1])->GetValue();

	// from which we can find the track structure
	MP4Track* pSampleTrack =
		FindTrackFromRefIndex(trackRefIndex);

	// next find the desired atom in the track's sample description table
	u_int32_t sampleDescrIndex =
		((MP4Integer32Property*)m_pProperties[3])->GetValue();

	MP4Atom* pTrakAtom =
		pSampleTrack->GetTrakAtom();

	char sdName[64];
	sprintf(sdName, "trak.mdia.minf.stbl.stsd.*[%u]", sampleDescrIndex);

	MP4Atom* pSdAtom =
		pTrakAtom->FindAtom(sdName);

	// bad reference
	if (pSdAtom == NULL) {
		throw new MP4Error("invalid sample description index",
			"MP4RtpSampleDescriptionData::GetData");
	}

	// check validity of the upcoming copy
	u_int16_t length = 
		((MP4Integer16Property*)m_pProperties[2])->GetValue();
	u_int32_t offset =
		((MP4Integer32Property*)m_pProperties[4])->GetValue();

	if (offset + length > pSdAtom->GetSize()) {
		throw new MP4Error("offset and/or length are too large", 
			"MP4RtpSampleDescriptionData::GetData");
	}

	// now we use the raw file to get the desired bytes

	MP4File* pFile = GetPacket()->GetHint()->GetTrack()->GetFile();

	u_int64_t orgPos = pFile->GetPosition();

	// It's not entirely clear from the spec whether the offset is from 
	// the start of the sample descirption atom, or the start of the atom's
	// data. I believe it is the former, but the commented out code will 
	// realize the latter interpretation if I turn out to be wrong.
	u_int64_t dataPos = pSdAtom->GetStart();
	//u_int64_t dataPos = pSdAtom->GetEnd() - pSdAtom->GetSize();

	pFile->SetPosition(dataPos + offset);

	pFile->ReadBytes(pDest, length);

	pFile->SetPosition(orgPos);
}

