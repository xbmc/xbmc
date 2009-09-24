/*
 * Silverwave Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
  * Ver 1.0 2007-02-12 by Asteron  http://asteron.projects.googlepages.com/home
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2  of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "Water.h"
#include "Effect.h"
#include "XmlDocument.h"
#include "Util.h"
#include <cstdlib>

// use the 'dummy' dx8 lib - this allow you to make
// DX8 calls which XBMC will emulate for you.
#pragma comment (lib, "lib/xbox_dx8.lib" )

WaterSettings world;
AnimationEffect * effects[] = {

  new EffectBoil(),
  new EffectTwist(),
  new EffectBullet(),
  new EffectRain(),
  new EffectSwirl(),
  new EffectXBMCLogo(),
  NULL,
  //new EffectText(),
};
  
LPDIRECT3DDEVICE8 g_pd3dDevice;
LPDIRECT3DTEXTURE8  g_Texture = NULL;

static  char m_szScrName[1024];
int  m_iWidth;
int m_iHeight;
D3DXVECTOR3 g_lightDir;
float g_shininess = 0.4f;

void CreateLight()
{

  // Fill in a light structure defining our light
  D3DLIGHT8 light;
  memset( &light, 0, sizeof(D3DLIGHT8) );
  light.Type    = D3DLIGHT_POINT;
  light.Ambient = (D3DXCOLOR)D3DCOLOR_RGBA(0,0,0,255);
  light.Diffuse = (D3DXCOLOR)D3DCOLOR_RGBA(255,255,255,255);
  light.Specular = (D3DXCOLOR)D3DCOLOR_RGBA(150,150,150,255);
  light.Range   = 300.0f;
  light.Position = D3DXVECTOR3(0,-5,5);
  light.Attenuation0 = 0.5f;
  light.Attenuation1 = 0.02f;
  light.Attenuation2 = 0.0f;

  // Create a direction for our light - it must be normalized 
  light.Direction = g_lightDir;

  // Tell the device about the light and turn it on
  g_pd3dDevice->SetLight( 0, &light );
  g_pd3dDevice->LightEnable( 0, TRUE ); 
  d3dSetRenderState( D3DRS_LIGHTING, TRUE );

}



////////////////////////////////////////////////////////////////////////////////

// stuff for the background plane
struct BG_VERTEX 
{
    D3DXVECTOR4 position;
    DWORD       color;
};
BG_VERTEX g_BGVertices[4];
////////////////////////////////////////////////////////////////////////////////
// fill in background vertex array with values that will
// completely cover screen
void SetupGradientBackground( DWORD dwTopColor, DWORD dwBottomColor )
{
  float x1 = -0.5f;
  float y1 = -0.5f;
  float x2 = (float)m_iWidth - 0.5f;
  float y2 = (float)m_iHeight - 0.5f;
  
  g_BGVertices[0].position = D3DXVECTOR4( x2, y1, 0.0f, 1.0f );
  g_BGVertices[0].color = dwTopColor;

  g_BGVertices[1].position = D3DXVECTOR4( x2, y2, 0.0f, 1.0f );
  g_BGVertices[1].color = dwBottomColor;

  g_BGVertices[2].position = D3DXVECTOR4( x1, y1, 0.0f, 1.0f );
  g_BGVertices[2].color = dwTopColor;

  g_BGVertices[3].position = D3DXVECTOR4( x1, y2, 0.0f, 1.0f );
  g_BGVertices[3].color = dwBottomColor;

  return;
}
///////////////////////////////////////////////////////////////////////////////
void RenderGradientBackground()
{
    // clear textures
  g_pd3dDevice->SetTexture( 0, NULL );
  g_pd3dDevice->SetTexture( 1, NULL );
  d3dSetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
  d3dSetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

  // don't write to z-buffer
  d3dSetRenderState( D3DRS_ZENABLE, FALSE ); 

  g_pd3dDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
  g_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, g_BGVertices, sizeof(BG_VERTEX) );

  return;
}

void LoadEffects()
{
  int i = 0;
  while(effects[i] != NULL)
    effects[i++]->init(&world);
  world.effectCount = i;
}

void SetMaterial()
{
  D3DMATERIAL8 mat;

  // Set the RGBA for diffuse reflection.
  if (world.isTextureMode)
  {
    mat.Diffuse.r = 1.0f;
    mat.Diffuse.g = 1.0f;
    mat.Diffuse.b = 1.0f;
    mat.Diffuse.a = 1.0f;
  }
  else
  {
    mat.Diffuse.r = 0.5f;
    mat.Diffuse.g = 0.5f;
    mat.Diffuse.b = 0.5f;
    mat.Diffuse.a = 1.0f;
  }

  // Set the RGBA for ambient reflection.
  mat.Ambient.r = 0.5f;
  mat.Ambient.g = 0.5f;
  mat.Ambient.b = 0.5f;
  mat.Ambient.a = 1.0f;

  // Set the color and sharpness of specular highlights.
  mat.Specular.r = g_shininess;
  mat.Specular.g = g_shininess;
  mat.Specular.b = g_shininess;
  mat.Specular.a = 1.0f;
  mat.Power = 100.0f;

  // Set the RGBA for emissive color.
  mat.Emissive.r = 0.0f;
  mat.Emissive.g = 0.0f;
  mat.Emissive.b = 0.0f;
  mat.Emissive.a = 0.0f;

  g_pd3dDevice->SetMaterial(&mat);
}

void LoadTexture()
{
  // Setup our texture
  //long hFile;
  int numTextures = 0;
  static char szSearchPath[512];
  static char szPath[512];
  static char foundTexture[1024];

  strcpy(szPath,world.szTextureSearchPath);
  if (world.szTextureSearchPath[strlen(world.szTextureSearchPath) - 1] != '\\')
    strcat(szPath,"\\");
  strcpy(szSearchPath,szPath);
  strcat(szSearchPath,"*");

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile( szSearchPath, &fd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
      int len = (int)strlen(fd.cFileName);
      if (len < 4 || (strcmpi(fd.cFileName + len - 4, ".txt") == 0))
              continue;
    
      if (rand() % (numTextures+1) == 0) // after n textures each has 1/n prob
      {
        strcpy(foundTexture,szPath);
        strcat(foundTexture,fd.cFileName);
      }
      numTextures++;      
		}while( FindNextFile( hFind, &fd));
		FindClose( hFind);
  }


  if (g_Texture != NULL && numTextures > 0)
  {
    g_Texture->Release();
    g_Texture = NULL;
  }
    
  if (numTextures > 0)
    D3DXCreateTextureFromFileA(g_pd3dDevice, foundTexture, &g_Texture);
}


// XBMC has loaded us into memory,
// we should set our core values
// here and load any settings we
// may have from our config file
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName, float ratio)
{
  memset(&world,0,sizeof(WaterSettings));

  strcpy(m_szScrName,szScreenSaverName);
  g_pd3dDevice = pd3dDevice;

  m_iWidth = iWidth;
  m_iHeight = iHeight;

  world.scaleX = 1.0f;
  if ( (ratio * iWidth / iHeight) > 1.5)
    world.scaleX = 1/1.333f;//0.91158f/ratio; widescreen mode
  // Load the settings
  LoadSettings();
  CreateLight();
  LoadEffects();
  
  if (world.isTextureMode)
  {
    LoadTexture();
    world.effectCount--; //get rid of logo effect
  }

  world.effectType = rand()%world.effectCount;
  //world.effectType = 5;
  world.frame = 0;
  world.nextEffectTime = 0;


  
  /*
  char buff[100];
  for(int j = 0; j < 15; j++)
  {
    buff[j] = (int)ratio + '0';
    ratio = (ratio - (int)ratio)*10;
  }
  buff[15] = 0;
  ((EffectText*)(effects[6]))->drawString(buff,0.6f,0.6f,1.3f,0.06f,-7.0f,7.8f);*/
  //world.widescreen = (float)iWidth/ (float)iHeight > 1.7f;
}

void SetCamera()
{

  D3DXMATRIX m_World;
  D3DXMatrixIdentity( &m_World );
  d3dSetTransform(D3DTS_WORLD, &m_World);

  D3DXMATRIX m_View;// = ViewMatrix(D3DXVECTOR(0,-20,20), D3DXVECTOR(0,0,0), D3DXVECTOR(0,-1,0), 0);
  D3DXVECTOR3 from = D3DXVECTOR3(0,14,14);
  D3DXVECTOR3 to   = D3DXVECTOR3(0,3,0);
  D3DXVECTOR3 up   = D3DXVECTOR3(0,-0.707f,0.707f);
  /*D3DXVECTOR3 from = D3DXVECTOR3(0,0,20);
  D3DXVECTOR3 to   = D3DXVECTOR3(0,0,0);
  D3DXVECTOR3 up   = D3DXVECTOR3(0,-1,0);*/

  D3DXMatrixLookAtLH(&m_View,&from,&to,&up);
  d3dSetTransform(D3DTS_VIEW, &m_View);

  float aspectRatio = 720.0f / 480.0f;
  D3DXMATRIX m_Projection; 
  D3DXMatrixPerspectiveFovLH(&m_Projection, D3DX_PI/4, aspectRatio, 1.0, 1000.0);
  d3dSetTransform(D3DTS_PROJECTION, &m_Projection);
}

void SetDefaults()
{
  //world.effectType = rand()%world.effectCount;
  //world.effectType = 5;
  world.frame = 0;
  world.nextEffectTime = 0;
  world.isWireframe = false;
  world.isTextureMode = false;
}

void SetupRenderState()
{
  SetCamera();
  SetMaterial();

  d3dSetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	d3dSetRenderState( D3DRS_LIGHTING, TRUE );
	d3dSetRenderState( D3DRS_ZENABLE, TRUE); //D3DZB_TRUE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	d3dSetRenderState( D3DRS_NORMALIZENORMALS, FALSE );
  d3dSetRenderState( D3DRS_SPECULARENABLE, g_shininess > 0 );
  if(world.isWireframe)
    d3dSetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  else
    d3dSetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);


  if (world.isTextureMode)
  {
    d3dSetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_MODULATE);
	  d3dSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d3dSetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	  d3dSetTextureStageState(0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);
	  d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	  d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	  d3dSetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
	  d3dSetTextureStageState(0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
	  d3dSetTextureStageState(0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
	  d3dSetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE);
	  d3dSetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);
    
    g_pd3dDevice->SetTexture( 0, g_Texture );
	  g_pd3dDevice->SetTexture( 1, NULL );
  }
}




// XBMC tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by XBMC.
extern "C" void Start()
{
//  SetupGradientBackground( D3DCOLOR_RGBA(255,0,0,255), D3DCOLOR_RGBA(0,0,0,255) );

  SetCamera();
  return;
}

// XBMC tells us to render a frame of
// our screensaver. This is called on
// each frame render in XBMC, you should
// render a single frame only - the DX
// device will already have been cleared.

extern "C" void Render()
{
  //RenderGradientBackground();
  CreateLight();
  SetupRenderState();

  world.frame++;

  if (world.isTextureMode && world.nextTextureTime>0 && (world.frame % world.nextTextureTime) == 0)
    LoadTexture();

  if (world.frame > world.nextEffectTime)
  {
    if ((rand() % 3)==0)
      incrementColor();
    //static limit = 0;if (limit++>3)
    world.effectType += 1;//+rand() % (ANIM_MAX-1);
    world.effectType %= world.effectCount;
    effects[world.effectType]->reset();
    world.nextEffectTime = world.frame + effects[world.effectType]->minDuration() + 
      rand() % (effects[world.effectType]->maxDuration() - effects[world.effectType]->minDuration());
  }
  effects[world.effectType]->apply();
  world.waterField->Step();
  world.waterField->Render();
  return;
}

// XBMC tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
extern "C" void Stop()
{
  delete world.waterField;
  world.waterField = NULL;
  for (int i = 0; effects[i] != NULL; i++)
    delete effects[i];
  if (g_Texture != NULL)
  {
    g_Texture->Release();
    g_Texture = NULL;
  }
  return;
}


// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func
// is called.
void LoadSettings()
{
  XmlNode node, childNode;
  CXmlDocument doc;
  
  char szXMLFile[1024];
  strcpy(szXMLFile, "Q:\\screensavers\\");
  strcat(szXMLFile, m_szScrName);
  strcat(szXMLFile, ".xml");

  OutputDebugString("Loading XML: ");
  OutputDebugString(szXMLFile);
  float xmin = -10.0f, 
    xmax = 10.0f, 
    ymin = -10.0f, 
    ymax = 10.0f, 
    height = 0.0f, 
    elasticity = 0.5f, 
    viscosity = 0.05f, 
    tension = 1.0f, 
    blendability = 0.04f;

  int xdivs = 50; 
  int ydivs = 50;
  int divs = 50;

  SetDefaults();

  g_lightDir = D3DXVECTOR3(0.0f,0.6f,-0.8f);
  // Load the config file
  if (doc.Load(szXMLFile) >= 0)
  {
    node = doc.GetNextNode(XML_ROOT_NODE);
    while(node > 0)
    {
      if (strcmpi(doc.GetNodeTag(node),"screensaver"))
      {
        node = doc.GetNextNode(node);
        continue;
      }
      if (childNode = doc.GetChildNode(node,"wireframe")){
        world.isWireframe = strcmp(doc.GetNodeText(childNode),"true") == 0;
      }
      if (childNode = doc.GetChildNode(node,"elasticity")){
        elasticity = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"viscosity")){
        viscosity = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"tension")){
        tension = (float)atof(doc.GetNodeText(childNode));
      }      
      if (childNode = doc.GetChildNode(node,"blendability")){
        tension = (float)atof(doc.GetNodeText(childNode));
      }      
      if (childNode = doc.GetChildNode(node,"xmin")){
        xmin = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"xmax")){
        xmax = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"ymin")){
        ymin = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"ymax")){
        ymax = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lightx")){
        g_lightDir.x = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lighty")){
        g_lightDir.y = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lightz")){
        g_lightDir.z = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"shininess")){
        g_shininess = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"texturefolder")){
        strcpy(world.szTextureSearchPath, doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"texturemode")){
        world.isTextureMode = strcmp(doc.GetNodeText(childNode),"true") == 0;
      }
      if (childNode = doc.GetChildNode(node,"nexttexture")){
        world.nextTextureTime = (int)(30*(float)atof(doc.GetNodeText(childNode)));
      }
      if (childNode = doc.GetChildNode(node,"quality")){
        divs = atoi(doc.GetNodeText(childNode));
      }
      node = doc.GetNextNode(node);
    }
    doc.Close();
  }
  float scaleRatio = (xmax-xmin)/(ymax-ymin);
  int totalPoints = (int)(divs * divs * scaleRatio);
  //world.scaleX = 0.5f;
  xdivs = (int)sqrt(totalPoints * scaleRatio / world.scaleX);
  ydivs = totalPoints / xdivs;
  //xdivs = 144; ydivs= 90;
  world.waterField = new WaterField(xmin, xmax, ymin, ymax, xdivs, ydivs, height, elasticity, viscosity, tension, blendability, world.isTextureMode);
}

extern "C" void GetInfo(SCR_INFO* pInfo)
{
  // not used, but can be used to pass info
  // back to XBMC if required in the future
  return;
}

extern "C" 
{

  struct ScreenSaver
  {
  public:
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float ratio);
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* Stop) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
  } ;


  void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
  {
    pScr->Create = Create;
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->Stop = Stop;
    pScr->GetInfo = GetInfo;
  }
};
