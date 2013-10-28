#include "UpdateDialogWin32.h"

#include "AppInfo.h"
#include "Log.h"

// enable themed controls
// see http://msdn.microsoft.com/en-us/library/bb773175%28v=vs.85%29.aspx
// for details
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static const char* updateDialogClassName = "UPDATEDIALOG";

static std::map<HWND,UpdateDialogWin32*> windowDialogMap;

// enable the standard Windows font for a widget
// (typically Tahoma or Segoe UI)
void setDefaultFont(HWND window)
{
	SendMessage(window, WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
}

LRESULT WINAPI updateDialogWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::map<HWND,UpdateDialogWin32*>::const_iterator iter = windowDialogMap.find(window);
	if (iter != windowDialogMap.end())
	{
		return iter->second->windowProc(window,message,wParam,lParam);
	}
	else
	{
		return DefWindowProc(window,message,wParam,lParam);
	}
};

void registerWindowClass()
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex,sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX); 

	HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= updateDialogWindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hIcon          = LoadIcon(GetModuleHandle(0),"IDI_APPICON");
	wcex.hCursor		= LoadCursor(0,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)background;
	wcex.lpszMenuName	= (LPCTSTR)0;
	wcex.lpszClassName	= updateDialogClassName;
	wcex.hIconSm		= 0;
	wcex.hInstance      = GetModuleHandle(0);

	RegisterClassEx(&wcex);
}

UpdateDialogWin32::UpdateDialogWin32()
: m_hadError(false)
{
	registerWindowClass();
}

UpdateDialogWin32::~UpdateDialogWin32()
{
	for (std::map<HWND,UpdateDialogWin32*>::iterator iter = windowDialogMap.begin();
	     iter != windowDialogMap.end();
		 iter++)
	{
		if (iter->second == this)
		{
			std::map<HWND,UpdateDialogWin32*>::iterator oldIter = iter;
			++iter;
			windowDialogMap.erase(oldIter);
		}
		else
		{
			++iter;
		}
	}
}

void UpdateDialogWin32::init(int /* argc */, char** /* argv */)
{
	int width = 300;
	int height = 130;

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	m_window.CreateEx(0 /* dwExStyle */,
	                  updateDialogClassName /* class name */,
                      AppInfo::name().c_str(),
                      style,
					  0, 0, width, height,
					  0 /* parent */, 0 /* menu */, 0 /* reserved */);
	m_progressBar.Create(&m_window);
	m_finishButton.Create(&m_window);
	m_progressLabel.Create(&m_window);

	installWindowProc(&m_window);
	installWindowProc(&m_finishButton);

	setDefaultFont(m_progressLabel);
	setDefaultFont(m_finishButton);
	
	m_progressBar.SetRange(0,100);
	m_finishButton.SetWindowText("Finish");
	m_finishButton.EnableWindow(false);
	m_progressLabel.SetWindowText("Installing Updates");

	m_window.SetWindowPos(0,0,0,width,height,0);
	m_progressBar.SetWindowPos(0,10,40,width - 30,20,0);
	m_progressLabel.SetWindowPos(0,10,15,width - 30,20,0);
	m_finishButton.SetWindowPos(0,width-100,70,80,25,0);
	m_window.CenterWindow();
	m_window.ShowWindow();
}

void UpdateDialogWin32::exec()
{
	m_app.Run();
}

void UpdateDialogWin32::updateError(const std::string& errorMessage)
{
	UpdateMessage* message = new UpdateMessage(UpdateMessage::UpdateFailed);
	message->message = errorMessage;
	SendNotifyMessage(m_window.GetHwnd(),WM_USER,reinterpret_cast<WPARAM>(message),0);
}

void UpdateDialogWin32::updateProgress(int percentage)
{
	UpdateMessage* message = new UpdateMessage(UpdateMessage::UpdateProgress);
	message->progress = percentage;
	SendNotifyMessage(m_window.GetHwnd(),WM_USER,reinterpret_cast<WPARAM>(message),0);
}

void UpdateDialogWin32::updateFinished()
{
	UpdateMessage* message = new UpdateMessage(UpdateMessage::UpdateFinished);
	SendNotifyMessage(m_window.GetHwnd(),WM_USER,reinterpret_cast<WPARAM>(message),0);
	UpdateDialog::updateFinished();
}

void UpdateDialogWin32::quit()
{
	PostThreadMessage(GetWindowThreadProcessId(m_window.GetHwnd(), 0 /* process ID */), WM_QUIT, 0, 0);
}

LRESULT WINAPI UpdateDialogWin32::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		if (window == m_window.GetHwnd())
		{
			return 0;
		}
		break;
	case WM_COMMAND:
		{
			if (reinterpret_cast<HWND>(lParam) == m_finishButton.GetHwnd())
			{
				quit();
			}
		}
		break;
	case WM_USER:
		{
			if (window == m_window.GetHwnd())
			{
				UpdateMessage* message = reinterpret_cast<UpdateMessage*>(wParam);
				switch (message->type)
				{
				case UpdateMessage::UpdateFailed:
					{
						m_hadError = true;
						std::string text = AppInfo::updateErrorMessage(message->message);
						MessageBox(m_window.GetHwnd(),text.c_str(),"Update Problem",MB_OK);
					}
					break;
				case UpdateMessage::UpdateProgress:
					m_progressBar.SetPos(message->progress);
					break;
				case UpdateMessage::UpdateFinished:
					{
						std::string message;
						m_finishButton.EnableWindow(true);
						if (m_hadError)
						{
							message = "Update failed.";
						}
						else
						{
							message = "Updates installed.";
						}
						message += "  Click 'Finish' to restart the application.";
						m_progressLabel.SetWindowText(message.c_str());
					}
					break;
				}
				delete message;
			}
		}
		break;
	}
	return DefWindowProc(window,message,wParam,lParam);
}

void UpdateDialogWin32::installWindowProc(CWnd* window)
{
	windowDialogMap[window->GetHwnd()] = this;
}
