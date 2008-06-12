////////////////////////////////////////////////////////////////////////////
//
// Planestate Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
////////////////////////////////////////////////////////////////////////////
//
// Animates a float value in diffrent ways
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "animatorfloat.h"

////////////////////////////////////////////////////////////////////////////
//
CFloatAnimator::CFloatAnimator(u32 id, f32 value) : CValueAnimator(AVT_FLOAT, id)
{
	Init(id, value);
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::Init(u32 id, f32 value)
{
	CValueAnimator::Init(AVT_FLOAT, id);

	m_Type		= AVT_FLOAT;
	m_AnimMode	= AM_NONE;
	m_State		= FAS_NORMAL;

	m_Time		= 0.0;

	m_Min	= 0.0f;
	m_Max	= value>0.0f?value:1.0f;
	m_Value	= m_NewValue= m_OldValue	= value;

	// Delay
	m_DelayAM = AM_RAND;			// Only NONE and RAND is valid
	m_CurDelay	= 0.0f;
	m_MinDelay	= 0.0f;
	m_MaxDelay	= 3.0f;

	// Interpolation time (To move to the next value)
	m_ITimeAM	= AM_RAND;					// Only NONE and RAND is valid
	m_ITime		= m_CurITime	= 0.0f;		// Force him to get a new dest value directly
	m_MinITime	= 3.0f;
	m_MaxITime	=20.0f;
}

////////////////////////////////////////////////////////////////////////////
// Updates the slider to the new position
//
void		CFloatAnimator::Update(f32 deltaTime)
{
	CValueAnimator::Update(deltaTime);
	m_Time += deltaTime;

	bool	newValue = false;
	if (m_State == FAS_NORMAL)
	{
		if (m_Time >= m_CurITime)
		{
			GetNewValue(m_CurDelay, m_CurDelay, m_MinDelay, m_MaxDelay, m_DelayAM);

			if (m_CurDelay > FLOATEPSILON)
			{
				m_State = FAS_WAITING;
				m_Time	= 0.0f;
				m_Value = m_OldValue = m_NewValue;
			}
			else
			{
				// Could be that we have a very low framerate
				while ((m_Time > m_CurITime) && (m_CurITime > 0.0f))
					m_Time -= m_CurITime;
				newValue = true;
				m_OldValue = m_NewValue;
			}
		}
	}
	else	// Waiting
	{
		if (m_Time >= m_CurDelay)
		{
			m_State	= FAS_NORMAL;
			m_Time	= 0.0f;
			newValue= true;
		}
	}

	if (newValue)
	{
		GetNewValue(m_OldValue, m_NewValue, m_Min, m_Max, m_AnimMode);
		GetNewValue(m_ITime, m_CurITime, m_MinITime, m_MaxITime, m_ITimeAM);
	}

	if (m_State == FAS_NORMAL)
	{
		bool linear = false;
		if ((m_AnimMode == AM_LOOPUP) || (m_AnimMode == AM_LOOPDOWN))
			linear = true;
		f32 timeScale = 1.0f;
		if (m_CurITime>0.0001f)
			timeScale = m_Time / m_CurITime;

		m_Value	= InterpolateFloat(m_OldValue, m_NewValue, timeScale, linear);
	}
	else
	{
		m_Value = m_OldValue;
	}
	m_Value = Clamp(m_Value, m_Min, m_Max);
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetValue(f32 value)
{
	m_OldValue = m_NewValue = m_Value = value;
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetMin(f32 value)
{
	m_Min = value;
	m_Value = Clamp(m_Value, m_Min, m_Max);
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetMax(f32 value)
{
	m_Max = value;
	m_Value = Clamp(m_Value, m_Min, m_Max);
}

////////////////////////////////////////////////////////////////////////////
//
f32		CFloatAnimator::GetMin(void)
{
	return m_Min;
}

////////////////////////////////////////////////////////////////////////////
//
f32		CFloatAnimator::GetMax(void)
{
	return m_Max;
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetITime(f32 time)
{
	m_ITime = time;
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetMinMaxITime(f32 min, f32 max, EAMode mode)
{
	assert((mode == AM_NONE) || (mode == AM_RAND));
	m_ITimeAM	= mode;
	m_MinITime	= min;
	m_MaxITime	= max;
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetDelay(f32 delay)
{
	m_CurDelay = delay;
}

////////////////////////////////////////////////////////////////////////////
//
void	CFloatAnimator::SetMinMaxDelay(f32 min, f32 max, EAMode mode)
{
	assert((mode == AM_NONE) || (mode == AM_RAND));
	m_DelayAM	= mode;
	m_MinDelay	= min;
	m_MaxDelay	= max;
}

////////////////////////////////////////////////////////////////////////////
//
f32		CFloatAnimator::GetValue(void)
{
	return m_Value;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Generates a new value based on the animation mode
// Will also adjust the current value if needed
//
void	CFloatAnimator::GetNewValue(f32& fromValue, f32& toValue, f32 min, f32 max, EAMode mode)
{
	switch (mode)
	{
		case AM_NONE:	toValue = fromValue;						break;
		case AM_RAND:	toValue = min + (RandFloat()*(max-min));	break;
		case AM_LOOPUP:
			if (ISEQUAL(fromValue, max, 0.01f))
				fromValue	= min;
			toValue		= max;
			break;
		case AM_LOOPDOWN:
			if (ISEQUAL(fromValue, min, 0.01f))
				fromValue	= max;
			toValue		= min;
			break;
		case AM_PINGPONG:
			if (ISEQUAL(fromValue, min, 0.01f))
			{
				toValue = max;
			}
			else
			{
				toValue = min;
			}
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
CColorAnimator::CColorAnimator(u32 id, CRGBA& value) : CValueAnimator(AVT_COLOR, id)
{
	for (int i=0; i<4; i++)
	{
		m_Values[i].Init(i, value.col[i]);
		m_Values[i].SetMin(0.0f);
		m_Values[i].SetMax(1.0f);
	}
}

////////////////////////////////////////////////////////////////////////////
//
void		CColorAnimator::Update(f32 deltaTime)
{
	CValueAnimator::Update(deltaTime);
	for (int i=0; i<4; i++)
	{
		m_Values[i].Update(deltaTime);
	}
}

////////////////////////////////////////////////////////////////////////////
//
CRGBA		CColorAnimator::GetValue(void)
{
	CRGBA	value;
	for (int i=0; i<4; i++)
	{
		value.col[i] = m_Values[i].GetValue();
	}
	return value;
}

////////////////////////////////////////////////////////////////////////////
//
void	CColorAnimator::SetValue(CRGBA value)
{
	for (int i=0; i<4; i++)
	{
		m_Values[i].SetValue(value.col[i]);
	}
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
CVectorAnimator::CVectorAnimator(u32 id, CVector& value) : CValueAnimator(AVT_VECTOR, id)
{
	m_Values[0].Init(0, value.x);
	m_Values[1].Init(1, value.y);
	m_Values[2].Init(2, value.z);
}

////////////////////////////////////////////////////////////////////////////
//
void		CVectorAnimator::Update(f32 deltaTime)
{
	CValueAnimator::Update(deltaTime);
	for (int i=0; i<3; i++)
	{
		m_Values[i].Update(deltaTime);
	}
}

////////////////////////////////////////////////////////////////////////////
//
CVector		CVectorAnimator::GetValue(void)
{
	CVector	value;
	for (int i=0; i<3; i++)
	{
		value[i] = m_Values[i].GetValue();
	}
	return value;
}

////////////////////////////////////////////////////////////////////////////
//
void	CVectorAnimator::SetValue(CVector value)
{
	for (int i=0; i<3; i++)
	{
		m_Values[i].SetValue(value[i]);
	}
}





