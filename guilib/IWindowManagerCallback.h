/*!
	\file IWindowManagerCallback.h
	\brief 
	*/

#pragma once

/*!
	\ingroup winman
	\brief 
	*/
class IWindowManagerCallback
{
public:
	IWindowManagerCallback(void);
	virtual ~IWindowManagerCallback(void);

	virtual void FrameMove()=0;
	virtual void Render()=0;
	virtual void Process()=0;
};
