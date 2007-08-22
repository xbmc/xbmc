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

#ifndef __MP4_TRACK_INCLUDED__
#define __MP4_TRACK_INCLUDED__

typedef u_int32_t MP4ChunkId;

// forward declarations
class MP4File;
class MP4Atom;
class MP4Property;
class MP4IntegerProperty;
class MP4Integer16Property;
class MP4Integer32Property;
class MP4Integer64Property;
class MP4StringProperty;

class MP4Track {
public:
	MP4Track(MP4File* pFile, MP4Atom* pTrakAtom);

	virtual ~MP4Track();

	MP4TrackId GetId() {
		return m_trackId;
	}

	const char* GetType();

	void SetType(const char* type);

	MP4File* GetFile() {
		return m_pFile;
	}

	MP4Atom* GetTrakAtom() {
		return m_pTrakAtom;
	}

	void ReadSample(
		// input parameters
		MP4SampleId sampleId,
		// output parameters
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes, 
		MP4Timestamp* pStartTime = NULL, 
		MP4Duration* pDuration = NULL,
		MP4Duration* pRenderingOffset = NULL, 
		bool* pIsSyncSample = NULL);

	void WriteSample(
		const u_int8_t* pBytes, 
		u_int32_t numBytes,
		MP4Duration duration = 0,
		MP4Duration renderingOffset = 0, 
		bool isSyncSample = true);

	virtual void FinishWrite();

	u_int64_t 	GetDuration();		// in track timeScale units
	u_int32_t	GetTimeScale();
	u_int32_t	GetNumberOfSamples();
	u_int32_t	GetSampleSize(MP4SampleId sampleId);
	u_int32_t	GetMaxSampleSize();
	u_int64_t 	GetTotalOfSampleSizes();
	u_int32_t	GetAvgBitrate();	// in bps
	u_int32_t	GetMaxBitrate();	// in bps

	MP4Duration GetFixedSampleDuration();
	bool		SetFixedSampleDuration(MP4Duration duration);

	void		GetSampleTimes(MP4SampleId sampleId,
					MP4Timestamp* pStartTime, MP4Duration* pDuration);

	bool		IsSyncSample(MP4SampleId sampleId);

	MP4SampleId GetSampleIdFromTime(
		MP4Timestamp when, 
		bool wantSyncSample = false);

	MP4Duration	GetSampleRenderingOffset(MP4SampleId sampleId);
	void		SetSampleRenderingOffset(MP4SampleId sampleId,
					MP4Duration renderingOffset);

	MP4EditId	AddEdit(
		MP4EditId editId = MP4_INVALID_EDIT_ID);

	void		DeleteEdit(
		MP4EditId editId);

	MP4Timestamp GetEditStart(
		MP4EditId editId);

	MP4Timestamp GetEditTotalDuration(
		MP4EditId editId);

	MP4SampleId GetSampleIdFromEditTime(
		MP4Timestamp editWhen, 
		MP4Timestamp* pStartTime = NULL, 
		MP4Duration* pDuration = NULL);

	static const char* NormalizeTrackType(const char* type);

	// special operation for use during hint track packet assembly
	void ReadSampleFragment(
		MP4SampleId sampleId,
		u_int32_t sampleOffset,
		u_int16_t sampleLength,
		u_int8_t* pDest);

	// special operations for use during optimization

	u_int32_t GetNumberOfChunks();

	MP4Timestamp GetChunkTime(MP4ChunkId chunkId);

	void ReadChunk(MP4ChunkId chunkId, 
		u_int8_t** ppChunk, u_int32_t* pChunkSize);

	void RewriteChunk(MP4ChunkId chunkId, 
		u_int8_t* pChunk, u_int32_t chunkSize);

protected:
	bool		InitEditListProperties();

	FILE*		GetSampleFile(MP4SampleId sampleId);
	u_int64_t	GetSampleFileOffset(MP4SampleId sampleId);
	u_int32_t	GetSampleStscIndex(MP4SampleId sampleId);
	u_int32_t	GetChunkStscIndex(MP4ChunkId chunkId);
	u_int32_t	GetChunkSize(MP4ChunkId chunkId);
	u_int32_t	GetSampleCttsIndex(MP4SampleId sampleId, 
					MP4SampleId* pFirstSampleId = NULL);
	MP4SampleId	GetNextSyncSample(MP4SampleId sampleId);

	void UpdateSampleSizes(MP4SampleId sampleId, 
		u_int32_t numBytes);
	bool IsChunkFull(MP4SampleId sampleId);
	void UpdateSampleToChunk(MP4SampleId sampleId,
		 MP4ChunkId chunkId, u_int32_t samplesPerChunk);
	void UpdateChunkOffsets(u_int64_t chunkOffset);
	void UpdateSampleTimes(MP4Duration duration);
	void UpdateRenderingOffsets(MP4SampleId sampleId, 
		MP4Duration renderingOffset);
	void UpdateSyncSamples(MP4SampleId sampleId, 
		bool isSyncSample);

	MP4Atom* AddAtom(char* parentName, char* childName);

	void UpdateDurations(MP4Duration duration);
	MP4Duration ToMovieDuration(MP4Duration trackDuration);

	void UpdateModificationTimes();

	void WriteChunkBuffer();

protected:
	MP4File*	m_pFile;
	MP4Atom* 	m_pTrakAtom;		// moov.trak[]
	MP4TrackId	m_trackId;			// moov.trak[].tkhd.trackId
	MP4StringProperty* m_pTypeProperty;	// moov.trak[].mdia.hdlr.handlerType

	u_int32_t	m_lastStsdIndex;
	FILE*	 	m_lastSampleFile;

	// for efficient construction of hint track packets
	MP4SampleId	m_cachedReadSampleId;
	u_int8_t* 	m_pCachedReadSample;
	u_int32_t	m_cachedReadSampleSize;

	// for writing
	MP4SampleId m_writeSampleId;
	MP4Duration m_fixedSampleDuration;
	u_int8_t* 	m_pChunkBuffer;
	u_int32_t	m_chunkBufferSize;
	u_int32_t	m_chunkSamples;
	MP4Duration m_chunkDuration;

	// controls for chunking
	u_int32_t 	m_samplesPerChunk;
	MP4Duration m_durationPerChunk;

	MP4Integer32Property* m_pTimeScaleProperty;
	MP4IntegerProperty* m_pTrackDurationProperty;		// 32 or 64 bits
	MP4IntegerProperty* m_pMediaDurationProperty;		// 32 or 64 bits
	MP4IntegerProperty* m_pTrackModificationProperty;	// 32 or 64 bits
	MP4IntegerProperty* m_pMediaModificationProperty;	// 32 or 64 bits

	MP4Integer32Property* m_pStszFixedSampleSizeProperty;
	MP4Integer32Property* m_pStszSampleCountProperty;
	MP4Integer32Property* m_pStszSampleSizeProperty;

	MP4Integer32Property* m_pStscCountProperty;
	MP4Integer32Property* m_pStscFirstChunkProperty;
	MP4Integer32Property* m_pStscSamplesPerChunkProperty;
	MP4Integer32Property* m_pStscSampleDescrIndexProperty;
	MP4Integer32Property* m_pStscFirstSampleProperty;

	MP4Integer32Property* m_pChunkCountProperty;
	MP4IntegerProperty*   m_pChunkOffsetProperty;		// 32 or 64 bits

	MP4Integer32Property* m_pSttsCountProperty;
	MP4Integer32Property* m_pSttsSampleCountProperty;
	MP4Integer32Property* m_pSttsSampleDeltaProperty;

	MP4Integer32Property* m_pCttsCountProperty;
	MP4Integer32Property* m_pCttsSampleCountProperty;
	MP4Integer32Property* m_pCttsSampleOffsetProperty;

	MP4Integer32Property* m_pStssCountProperty;
	MP4Integer32Property* m_pStssSampleProperty;

	MP4Integer32Property* m_pElstCountProperty;
	MP4IntegerProperty*   m_pElstMediaTimeProperty;		// 32 or 64 bits
	MP4IntegerProperty*   m_pElstDurationProperty;		// 32 or 64 bits
	MP4Integer16Property* m_pElstRateProperty;
	MP4Integer16Property* m_pElstReservedProperty;
};

MP4ARRAY_DECL(MP4Track, MP4Track*);

#endif /* __MP4_TRACK_INCLUDED__ */
