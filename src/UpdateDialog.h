#pragma once

#include "UpdateObserver.h"

/** Base class for the updater's UI, sub-classed
 * by the different platform implementations.
 */
class UpdateDialog : public UpdateObserver
{
	public:
		UpdateDialog();
		virtual ~UpdateDialog() {};

		/** Sets whether the updater should automatically
		  * exit once the update has been installed.
		  */
		void setAutoClose(bool autoClose);
		bool autoClose() const;

		virtual void init(int argc, char** argv) = 0;
		virtual void exec() = 0;
		virtual void quit() = 0;

		virtual void updateFinished();	
	
	private:
		bool m_autoClose;
};

