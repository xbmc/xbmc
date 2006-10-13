#include "stdafx.h"
#include <objbase.h>
#include "Form1.h"
#include "FormSnapshot.h"
#include "source\DataManager.h"

using namespace XboxMemoryUsage;

//static Form1 form1;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	System::Threading::Thread::CurrentThread->ApartmentState = System::Threading::ApartmentState::STA;
	Form1* form = new Form1();
	FormSnapShot* formSnapShot = new FormSnapShot();
  DataManager* dataManager = new DataManager();
  FormManager* formManager = new FormManager(dataManager, form, formSnapShot);
  
  CLogHelper::GetInstance(formManager)->Log("Initializing log system");

	CController* pController = new CController(dataManager, formManager);
  form->pController = pController;
  formSnapShot->pController = pController;
  
  pController->Log("Initilializing (CoInitialize)");
  HRESULT hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    pController->Log("CoInitialize failed\n");
    return -1;
  }
  pController->Log("Initilializing succesfull");
  pController->Log("Application running...");
  
  formSnapShot->Show();
	Application::Run(form);
	return 0;
}
