#pragma once

typedef enum TrackType {
	TypeVideo		= 1,
	TypeAudio		= 2,
	TypeComplex		= 3,
	TypeLogo		= 0x10,
	TypeSubtitle	= 0x11,
	TypeControl		= 0x20
};

#pragma pack(push, 1)

struct TrackElement {
    WORD Size;				// Size of this structure
    BYTE Type;				// See TrackType
	BOOL FlagDefault;		// Set if the track is the default for its TrackType.
	BOOL FlagLacing;		// Set if the track may contain blocks using lacing.
	UINT MinCache;			// The minimum number of frames a player should be able to cache during playback.
	UINT MaxCache;			// The maximum cache size required to store referenced frames in and the current frame. 0 means no cache is needed.
	CHAR Language[4];		// Specifies the language of the track, in the ISO-639-2 form. (end with '\0')
};

struct TrackExtendedInfoVideo {
	WORD Size;				// Size of this structure
	BOOL Interlaced;		// Set if the video is interlaced.
	UINT PixelWidth;		// Width of the encoded video frames in pixels.
	UINT PixelHeight;		// Height of the encoded video frames in pixels.
	UINT DisplayWidth;		// Width of the video frames to display.
	UINT DisplayHeight;		// Height of the video frames to display.
	BYTE DisplayUnit;		// Type of the unit for DisplayWidth/Height (0: pixels, 1: centimeters, 2: inches).
	BYTE AspectRatioType;	// Specify the possible modifications to the aspect ratio (0: free resizing, 1: keep aspect ratio, 2: fixed).
};

struct TrackExtendedInfoAudio {
	WORD Size;						// Size of this structure
	FLOAT SamplingFreq;				// Sampling frequency in Hz.
	FLOAT OutputSamplingFrequency;	// Real output sampling frequency in Hz (used for SBR techniques).
	UINT Channels;					// Numbers of channels in the track.
	UINT BitDepth;					// Bits per sample, mostly used for PCM.
};

#pragma pack(pop)

[uuid("03E98D51-DDE7-43aa-B70C-42EF84A3A23D")]
interface ITrackInfo : public IUnknown
{
	STDMETHOD_(UINT, GetTrackCount) () = 0;

	// \param aTrackIdx the track index (from 0 to GetTrackCount()-1)
	STDMETHOD_(BOOL, GetTrackInfo) (UINT aTrackIdx, struct TrackElement* pStructureToFill) = 0;

	// Get an extended information struct relative to the track type
	STDMETHOD_(BOOL, GetTrackExtendedInfo) (UINT aTrackIdx, void* pStructureToFill) = 0;

	STDMETHOD_(BSTR, GetTrackCodecID) (UINT aTrackIdx) = 0;
	STDMETHOD_(BSTR, GetTrackName) (UINT aTrackIdx) = 0;
	STDMETHOD_(BSTR, GetTrackCodecName) (UINT aTrackIdx) = 0;
	STDMETHOD_(BSTR, GetTrackCodecInfoURL) (UINT aTrackIdx) = 0;
	STDMETHOD_(BSTR, GetTrackCodecDownloadURL) (UINT aTrackIdx) = 0;
};
