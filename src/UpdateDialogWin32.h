#pragma once

#include "Platform.h"
#include "UpdateObserver.h"

#include "wincore.h"
#include "controls.h"
#include "stdcontrols.h"

class UpdateDialogWin32 : public UpdateObserver
{
	public:
		UpdateDialogWin32();
		~UpdateDialogWin32();

		void init();
		void exec();

		// implements UpdateObserver
		virtual void updateError(const std::string& errorMessage);
		virtual bool updateRetryCancel(const std::string& message);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

		LRESULT WINAPI windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		struct Message
		{
			enum Type
			{
				UpdateFailed,
				UpdateProgress,
				UpdateFinished
			};

			Message(Type _type)
			: type(_type)
			, progress(0)
			{
			}

			Type type;
			std::string message;
			int progress;
		};

		void installWindowProc(CWnd* window);

		CWinApp m_app;
		CWnd m_window;
		CStatic m_progressLabel;
		CProgressBar m_progressBar;
		CButton m_finishButton;
};

