////////////////////////////////////////////////////////////////////////////
// Float animator
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FLOATANIMATOR_H
#define __FLOATANIMATOR_H

/***************************** D E F I N E S *******************************/

enum EVType
{
	AVT_NONE, AVT_FLOAT, AVT_COLOR, AVT_VECTOR,
};
 
// Animation Mode
enum EAMode
{
	AM_NONE, AM_RAND, AM_LOOPUP, AM_LOOPDOWN, AM_PINGPONG, NUMANIMMODES, AM_MAKELONG = 0x7fffffff
};

enum EFAState
{
	FAS_NONE, FAS_NORMAL, FAS_WAITING, FAS_MAKELONG = 0x7fffffff
};

/****************************** M A C R O S ********************************/
/************************** S T R U C T U R E S ****************************/

////////////////////////////////////////////////////////////////////////////
//
class CValueAnimator
{
public:
	EVType		m_Type;
	u32			m_Id;
		
						CValueAnimator(EVType type)				{ m_Type = type; }
						CValueAnimator(EVType type, u32 id)		{ Init(type, id); }
			void		Init(EVType type, u32 id)				{ m_Type = type; m_Id = id; }
	virtual	void		Update(f32 deltaTime)					{ }
};

////////////////////////////////////////////////////////////////////////////
// Animates a float value
//
class CFloatAnimator : public CValueAnimator
{
public:
	EFAState	m_State;

	EAMode		m_AnimMode;

	f32			m_Value;
	f32			m_Min;
	f32			m_Max;

	// Delay
	EAMode		m_DelayAM;				// Only NONE and RAND is valid
	f32			m_CurDelay;
	f32			m_MinDelay;
	f32			m_MaxDelay;

	// Interpolation time (To move to the next value)
	EAMode		m_ITimeAM;				// Only NONE and RAND is valid
	f32			m_ITime;
	f32			m_MinITime;
	f32			m_MaxITime;

	// Runtime data
	f32			m_Time;
	f32			m_CurITime;				// Current interpolation time

	f32			m_NewValue, m_OldValue;
	f32			m_NewMax, m_OldMax;

				CFloatAnimator() : CValueAnimator(AVT_FLOAT) { };
				CFloatAnimator(u32 id, f32 value);
	void		Init(u32 id, f32 value);

	virtual	void	Update(f32 deltaTime);

	void		SetMin(f32 value);
	void		SetValue(f32 value);
	void		SetMax(f32 value);
	f32			GetMin(void);
	f32			GetValue(void);
	f32			GetMax(void);

	void		SetMinMax(f32 min, f32 max)		{ SetMin(min); SetMax(max); }

	void		SetITime(f32 time);
	void		SetMinMaxITime(f32 min, f32 max, EAMode mode);
	void		SetDelay(f32 delay);
	void		SetMinMaxDelay(f32 min, f32 max, EAMode mode);

protected:
	void		GetNewValue(f32& fromValue, f32& toValue, f32 min, f32 max, EAMode mode);
};

////////////////////////////////////////////////////////////////////////////
// Animates a color (CRGBA) value
//
class CColorAnimator : public CValueAnimator
{
public:
	CFloatAnimator	m_Values[4];

					CColorAnimator(u32 id, CRGBA& value);
	virtual	void	Update(f32 deltaTime);

	void		SetValue(CRGBA value);
	CRGBA		GetValue(void);
};

////////////////////////////////////////////////////////////////////////////
// Animates a vector value
//
class CVectorAnimator : public CValueAnimator
{
public:
	CFloatAnimator	m_Values[3];

					CVectorAnimator(u32 id, CVector& value);
	virtual	void	Update(f32 deltaTime);

	void		SetValue(CVector value);
	CVector		GetValue(void);
};

/***************************** G L O B A L S *******************************/
/***************************** I N L I N E S *******************************/


#endif

