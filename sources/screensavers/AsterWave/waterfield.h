#pragma once

//#include <cstdlib>         // for atoi
//#include <cstdio>          // for sprintf
#include "Util.h"

#define STEP_TIME 0.1f

struct WaterPoint
{
	float height;
	float velocity;
  DWORD color;
  DWORD avecolor;
  D3DXVECTOR3 normal;
};

class WaterField
{
public:
	WaterField();
	WaterField(float xmin, float xmax, float ymin, float ymax, int xdivs, int ydivs, float height, float elasticity, float viscosity, float tension, float blendability, bool textureMode);
	~WaterField();
	void Init(float xmin, float xmax, float ymin, float ymax, int xdivs, int ydivs, float height, float elasticity, float viscosity, float tension, float blendability, bool textureMode);

  void SetHeight(float xNearest, float yNearest, float spread, float newHeight, DWORD color=INVALID_COLOR);
	void DrawLine(float xStart, float yStart, float xEnd, float yEnd, float width, float newHeight,float strength, DWORD color=INVALID_COLOR);
	float GetHeight(float xNearest, float yNearest);
	void Render();
	void Step();
	void Step(float time);
  float xMin(){return myXmin;}
  float xMax(){return myXmax;}
  float yMin(){return myYmin;}
  float yMax(){return myYmax;}


private:
	void GetIndexNearestXY(float x, float y, int *i, int *j);
	void SetNormalForPoint(int i, int j);
  D3DXVECTOR3 * NormalForPoints(D3DXVECTOR3 * norm, int i, int j, int ai, int aj, int bi, int bj);
	float myXmin;
	float myYmin;
	float myXmax;
	float myYmax;
	int myXdivs;
	int myYdivs;
	float myHeight;
  float m_xdivdist;
  float m_ydivdist;
	float m_elasticity;
	float m_viscosity;
	float m_tension;
  float m_blendability;
  bool m_textureMode;
	WaterPoint** myPoints;
};
