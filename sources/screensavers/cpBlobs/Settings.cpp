#include "XmlDocument.h"
#include <stdio.h>
#include "Blobby.h"

extern float g_fFOV;
extern float g_fAspectRatio;

extern float g_fTickSpeed;
extern D3DXVECTOR3 g_WorldRotSpeeds;
extern char g_strCubemap[1024];
extern char g_strDiffuseCubemap[1024];
extern char g_strSpecularCubemap[1024];

extern bool g_bShowCube;
	
extern DWORD g_BlendStyle;

extern Blobby *m_pBlobby;

extern DWORD g_BGTopColor;
extern DWORD g_BGBottomColor;

extern char m_szScrName[1024];

///////////////////////////////////////////////////////////////////////////////

void SetDefaults()
{
	// set any default values for your screensaver's parameters
	g_fFOV = 45.0f;
	g_fAspectRatio = 1.33f;

	g_WorldRotSpeeds.x = 1.0f;
	g_WorldRotSpeeds.y = 0.5f;
	g_WorldRotSpeeds.z = 0.25f;
	strcpy( g_strCubemap, "data\\nvlobby_cube_mipmap.dds" );
	strcpy( g_strDiffuseCubemap, "data\\nvlobby_cube_mipmap_diffuse.dds" );
	strcpy( g_strSpecularCubemap, "data\\nvlobby_cube_mipmap_specular.dds" );

	m_pBlobby->m_fMoveScale = 0.3f;

	g_bShowCube = true;

	m_pBlobby->m_BlobPoints[0].m_Position.x = 0.5f;
	m_pBlobby->m_BlobPoints[0].m_Position.y = 0.5f;
	m_pBlobby->m_BlobPoints[0].m_Position.z = 0.5f;
	m_pBlobby->m_BlobPoints[0].m_fInfluence = 0.25f;
	m_pBlobby->m_BlobPoints[0].m_Speeds.x = 2.0f;
	m_pBlobby->m_BlobPoints[0].m_Speeds.y = 4.0f;
	m_pBlobby->m_BlobPoints[0].m_Speeds.z = 0.0f;
	m_pBlobby->m_BlobPoints[1].m_Position.x = 0.6f;
	m_pBlobby->m_BlobPoints[1].m_Position.y = 0.5f;
	m_pBlobby->m_BlobPoints[1].m_Position.z = 0.5f;
	m_pBlobby->m_BlobPoints[1].m_fInfluence = 0.51f;
	m_pBlobby->m_BlobPoints[1].m_Speeds.x = -4.0f;
	m_pBlobby->m_BlobPoints[1].m_Speeds.y = 2.0f;
	m_pBlobby->m_BlobPoints[1].m_Speeds.z = 0.0f;
	m_pBlobby->m_BlobPoints[2].m_Position.x = 0.3f;
	m_pBlobby->m_BlobPoints[2].m_Position.y = 0.5f;
	m_pBlobby->m_BlobPoints[2].m_Position.z = 0.3f;
	m_pBlobby->m_BlobPoints[2].m_fInfluence = 0.1f;
	m_pBlobby->m_BlobPoints[2].m_Speeds.x = -2.0f;
	m_pBlobby->m_BlobPoints[2].m_Speeds.y = 0.0f;
	m_pBlobby->m_BlobPoints[2].m_Speeds.z = 3.0f;
	m_pBlobby->m_BlobPoints[3].m_Position.x = 0.5f;
	m_pBlobby->m_BlobPoints[3].m_Position.y = 0.5f;
	m_pBlobby->m_BlobPoints[3].m_Position.z = 0.5f;
	m_pBlobby->m_BlobPoints[3].m_fInfluence = 0.25f;
	m_pBlobby->m_BlobPoints[3].m_Speeds.x = 0.0f;
	m_pBlobby->m_BlobPoints[3].m_Speeds.y = 2.0f;
	m_pBlobby->m_BlobPoints[3].m_Speeds.z = 1.0f;
	m_pBlobby->m_BlobPoints[4].m_Position.x = 0.5f;
	m_pBlobby->m_BlobPoints[4].m_Position.y = 0.5f;
	m_pBlobby->m_BlobPoints[4].m_Position.z = 0.5f;
	m_pBlobby->m_BlobPoints[4].m_fInfluence = 0.15f;
	m_pBlobby->m_BlobPoints[4].m_Speeds.x = 0.5f;
	m_pBlobby->m_BlobPoints[4].m_Speeds.y = 0.0f;
	m_pBlobby->m_BlobPoints[4].m_Speeds.z = 1.0f;

	m_pBlobby->SetDensity( 32 );
	m_pBlobby->m_TargetValue = 24.0f;

	return;
}

///////////////////////////////////////////////////////////////////////////////

// helper functions for parsing vectors etc. from a given line of text
void LoadVector( D3DXVECTOR3 *pVec, char *pStr )
{
	float x, y, z;
	sscanf( pStr, "%f %f %f", &x, &y, &z );
	pVec->x = x;
	pVec->y = y;
	pVec->z = z;
}

void LoadColor( DWORD *pCol, char *pStr )
{
	int r, g, b;
	sscanf( pStr, "%d %d %d", &r, &g, &b );
	*pCol = D3DCOLOR_RGBA( r, g, b, 255 );
}

void LoadBlob( BlobPoint *pPoint, char *pStr )
{
	float x, y, z, inf, vx, vy, vz;
	sscanf( pStr, "%f %f %f %f %f %f %f", &x, &y, &z, &inf, &vx, &vy, &vz );

	pPoint->m_Position.x = x;
	pPoint->m_Position.y = y;
	pPoint->m_Position.z = z;

	pPoint->m_fInfluence = inf;

	pPoint->m_Speeds.x = vx;
	pPoint->m_Speeds.y = vy;
	pPoint->m_Speeds.z = vz;
}

///////////////////////////////////////////////////////////////////////////////

// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func
// is called.
void LoadSettings()
{
	XmlNode node, childNode;
	CXmlDocument doc;
	
	// Set up the defaults
	SetDefaults();

	char szXMLFile[1024];
	strcpy(szXMLFile, "Q:\\screensavers\\");
	strcat(szXMLFile, m_szScrName);
	strcat(szXMLFile, ".xml");

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
			
			if ( childNode = doc.GetChildNode( node, "fov" ) )
				g_fFOV = (float)atof( doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "aspectratio" ) )
				g_fAspectRatio = (float)atof( doc.GetNodeText( childNode ) );
			
			
			if (childNode = doc.GetChildNode(node,"showcube"))
				g_bShowCube = !strcmpi(doc.GetNodeText(childNode),"true");

			if ( childNode = doc.GetChildNode( node, "bgtopcolor" ) )
				LoadColor( &g_BGTopColor, doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "bgbottomcolor" ) )
				LoadColor( &g_BGBottomColor, doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "globalspeed" ) )
				g_fTickSpeed = (float)atof( doc.GetNodeText( childNode ) );
			
			if ( childNode = doc.GetChildNode( node, "worldrot" ) )
				LoadVector( &g_WorldRotSpeeds, doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "numblobs" ) )
				m_pBlobby->m_iNumPoints = atoi( doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "cubemap" ) )
				strcpy( g_strCubemap, doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "diffusecubemap" ) )
				strcpy( g_strDiffuseCubemap, doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "specularcubemap" ) )
				strcpy( g_strSpecularCubemap, doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "blendstyle" ) )
				g_BlendStyle = atoi( doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "movescale" ) )
				m_pBlobby->m_fMoveScale = (float)atof( doc.GetNodeText( childNode ) );

			if ( childNode = doc.GetChildNode( node, "smoothness" ) )
				m_pBlobby->SetDensity( atoi( doc.GetNodeText( childNode ) ) );

			if ( childNode = doc.GetChildNode( node, "blobbiness" ) )
				m_pBlobby->m_TargetValue = (float)atof( doc.GetNodeText( childNode ) );


			if ( childNode = doc.GetChildNode( node, "blob1" ) )
				LoadBlob( &m_pBlobby->m_BlobPoints[0], doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "blob2" ) )
				LoadBlob( &m_pBlobby->m_BlobPoints[1], doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "blob3" ) )
				LoadBlob( &m_pBlobby->m_BlobPoints[2], doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "blob4" ) )
				LoadBlob( &m_pBlobby->m_BlobPoints[3], doc.GetNodeText( childNode ) );
			if ( childNode = doc.GetChildNode( node, "blob5" ) )
				LoadBlob( &m_pBlobby->m_BlobPoints[4], doc.GetNodeText( childNode ) );

			node = doc.GetNextNode(node);
		}
		doc.Close();
	}

}

