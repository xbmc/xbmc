#ifndef __FRACTALCONTROL_H_
#define __FRACTALCONTROL_H_

//#include <GL/glut.h>       // also includes glu and gl correctly
//#include <cstdlib>         // for atoi
//#include <cstdio>          // for sprintf
//#include <cmath>
#include "fractalobject.h"
#include "Fractal.h"

#define MAX_PRESETS 10

class FractalController
{
public:
	FractalController(Fractal* fractal, FractalSettings * settings);

	void HandleKeyBoardHit(char key);
	void StartMorph();
	void UpdateFractalData();
	void LerpToDesired(float ratio);
	void CopyFractalData(FractalData *src, FractalData *dest);
	void SetDesiredToPreset(int i);
  void SetToRandomFractal(int numTransforms = -1);
	void LerpXYZ(float ratio, XYZ *src1, XYZ *src2, XYZ *dst);
	void GeneratePresets();
	void SetAnimation(bool on);
	void SetMorphSpeed(float time);
	void AnimateDesired();
	void SetSelection(bool on, int selection);
	void IncrementSelectedXYZ(float dx, float dy, float dz);
	void FindCutoff(int numtransforms);


private:
	bool myAnimating;
	float myRatio;
	float mySpeed;
	float myCounter;

	bool selectionOn;
	int mySelectedTransform;
	int mySelectedXYZ;

	Fractal * myFractal;
	FractalData *activeFractalData;
	FractalData myStartingFractalData;
	FractalData myDesiredFractalData;
	FractalData myAnimationBase;

	int myNumPresets;
	FractalData presets[MAX_PRESETS];
  FractalSettings * settings;
};

#endif
