/**

\page doc_adv_timeout Timeout long running scripts

To prevent long running scripts to freeze the application it may be necessary
to add a way to timeout the execution. This article presents two different
ways to do so.

\section doc_adv_timeout_1 With the line callback

The line callback feature can be used to perform some special treatment 
during execution of the scripts. The callback is called for every script 
statement, which for example makes it possible to verify if the script has  
executed for too long time and if so suspend the execution to be resumed at
a later time.

Before calling the context's \ref asIScriptContext::Execute "Execute" method, 
set the callback function like so:

\code
int ExecuteScriptWithTimeOut(asIScriptContext *ctx)
{
  // Define the timeout as 1 second
  DWORD timeOut = timeGetTime() + 1000;

  // Set up the line callback that will timout the script
  ctx->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);

  // Execute the script
  int status = ctx->Execute();

  // If the status is asEXECUTION_SUSPENDED the script can
  // be resumed by calling this function again.
  return status;
}

// The line callback function is called by the VM for each statement that is executed
void LineCallback(asIScriptContext *ctx, DWORD *timeOut)
{
  // If the time out is reached we suspend the script
  if( *timeOut < timeGetTime() )
    ctx->Suspend();
}
\endcode

Take a look at the sample \ref doc_samples_events to 
see this working.

\section doc_adv_timeout_2 With a secondary thread

A second thread can be set up to suspend the execution after the timeout. This thread
can then be put to sleep so that it doesn't impact performance during the execution.
When the thread wakes up it should call the context's \ref asIScriptContext::Suspend "Suspend" method.

The below shows some possible code for doing this. Note that the code for setting up 
the thread is fictive, as this is different for each target OS.

\code
// The variables that are shared between the threads
asIScriptContext *threadCtx;
int threadId;

// This function will be executed in the secondary thread
void SuspendThread()
{
  // Put the thread to sleep until the timeout (1 second)
  Sleep(1000);
  
  // When we wake-up we call the context's Suspend method
  ctx->Suspend();
}

// This function sets up the timeout thread and executes the script
int ExecuteScriptWithTimeOut(asIScriptContext *ctx)
{
  // Set the shared context pointer before creating the thread
  threadCtx = ctx;

  // Create the thread that will immediately go to sleep
  threadId = CreateThread(SuspendThread);
  
  // Execute the script
  int status = ctx->Execute();
  
  // Destroy the secondary thread before releasing the context
  DestroyThread(threadId);
  
  // Clear the global variables
  threadId = 0;
  threadCtx = 0;
  
  // If the status is asEXECUTION_SUSPENDED the script can
  // be resumed by calling this function again.
  
  return status;
}
\endcode

Observe that this way of doing it is safe even if the AngelScript library has been
built without \ref doc_adv_multithread "multithread support". 
*/
