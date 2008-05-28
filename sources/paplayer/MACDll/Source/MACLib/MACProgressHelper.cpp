#include "All.h"
#include "MACProgressHelper.h"

CMACProgressHelper::CMACProgressHelper(int nTotalSteps, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    m_pKillFlag = pKillFlag;
    
    m_bUseCallback = FALSE;
    if (ProgressCallback != NULL)
    {
        m_bUseCallback = TRUE;
        m_CallbackFunction = ProgressCallback;
    }

    m_pPercentageDone = pPercentageDone;

    m_nTotalSteps = nTotalSteps;
    m_nCurrentStep = 0;
    m_nLastCallbackFiredPercentageDone = 0;

    UpdateProgress(0);
}

CMACProgressHelper::~CMACProgressHelper()
{

}

void CMACProgressHelper::UpdateProgress(int nCurrentStep, BOOL bForceUpdate)
{
    // update the step
    if (nCurrentStep == -1)
        m_nCurrentStep++;
    else
        m_nCurrentStep = nCurrentStep;

    // figure the percentage done
    float fPercentageDone = float(m_nCurrentStep) / float(max(m_nTotalSteps, 1));
    int nPercentageDone = (int) (fPercentageDone * 1000 * 100);
    if (nPercentageDone > 100000) nPercentageDone = 100000;

    // update the percent done pointer
    if (m_pPercentageDone)
    {
        *m_pPercentageDone = nPercentageDone;
    }

    // fire the callback
    if (m_bUseCallback)
    {
        if (bForceUpdate || (nPercentageDone - m_nLastCallbackFiredPercentageDone) >= 1000)
        {
            m_CallbackFunction(nPercentageDone);
            m_nLastCallbackFiredPercentageDone = nPercentageDone;
        }
    }
}

int CMACProgressHelper::ProcessKillFlag(BOOL bSleep)
{
    // process any messages (allows repaint, etc.)
    if (bSleep)
    {
        PUMP_MESSAGE_LOOP
    }

    if (m_pKillFlag)
    {
        while (*m_pKillFlag == KILL_FLAG_PAUSE)
        {
            SLEEP(50);
            PUMP_MESSAGE_LOOP
        }

        if ((*m_pKillFlag != KILL_FLAG_CONTINUE) && (*m_pKillFlag != KILL_FLAG_PAUSE))
        {
            return -1;
        }
    }

    return ERROR_SUCCESS;
}
