#pragma once

class IWindowManagerCallback
{
public:
	IWindowManagerCallback(void);
	virtual ~IWindowManagerCallback(void);

	virtual void FrameMove()=0;
	virtual void Render()=0;
};
