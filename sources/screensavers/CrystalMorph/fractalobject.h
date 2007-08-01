#ifndef __FRACTAL_H_
#define __FRACTAL_H_

#include "glutil.h"
#include "sphere.h"
#include "Util.h"

#define STEP_TIME 0.1

#define MAX_TRANSFORMS 20

typedef enum {
	FRACTAL_BASE_SPHERE,
    FRACTAL_BASE_PYRAMID,
    FRACTAL_BASE_CUBE
} FractalBaseObject;

struct XYZ
{
	float x;
	float y;
	float z;
};

struct FractalTransform
{
	XYZ translation;
	XYZ scaling;
	XYZ rotation;
	
  MorphColor color;
	void Init();
};

struct FractalData
{
	int numTransforms;
	FractalTransform transforms[MAX_TRANSFORMS];
	FractalBaseObject base;
	
	void Init();
};


class Fractal
{
public:
	Fractal();
	~Fractal();
	Fractal(int numTransforms);

	void Init(int numTransforms);

	void Render();
	void RenderChild(int depth, int parentTransform, ColorRGB childColor);
	//void RenderSelection(int depth);
	void RenderBase(ColorRGB color);
	void SetSelection(bool drawSelection, int numSelected);
	void ToggleCutoffType();
	void IncrementCutoff(bool up);
	void SetCutoff(int cutoff);
	FractalData *GetDataHandle();
	void setRedBlueRender(bool);

private:

	void ApplyTransform(int iTransform);
	void InvertTransform(int iTransform);
	int myCutoffDepth;
	int mySelectedTransform;
  float myColorLerpAmount;
	bool selectionOn;
	FractalData myData;
	
	bool redBlueRender;
	C_Sphere * mySphere;

};

#endif
