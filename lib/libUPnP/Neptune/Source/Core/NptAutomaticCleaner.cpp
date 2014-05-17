/*****************************************************************
|
|   Neptune - Automatic Cleaner
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptAutomaticCleaner.h"
#include "NptThreads.h"

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::NPT_AutomaticCleaner
+---------------------------------------------------------------------*/
NPT_AutomaticCleaner::NPT_AutomaticCleaner() :
    m_TlsContext(NULL),
    m_HttpConnectionManager(NULL)
{
}

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::~NPT_AutomaticCleaner
+---------------------------------------------------------------------*/
NPT_AutomaticCleaner::~NPT_AutomaticCleaner()
{
    // When using TLS, the order to destroy singletons is important as
    // connections may still need the TLS context up until they're
    // cleaned up
    delete m_HttpConnectionManager;
    delete m_TlsContext;
    
    // Finally we can destroy the rest such as the NPT_HttpClient::ConnectionCanceller
    m_Singletons.Apply(NPT_ObjectDeleter<Singleton>());
}

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::GetInstance
+---------------------------------------------------------------------*/
NPT_AutomaticCleaner*
NPT_AutomaticCleaner::GetInstance()
{
    if (Instance) return Instance;
    
    NPT_SingletonLock::GetInstance().Lock();
    if (Instance == NULL) {
        // create the shared instance
        Instance = new NPT_AutomaticCleaner();
    }
    NPT_SingletonLock::GetInstance().Unlock();
    
    return Instance;
}
NPT_AutomaticCleaner* NPT_AutomaticCleaner::Instance = NULL;
NPT_AutomaticCleaner::Cleaner NPT_AutomaticCleaner::Cleaner::AutomaticCleaner;

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::RegisterTlsContext
+---------------------------------------------------------------------*/
NPT_Result
NPT_AutomaticCleaner::RegisterTlsContext(Singleton* singleton)
{
    m_TlsContext = singleton;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::RegisterHttpConnectionManager
+---------------------------------------------------------------------*/
NPT_Result
NPT_AutomaticCleaner::RegisterHttpConnectionManager(Singleton* singleton)
{
    m_HttpConnectionManager = singleton;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AutomaticCleaner::Register
+---------------------------------------------------------------------*/
NPT_Result
NPT_AutomaticCleaner::Register(Singleton *singleton)
{
    // Prevent double insertion
    m_Singletons.Remove(singleton);
    return m_Singletons.Insert(m_Singletons.GetFirstItem(), singleton);
}
