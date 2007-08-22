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

#ifndef __MP4_FILE_INCLUDED__
#define __MP4_FILE_INCLUDED__

// forward declarations
class MP4Atom;
class MP4Property;
class MP4Float32Property;
class MP4StringProperty;
class MP4BytesProperty;
class MP4Descriptor;
class MP4DescriptorProperty;


#ifdef USE_FILE_CALLBACKS
typedef u_int32_t (*MP4OpenCallback)(const char *pName, const char *mode, void *userData);
typedef void (*MP4CloseCallback)(void *userData);
typedef u_int32_t (*MP4ReadCallback)(void *pBuffer, unsigned int nBytesToRead, void *userData);
typedef u_int32_t (*MP4WriteCallback)(void *pBuffer, unsigned int nBytesToWrite, void *userData);
typedef int32_t (*MP4SetposCallback)(u_int32_t pos, void *userData);
typedef int64_t (*MP4GetposCallback)(void *userData);
typedef int64_t (*MP4FilesizeCallback)(void *userData);
#endif


class MP4File {
public: /* equivalent to MP4 library API */
	MP4File(u_int32_t verbosity = 0);
	~MP4File();

	/* file operations */
	void Read(const char* fileName);
	void Create(const char* fileName, bool use64bits);
	void Modify(const char* fileName);
	void Optimize(const char* orgFileName, 
		const char* newFileName = NULL);
	void Dump(FILE* pDumpFile = NULL, bool dumpImplicits = false);
	void Close();

	/* library property per file */

	u_int32_t GetVerbosity() {
		return m_verbosity;
	}
	void SetVerbosity(u_int32_t verbosity) {
		m_verbosity = verbosity;
	}

	bool Use64Bits() {
		return m_use64bits;
	}

	/* file properties */

	u_int64_t GetIntegerProperty(const char* name);
	float GetFloatProperty(const char* name);
	const char* GetStringProperty(const char* name);
	void GetBytesProperty(const char* name,
		u_int8_t** ppValue, u_int32_t* pValueSize);

	void SetIntegerProperty(const char* name, u_int64_t value);
	void SetFloatProperty(const char* name, float value);
	void SetStringProperty(const char* name, const char* value);
	void SetBytesProperty(const char* name, 
		const u_int8_t* pValue, u_int32_t valueSize);

	// file level convenience functions

	MP4Duration GetDuration();
	void SetDuration(MP4Duration value);

	u_int32_t GetTimeScale();
	void SetTimeScale(u_int32_t value);

	u_int8_t GetODProfileLevel();
	void SetODProfileLevel(u_int8_t value);

	u_int8_t GetSceneProfileLevel();
	void SetSceneProfileLevel(u_int8_t value);

	u_int8_t GetVideoProfileLevel();
	void SetVideoProfileLevel(u_int8_t value);

	u_int8_t GetAudioProfileLevel();
	void SetAudioProfileLevel(u_int8_t value);

	u_int8_t GetGraphicsProfileLevel();
	void SetGraphicsProfileLevel(u_int8_t value);

	const char* GetSessionSdp();
	void SetSessionSdp(const char* sdpString);
	void AppendSessionSdp(const char* sdpString);

	/* track operations */

	MP4TrackId AddTrack(const char* type, u_int32_t timeScale = 1000);
	void DeleteTrack(MP4TrackId trackId);

	u_int32_t GetNumberOfTracks(const char* type = NULL, u_int8_t subType = 0);

	MP4TrackId AllocTrackId();
	MP4TrackId FindTrackId(u_int16_t trackIndex, 
		const char* type = NULL, u_int8_t subType = 0);
	u_int16_t FindTrackIndex(MP4TrackId trackId);
	u_int16_t FindTrakAtomIndex(MP4TrackId trackId);

	/* track properties */

	u_int64_t GetTrackIntegerProperty(
		MP4TrackId trackId, const char* name);
	float GetTrackFloatProperty(
		MP4TrackId trackId, const char* name);
	const char* GetTrackStringProperty(
		MP4TrackId trackId, const char* name);
	void GetTrackBytesProperty(
		MP4TrackId trackId, const char* name,
		u_int8_t** ppValue, u_int32_t* pValueSize);

	void SetTrackIntegerProperty(
		MP4TrackId trackId, const char* name, int64_t value);
	void SetTrackFloatProperty(
		MP4TrackId trackId, const char* name, float value);
	void SetTrackStringProperty(
		MP4TrackId trackId, const char* name, const char* value);
	void SetTrackBytesProperty(
		MP4TrackId trackId, const char* name, 
		const u_int8_t* pValue, u_int32_t valueSize);

	/* sample operations */

	u_int32_t GetSampleSize(MP4TrackId trackId, MP4SampleId sampleId);

	u_int32_t GetTrackMaxSampleSize(MP4TrackId trackId);

	MP4SampleId GetSampleIdFromTime(MP4TrackId trackId, 
		MP4Timestamp when, bool wantSyncSample = false);

	MP4Timestamp GetSampleTime(
		MP4TrackId trackId, MP4SampleId sampleId);

	MP4Duration GetSampleDuration(
		MP4TrackId trackId, MP4SampleId sampleId);

	MP4Duration GetSampleRenderingOffset(
		MP4TrackId trackId, MP4SampleId sampleId);

	bool GetSampleSync(
		MP4TrackId trackId, MP4SampleId sampleId);

	void ReadSample(
		// input parameters
		MP4TrackId trackId, 
		MP4SampleId sampleId,
		// output parameters
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes, 
		MP4Timestamp* pStartTime = NULL, 
		MP4Duration* pDuration = NULL,
		MP4Duration* pRenderingOffset = NULL, 
		bool* pIsSyncSample = NULL);

	void WriteSample(
		MP4TrackId trackId,
		const u_int8_t* pBytes, 
		u_int32_t numBytes,
		MP4Duration duration = 0,
		MP4Duration renderingOffset = 0, 
		bool isSyncSample = true);

	void SetSampleRenderingOffset(
		MP4TrackId trackId, 
		MP4SampleId sampleId,
		MP4Duration renderingOffset);

	/* track level convenience functions */

	MP4TrackId AddSystemsTrack(const char* type);

	MP4TrackId AddODTrack();

	MP4TrackId AddSceneTrack();

	MP4TrackId AddAudioTrack(
		u_int32_t timeScale, 
		MP4Duration sampleDuration,
		u_int8_t audioType);

	MP4TrackId AddEncAudioTrack( // ismacrypt
		u_int32_t timeScale, 
		MP4Duration sampleDuration,
		u_int8_t audioType);

	MP4TrackId AddVideoTrack(
		u_int32_t timeScale, 
		MP4Duration sampleDuration,
		u_int16_t width, 
		u_int16_t height, 
		u_int8_t videoType);
	
	MP4TrackId AddEncVideoTrack( // ismacrypt
		u_int32_t timeScale, 
		MP4Duration sampleDuration,
		u_int16_t width, 
		u_int16_t height, 
		u_int8_t videoType);

	MP4TrackId AddHintTrack(MP4TrackId refTrackId);

	MP4SampleId GetTrackNumberOfSamples(MP4TrackId trackId);

	const char* GetTrackType(MP4TrackId trackId);

	MP4Duration GetTrackDuration(MP4TrackId trackId);

	u_int32_t GetTrackTimeScale(MP4TrackId trackId);
	void SetTrackTimeScale(MP4TrackId trackId, u_int32_t value);

	// replacement to GetTrackAudioType and GetTrackVideoType	
	u_int8_t GetTrackEsdsObjectTypeId(MP4TrackId trackId);

	u_int8_t GetTrackAudioType(MP4TrackId trackId);
	u_int8_t GetTrackAudioMpeg4Type(MP4TrackId trackId);
	u_int8_t GetTrackVideoType(MP4TrackId trackId);

	MP4Duration GetTrackFixedSampleDuration(MP4TrackId trackId);

	float GetTrackVideoFrameRate(MP4TrackId trackId);

	void GetTrackESConfiguration(MP4TrackId trackId, 
		u_int8_t** ppConfig, u_int32_t* pConfigSize);
	void SetTrackESConfiguration(MP4TrackId trackId, 
		const u_int8_t* pConfig, u_int32_t configSize);

	const char* GetHintTrackSdp(MP4TrackId hintTrackId);
	void SetHintTrackSdp(MP4TrackId hintTrackId, const char* sdpString);
	void AppendHintTrackSdp(MP4TrackId hintTrackId, const char* sdpString);

	// ISMA specific functions

	void MakeIsmaCompliant(bool addIsmaComplianceSdp = true);

	void CreateIsmaIodFromParams(
		u_int8_t videoProfile,
		u_int32_t videoBitrate,
		u_int8_t* videoConfig,
		u_int32_t videoConfigLength,
		u_int8_t audioProfile,
		u_int32_t audioBitrate,
		u_int8_t* audioConfig,
		u_int32_t audioConfigLength,
		u_int8_t** ppBytes,
		u_int64_t* pNumBytes);

	// time convenience functions

	u_int64_t ConvertFromMovieDuration(
		MP4Duration duration,
		u_int32_t timeScale);

	u_int64_t ConvertFromTrackTimestamp(
		MP4TrackId trackId, 
		MP4Timestamp timeStamp,
		u_int32_t timeScale);

	MP4Timestamp ConvertToTrackTimestamp(
		MP4TrackId trackId, 
		u_int64_t timeStamp,
		u_int32_t timeScale);

	u_int64_t ConvertFromTrackDuration(
		MP4TrackId trackId, 
		MP4Duration duration,
		u_int32_t timeScale);

	MP4Duration ConvertToTrackDuration(
		MP4TrackId trackId, 
		u_int64_t duration,
		u_int32_t timeScale);

	// specialized operations

	void GetHintTrackRtpPayload(
		MP4TrackId hintTrackId,
		char** ppPayloadName = NULL,
		u_int8_t* pPayloadNumber = NULL,
		u_int16_t* pMaxPayloadSize = NULL,
		char **ppEncodingParams = NULL);

	void SetHintTrackRtpPayload(
		MP4TrackId hintTrackId,
		const char* payloadName,
		u_int8_t* pPayloadNumber,
		u_int16_t maxPayloadSize,
		const char *encoding_params,
		bool include_rtp_map,
		bool include_mpeg4_esid);

	MP4TrackId GetHintTrackReferenceTrackId(
		MP4TrackId hintTrackId);

	void ReadRtpHint(
		MP4TrackId hintTrackId,
		MP4SampleId hintSampleId,
		u_int16_t* pNumPackets = NULL);

	u_int16_t GetRtpHintNumberOfPackets(
		MP4TrackId hintTrackId);

	int8_t GetRtpPacketBFrame(
		MP4TrackId hintTrackId,
		u_int16_t packetIndex);

	int32_t GetRtpPacketTransmitOffset(
		MP4TrackId hintTrackId,
		u_int16_t packetIndex);

	void ReadRtpPacket(
		MP4TrackId hintTrackId,
		u_int16_t packetIndex,
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes,
		u_int32_t ssrc = 0,
		bool includeHeader = true,
		bool includePayload = true);

	MP4Timestamp GetRtpTimestampStart(
		MP4TrackId hintTrackId);

	void SetRtpTimestampStart(
		MP4TrackId hintTrackId,
		MP4Timestamp rtpStart);

	void AddRtpHint(
		MP4TrackId hintTrackId,
		bool isBframe, 
		u_int32_t timestampOffset);

	void AddRtpPacket(
		MP4TrackId hintTrackId, 
		bool setMbit,
		int32_t transmitOffset);

	void AddRtpImmediateData(
		MP4TrackId hintTrackId,
		const u_int8_t* pBytes,
		u_int32_t numBytes);

	void AddRtpSampleData(
		MP4TrackId hintTrackId,
		MP4SampleId sampleId,
		u_int32_t dataOffset,
		u_int32_t dataLength);

	void AddRtpESConfigurationPacket(
		MP4TrackId hintTrackId);

	void WriteRtpHint(
		MP4TrackId hintTrackId,
		MP4Duration duration,
		bool isSyncSample);

	u_int8_t AllocRtpPayloadNumber();

	// edit list related

	char* MakeTrackEditName(
		MP4TrackId trackId,
		MP4EditId editId,
		const char* name);

	MP4EditId AddTrackEdit(
		MP4TrackId trackId,
		MP4EditId editId = MP4_INVALID_EDIT_ID);

	void DeleteTrackEdit(
		MP4TrackId trackId,
		MP4EditId editId);

	u_int32_t GetTrackNumberOfEdits(
		MP4TrackId trackId);

	MP4Timestamp GetTrackEditStart(
		MP4TrackId trackId,
		MP4EditId editId);

	MP4Duration GetTrackEditTotalDuration(
		MP4TrackId trackId,
		MP4EditId editId);

	MP4Timestamp GetTrackEditMediaStart(
		MP4TrackId trackId,
		MP4EditId editId);

	void SetTrackEditMediaStart(
		MP4TrackId trackId,
		MP4EditId editId,
		MP4Timestamp startTime);

	MP4Duration GetTrackEditDuration(
		MP4TrackId trackId,
		MP4EditId editId);

	void SetTrackEditDuration(
		MP4TrackId trackId,
		MP4EditId editId,
		MP4Duration duration);

	bool GetTrackEditDwell(
		MP4TrackId trackId,
		MP4EditId editId);

	void SetTrackEditDwell(
		MP4TrackId trackId,
		MP4EditId editId,
		bool dwell);

	MP4SampleId GetSampleIdFromEditTime(
		MP4TrackId trackId,
		MP4Timestamp when,
		MP4Timestamp* pStartTime = NULL,
		MP4Duration* pDuration = NULL);

    /* iTunes metadata handling */
    bool CreateMetadataAtom(const char* name);
    bool MetadataDelete();

    /* set metadata */
    bool SetMetadataName(const char* value);
    bool SetMetadataWriter(const char* value);
    bool SetMetadataAlbum(const char* value);
    bool SetMetadataArtist(const char* value);
    bool SetMetadataTool(const char* value);
    bool SetMetadataComment(const char* value);
    bool SetMetadataYear(const char* value);
    bool SetMetadataTrack(u_int16_t track, u_int16_t totalTracks);
    bool SetMetadataDisk(u_int16_t disk, u_int16_t totalDisks);
    bool SetMetadataGenre(const char* genreIndex);
    bool SetMetadataTempo(u_int16_t tempo);
    bool SetMetadataCompilation(u_int8_t compilation);
    bool SetMetadataCoverArt(u_int8_t *coverArt, u_int32_t size);
    bool SetMetadataFreeForm(char *name, u_int8_t* pValue, u_int32_t valueSize);

    /* get metadata */
    bool GetMetadataByIndex(u_int32_t index,
        const char** ppName,
        u_int8_t** ppValue, u_int32_t* pValueSize);
    bool GetMetadataName(char** value);
    bool GetMetadataWriter(char** value);
    bool GetMetadataAlbum(char** value);
    bool GetMetadataArtist(char** value);
    bool GetMetadataTool(char** value);
    bool GetMetadataComment(char** value);
    bool GetMetadataYear(char** value);
    bool GetMetadataTrack(u_int16_t* track, u_int16_t* totalTracks);
    bool GetMetadataDisk(u_int16_t* disk, u_int16_t* totalDisks);
    bool GetMetadataGenre(char** genre);
    bool GetMetadataTempo(u_int16_t* tempo);
    bool GetMetadataCompilation(u_int8_t* compilation);
    bool GetMetadataCoverArt(u_int8_t **coverArt, u_int32_t* size);
    bool GetMetadataFreeForm(char *name, u_int8_t** pValue, u_int32_t* valueSize);


	/* end of MP4 API */

	/* "protected" interface to be used only by friends in library */

	u_int64_t GetPosition(FILE* pFile = NULL);
	void SetPosition(u_int64_t pos, FILE* pFile = NULL);

	u_int64_t GetSize();

	u_int32_t ReadBytes(
		u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile = NULL);
	u_int64_t ReadUInt(u_int8_t size);
	u_int8_t ReadUInt8();
	u_int16_t ReadUInt16();
	u_int32_t ReadUInt24();
	u_int32_t ReadUInt32();
	u_int64_t ReadUInt64();
	float ReadFixed16();
	float ReadFixed32();
	float ReadFloat();
	char* ReadString();
	char* ReadCountedString(
		u_int8_t charSize = 1, bool allowExpandedCount = false);
	u_int64_t ReadBits(u_int8_t numBits);
	void FlushReadBits();
	u_int32_t ReadMpegLength();

	u_int32_t PeekBytes(
		u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile = NULL);

	void WriteBytes(u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile = NULL);
	void WriteUInt(u_int64_t value, u_int8_t size);
	void WriteUInt8(u_int8_t value);
	void WriteUInt16(u_int16_t value);
	void WriteUInt24(u_int32_t value);
	void WriteUInt32(u_int32_t value);
	void WriteUInt64(u_int64_t value);
	void WriteFixed16(float value);
	void WriteFixed32(float value);
	void WriteFloat(float value);
	void WriteString(char* string);
	void WriteCountedString(char* string, 
		u_int8_t charSize = 1, bool allowExpandedCount = false);
	void WriteBits(u_int64_t bits, u_int8_t numBits);
	void PadWriteBits(u_int8_t pad = 0);
	void FlushWriteBits();
	void WriteMpegLength(u_int32_t value, bool compact = false);

	void EnableMemoryBuffer(
		u_int8_t* pBytes = NULL, u_int64_t numBytes = 0);
	void DisableMemoryBuffer(
		u_int8_t** ppBytes = NULL, u_int64_t* pNumBytes = NULL);

	char GetMode() {
		return m_mode;
	}

	MP4Track* GetTrack(MP4TrackId trackId);

	MP4Duration UpdateDuration(MP4Duration duration);

	MP4Atom* FindAtom(const char* name);

	MP4Atom* AddChildAtom(
		const char* parentName, 
		const char* childName);

	MP4Atom* AddChildAtom(
		MP4Atom* pParentAtom, 
		const char* childName);

	MP4Atom* InsertChildAtom(
		const char* parentName, 
		const char* childName, 
		u_int32_t index);

	MP4Atom* InsertChildAtom(
		MP4Atom* pParentAtom, 
		const char* childName, 
		u_int32_t index);

	MP4Atom* AddDescendantAtoms(
		const char* ancestorName, 
		const char* childName);

	MP4Atom* AddDescendantAtoms(
		MP4Atom* pAncestorAtom,
		const char* childName);

#ifdef USE_FILE_CALLBACKS
    MP4OpenCallback m_MP4fopen;
    MP4CloseCallback m_MP4fclose;
    MP4ReadCallback m_MP4fread;
    MP4WriteCallback m_MP4fwrite;
    MP4SetposCallback m_MP4fsetpos;
    MP4GetposCallback m_MP4fgetpos;
    MP4FilesizeCallback m_MP4filesize;

    void *m_userData;
#endif

protected:
	void Open(const char* fmode);
	void ReadFromFile();
	void GenerateTracks();
	void BeginWrite();
	void FinishWrite();
	void CacheProperties();
	void RewriteMdat(FILE* pReadFile, FILE* pWriteFile);

	const char* TempFileName();
	void Rename(const char* existingFileName, const char* newFileName);

	void ProtectWriteOperation(char* where);

	void FindIntegerProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindFloatProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindStringProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindBytesProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	bool FindProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	void AddTrackToIod(MP4TrackId trackId);

	void RemoveTrackFromIod(MP4TrackId trackId);

	void AddTrackToOd(MP4TrackId trackId);

	void RemoveTrackFromOd(MP4TrackId trackId);

	void GetTrackReferenceProperties(const char* trefName,
		MP4Property** ppCountProperty, MP4Property** ppTrackIdProperty);

	void AddTrackReference(const char* trefName, MP4TrackId refTrackId);

	u_int32_t FindTrackReference(const char* trefName, MP4TrackId refTrackId);

	void RemoveTrackReference(const char* trefName, MP4TrackId refTrackId);

	void AddDataReference(MP4TrackId trackId, const char* url);

	char* MakeTrackName(MP4TrackId trackId, const char* name);

	u_int8_t ConvertTrackTypeToStreamType(const char* trackType);

	void CreateIsmaIodFromFile(
		MP4TrackId odTrackId,
		MP4TrackId sceneTrackId,
		MP4TrackId audioTrackId, 
		MP4TrackId videoTrackId,
		u_int8_t** ppBytes,
		u_int64_t* pNumBytes);

	MP4Descriptor* CreateESD(
		MP4DescriptorProperty* pEsProperty,
		u_int32_t esid,
		u_int8_t objectType,
		u_int8_t streamType,
		u_int32_t bufferSize,
		u_int32_t bitrate,
		u_int8_t* pConfig,
		u_int32_t configLength,
		char* url);

	void CreateIsmaODUpdateCommandFromFileForFile(
		MP4TrackId odTrackId,
		MP4TrackId audioTrackId, 
		MP4TrackId videoTrackId,
		u_int8_t** ppBytes,
		u_int64_t* pNumBytes);

	void CreateIsmaODUpdateCommandFromFileForStream(
		MP4TrackId audioTrackId, 
		MP4TrackId videoTrackId,
		u_int8_t** ppBytes,
		u_int64_t* pNumBytes);

	void CreateIsmaODUpdateCommandForStream(
		MP4DescriptorProperty* pAudioEsdProperty, 
		MP4DescriptorProperty* pVideoEsdProperty,
		u_int8_t** ppBytes,
		u_int64_t* pNumBytes);

	void CreateIsmaSceneCommand(
		bool hasAudio,
		bool hasVideo,
		u_int8_t** ppBytes, 
		u_int64_t* pNumBytes);

protected:
	char*			m_fileName;
	FILE*			m_pFile;
	u_int64_t		m_orgFileSize;
	u_int64_t		m_fileSize;
	MP4Atom*		m_pRootAtom;
	MP4Integer32Array m_trakIds;
	MP4TrackArray	m_pTracks;
	MP4TrackId		m_odTrackId;
	u_int32_t		m_verbosity;
	char			m_mode;
	bool			m_use64bits;
	bool			m_useIsma;

	// cached properties
	MP4IntegerProperty*		m_pModificationProperty;
	MP4Integer32Property*	m_pTimeScaleProperty;
	MP4IntegerProperty*		m_pDurationProperty;

	// read/write in memory
	u_int8_t*	m_memoryBuffer;
	u_int64_t	m_memoryBufferPosition;
	u_int64_t	m_memoryBufferSize;

	// bit read/write buffering
	u_int8_t	m_numReadBits;
	u_int8_t	m_bufReadBits;
	u_int8_t	m_numWriteBits;
	u_int8_t	m_bufWriteBits;

#ifdef USE_FILE_CALLBACKS
    static u_int32_t MP4fopen_cb(const char *pName, const char *mode, void *userData);
    static void MP4fclose_cb(void *userData);
    static u_int32_t MP4fread_cb(void *pBuffer, unsigned int nBytesToRead, void *userData);
    static u_int32_t MP4fwrite_cb(void *pBuffer, unsigned int nBytesToWrite, void *userData);
    static int32_t MP4fsetpos_cb(u_int32_t pos, void *userData);
    static int64_t MP4fgetpos_cb(void *userData);
    static int64_t MP4filesize_cb(void *userData);
#endif
};

#endif /* __MP4_FILE_INCLUDED__ */
