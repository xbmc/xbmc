//////////////////////////////////////////////////////////////////
// FRACTAL.CPP
//
//////////////////////////////////////////////////////////////////
#include "fractalobject.h"       // also includes glu and gl correctly
//#include "materials.h"



extern LPDIRECT3DDEVICE8 g_pd3dDevice;
// stuff for the environment cube
struct FracVertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
//	DWORD color; // The vertex colour.
};

//#define FVF_FRACVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE )
#define FVF_FRACVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL )
// man, how many times have you typed (or pasted) this data for a cube's
// vertices and normals, eh?
FracVertex g_cubeVertices[] =
{
  {D3DXVECTOR3(-0.5f, 0.5f,-0.5f), D3DXVECTOR3(0.0f, 0.0f,-1.0f)},// D3DCOLOR_RGBA(0,255,0,255)},
  {D3DXVECTOR3( 0.5f, 0.5f,-0.5f), D3DXVECTOR3(0.0f, 0.0f,-1.0f)},// D3DCOLOR_RGBA(255,255,0,255)},
  {D3DXVECTOR3(-0.5f,-0.5f,-0.5f), D3DXVECTOR3(0.0f, 0.0f,-1.0f)},// D3DCOLOR_RGBA(0,0,0,255)},
  {D3DXVECTOR3( 0.5f,-0.5f,-0.5f), D3DXVECTOR3(0.0f, 0.0f,-1.0f)},// D3DCOLOR_RGBA(255,0,0,255)},

  {D3DXVECTOR3(-0.5f, 0.5f, 0.5f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)},// D3DCOLOR_RGBA(0,255,255,255)},
  {D3DXVECTOR3(-0.5f,-0.5f, 0.5f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)},// D3DCOLOR_RGBA(0,255,255,255)},
  {D3DXVECTOR3( 0.5f, 0.5f, 0.5f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)},// D3DCOLOR_RGBA(255,255,255,255)},
  {D3DXVECTOR3( 0.5f,-0.5f, 0.5f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)},// D3DCOLOR_RGBA(255,0,255,255)},

  {D3DXVECTOR3(-0.5f, 0.5f, 0.5f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},// D3DCOLOR_RGBA(0,255,255,255)},
  {D3DXVECTOR3( 0.5f, 0.5f, 0.5f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},// D3DCOLOR_RGBA(255,255,255,255)},
  {D3DXVECTOR3(-0.5f, 0.5f,-0.5f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},// D3DCOLOR_RGBA(0,255,0,255)},
  {D3DXVECTOR3( 0.5f, 0.5f,-0.5f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},// D3DCOLOR_RGBA(255,255,0,255)},

  {D3DXVECTOR3(-0.5f,-0.5f, 0.5f), D3DXVECTOR3(0.0f,-1.0f, 0.0f)},// D3DCOLOR_RGBA(0,0,255,255)},
  {D3DXVECTOR3(-0.5f,-0.5f,-0.5f), D3DXVECTOR3(0.0f,-1.0f, 0.0f)},// D3DCOLOR_RGBA(0,0,0,255)},
  {D3DXVECTOR3( 0.5f,-0.5f, 0.5f), D3DXVECTOR3(0.0f,-1.0f, 0.0f)},// D3DCOLOR_RGBA(255,0,255,255)},
  {D3DXVECTOR3( 0.5f,-0.5f,-0.5f), D3DXVECTOR3(0.0f,-1.0f, 0.0f)},// D3DCOLOR_RGBA(255,0,0,255)},

  {D3DXVECTOR3( 0.5f, 0.5f,-0.5f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(255,255,0,255)},
  {D3DXVECTOR3( 0.5f, 0.5f, 0.5f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(255,255,255,255)},
  {D3DXVECTOR3( 0.5f,-0.5f,-0.5f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(255,0,0,255)},
  {D3DXVECTOR3( 0.5f,-0.5f, 0.5f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(255,0,255,255)},

  {D3DXVECTOR3(-0.5f, 0.5f,-0.5f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(0,255,0,255)},
  {D3DXVECTOR3(-0.5f,-0.5f,-0.5f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(0,0,0,255)},
  {D3DXVECTOR3(-0.5f, 0.5f, 0.5f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(0,255,255,255)},
  {D3DXVECTOR3(-0.5f,-0.5f, 0.5f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},// D3DCOLOR_RGBA(0,0,255,255)}
};

FracVertex g_pyramidVertices[] =
{
  {D3DXVECTOR3( 0.5f, 0.5f, 0.5f), D3DXVECTOR3( 0.5f, 0.5f, 0.5f)}, //0xFFFF0000},
  {D3DXVECTOR3( 0.5f,-0.5f,-0.5f), D3DXVECTOR3( 0.5f,-0.5f,-0.5f)}, //0xFF00FF00},
  {D3DXVECTOR3(-0.5f, 0.5f,-0.5f), D3DXVECTOR3(-0.5f, 0.5f,-0.5f)}, //0xFF0000FF},
  {D3DXVECTOR3(-0.5f,-0.5f, 0.5f), D3DXVECTOR3(-0.5f,-0.5f, 0.5f)}, //0xFFFFFF00},
  {D3DXVECTOR3( 0.5f, 0.5f, 0.5f), D3DXVECTOR3( 0.5f, 0.5f, 0.5f)}, //0xFFFF0000},
  {D3DXVECTOR3( 0.5f,-0.5f,-0.5f), D3DXVECTOR3( 0.5f,-0.5f,-0.5f)}//, 0xFF00FF00}
};

LPDIRECT3DVERTEXBUFFER8 g_pCubeVertexBuffer = NULL;
LPDIRECT3DVERTEXBUFFER8 g_pPyramidVertexBuffer = NULL;

Fractal::Fractal()
{
	Init(4);
}

Fractal::Fractal(int numTransforms)
{
	Init(numTransforms);
}

Fractal::~Fractal()
{
	g_pCubeVertexBuffer->Release();
	g_pPyramidVertexBuffer->Release();
	delete mySphere;
}


void FractalTransform::Init()
{
	translation.x = translation.y = translation.z = 0.0;
	scaling.x = scaling.y = scaling.z = 1.0;
	rotation.x = rotation.y = rotation.z = 0.0;
}

void FractalData::Init()
{
	numTransforms = 1;
	for (int i = 0; i < MAX_TRANSFORMS; i++)
	{
		transforms[i].Init();
	}
	base = FRACTAL_BASE_PYRAMID;
}

void initBuffer(LPDIRECT3DVERTEXBUFFER8* buffer, FracVertex * vertices, int numVertices)
{
	g_pd3dDevice->CreateVertexBuffer( numVertices*sizeof(FracVertex),    
								D3DUSAGE_WRITEONLY, 
								FVF_FRACVERTEX,
								D3DPOOL_MANAGED, 
								buffer );
	void *pVertices = NULL;
    (*buffer)->Lock( 0, numVertices*sizeof(FracVertex), (BYTE**)&pVertices, 0 );
    memcpy( pVertices, vertices, numVertices*sizeof(FracVertex) );
    (*buffer)->Unlock();

}

//Inits the Transforms
void Fractal::Init(int numTransforms)
{
	myData.Init();

	myData.numTransforms = numTransforms;
	myCutoffDepth = 5;
	selectionOn = false;
	mySelectedTransform = 0;
  myColorLerpAmount = 0.7;
	redBlueRender = false;
	mySphere = new C_Sphere();

	glInit();
	initBuffer(&g_pCubeVertexBuffer, g_cubeVertices, 24);
	initBuffer(&g_pPyramidVertexBuffer, g_pyramidVertices, 6);
}

//Renders the fractal
void Fractal::Render()
{
	/*
	if(selectionOn)
		RenderSelection(0);
	if(!redBlueRender)
	{
		if(myData.base == FRACTAL_BASE_CUBE)
			SetMaterial(GREEN_PLASTIC);
		else
			SetMaterial(BLUE_WATER);
	}
	else
	{
		SetMaterial(PURPLE_METAL);
	}*/

  for(int i = 0; i < myData.numTransforms; i++)
    myData.transforms[i].color.incrementColor();

	for(int i = 0; i < myData.numTransforms; i++)
	{
		ApplyTransform(i);
/*		if(selectionOn)
		{
			if(i == mySelectedTransform)
				RenderSelection(1);
			else
				RenderSelection(2);
		}*/
    RenderChild(0, i, myData.transforms[i].color.getColor());
		InvertTransform(i);
	}
}

//The Recursively called render function
void Fractal::RenderChild(int depth, int parentTransform, ColorRGB childColor)
{
	depth++;

	if(depth >= myCutoffDepth)
	{
		RenderBase(childColor);
		return;
	}

	for(int i = 0; i < myData.numTransforms; i++)
	{
		ApplyTransform(i);
    RenderChild(depth, parentTransform, LerpColor(myData.transforms[i].color.getColor(), childColor, myColorLerpAmount));
		InvertTransform(i);
	}


}

/*
//Draws the wireframe selection boxes
void Fractal::RenderSelection(int depth)
{
	if(!redBlueRender)
	if(depth == 0)
	{
		SetMaterial(DULL_PINK);
		glColor3f(1.0,1.0,1.0);
	}
	else if (depth == 1)
	{
		SetMaterial(BRASS);
		glColor3f(0.0,1.0,1.0);
	}
	else if (depth == 2)
	{
		SetMaterial(RED_METAL);
		glColor3f(0.8,.8,0.3);
	}

	glutWireCube(1.0);
}*/


//applys the transform to the matrix
void Fractal::ApplyTransform(int i)
{
	glPushMatrix();
	glScalef(myData.transforms[i].scaling.x, myData.transforms[i].scaling.y, myData.transforms[i].scaling.z);
	glTranslatef(myData.transforms[i].translation.x, myData.transforms[i].translation.y, myData.transforms[i].translation.z);
	glRotatef(myData.transforms[i].rotation.x, myData.transforms[i].rotation.y, myData.transforms[i].rotation.z);
}

//Inverts the transform.. just pops the matrix off the stack.
void Fractal::InvertTransform(int iTransform)
{
	glPopMatrix();
}

//Sets whether selection is on and which transform is selected
void Fractal::SetSelection(bool drawSelection, int numSelected)
{
	selectionOn = drawSelection;
	mySelectedTransform = numSelected % myData.numTransforms;
}

FractalData *Fractal::GetDataHandle()
{
	return & myData;
}

//Increments cutoff
void Fractal::IncrementCutoff(bool up)
{
		myCutoffDepth += up ? 1 : -1;
		if (myCutoffDepth == 0)
			myCutoffDepth = 1;
}

void Fractal::SetCutoff(int cutoff)
{
		myCutoffDepth = cutoff;
    myColorLerpAmount = pow(0.4f,1.0f/((float)cutoff));
}

void Fractal::setRedBlueRender(bool red)
{
	redBlueRender = red;
}

// draws the base shape be it cube, pyramid or whatever
void Fractal::RenderBase(ColorRGB color)
{
	glApply();
  D3DMATERIAL8 mat;
  memset(&mat,0,sizeof(D3DMATERIAL8));
  mat.Diffuse.a = 1.0f;
  mat.Diffuse.r = ((float)((color&0xFF0000)>>16))/255.0f;
  mat.Diffuse.g = ((float)((color&0xFF00)>>8))/255.0f;
  mat.Diffuse.b = ((float)((color&0xFF)))/255.0f;
  g_pd3dDevice->SetMaterial(&mat);
	if(myData.base == FRACTAL_BASE_SPHERE)
	{
//    mySphere->SetColor(color);
		mySphere->Render3D();
	}
	else if(myData.base == FRACTAL_BASE_PYRAMID)
	{
		g_pd3dDevice->SetVertexShader( FVF_FRACVERTEX );
		g_pd3dDevice->SetStreamSource( 0, g_pPyramidVertexBuffer, sizeof(FracVertex) );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 4 );
	}
	else
	{
		g_pd3dDevice->SetVertexShader( FVF_FRACVERTEX );
		g_pd3dDevice->SetStreamSource( 0, g_pCubeVertexBuffer, sizeof(FracVertex) );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  4, 2 );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  8, 2 );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );
	}
	
}
