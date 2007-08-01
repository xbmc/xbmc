#pragma once

#include <xtl.h>
#pragma comment (lib, "lib/xbox_dx8.lib" )

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE )
struct CUSTOMVERTEX
{
	float x, y, z; // The untransformed position for the vertex.
	float nx, ny, nz; // Normal vector for lighting calculations	
  DWORD color; // The vertex colour.
};

#define M_PI 3.14159265358979323846f
#define ColorRGB D3DCOLOR

struct Color
{
  char a;
  char r;
  char g;
  char b;
};

#define INVALID_COLOR 0x00000000

#define LerpColor(s,e,r) (0xFF000000+((int)(((((e)&0xFF0000)*r)+((s)&0xFF0000)*(1.0f-r)))&0xFF0000)+((int)(((((e)&0xFF00)*r)+((s)&0xFF00)*(1.0f-r)))&0xFF00)+((int)(((((e)&0xFF)*r)+((s)&0xFF)*(1.0f-r)))&0xFF))
#define AddColor(s,e,r) (0xFF000000+((int)(((((e)&0xFF0000)*r)+((s)&0xFF0000)))&0xFF0000)+((int)(((((e)&0xFF00)*r)+((s)&0xFF00)))&0xFF00)+((int)(((((e)&0xFF)*r)+((s)&0xFF)))&0xFF))

DWORD LerpColor3(DWORD s,DWORD e,float r);

inline Color AddColor2(Color s,Color e,float r) {
  s.r+=(char)(e.r*r); 
  s.g+=(char)(e.g*r); 
  s.b+=(char)(e.b*r);
  return s;
};
inline Color LerpColor2(Color s,Color e,float r) {
  Color temp; 
  temp.r=temp.b=temp.g=0; 
  temp.r=(char)(s.r*(1.0f-r)+e.r*r); 
  temp.g=(char)(s.g*(1.0f-r)+e.g*r); 
  temp.b=(char)(s.b*(1.0f-r)+e.b*r); 
  return temp;
};


ColorRGB HSVtoRGB( float h, float s, float v );
inline float frand(){return ((float) rand() / (float) RAND_MAX);};

#define iMin(a,b) ((a)<(b)?(a):(b))
#define iMax(a,b) ((a)>(b)?(a):(b))

void incrementColor();
D3DCOLOR randColor();
void TransformCoord(D3DXVECTOR3 * pOut, D3DXVECTOR3 * pIn, D3DXMATRIX * pMat);



class MorphColor
{
  public:
    MorphColor(int speed = 50);
    ColorRGB getColor(){return currentColor;};
    void incrementColor();
  
  private:
    void reset(ColorRGB color);
    ColorRGB currentColor;
    ColorRGB startColor;
    ColorRGB endColor;
    float ratio;
    int steps;
    int colorSpeed;
};
