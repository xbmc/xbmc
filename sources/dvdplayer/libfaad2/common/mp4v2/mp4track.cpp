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
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 */

#include "mp4common.h"

MP4Track::MP4Track(MP4File* pFile, MP4Atom* pTrakAtom) 
{
	m_pFile = pFile;
	m_pTrakAtom = pTrakAtom;

	m_lastStsdIndex = 0;
	m_lastSampleFile = NULL;

	m_cachedReadSampleId = MP4_INVALID_SAMPLE_ID;
	m_pCachedReadSample = NULL;
	m_cachedReadSampleSize = 0;

	m_writeSampleId = 1;
	m_fixedSampleDuration = 0;
	m_pChunkBuffer = NULL;
	m_chunkBufferSize = 0;
	m_chunkSamples = 0;
	m_chunkDuration = 0;

	m_samplesPerChunk = 0;
	m_durationPerChunk = 0;

	bool success = true;

	MP4Integer32Property* pTrackIdProperty;
	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.trackId",
		(MP4Property**)&pTrackIdProperty);
	if (success) {
		m_trackId = pTrackIdProperty->GetValue();
	}

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.timeScale", 
		(MP4Property**)&m_pTimeScaleProperty);
	if (success) {
		// default chunking is 1 second of samples
		m_durationPerChunk = m_pTimeScaleProperty->GetValue();
	}

	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.duration", 
		(MP4Property**)&m_pTrackDurationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.duration", 
		(MP4Property**)&m_pMediaDurationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.modificationTime", 
		(MP4Property**)&m_pTrackModificationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.modificationTime", 
		(MP4Property**)&m_pMediaModificationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.hdlr.handlerType",
		(MP4Property**)&m_pTypeProperty);

	// get handles on sample size information

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.sampleSize",
		(MP4Property**)&m_pStszFixedSampleSizeProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.sampleCount",
		(MP4Property**)&m_pStszSampleCountProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.entries.sampleSize",
		(MP4Property**)&m_pStszSampleSizeProperty);

	// get handles on information needed to map sample id's to file offsets

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entryCount",
		(MP4Property**)&m_pStscCountProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.firstChunk",
		(MP4Property**)&m_pStscFirstChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.samplesPerChunk",
		(MP4Property**)&m_pStscSamplesPerChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.sampleDescriptionIndex",
		(MP4Property**)&m_pStscSampleDescrIndexProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.firstSample",
		(MP4Property**)&m_pStscFirstSampleProperty);

	bool haveStco = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stco.entryCount",
		(MP4Property**)&m_pChunkCountProperty);

	if (haveStco) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stco.entries.chunkOffset",
			(MP4Property**)&m_pChunkOffsetProperty);
	} else {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.co64.entryCount",
			(MP4Property**)&m_pChunkCountProperty);

		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.co64.entries.chunkOffset",
			(MP4Property**)&m_pChunkOffsetProperty);
	}

	// get handles on sample timing info

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entryCount",
		(MP4Property**)&m_pSttsCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries.sampleCount",
		(MP4Property**)&m_pSttsSampleCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries.sampleDelta",
		(MP4Property**)&m_pSttsSampleDeltaProperty);
	
	// get handles on rendering offset info if it exists

	m_pCttsCountProperty = NULL;
	m_pCttsSampleCountProperty = NULL;
	m_pCttsSampleOffsetProperty = NULL;

	bool haveCtts = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.ctts.entryCount",
		(MP4Property**)&m_pCttsCountProperty);

	if (haveCtts) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries.sampleCount",
			(MP4Property**)&m_pCttsSampleCountProperty);

		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries.sampleOffset",
			(MP4Property**)&m_pCttsSampleOffsetProperty);
	}

	// get handles on sync sample info if it exists

	m_pStssCountProperty = NULL;
	m_pStssSampleProperty = NULL;

	bool haveStss = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stss.entryCount",
		(MP4Property**)&m_pStssCountProperty);

	if (haveStss) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stss.entries.sampleNumber",
			(MP4Property**)&m_pStssSampleProperty);
	}

	// edit list
	InitEditListProperties();

	// was everything found?
	if (!success) {
		throw new MP4Error("invalid track", "MP4Track::MP4Track");
	}
}

MP4Track::~MP4Track()
{
	MP4Free(m_pCachedReadSample);
	MP4Free(m_pChunkBuffer);
}

const char* MP4Track::GetType()
{
	return m_pTypeProperty->GetValue();
}

void MP4Track::SetType(const char* type) 
{
	m_pTypeProperty->SetValue(NormalizeTrackType(type));
}

void MP4Track::ReadSample(
	MP4SampleId sampleId,
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime, 
	MP4Duration* pDuration,
	MP4Duration* pRenderingOffset, 
	bool* pIsSyncSample)
{
	if (sampleId == MP4_INVALID_SAMPLE_ID) {
		throw new MP4Error("sample id can't be zero", 
			"MP4Track::ReadSample");
	}

	// handle unusual case of wanting to read a sample
	// that is still sitting in the write chunk buffer
	if (m_pChunkBuffer && sampleId >= m_writeSampleId - m_chunkSamples) {
		WriteChunkBuffer();
	}

	FILE* pFile = GetSampleFile(sampleId);

	if (pFile == (FILE*)-1) {
		throw new MP4Error("sample is located in an inaccessible file",
			"MP4Track::ReadSample");
	}

	u_int64_t fileOffset = GetSampleFileOffset(sampleId);

	u_int32_t sampleSize = GetSampleSize(sampleId);
	if (*ppBytes != NULL && *pNumBytes < sampleSize) {
		throw new MP4Error("sample buffer is too small",
			 "MP4Track::ReadSample");
	}
	*pNumBytes = sampleSize;

	VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
		printf("ReadSample: track %u id %u offset 0x"LLX" size %u (0x%x)\n",
			m_trackId, sampleId, fileOffset, *pNumBytes, *pNumBytes));

	bool bufferMalloc = false;
	if (*ppBytes == NULL) {
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		bufferMalloc = true;
	}

	u_int64_t oldPos = m_pFile->GetPosition(pFile); // only used in mode == 'w'
	try { 
		m_pFile->SetPosition(fileOffset, pFile);
		m_pFile->ReadBytes(*ppBytes, *pNumBytes, pFile);

		if (pStartTime || pDuration) {
			GetSampleTimes(sampleId, pStartTime, pDuration);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  start "LLU" duration "LLD"\n",
					(pStartTime ? *pStartTime : 0), 
					(pDuration ? *pDuration : 0)));
		}
		if (pRenderingOffset) {
			*pRenderingOffset = GetSampleRenderingOffset(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  renderingOffset "LLD"\n",
					*pRenderingOffset));
		}
		if (pIsSyncSample) {
			*pIsSyncSample = IsSyncSample(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  isSyncSample %u\n",
					*pIsSyncSample));
		}
	}

	catch (MP4Error* e) {
		if (bufferMalloc) {
			// let's not leak memory
			MP4Free(*ppBytes);
			*ppBytes = NULL;
		}
		if (m_pFile->GetMode() == 'w') {
			m_pFile->SetPosition(oldPos, pFile);
		}
		throw e;
	}

	if (m_pFile->GetMode() == 'w') {
		m_pFile->SetPosition(oldPos, pFile);
	}
}

void MP4Track::ReadSampleFragment(
	MP4SampleId sampleId,
	u_int32_t sampleOffset,
	u_int16_t sampleLength,
	u_int8_t* pDest)
{
	if (sampleId == MP4_INVALID_SAMPLE_ID) {
		throw new MP4Error("invalid sample id", 
			"MP4Track::ReadSampleFragment");
	}

	if (sampleId != m_cachedReadSampleId) {
		MP4Free(m_pCachedReadSample);
		m_pCachedReadSample = NULL;
		m_cachedReadSampleSize = 0;
		m_cachedReadSampleId = MP4_INVALID_SAMPLE_ID;

		ReadSample(
			sampleId,
			&m_pCachedReadSample,
			&m_cachedReadSampleSize);

		m_cachedReadSampleId = sampleId;
	}

	if (sampleOffset + sampleLength > m_cachedReadSampleSize) {
		throw new MP4Error("offset and/or length are too large", 
			"MP4Track::ReadSampleFragment");
	}

	memcpy(pDest, &m_pCachedReadSample[sampleOffset], sampleLength);
}

void MP4Track::WriteSample(
	const u_int8_t* pBytes, 
	u_int32_t numBytes,
	MP4Duration duration, 
	MP4Duration renderingOffset, 
	bool isSyncSample)
{
	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("WriteSample: track %u id %u size %u (0x%x) ",
			m_trackId, m_writeSampleId, numBytes, numBytes));

	if (pBytes == NULL && numBytes > 0) {
		throw new MP4Error("no sample data", "MP4WriteSample");
	}

	if (duration == MP4_INVALID_DURATION) {
		duration = GetFixedSampleDuration();
	}

	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("duration "LLU"\n", duration));

	// append sample bytes to chunk buffer
	m_pChunkBuffer = (u_int8_t*)MP4Realloc(m_pChunkBuffer, 
		m_chunkBufferSize + numBytes);
	memcpy(&m_pChunkBuffer[m_chunkBufferSize], pBytes, numBytes);
	m_chunkBufferSize += numBytes;
	m_chunkSamples++;
	m_chunkDuration += duration;

	UpdateSampleSizes(m_writeSampleId, numBytes);

	UpdateSampleTimes(duration);

	UpdateRenderingOffsets(m_writeSampleId, renderingOffset);

	UpdateSyncSamples(m_writeSampleId, isSyncSample);

	if (IsChunkFull(m_writeSampleId)) {
		WriteChunkBuffer();
	}

	UpdateDurations(duration);

	UpdateModificationTimes();

	m_writeSampleId++;
}

void MP4Track::WriteChunkBuffer()
{
	if (m_chunkBufferSize == 0) {
		return;
	}

	u_int64_t chunkOffset = m_pFile->GetPosition();

	// write chunk buffer
	m_pFile->WriteBytes(m_pChunkBuffer, m_chunkBufferSize);

	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("WriteChunk: track %u offset 0x"LLX" size %u (0x%x) numSamples %u\n",
			m_trackId, chunkOffset, m_chunkBufferSize, 
			m_chunkBufferSize, m_chunkSamples));

	UpdateSampleToChunk(m_writeSampleId, 
		m_pChunkCountProperty->GetValue() + 1, 
		m_chunkSamples);

	UpdateChunkOffsets(chunkOffset);

	// clean up chunk buffer
	MP4Free(m_pChunkBuffer);
	m_pChunkBuffer = NULL;
	m_chunkBufferSize = 0;
	m_chunkSamples = 0;
	m_chunkDuration = 0;
}

void MP4Track::FinishWrite()
{
	// write out any remaining samples in chunk buffer
	WriteChunkBuffer();

	// record buffer size and bitrates
	MP4BitfieldProperty* pBufferSizeProperty;

	if (m_pTrakAtom->FindProperty(
	  "trak.mdia.minf.stbl.stsd.*.esds.decConfigDescr.bufferSizeDB",
	  (MP4Property**)&pBufferSizeProperty)) {
		pBufferSizeProperty->SetValue(GetMaxSampleSize());
	}

	MP4Integer32Property* pBitrateProperty;

	if (m_pTrakAtom->FindProperty(
	  "trak.mdia.minf.stbl.stsd.*.esds.decConfigDescr.maxBitrate",
	  (MP4Property**)&pBitrateProperty)) {
		pBitrateProperty->SetValue(GetMaxBitrate());
	}

	if (m_pTrakAtom->FindProperty(
	  "trak.mdia.minf.stbl.stsd.*.esds.decConfigDescr.avgBitrate",
	  (MP4Property**)&pBitrateProperty)) {
		pBitrateProperty->SetValue(GetAvgBitrate());
	}
}

bool MP4Track::IsChunkFull(MP4SampleId sampleId)
{
	if (m_samplesPerChunk) {
		return m_chunkSamples >= m_samplesPerChunk;
	}

	ASSERT(m_durationPerChunk);
	return m_chunkDuration >= m_durationPerChunk;
}

u_int32_t MP4Track::GetNumberOfSamples()
{
	return m_pStszSampleCountProperty->GetValue();
}

u_int32_t MP4Track::GetSampleSize(MP4SampleId sampleId)
{
	u_int32_t fixedSampleSize = 
		m_pStszFixedSampleSizeProperty->GetValue(); 

	if (fixedSampleSize != 0) {
		return fixedSampleSize;
	}
	return m_pStszSampleSizeProperty->GetValue(sampleId - 1);
}

u_int32_t MP4Track::GetMaxSampleSize()
{
	u_int32_t fixedSampleSize = 
		m_pStszFixedSampleSizeProperty->GetValue(); 

	if (fixedSampleSize != 0) {
		return fixedSampleSize;
	}

	u_int32_t maxSampleSize = 0;
	u_int32_t numSamples = m_pStszSampleSizeProperty->GetCount();
	for (MP4SampleId sid = 1; sid <= numSamples; sid++) {
		u_int32_t sampleSize =
			m_pStszSampleSizeProperty->GetValue(sid - 1);
		if (sampleSize > maxSampleSize) {
			maxSampleSize = sampleSize;
		}
	}
	return maxSampleSize;
}

u_int64_t MP4Track::GetTotalOfSampleSizes()
{
	u_int32_t fixedSampleSize = 
		m_pStszFixedSampleSizeProperty->GetValue(); 

	// if fixed sample size, just need to multiply by number of samples
	if (fixedSampleSize != 0) {
		return fixedSampleSize * GetNumberOfSamples();
	}

	// else non-fixed sample size, sum them
	u_int64_t totalSampleSizes = 0;
	u_int32_t numSamples = m_pStszSampleSizeProperty->GetCount();
	for (MP4SampleId sid = 1; sid <= numSamples; sid++) {
		u_int32_t sampleSize =
			m_pStszSampleSizeProperty->GetValue(sid - 1);
		totalSampleSizes += sampleSize;
	}
	return totalSampleSizes;
}

void MP4Track::UpdateSampleSizes(MP4SampleId sampleId, u_int32_t numBytes)
{
	// for first sample
	if (sampleId == 1) {
		if (numBytes > 0) {
			// presume sample size is fixed
			m_pStszFixedSampleSizeProperty->SetValue(numBytes); 
		} else {
			// special case of first sample is zero bytes in length
			// leave m_pStszFixedSampleSizeProperty at 0
			// start recording variable sample sizes
			m_pStszSampleSizeProperty->AddValue(0);
		}

	} else { // sampleId > 1
		u_int32_t fixedSampleSize = 
			m_pStszFixedSampleSizeProperty->GetValue(); 

		if (fixedSampleSize == 0 || numBytes != fixedSampleSize) {
			// sample size is not fixed

			if (fixedSampleSize) {
				// need to clear fixed sample size
				m_pStszFixedSampleSizeProperty->SetValue(0); 

				// and create sizes for all previous samples
				for (MP4SampleId sid = 1; sid < sampleId; sid++) {
					m_pStszSampleSizeProperty->AddValue(fixedSampleSize);
				}
			}

			// add size value for this sample
			m_pStszSampleSizeProperty->AddValue(numBytes);
		}
	}

	m_pStszSampleCountProperty->IncrementValue();
}

u_int32_t MP4Track::GetAvgBitrate()
{
	if (GetDuration() == 0) {
		return 0;
	}

	u_int64_t durationSecs =
		MP4ConvertTime(GetDuration(), GetTimeScale(), MP4_SECS_TIME_SCALE);

	if (GetDuration() % GetTimeScale() != 0) {
		durationSecs++;
	}

	return (GetTotalOfSampleSizes() * 8) / durationSecs;
}

u_int32_t MP4Track::GetMaxBitrate()
{
	u_int32_t timeScale = GetTimeScale();
	MP4SampleId numSamples = GetNumberOfSamples();
	u_int32_t maxBytesPerSec = 0;
	u_int32_t bytesThisSec = 0;
	MP4Timestamp thisSec = 0;

	for (MP4SampleId sid = 1; sid <= numSamples; sid++) {
		u_int32_t sampleSize;
		MP4Timestamp sampleTime;

		sampleSize = GetSampleSize(sid);

		GetSampleTimes(sid, &sampleTime, NULL);

		// sample counts for current second
		if (sampleTime < thisSec + timeScale) {
			bytesThisSec += sampleSize;
		} else { // sample is in a future second
			if (bytesThisSec > maxBytesPerSec) {
				maxBytesPerSec = bytesThisSec;
			}

			thisSec = sampleTime - (sampleTime % timeScale);
			bytesThisSec = sampleSize;
		}
	}

	// last second (or partial second) 
	if (bytesThisSec > maxBytesPerSec) {
		maxBytesPerSec = bytesThisSec;
	}

	return maxBytesPerSec * 8;
}

u_int32_t MP4Track::GetSampleStscIndex(MP4SampleId sampleId)
{
	u_int32_t stscIndex;
	u_int32_t numStscs = m_pStscCountProperty->GetValue();

	if (numStscs == 0) {
		throw new MP4Error("No data chunks exist", "GetSampleStscIndex");
	}

	for (stscIndex = 0; stscIndex < numStscs; stscIndex++) {
		if (sampleId < m_pStscFirstSampleProperty->GetValue(stscIndex)) {
			ASSERT(stscIndex != 0);
			stscIndex -= 1;
			break;
		}
	}
	if (stscIndex == numStscs) {
		ASSERT(stscIndex != 0);
		stscIndex -= 1;
	}

	return stscIndex;
}

FILE* MP4Track::GetSampleFile(MP4SampleId sampleId)
{
	u_int32_t stscIndex =
		GetSampleStscIndex(sampleId);

	u_int32_t stsdIndex = 
		m_pStscSampleDescrIndexProperty->GetValue(stscIndex);

	// check if the answer will be the same as last time
	if (m_lastStsdIndex && stsdIndex == m_lastStsdIndex) {
		return m_lastSampleFile;
	}

	MP4Atom* pStsdAtom = 
		m_pTrakAtom->FindAtom("trak.mdia.minf.stbl.stsd");
	ASSERT(pStsdAtom);

	MP4Atom* pStsdEntryAtom = 
		pStsdAtom->GetChildAtom(stsdIndex - 1);
	ASSERT(pStsdEntryAtom);

	MP4Integer16Property* pDrefIndexProperty = NULL;
	pStsdEntryAtom->FindProperty(
		"*.dataReferenceIndex",
		(MP4Property**)&pDrefIndexProperty);
	
	if (pDrefIndexProperty == NULL) {
		throw new MP4Error("invalid stsd entry", "GetSampleFile");
	}

	u_int32_t drefIndex =
		pDrefIndexProperty->GetValue();

	MP4Atom* pDrefAtom =
		m_pTrakAtom->FindAtom("trak.mdia.minf.dinf.dref");
	ASSERT(pDrefAtom);

	MP4Atom* pUrlAtom =
		pDrefAtom->GetChildAtom(drefIndex - 1);
	ASSERT(pUrlAtom);

	FILE* pFile;

	if (pUrlAtom->GetFlags() & 1) {
		pFile = NULL;	// self-contained
	} else {
#ifndef USE_FILE_CALLBACKS
		MP4StringProperty* pLocationProperty = NULL;
		pUrlAtom->FindProperty(
			"*.location", 
			(MP4Property**)&pLocationProperty);
		ASSERT(pLocationProperty);

		const char* url = pLocationProperty->GetValue();

		VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
			printf("dref url = %s\n", url));

		pFile = (FILE*)-1;

		// attempt to open url if it's a file url 
		// currently this is the only thing we understand
		if (!strncmp(url, "file:", 5)) {
			const char* fileName = url + 5;
			if (!strncmp(fileName, "//", 2)) {
				fileName = strchr(fileName + 2, '/');
			}
			if (fileName) {
				pFile = fopen(fileName, "rb");
				if (!pFile) {
					pFile = (FILE*)-1;
				}
			}
		} 
#else
        throw new MP4Error(errno, "Function not supported when using callbacks", "GetSampleFile");
#endif
	}

	if (m_lastSampleFile) {
#ifndef USE_FILE_CALLBACKS
		fclose(m_lastSampleFile);
#else
        throw new MP4Error(errno, "Function not supported when using callbacks", "GetSampleFile");
#endif
	}

	// cache the answer
	m_lastStsdIndex = stsdIndex;
	m_lastSampleFile = pFile;

	return pFile;
}

u_int64_t MP4Track::GetSampleFileOffset(MP4SampleId sampleId)
{
	u_int32_t stscIndex =
		GetSampleStscIndex(sampleId);

	u_int32_t firstChunk = 
		m_pStscFirstChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSample = 
		m_pStscFirstSampleProperty->GetValue(stscIndex);

	u_int32_t samplesPerChunk = 
		m_pStscSamplesPerChunkProperty->GetValue(stscIndex);

	MP4ChunkId chunkId = firstChunk +
		((sampleId - firstSample) / samplesPerChunk);

	u_int64_t chunkOffset = m_pChunkOffsetProperty->GetValue(chunkId - 1);

	MP4SampleId firstSampleInChunk = 
		sampleId - ((sampleId - firstSample) % samplesPerChunk);

	// need cumulative samples sizes from firstSample to sampleId - 1
	u_int32_t sampleOffset = 0;
	for (MP4SampleId i = firstSampleInChunk; i < sampleId; i++) {
		sampleOffset += GetSampleSize(i);
	}

	return chunkOffset + sampleOffset;
}

void MP4Track::UpdateSampleToChunk(MP4SampleId sampleId,
	 MP4ChunkId chunkId, u_int32_t samplesPerChunk)
{
	u_int32_t numStsc = m_pStscCountProperty->GetValue();

	// if samplesPerChunk == samplesPerChunk of last entry
	if (numStsc && samplesPerChunk == 
	  m_pStscSamplesPerChunkProperty->GetValue(numStsc-1)) {

		// nothing to do

	} else {
		// add stsc entry
		m_pStscFirstChunkProperty->AddValue(chunkId);
		m_pStscSamplesPerChunkProperty->AddValue(samplesPerChunk);
		m_pStscSampleDescrIndexProperty->AddValue(1);
		m_pStscFirstSampleProperty->AddValue(sampleId - samplesPerChunk + 1);

		m_pStscCountProperty->IncrementValue();
	}
}

void MP4Track::UpdateChunkOffsets(u_int64_t chunkOffset)
{
	if (m_pChunkOffsetProperty->GetType() == Integer32Property) {
		((MP4Integer32Property*)m_pChunkOffsetProperty)->AddValue(chunkOffset);
	} else {
		((MP4Integer64Property*)m_pChunkOffsetProperty)->AddValue(chunkOffset);
	}
	m_pChunkCountProperty->IncrementValue();
}

MP4Duration MP4Track::GetFixedSampleDuration()
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	if (numStts == 0) {
		return m_fixedSampleDuration;
	}
	if (numStts != 1) {
		return MP4_INVALID_DURATION;	// sample duration is not fixed
	}
	return m_pSttsSampleDeltaProperty->GetValue(0);
}

bool MP4Track::SetFixedSampleDuration(MP4Duration duration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	// setting this is only allowed before samples have been written
	if (numStts != 0) {
		return false;
	}
	m_fixedSampleDuration = duration;
	return true;
}

void MP4Track::GetSampleTimes(MP4SampleId sampleId,
	MP4Timestamp* pStartTime, MP4Duration* pDuration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			if (pStartTime) {
			  *pStartTime = (sampleId - sid);
			  *pStartTime *= sampleDelta;
			  *pStartTime += elapsed;
			}
			if (pDuration) {
				*pDuration = sampleDelta;
			}
			return;
		}
		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw new MP4Error("sample id out of range", 
		"MP4Track::GetSampleTimes");
}

MP4SampleId MP4Track::GetSampleIdFromTime(
	MP4Timestamp when, 
	bool wantSyncSample) 
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		if (sampleDelta == 0 && sttsIndex < numStts - 1) {
			VERBOSE_READ(m_pFile->GetVerbosity(),
				printf("Warning: Zero sample duration, stts entry %u\n",
				sttsIndex));
		}

		MP4Duration d = when - elapsed;

		if (d <= sampleCount * sampleDelta) {
			MP4SampleId sampleId = sid;
			if (sampleDelta) {
				sampleId += (d / sampleDelta);
			}

			if (wantSyncSample) {
				return GetNextSyncSample(sampleId);
			}
			return sampleId;
		}

		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw new MP4Error("time out of range", 
		"MP4Track::GetSampleIdFromTime");

	return 0; // satisfy MS compiler
}

void MP4Track::UpdateSampleTimes(MP4Duration duration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	// if duration == duration of last entry
	if (numStts 
	  && duration == m_pSttsSampleDeltaProperty->GetValue(numStts-1)) {
		// increment last entry sampleCount
		m_pSttsSampleCountProperty->IncrementValue(1, numStts-1);

	} else {
		// add stts entry, sampleCount = 1, sampleDuration = duration
		m_pSttsSampleCountProperty->AddValue(1);
		m_pSttsSampleDeltaProperty->AddValue(duration);
		m_pSttsCountProperty->IncrementValue();;
	}
}

u_int32_t MP4Track::GetSampleCttsIndex(MP4SampleId sampleId, 
	MP4SampleId* pFirstSampleId)
{
	u_int32_t numCtts = m_pCttsCountProperty->GetValue();

	MP4SampleId sid = 1;
	
	for (u_int32_t cttsIndex = 0; cttsIndex < numCtts; cttsIndex++) {
		u_int32_t sampleCount = 
			m_pCttsSampleCountProperty->GetValue(cttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			if (pFirstSampleId) {
				*pFirstSampleId = sid;
			}
			return cttsIndex;
		}
		sid += sampleCount;
	}

	throw new MP4Error("sample id out of range", 
		"MP4Track::GetSampleCttsIndex");
	return 0; // satisfy MS compiler
}

MP4Duration MP4Track::GetSampleRenderingOffset(MP4SampleId sampleId)
{
	if (m_pCttsCountProperty == NULL) {
		return 0;
	}
	if (m_pCttsCountProperty->GetValue() == 0) {
		return 0;
	}

	u_int32_t cttsIndex = GetSampleCttsIndex(sampleId);

	return m_pCttsSampleOffsetProperty->GetValue(cttsIndex);
}

void MP4Track::UpdateRenderingOffsets(MP4SampleId sampleId, 
	MP4Duration renderingOffset)
{
	// if ctts atom doesn't exist
	if (m_pCttsCountProperty == NULL) {

		// no rendering offset, so nothing to do
		if (renderingOffset == 0) {
			return;
		}

		// else create a ctts atom
		MP4Atom* pCttsAtom = AddAtom("trak.mdia.minf.stbl", "ctts");

		// and get handles on the properties
		pCttsAtom->FindProperty(
			"ctts.entryCount",
			(MP4Property**)&m_pCttsCountProperty);

		pCttsAtom->FindProperty(
			"ctts.entries.sampleCount",
			(MP4Property**)&m_pCttsSampleCountProperty);

		pCttsAtom->FindProperty(
			"ctts.entries.sampleOffset",
			(MP4Property**)&m_pCttsSampleOffsetProperty);

		// if this is not the first sample
		if (sampleId > 1) {
			// add a ctts entry for all previous samples
			// with rendering offset equal to zero
			m_pCttsSampleCountProperty->AddValue(sampleId - 1);
			m_pCttsSampleOffsetProperty->AddValue(0);
			m_pCttsCountProperty->IncrementValue();;
		}
	}

	// ctts atom exists (now)

	u_int32_t numCtts = m_pCttsCountProperty->GetValue();

	// if renderingOffset == renderingOffset of last entry
	if (numCtts && renderingOffset
	   == m_pCttsSampleOffsetProperty->GetValue(numCtts-1)) {

		// increment last entry sampleCount
		m_pCttsSampleCountProperty->IncrementValue(1, numCtts-1);

	} else {
		// add ctts entry, sampleCount = 1, sampleOffset = renderingOffset
		m_pCttsSampleCountProperty->AddValue(1);
		m_pCttsSampleOffsetProperty->AddValue(renderingOffset);
		m_pCttsCountProperty->IncrementValue();
	}
}

void MP4Track::SetSampleRenderingOffset(MP4SampleId sampleId,
	 MP4Duration renderingOffset)
{
	// check if any ctts entries exist
	if (m_pCttsCountProperty == NULL
	  || m_pCttsCountProperty->GetValue() == 0) {
		// if not then Update routine can be used 
		// to create a ctts entry for samples before this one
		// and a ctts entry for this sample 
		UpdateRenderingOffsets(sampleId, renderingOffset);

		// but we also need a ctts entry 
		// for all samples after this one
		u_int32_t afterSamples = GetNumberOfSamples() - sampleId;

		if (afterSamples) {
			m_pCttsSampleCountProperty->AddValue(afterSamples);
			m_pCttsSampleOffsetProperty->AddValue(0);
			m_pCttsCountProperty->IncrementValue();;
		}

		return;
	}

	MP4SampleId firstSampleId;
	u_int32_t cttsIndex = GetSampleCttsIndex(sampleId, &firstSampleId);

	// do nothing in the degenerate case
	if (renderingOffset == 
	  m_pCttsSampleOffsetProperty->GetValue(cttsIndex)) {
		return;
	}

	u_int32_t sampleCount =
		m_pCttsSampleCountProperty->GetValue(cttsIndex);

	// if this sample has it's own ctts entry
	if (sampleCount == 1) {
		// then just set the value, 
		// note we don't attempt to collapse entries
		m_pCttsSampleOffsetProperty->SetValue(renderingOffset, cttsIndex);
		return;
	}

	MP4SampleId lastSampleId = firstSampleId + sampleCount - 1;

	// else we share this entry with other samples
	// we need to insert our own entry
	if (sampleId == firstSampleId) {
		// our sample is the first one
		m_pCttsSampleCountProperty->
			InsertValue(1, cttsIndex);
		m_pCttsSampleOffsetProperty->
			InsertValue(renderingOffset, cttsIndex);

		m_pCttsSampleCountProperty->
			SetValue(sampleCount - 1, cttsIndex + 1);

		m_pCttsCountProperty->IncrementValue();

	} else if (sampleId == lastSampleId) {
		// our sample is the last one
		m_pCttsSampleCountProperty->
			InsertValue(1, cttsIndex + 1);
		m_pCttsSampleOffsetProperty->
			InsertValue(renderingOffset, cttsIndex + 1);

		m_pCttsSampleCountProperty->
			SetValue(sampleCount - 1, cttsIndex);

		m_pCttsCountProperty->IncrementValue();

	} else {
		// our sample is in the middle, UGH!

		// insert our new entry
		m_pCttsSampleCountProperty->
			InsertValue(1, cttsIndex + 1);
		m_pCttsSampleOffsetProperty->
			InsertValue(renderingOffset, cttsIndex + 1);

		// adjust count of previous entry
		m_pCttsSampleCountProperty->
			SetValue(sampleId - firstSampleId, cttsIndex);

		// insert new entry for those samples beyond our sample
		m_pCttsSampleCountProperty->
			InsertValue(lastSampleId - sampleId, cttsIndex + 2);
		u_int32_t oldRenderingOffset =
			m_pCttsSampleOffsetProperty->GetValue(cttsIndex);
		m_pCttsSampleOffsetProperty->
			InsertValue(oldRenderingOffset, cttsIndex + 2);

		m_pCttsCountProperty->IncrementValue(2);
	}
}

bool MP4Track::IsSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return true;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId == syncSampleId) {
			return true;
		} 
		if (sampleId < syncSampleId) {
			break;
		}
	}

	return false;
}

// N.B. "next" is inclusive of this sample id
MP4SampleId MP4Track::GetNextSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return sampleId;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId > syncSampleId) {
			continue;
		}
		return syncSampleId;
	}

	// LATER check stsh for alternate sample

	return MP4_INVALID_SAMPLE_ID;
}

void MP4Track::UpdateSyncSamples(MP4SampleId sampleId, bool isSyncSample)
{
	if (isSyncSample) {
		// if stss atom exists, add entry
		if (m_pStssCountProperty) {
			m_pStssSampleProperty->AddValue(sampleId);
			m_pStssCountProperty->IncrementValue();
		} // else nothing to do (yet)

	} else { // !isSyncSample
		// if stss atom doesn't exist, create one
		if (m_pStssCountProperty == NULL) {

			MP4Atom* pStssAtom = AddAtom("trak.mdia.minf.stbl", "stss");

			pStssAtom->FindProperty(
				"stss.entryCount",
				(MP4Property**)&m_pStssCountProperty);

			pStssAtom->FindProperty(
				"stss.entries.sampleNumber",
				(MP4Property**)&m_pStssSampleProperty);

			// set values for all samples that came before this one
			for (MP4SampleId sid = 1; sid < sampleId; sid++) {
				m_pStssSampleProperty->AddValue(sid);
				m_pStssCountProperty->IncrementValue();
			}
		} // else nothing to do
	}
}

MP4Atom* MP4Track::AddAtom(char* parentName, char* childName)
{
	MP4Atom* pChildAtom = MP4Atom::CreateAtom(childName);

	MP4Atom* pParentAtom = m_pTrakAtom->FindAtom(parentName);
	ASSERT(pParentAtom);

	pParentAtom->AddChildAtom(pChildAtom);

	pChildAtom->Generate();

	return pChildAtom;
}

u_int64_t MP4Track::GetDuration()
{
	return m_pMediaDurationProperty->GetValue();
}

u_int32_t MP4Track::GetTimeScale()
{
	return m_pTimeScaleProperty->GetValue();
}

void MP4Track::UpdateDurations(MP4Duration duration)
{
	// update media, track, and movie durations
	m_pMediaDurationProperty->SetValue(
		m_pMediaDurationProperty->GetValue() + duration);

	MP4Duration movieDuration = ToMovieDuration(duration);
	m_pTrackDurationProperty->SetValue(
		m_pTrackDurationProperty->GetValue() + movieDuration);

	m_pFile->UpdateDuration(m_pTrackDurationProperty->GetValue());
}

MP4Duration MP4Track::ToMovieDuration(MP4Duration trackDuration)
{
	return (trackDuration * m_pFile->GetTimeScale()) 
		/ m_pTimeScaleProperty->GetValue();
}

void MP4Track::UpdateModificationTimes()
{
	// update media and track modification times
	MP4Timestamp now = MP4GetAbsTimestamp();
	m_pMediaModificationProperty->SetValue(now);
	m_pTrackModificationProperty->SetValue(now);
}

u_int32_t MP4Track::GetNumberOfChunks()
{
	return m_pChunkOffsetProperty->GetCount();
}

u_int32_t MP4Track::GetChunkStscIndex(MP4ChunkId chunkId)
{
	u_int32_t stscIndex;
	u_int32_t numStscs = m_pStscCountProperty->GetValue();

	ASSERT(chunkId);
	ASSERT(numStscs > 0);

	for (stscIndex = 0; stscIndex < numStscs; stscIndex++) {
		if (chunkId < m_pStscFirstChunkProperty->GetValue(stscIndex)) {
			ASSERT(stscIndex != 0);
			break;
		}
	}
	return stscIndex - 1;
}

MP4Timestamp MP4Track::GetChunkTime(MP4ChunkId chunkId)
{
	u_int32_t stscIndex = GetChunkStscIndex(chunkId);

	MP4ChunkId firstChunkId = 
		m_pStscFirstChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSample = 
		m_pStscFirstSampleProperty->GetValue(stscIndex);

	u_int32_t samplesPerChunk = 
		m_pStscSamplesPerChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSampleInChunk = 
		firstSample + ((chunkId - firstChunkId) * samplesPerChunk);

	MP4Timestamp chunkTime;

	GetSampleTimes(firstSampleInChunk, &chunkTime, NULL);

	return chunkTime;
}

u_int32_t MP4Track::GetChunkSize(MP4ChunkId chunkId)
{
	u_int32_t stscIndex = GetChunkStscIndex(chunkId);

	MP4ChunkId firstChunkId = 
		m_pStscFirstChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSample = 
		m_pStscFirstSampleProperty->GetValue(stscIndex);

	u_int32_t samplesPerChunk = 
		m_pStscSamplesPerChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSampleInChunk = 
		firstSample + ((chunkId - firstChunkId) * samplesPerChunk);

	// need cumulative sizes of samples in chunk 
	u_int32_t chunkSize = 0;
	for (u_int32_t i = 0; i < samplesPerChunk; i++) {
		chunkSize += GetSampleSize(firstSampleInChunk + i);
	}

	return chunkSize;
}

void MP4Track::ReadChunk(MP4ChunkId chunkId, 
	u_int8_t** ppChunk, u_int32_t* pChunkSize)
{
	ASSERT(chunkId);
	ASSERT(ppChunk);
	ASSERT(pChunkSize);

	u_int64_t chunkOffset = 
		m_pChunkOffsetProperty->GetValue(chunkId - 1);

	*pChunkSize = GetChunkSize(chunkId);
	*ppChunk = (u_int8_t*)MP4Malloc(*pChunkSize);

	VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
		printf("ReadChunk: track %u id %u offset 0x"LLX" size %u (0x%x)\n",
			m_trackId, chunkId, chunkOffset, *pChunkSize, *pChunkSize));

	u_int64_t oldPos = m_pFile->GetPosition(); // only used in mode == 'w'
	try {
		m_pFile->SetPosition(chunkOffset);
		m_pFile->ReadBytes(*ppChunk, *pChunkSize);
	}
	catch (MP4Error* e) {
		// let's not leak memory
		MP4Free(*ppChunk);
		*ppChunk = NULL;

		if (m_pFile->GetMode() == 'w') {
			m_pFile->SetPosition(oldPos);
		}
		throw e;
	}

	if (m_pFile->GetMode() == 'w') {
		m_pFile->SetPosition(oldPos);
	}
}

void MP4Track::RewriteChunk(MP4ChunkId chunkId, 
	u_int8_t* pChunk, u_int32_t chunkSize)
{
	u_int64_t chunkOffset = m_pFile->GetPosition();

	m_pFile->WriteBytes(pChunk, chunkSize);

	m_pChunkOffsetProperty->SetValue(chunkOffset, chunkId - 1);

	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("RewriteChunk: track %u id %u offset 0x"LLX" size %u (0x%x)\n",
			m_trackId, chunkId, chunkOffset, chunkSize, chunkSize)); 
}

// map track type name aliases to official names

const char* MP4Track::NormalizeTrackType(const char* type)
{
	if (!strcasecmp(type, "vide")
	  || !strcasecmp(type, "video")
	  || !strcasecmp(type, "mp4v")
	  || !strcasecmp(type, "encv")) {
		return MP4_VIDEO_TRACK_TYPE;
	}

	if (!strcasecmp(type, "soun")
	  || !strcasecmp(type, "sound")
	  || !strcasecmp(type, "audio")
	  || !strcasecmp(type, "enca")  // ismacrypt 
	  || !strcasecmp(type, "mp4a")) {
		return MP4_AUDIO_TRACK_TYPE;
	}

	if (!strcasecmp(type, "sdsm")
	  || !strcasecmp(type, "scene")
	  || !strcasecmp(type, "bifs")) {
		return MP4_SCENE_TRACK_TYPE;
	}

	if (!strcasecmp(type, "odsm")
	  || !strcasecmp(type, "od")) {
		return MP4_OD_TRACK_TYPE;
	}

	return type;
}

bool MP4Track::InitEditListProperties()
{
	m_pElstCountProperty = NULL;
	m_pElstMediaTimeProperty = NULL;
	m_pElstDurationProperty = NULL;
	m_pElstRateProperty = NULL;
	m_pElstReservedProperty = NULL;

	MP4Atom* pElstAtom =
		m_pTrakAtom->FindAtom("trak.edts.elst");

	if (!pElstAtom) {
		return false;
	}

	pElstAtom->FindProperty(
		"elst.entryCount",
		(MP4Property**)&m_pElstCountProperty);

	pElstAtom->FindProperty(
		"elst.entries.mediaTime",
		(MP4Property**)&m_pElstMediaTimeProperty);

	pElstAtom->FindProperty(
		"elst.entries.segmentDuration",
		(MP4Property**)&m_pElstDurationProperty);

	pElstAtom->FindProperty(
		"elst.entries.mediaRate",
		(MP4Property**)&m_pElstRateProperty);

	pElstAtom->FindProperty(
		"elst.entries.reserved",
		(MP4Property**)&m_pElstReservedProperty);

	return m_pElstCountProperty
		&& m_pElstMediaTimeProperty
		&& m_pElstDurationProperty
		&& m_pElstRateProperty
		&& m_pElstReservedProperty;
}

MP4EditId MP4Track::AddEdit(MP4EditId editId)
{
	if (!m_pElstCountProperty) {
		m_pFile->AddDescendantAtoms(m_pTrakAtom, "edts.elst");
		InitEditListProperties();
	}

	if (editId == MP4_INVALID_EDIT_ID) {
		editId = m_pElstCountProperty->GetValue() + 1;
	}

	m_pElstMediaTimeProperty->InsertValue(0, editId - 1);
	m_pElstDurationProperty->InsertValue(0, editId - 1);
	m_pElstRateProperty->InsertValue(1, editId - 1);
	m_pElstReservedProperty->InsertValue(0, editId - 1);

	m_pElstCountProperty->IncrementValue();

	return editId;
}

void MP4Track::DeleteEdit(MP4EditId editId)
{
	if (editId == MP4_INVALID_EDIT_ID) {
		throw new MP4Error("edit id can't be zero", 
			"MP4Track::DeleteEdit");
	}

	if (!m_pElstCountProperty
	  || m_pElstCountProperty->GetValue() == 0) {
		throw new MP4Error("no edits exist", 
			"MP4Track::DeleteEdit");
	}

	m_pElstMediaTimeProperty->DeleteValue(editId - 1);
	m_pElstDurationProperty->DeleteValue(editId - 1);
	m_pElstRateProperty->DeleteValue(editId - 1);
	m_pElstReservedProperty->DeleteValue(editId - 1);

	m_pElstCountProperty->IncrementValue(-1);

	// clean up if last edit is deleted
	if (m_pElstCountProperty->GetValue() == 0) {
		m_pElstCountProperty = NULL;
		m_pElstMediaTimeProperty = NULL;
		m_pElstDurationProperty = NULL;
		m_pElstRateProperty = NULL;
		m_pElstReservedProperty = NULL;

		m_pTrakAtom->DeleteChildAtom(
			m_pTrakAtom->FindAtom("trak.edts"));
	}
}

MP4Timestamp MP4Track::GetEditStart(
	MP4EditId editId) 
{
	if (editId == MP4_INVALID_EDIT_ID) {
		return MP4_INVALID_TIMESTAMP;
	} else if (editId == 1) {
		return 0;
	}
	return (MP4Timestamp)GetEditTotalDuration(editId - 1);
}	

MP4Duration MP4Track::GetEditTotalDuration(
	MP4EditId editId)
{
	u_int32_t numEdits = 0;

	if (m_pElstCountProperty) {
		numEdits = m_pElstCountProperty->GetValue();
	}

	if (editId == MP4_INVALID_EDIT_ID) {
		editId = numEdits;
	}

	if (numEdits == 0 || editId > numEdits) {
		return MP4_INVALID_DURATION;
	}

	MP4Duration totalDuration = 0;

	for (MP4EditId eid = 1; eid <= editId; eid++) {
		totalDuration += 
			m_pElstDurationProperty->GetValue(eid - 1);
	}

	return totalDuration;
}

MP4SampleId MP4Track::GetSampleIdFromEditTime(
	MP4Timestamp editWhen, 
	MP4Timestamp* pStartTime, 
	MP4Duration* pDuration)
{
	MP4SampleId sampleId = MP4_INVALID_SAMPLE_ID;
	u_int32_t numEdits = 0;

	if (m_pElstCountProperty) {
		numEdits = m_pElstCountProperty->GetValue();
	}

	if (numEdits) {
		MP4Duration editElapsedDuration = 0;

		for (MP4EditId editId = 1; editId <= numEdits; editId++) {
			// remember edit segment's start time (in edit timeline)
			MP4Timestamp editStartTime = 
				(MP4Timestamp)editElapsedDuration;

			// accumulate edit segment's duration
			editElapsedDuration += 
				m_pElstDurationProperty->GetValue(editId - 1);

			// calculate difference between the specified edit time
			// and the end of this edit segment
			if (editElapsedDuration - editWhen <= 0) {
				// the specified time has not yet been reached
				continue;
			}

			// 'editWhen' is within this edit segment

			// calculate the specified edit time
			// relative to just this edit segment
			MP4Duration editOffset =
				editWhen - editStartTime;

			// calculate the media (track) time that corresponds
			// to the specified edit time based on the edit list
			MP4Timestamp mediaWhen = 
				m_pElstMediaTimeProperty->GetValue(editId - 1)
				+ editOffset;

			// lookup the sample id for the media time
			sampleId = GetSampleIdFromTime(mediaWhen, false);

			// lookup the sample's media start time and duration
			MP4Timestamp sampleStartTime;
			MP4Duration sampleDuration;

			GetSampleTimes(sampleId, &sampleStartTime, &sampleDuration);

			// calculate the difference if any between when the sample
			// would naturally start and when it starts in the edit timeline 
			MP4Duration sampleStartOffset =
				mediaWhen - sampleStartTime;

			// calculate the start time for the sample in the edit time line
			MP4Timestamp editSampleStartTime =
				editWhen - MIN(editOffset, sampleStartOffset);

			MP4Duration editSampleDuration = 0;

			// calculate how long this sample lasts in the edit list timeline
			if (m_pElstRateProperty->GetValue(editId - 1) == 0) {
				// edit segment is a "dwell"
				// so sample duration is that of the edit segment
				editSampleDuration =
					m_pElstDurationProperty->GetValue(editId - 1);

			} else {
				// begin with the natural sample duration
				editSampleDuration = sampleDuration;

				// now shorten that if the edit segment starts
				// after the sample would naturally start 
				if (editOffset < sampleStartOffset) {
					editSampleDuration -= sampleStartOffset - editOffset;
				}

				// now shorten that if the edit segment ends
				// before the sample would naturally end
				if (editElapsedDuration 
				  < editSampleStartTime + sampleDuration) {
					editSampleDuration -= (editSampleStartTime + sampleDuration) 
						- editElapsedDuration;
				}
			}

			if (pStartTime) {
				*pStartTime = editSampleStartTime;
			}

			if (pDuration) {
				*pDuration = editSampleDuration;
			}

			VERBOSE_EDIT(m_pFile->GetVerbosity(),
				printf("GetSampleIdFromEditTime: when %llu "
					"sampleId %u start %llu duration %lld\n", 
					editWhen, sampleId, 
					editSampleStartTime, editSampleDuration));

			return sampleId;
		}

		throw new MP4Error("time out of range", 
			"MP4Track::GetSampleIdFromEditTime");

	} else { // no edit list
		sampleId = GetSampleIdFromTime(editWhen, false);

		if (pStartTime || pDuration) {
			GetSampleTimes(sampleId, pStartTime, pDuration);
		}
	}

	return sampleId;
}

