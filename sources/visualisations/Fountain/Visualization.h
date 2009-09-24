#include <xtl.h>

#define szTexFile "D:\\particle.bmp"


	struct VIS_INFO {
		bool bWantsFreq;
		int iSyncDelay;
		//		int iAudioDataLength;
		//		int iFreqDataLength;
	};

	void Create(LPDIRECT3DDEVICE8 pd3dDevice);
	void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	void Render();
	void Stop();
	void GetInfo(VIS_INFO* pInfo);


/////////////////////////
void InitParticles();

/////////////////////////