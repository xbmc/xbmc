
#pragma once

struct Rect {
	short left, top, right, bottom;
};
#define Rect_Defined


class GForceCore {
public:
	GForceCore(const char* szColorMaps, const char* szDeltaFields, const char* szParticles, const char* szWaveShapes);
	~GForceCore();

	void setDrawParameter(unsigned long *inFrameBuffer, int inPitch);
	void renderSample( long time , signed short int audioData[2][512]);
	void SetWinPort( Rect* inRect );
};

