//////////////////////////////////////////////////////////////////
// WATERFIELD.CPP
//
// This is a really neat water engine that uses some liquid physics
// drive deformations on a triangulated mesh.  This is meant to be
// reused and so is well commented :)
//
//////////////////////////////////////////////////////////////////
#include "waterfield.h"       
#include "Util.h"

extern LPDIRECT3DDEVICE8 g_pd3dDevice;
LPDIRECT3DVERTEXBUFFER8 g_pVertexBuffer = NULL; // Vertices Buffer

WaterField::WaterField()
{
	Init(-10,10,-10,10,80,80,0, 0.5f, 0.05f, 1.0f, 0.04f, false);
}

WaterField::WaterField(float xmin, float xmax, float ymin, float ymax, int xdivs, int ydivs, float height, float elasticity, float viscosity, float tension, float blendability, bool textureMode)
{
  Init(xmin, xmax, ymin, ymax, xdivs, ydivs, height, elasticity, viscosity, tension, blendability, textureMode);
}

WaterField::~WaterField()
{
	g_pVertexBuffer->Release();
}

/************************************************************
Init

Sets the bounds, tesselation, hieght of the water plane
and allocates triangulated mesh;
************************************************************/
void WaterField::Init(float xmin, float xmax, float ymin, float ymax, int xdivs, int ydivs, float height, float elasticity, float viscosity, float tension, float blendability, bool textureMode)
{
	myXmin = xmin;
	myYmin = ymin;
	myXmax = xmax;
	myYmax = ymax;
	myXdivs = xdivs;
	myYdivs = ydivs;
	myHeight = height;
	
  m_xdivdist = (float)(myXmax - myXmin) / (float)myXdivs;
	m_ydivdist = (float)(myYmax - myYmin) / (float)myYdivs;

	m_viscosity = viscosity;
	m_elasticity = elasticity;
	m_tension = tension;
  m_blendability = blendability;
  m_textureMode = textureMode;

	myPoints = new WaterPoint*[xdivs];
	for(int i = 0; i < xdivs; i++)
	{
	 myPoints[i] = new WaterPoint[ydivs];
	 for(int j = 0; j < ydivs; j++)
	 {
		myPoints[i][j].height = 0;
		myPoints[i][j].velocity = 0;
    myPoints[i][j].color = 0xFF808080;
    myPoints[i][j].avecolor = 0xFF000000;
	 }
	}

	//Create the vertex buffer from our device
  if (m_textureMode)
  {
    g_pd3dDevice->CreateVertexBuffer(2 * myYdivs*  sizeof(TEXTUREDVERTEX),
                                               D3DUSAGE_WRITEONLY, 
                                               D3DFVF_TEXTUREDVERTEX,
                                               D3DPOOL_MANAGED, 
                                               &g_pVertexBuffer);
  }
  else
  {
    g_pd3dDevice->CreateVertexBuffer(2 * myYdivs*  sizeof(COLORVERTEX),
                                               D3DUSAGE_WRITEONLY, 
                                               D3DFVF_COLORVERTEX,
                                               D3DPOOL_MANAGED, 
                                               &g_pVertexBuffer);    
  }

}


void WaterField::DrawLine(float xStart, float yStart, float xEnd, float yEnd, 
                          float width, float newHeight, float strength, DWORD color)
{
  int xa, xb, ya, yb;
  int radiusY = (int)((float)myYdivs*(width) / (myYmax - myYmin));
  //int radiusX = (int)((float)myXdivs*(width) / (myXmax - myXmin));
  int radiusX = radiusY;

  GetIndexNearestXY(xStart,yStart,&xa, &ya);
  GetIndexNearestXY(xEnd,yEnd,&xb,&yb);
  int maxstep = abs(yb - ya) > abs(xb - xa) ? abs(yb - ya) : abs(xb - xa);
 
  if (maxstep == 0)
    return;
  for (int i = 0; i <= maxstep; i++) {
    int x = xa + (xb-xa)*i/maxstep;
    int y = ya + (yb-ya)*i/maxstep;
    for (int k = -radiusX;  k <= radiusX; k++)
      //for (int l = abs(k)-radius;  l <= radius-abs(k); l++)
      for (int l = -radiusY;  l <= radiusY; l++)
        if((x+k)>=0 && (y+l)>=0 && (x+k)<myXdivs && (y+l)<myYdivs) {
          if(k*k+l*l <= radiusX*radiusY)
          {
            float ratio = 1.0f-sqrt((float)(k*k+l*l)/(float)(radiusX*radiusY));
            myPoints[x+k][y+l].height = strength*newHeight + (1-strength)*myPoints[x+k][y+l].height;
		        myPoints[x+k][y+l].velocity = (1-strength)*myPoints[x+k][y+l].velocity;
            if (color != INVALID_COLOR)
              myPoints[x+k][y+l].color = LerpColor((DWORD)myPoints[x+k][y+l].color, color, ratio);
          }
        }
  }
}


/************************************************************
SetHeight

Sets the points within spread of the nearest vertex to
(xNearest,yNearest) to a height of newHeight in a roughly
circular pattern.
************************************************************/
void WaterField::SetHeight(float xNearest, float yNearest, 
                           float spread, float newHeight, DWORD color)
{
	int xcenter;
	int ycenter;
  int radiusIndexY = (int)((float)myYdivs*(spread) / (myYmax - myYmin));
	//int radiusIndexX = (int)((float)myXdivs*(spread) / (myXmax - myXmin));
  int radiusIndexX = radiusIndexY;
	float ratio;
  float xd = ((float)(myXmax - myXmin)) / myXdivs;
  float yd = ((float)(myYmax - myYmin)) / myYdivs;
  if (spread <= 0) return;

	GetIndexNearestXY(xNearest,yNearest,&xcenter, &ycenter);

	for(int i = xcenter-radiusIndexX; i <= xcenter+radiusIndexX; i++)
 	 for(int j = ycenter-radiusIndexY; j <= ycenter+radiusIndexY; j++)
	    if( i>=0 && j>=0 && i<myXdivs && j<myYdivs)
	    {
        //float x = myXmin + xd * i;
        float x = myXmin + xd * i; //pretend its bigger
        float y = myYmin + yd * j;
			  ratio = 1.0f;
			  ratio = 1.0f-sqrt((float)((xNearest-x)*(xNearest-x)*yd*yd/xd/xd+(yNearest-y)*(yNearest-y))/(spread*spread));
			  if (ratio <= 0)
				  continue;
		    myPoints[i][j].height = ratio*newHeight + (1-ratio)*myPoints[i][j].height;
		    myPoints[i][j].velocity = (1-ratio)*myPoints[i][j].velocity;
        if (color != INVALID_COLOR)
          myPoints[i][j].color = LerpColor(myPoints[i][j].color, color, ratio);
	    }
}

/************************************************************
GetHeight

Gets the height of the nearest vertex to (xNearest,yNearest)
************************************************************/
float WaterField::GetHeight(float xNearest, float yNearest)
{
	int i,j;
	GetIndexNearestXY(xNearest,yNearest,&i,&j);
	return myPoints[i][j].height;
}


void WaterField::Step()
{
	Step(STEP_TIME);
}

/************************************************************
Step

This is where knowledge of physical systems comes in handy.
For every vertex the surface tension is computed as the
cumulative sum of the difference between the vertex height and its
neighbors.  This data is fed into the change in velocity.  The
other values added are for damped oscillation (one is a proportional
control and the other is differential) and so the three physical
quantities accounted for are elasticity, viscosity, and surface tension.

Height is just incremented by time*velocity.
************************************************************/
void WaterField::Step(float time)
{
	int i, j, k, l, mi, ni, mj, nj;
	float cumulativeTension = 0;
	int calRadius = 1;

  for(i=0; i<myXdivs; i++)
		for(j=0; j<myYdivs; j++)
		{
			cumulativeTension = 0;
      myPoints[i][j].avecolor = 0;
      ni = i-calRadius > 0 ? i-calRadius : 0;//
      ni = iMax(0,i-calRadius);
      mi = iMin(myXdivs-1, i+calRadius);
      nj = iMax(0,j-calRadius);
      mj = iMin(myYdivs-1, j+calRadius);
      /*int numCount = ((mi-ni+1)*(mj-nj+1));
      //numCount = 1.0f;
      int rcum = (numCount/2) << 16, gcum = (numCount/2) << 8, bcum = numCount/2;*/
			for(k=ni; k<=mi; k++)
        for(l=nj; l<=mj; l++)
				{
					cumulativeTension += myPoints[k][l].height- myPoints[i][j].height;
          /*rcum += myPoints[k][l].color & 0xFF0000;
          gcum += myPoints[k][l].color & 0x00FF00;
          bcum += myPoints[k][l].color & 0x0000FF;*/
        //  myPoints[i][j].avecolor = AddColor(myPoints[i][j].avecolor, myPoints[k][l].color, numCount);//1.0f/numCount++);
				}
      /*myPoints[i][j].avecolor = (rcum/numCount) && 0xFF0000 + (gcum/numCount) && 0xFF00 + (bcum/numCount) && 0xFF;
      //myPoints[i][j].avecolor = *((DWORD*)&(LerpColor2(*((Color*)&(myPoints[i][j].avecolor)), black, 0.75f)));//1.0f/numCount++);
      //myPoints[i][j].avecolor = LerpColor(myPoints[i][j].avecolor, black, 0.75f);//1.0f/numCount++);
      for(k=ni; k<=mi; k++)
      {
				cumulativeTension += myPoints[k][j].height- myPoints[i][j].height;
        myPoints[i][j].avecolor = AddColor(myPoints[i][j].avecolor, myPoints[k][j].color, numCount +0.5f);//1.0f/numCount++);
        //AddColor2(*((Color*)&(myPoints[i][j].avecolor)), *((Color*)&(myPoints[k][j].color)), numCount);//1.0f/numCount++);
			}
      for(l=nj; l<=mj; l++)
			{
				cumulativeTension += myPoints[i][l].height- myPoints[i][j].height;
        myPoints[i][j].avecolor = AddColor(myPoints[i][j].avecolor, myPoints[i][l].color, numCount+0.5f);//1.0f/numCount++);
        //AddColor2(*((Color*)&(myPoints[i][j].avecolor)), *((Color*)&(myPoints[i][l].color)), numCount);//1.0f/numCount++);
			}*/

			myPoints[i][j].velocity +=m_elasticity*(myHeight-myPoints[i][j].height)
				- m_viscosity * myPoints[i][j].velocity
				+ m_tension*cumulativeTension;
		}

    for(i=0; i<myXdivs; i++)
    for(j=0; j<myYdivs; j++)
    {
      myPoints[i][j].height += myPoints[i][j].velocity*time;
      //myPoints[i][j].color = LerpColor3(myPoints[i][j].color, myPoints[i][j].color, m_blendability);
      SetNormalForPoint(i,j);
    }

}

/************************************************************
Render

Renders the water to the screen relative to the currect origin.
Doesnt do any translations on the matrix.  Each row is done
in one individual triangular strip (so all interior vertices are
done twice :-|  I do not believe it is possible to do it all
as one triangular strip (and have every vertex specified once).
************************************************************/
void WaterField::Render()
{
	int i, j, k;
  
  if (!m_textureMode){
	  COLORVERTEX* pVertices;
    //for(int l=0;l<2;l++)
	  for(i=0; i<myXdivs-1; i++)
	  {
		  //Get a pointer to the vertex buffer vertices and lock the vertex buffer
		  g_pVertexBuffer->Lock(0, 2 * myYdivs*  sizeof(COLORVERTEX), (BYTE**)&pVertices, 0);

		  for(j=0; j<myYdivs; j++)
		  {
			  for (k=0; k<2; k++)
			  {
			  pVertices[2*j+k].x = myXmin + (float)((i+k)*m_xdivdist);//+ (float)((j)*m_xdivdist)/2.0f;
			  pVertices[2*j+k].y = myYmin + (float)(j*m_ydivdist);
			  pVertices[2*j+k].z = (float)(myPoints[i+k][j].height);
        pVertices[2*j+k].nx = myPoints[i+k][j].normal.x;
        pVertices[2*j+k].ny = myPoints[i+k][j].normal.y;
        pVertices[2*j+k].nz = myPoints[i+k][j].normal.z;
        pVertices[2*j+k].color = *((DWORD*)&(myPoints[i+k][j].color));
			  }
		  }
		      //Unlock the vertex buffer
		  g_pVertexBuffer->Unlock();
			  // Draw it
      g_pd3dDevice->SetVertexShader(D3DFVF_COLORVERTEX);
      g_pd3dDevice->SetStreamSource(0, g_pVertexBuffer, sizeof(COLORVERTEX));
		  g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2*(myYdivs-1));
      //g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2*(myYdivs-1), g_pVertexBuffer, sizeof(COLORVERTEX) );
	  }
  }
  else
  {
    TEXTUREDVERTEX* pVertices;
    //for(int l=0;l<2;l++)
	  for(i=0; i<myXdivs-1; i++)
	  {
		  //Get a pointer to the vertex buffer vertices and lock the vertex buffer
		  g_pVertexBuffer->Lock(0, 2 * myYdivs*  sizeof(TEXTUREDVERTEX), (BYTE**)&pVertices, 0);

		  for(j=0; j<myYdivs; j++)
		  {
			  for (k=0; k<2; k++)
			  {
			  pVertices[2*j+k].x = myXmin + (float)((i+k)*m_xdivdist);//+ (float)((j)*m_xdivdist)/2.0f;
			  pVertices[2*j+k].y = myYmin + (float)(j*m_ydivdist);
			  pVertices[2*j+k].z = (float)(myPoints[i+k][j].height);
        pVertices[2*j+k].nx = myPoints[i+k][j].normal.x;
        pVertices[2*j+k].ny = myPoints[i+k][j].normal.y;
        pVertices[2*j+k].nz = myPoints[i+k][j].normal.z;
        pVertices[2*j+k].tu = 0.0f+1.0f*(float)(i+k)/(float)myXdivs + 0.5f*myPoints[i+k][j].normal.x;
        pVertices[2*j+k].tv = 0.0f+1.0f*(float)j/(float)myYdivs + 0.5f*myPoints[i+k][j].normal.y;
			  }
		  }
		      //Unlock the vertex buffer
		  g_pVertexBuffer->Unlock();
			  // Draw it
      g_pd3dDevice->SetVertexShader(D3DFVF_TEXTUREDVERTEX);
      g_pd3dDevice->SetStreamSource(0, g_pVertexBuffer, sizeof(TEXTUREDVERTEX));
		  g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2*(myYdivs-1));
	  }
  }
}

/************************************************************
GetIndexNearestXY

Calculates the nearest water point index (i,j) from the world
position (x,y).
************************************************************/
void WaterField::GetIndexNearestXY(float x, float y, int *i, int *j)
{
	*i = x <= myXmin ? 0: x >= myXmax ? myXdivs-1:
		(int)((float)myXdivs*(x - myXmin) / (myXmax - myXmin)) ;
	*j = y <= myYmin ? 0: y >= myYmax ? myYdivs-1:
		(int)((float)myYdivs*(y - myYmin) / (myYmax - myYmin)) ;
}

/************************************************************
SetNormalForPoint

Calculates and sets a normal for any point ij in the water mesh
by taking the normal to the plane that goes through three neighboring
points.  This has the effect of smoothing out the lighting.
************************************************************/
void WaterField::SetNormalForPoint(int i, int j)
{
  /*
	int
	ai = i, aj = j<myYdivs-1 ? j+1 :j,
	bi = i>0 ? i-1: i, bj = j>0 ? j-1 : j,
	ci = i<myXdivs -1 ? i+1 : i, cj = j>0 ? j-1 :j;
  */

// Formula for the cross product p = v1 x v2
//  p.x = v1.y * v2.z - v2.y * v1.z;
//  p.y = v1.z * v2.x - v2.z * v1.x;
//  p.z = v1.x * v2.y - v2.x * v1.y;

//  these are the vectors to use.
//	v1 = (bi-ai)*xdivdist, (bj-aj)*ydivdist, (myPoints[bi][bj].height-myPoints[ai][aj].height)
// 	v2 = (ci-ai)*xdivdist, (cj-aj)*ydivdist, (myPoints[ci][cj].height-myPoints[ai][aj].height)
	/*
	pVertex->nx = 
	(float)(bj-aj)*ydivdist*(myPoints[ci][cj].height-myPoints[ai][aj].height)
	- (float)(cj-aj)*ydivdist*(myPoints[bi][bj].height-myPoints[ai][aj].height);
	
	pVertex->ny = 
	(myPoints[bi][bj].height-myPoints[ai][aj].height) * (float)(ci-ai)*xdivdist
	- (myPoints[ci][cj].height-myPoints[ai][aj].height) * (float)(bi-ai)*xdivdist;
	
	pVertex->nz = 
	(float)(bi-ai)*xdivdist * (float)(cj-aj)*ydivdist - (float)(ci-ai)*xdivdist * (float)(bj-aj)*ydivdist;
*/
  D3DXVECTOR3 * norm = &(myPoints[i][j].normal);
  memset(norm,0,sizeof(D3DXVECTOR3));
  D3DXVECTOR3 temp;
  int s = 2; //spread
  //for (s=1;s<3;s++)
  //if(i<s|| j < s || i>=myXdivs-s || j >=myYdivs-s)
  //{
	  //int
	  //ai = i, aj = j<myYdivs-s ? j+s :j,
	  //bi = i>=s ? i-s: i, bj = j>=s ? j-s : j,
	  //ci = i<myXdivs -s ? i+s : i, cj = j>=s ? j-s :j;
    //NormalForPoints(norm, ai,aj,bi,bj,ci,cj);
    int mi = i > s ? i-s : 0;
    int ni = i+s < myXdivs ? i+s : myXdivs-1;
    int mj = j > s ? j-s : 0;
    int nj = j+s < myYdivs ? j+s : myYdivs-1;

    NormalForPoints(norm, mi,j,ni,mj,ni,nj);
  /*}
  else
  {
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i+s,j,i-s,j+s,i-s,j-s));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i-s,j,i+s,j-s,i+s,j+s));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i,j,  i-s,j-s,i-0,j-s));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i,j,  i+s,j-s,i+s,j  ));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i,j,  i+s,j+s,i-0,j+s));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i,j,  i-s,j+s,i-s,j  ));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i+s,j,i-s,j+s,i-s,j-s));
    //D3DXVec3Add( norm,norm, NormalForPoints(&temp, i-s,j,i+s,j-s,i+s,j+s));  
    NormalForPoints(norm, i-s,j,i+s,j-s,i+s,j+s);

    //D3DXVec3Scale(&norm,&norm,1/7.0f);
  }
  */
}

D3DXVECTOR3 * WaterField::NormalForPoints(D3DXVECTOR3 * norm, int i, int j, int ai, int aj, int bi, int bj)
{
  D3DXVECTOR3 a = D3DXVECTOR3((ai-i)*m_xdivdist, (aj-j)*m_ydivdist, (myPoints[ai][aj].height-myPoints[i][j].height));
	D3DXVECTOR3 b = D3DXVECTOR3((bi-i)*m_xdivdist, (bj-j)*m_ydivdist, (myPoints[bi][bj].height-myPoints[i][j].height));
  D3DXVec3Cross( norm, &a, &b );
  D3DXVec3Normalize(norm,norm);
  return norm;
}
