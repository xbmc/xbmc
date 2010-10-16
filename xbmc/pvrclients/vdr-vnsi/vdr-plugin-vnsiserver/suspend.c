/*
 *  $Id: suspend.c,v 1.3 2008/10/22 11:59:32 schmirl Exp $
 */

#include "suspend.h"
#include "suspend.dat"
#include "config.h"

cSuspendLive::cSuspendLive(void)
		: cThread("Streamdev: server suspend")
{
}

cSuspendLive::~cSuspendLive() {
	Stop();
	Detach();
}

void cSuspendLive::Activate(bool On) {
	LOGCONSOLE("Activate cSuspendLive %d\n", On);
	if (On)
		Start();
	else
		Stop();
}

void cSuspendLive::Stop(void) {
	if (Running())
		Cancel(3);
}

void cSuspendLive::Action(void) {
	while (Running()) {
		DeviceStillPicture(suspend_mpg, sizeof(suspend_mpg));
		cCondWait::SleepMs(100);
	}
}

bool cSuspendCtl::m_Active = false;

cSuspendCtl::cSuspendCtl(void):
		cControl(m_Suspend = new cSuspendLive) {
	m_Active = true;
}

cSuspendCtl::~cSuspendCtl() {
	m_Active = false;
	DELETENULL(m_Suspend);
}

eOSState cSuspendCtl::ProcessKey(eKeys Key) {
	if (!m_Suspend->Active() || Key == kBack) {
		DELETENULL(m_Suspend);
		return osEnd;
	}
	return osContinue;
}
