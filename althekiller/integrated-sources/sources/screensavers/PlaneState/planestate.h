////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "animatorfloat.h"
#include "background.h"

/***************************** D E F I N E S *******************************/

#define NUMCFGS			4

/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
// Animation values for a float animator.
//
class CAnimValueCfg
{
public:
	f32		m_MinValue, m_MaxValue;
	EAMode	m_AnimMode;

	f32		m_MinDelay, m_MaxDelay;
	EAMode	m_DelayAM;				// Only NONE and RAND is valid

	f32		m_MinITime, m_MaxITime;
	EAMode	m_ITimeAM;				// Only NONE and RAND is valid
};


////////////////////////////////////////////////////////////////////////////
// A single plane
//
class CPSPlane
{
public:
	CVector		m_Rot;
	CVector		m_Scale;
	CVector		m_CenterAxisDist;
	CRGBA		m_Col;
	CVector		m_PivotPos;

	CMatrix		m_Transform;				// World transformation matrix

				CPSPlane()
				{
					m_Scale = CVector(1.0f, 1.0f, 1.0f);
					m_CenterAxisDist.Zero();
					m_Col.Set(1.0f, 1.0f, 1.0f, 1.0f);
					m_PivotPos.Zero();
				}

	void		Copy(const CPSPlane& src)
	{
		m_Rot		= src.m_Rot;
		m_Col		= src.m_Col;
		m_Scale		= src.m_Scale;
		m_PivotPos	= src.m_PivotPos;
		m_CenterAxisDist = src.m_CenterAxisDist;
	}
};

////////////////////////////////////////////////////////////////////////////
//
class CPlanestate
{
public:
					CPlanestate(f32 cfgProbability[NUMCFGS]);
					~CPlanestate();
	bool			RestoreDevice(CRenderD3D* render);
	void			InvalidateDevice(CRenderD3D* render);
	void			Update(f32 dt);
	bool			Draw(CRenderD3D* render);

protected:
	CBackground		m_Background;

	int				m_ConfigNr;					// The configuration we are showing

	int				m_NumPlanes;
	CPSPlane*		m_Planes;
	
	CVector			m_PlaneCAxisRot;
	f32				m_PlaneYDelta;
	CVector			m_CenterAxisRot;
	CVector			m_Size;
	f32				m_PlaneUpdateDelay;

	// The diffrent value animators
	CColorAnimator	m_ColAnim;
	CVectorAnimator	m_RotAnim;
	CVectorAnimator	m_CADAnim;
	CVectorAnimator	m_PPPAnim;
	CVectorAnimator	m_CARAnim;
	CVectorAnimator m_CRAnim;
	CFloatAnimator	m_PPDAnim;
	CVectorAnimator	m_PSAnim;
	CVectorAnimator	m_CMRAnim;

	// Device objects
	LPDIRECT3DVERTEXBUFFER8		m_VertexBuffer;
	LPDIRECT3DTEXTURE8			m_Texture;

	bool			CreatePlaneTexture(CRenderD3D* render);
	void			UpdatePlane(f32 dt);
};

/***************************** I N L I N E S *******************************/
