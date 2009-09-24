
#include "G-Force_core.h"
#include "G-Force.h"

GForce *gForce;

GForceCore::GForceCore(const char* szColorMaps, const char* szDeltaFields, const char* szParticles, const char* szWaveShapes) {
	gForce = new GForce(szColorMaps,szDeltaFields,szParticles,szWaveShapes);
}

GForceCore::~GForceCore() {
	delete gForce;
}

void GForceCore::setDrawParameter(unsigned long *inFrameBuffer, int inPitch) {
	gForce->setDrawParameter(inFrameBuffer, inPitch);
}

void GForceCore::renderSample( long time , signed short int audioData[2][512]) {
	static float floatBuffer[512];
	int i;
	for(i=0; i<512; i++)
		floatBuffer[i] = audioData[0][i] >> 8;

	gForce->RecordSample( time, floatBuffer, .0061f, 250 );
}

void GForceCore::SetWinPort( Rect* inRect ) {
	gForce->SetWinPort(NULL, inRect);
}

