////////////////////////////////////////////////////////////////////////////
// A column of chars
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "matrixtrails.h"
#include "column.h"

////////////////////////////////////////////////////////////////////////////
// 
CColumn::CColumn()
{
	m_NumChars = 0;
	m_Chars = null;
	m_Delay = m_CharDelay = RandFloat(gConfig.m_CharDelayMin, gConfig.m_CharDelayMax);
	m_FadeSpeed = RandFloat(gConfig.m_FadeSpeedMin, gConfig.m_FadeSpeedMax);
	m_CurChar = 0;
}

////////////////////////////////////////////////////////////////////////////
// 
CColumn::~CColumn()
{
	SAFE_DELETE_ARRAY(m_Chars);
}

////////////////////////////////////////////////////////////////////////////
// 
void		CColumn::Init(int numChars)
{
	m_NumChars = numChars;
	m_Chars = new CChar[m_NumChars];

	int startChar = Rand(m_NumChars);
	for (int i=0; i<startChar; i++)
	{
		Update(m_CharDelay+0.1f);
	}	
}

////////////////////////////////////////////////////////////////////////////
// 
void		CColumn::Update(f32 dt)
{
	for (int cNr=0; cNr<m_NumChars; cNr++)
	{
		m_Chars[cNr].m_Intensity = Clamp(m_Chars[cNr].m_Intensity-(dt*m_FadeSpeed), 0.0f, 1.0f);
	}
	m_Delay -= dt;
	if (m_Delay <= 0.0f)
	{	
		m_Delay = m_CharDelay;
		int prevChar = m_Chars[m_CurChar].m_CharNr;
		m_CurChar++;
		if (m_CurChar >= m_NumChars)
		{
			m_CurChar = 0;
			m_CharDelay = RandFloat(gConfig.m_CharDelayMin, gConfig.m_CharDelayMax);
		}
		do
		{
			m_Chars[m_CurChar].m_CharNr = rand()%gConfig.m_NumChars;
		} while (prevChar == m_Chars[m_CurChar].m_CharNr && gConfig.m_NumChars > 1);	// Check so we dont get the same char in a row
		m_Chars[m_CurChar].m_Intensity = 1.0f;
	}
}

////////////////////////////////////////////////////////////////////////////
// 
TRenderVertex*	CColumn::UpdateVertexBuffer(TRenderVertex* vert, f32 posX, f32 posY, const CVector& charSize, const CVector2& charSizeTex)
{
	int numCharsPerRow = (int)(1.0f/charSizeTex.x);
	for (int cNr=0; cNr<m_NumChars; cNr++)
	{
		DWORD	col;
		int charNr = m_Chars[cNr].m_CharNr;
		if (m_Chars[cNr].m_CharNr == 0)
			col = 0;		
		else if (cNr == m_CurChar)
			col = CRGBA(0.8f, 1.0f, 0.9f, 1.0f).RenderColor();	
		else
			col = CRGBA(gConfig.m_CharCol.r*m_Chars[cNr].m_Intensity, gConfig.m_CharCol.g*m_Chars[cNr].m_Intensity, gConfig.m_CharCol.b*m_Chars[cNr].m_Intensity, 1.0f).RenderColor();	

		f32 u = charSizeTex.x*(f32)(charNr % numCharsPerRow);
		f32 v = charSizeTex.y*(f32)(charNr / numCharsPerRow);
		
		vert->pos =	CVector(posX, posY+charSize.y, 0.0f);				vert->w = 0.0f;	vert->col =	col; vert->u = u; vert->v = v+charSizeTex.y;	vert++;
		vert->pos =	CVector(posX, posY, 0.0f);							vert->w = 0.0f;	vert->col =	col; vert->u = u; vert->v = v;					vert++;
		vert->pos =	CVector(posX+charSize.x, posY+charSize.y, 0.0f);	vert->w = 0.0f;	vert->col =	col; vert->u = u+charSizeTex.x; vert->v = v+charSizeTex.y;	vert++;
		vert->pos =	CVector(posX+charSize.x, posY, 0.0f);				vert->w = 0.0f;	vert->col =	col; vert->u = u+charSizeTex.x; vert->v = v;	vert++;
		posY += charSize.y;
	}
	return vert;
}















