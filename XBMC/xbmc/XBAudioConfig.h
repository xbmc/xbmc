#pragma once

#define XC_AUDIO_FLAGS 9

class XBAudioConfig
{
public:
	XBAudioConfig();
	~XBAudioConfig();

	bool HasDigitalOutput();

	bool GetAC3Enabled();
	void SetAC3Enabled(bool bEnable);
	bool GetDTSEnabled();
	void SetDTSEnabled(bool bEnable);
	bool NeedsSave();
	void Save();

private:
	DWORD m_dwAudioFlags;
};

extern XBAudioConfig g_audioConfig;