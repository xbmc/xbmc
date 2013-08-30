#include "UpdateDialog.h"

UpdateDialog::UpdateDialog()
: m_autoClose(false)
{
}

void UpdateDialog::setAutoClose(bool autoClose)
{
	m_autoClose = autoClose;
}

bool UpdateDialog::autoClose() const
{
	return m_autoClose;
}

void UpdateDialog::updateFinished()
{
	if (m_autoClose)
	{
		quit();
	}
}

