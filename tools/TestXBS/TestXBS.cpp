#include "TestXBS.h"

DWORD oldTime, currentTime;

void initDX8(HWND hWnd) {
    //First of all, create the main D3D object. If it is created successfully we 
    //should get a pointer to an IDirect3D8 interface.
    g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);

    //Get the current display mode
    D3DDISPLAYMODE d3ddm;
    g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    //Create a structure to hold the settings for our device
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    //Fill the structure. 
    //We want our program to be windowed, and set the back buffer to a format
    //that matches our current display mode
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
	
	//For depth buffering (e.g.) the z-buffer
	//d3dpp.BackBufferCount=1;
	//d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	//d3dpp.EnableAutoDepthStencil = TRUE;

    //Create a Direct3D device.
    g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pD3DDevice);
 
	/*
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	g_pD3DDevice->SetTextureStageState(0,D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(0,D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	*/
	g_pD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
	g_pD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);

	//Turn on back face culling. This is becuase we want to hide the back of our polygons
    g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);//D3DCULL_CCW);

	//Turn on Wireframe for debugging
	//g_pD3DDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);

	//Turn off lighting becuase we are specifying that our vertices have colour
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	//Turn off z-buffering
	g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
}

void cleanupDX8() {
	xbsFuncs.Stop();
	g_pD3DDevice->Release();
	g_pD3DDevice = NULL;

	g_pD3D->Release();
	g_pD3D = NULL;
}

void Render() {
	oldTime = currentTime;
	
	// Clear the back buffer
	g_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
	
	//begin scene
	g_pD3DDevice->BeginScene();

	//call Screensaver's Render function
	xbsFuncs.Render();
	
	//end scene
	g_pD3DDevice->EndScene();
	
	//display scene
	g_pD3DDevice->Present( NULL, NULL, NULL, NULL );

	//limit to 30fps
	currentTime = GetTickCount();
	if (currentTime - oldTime < 1/30)
		Sleep(1/20 - (currentTime - oldTime));
}

/* Handle all messages for the main window here                            */
long _stdcall MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_DESTROY) {
		// tidy up before leaving
		cleanupDX8();
        PostQuitMessage(0);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* Program entry point.                                                    */
int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdLine, int cmdShow) {
    MSG msg;
	char szname[] = "DirectX3D";
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      szname, NULL };
    RegisterClassEx( &wc );
    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW,
					szname, "XBS Test Harness", 
					WS_OVERLAPPEDWINDOW,//for fullscreen make into WS_POPUP
					WindowXpos, WindowYpos, WindowXres, WindowYres,
					GetDesktopWindow(), NULL, wc.hInstance, NULL);
    
    // Initilise or directX code here!
	// INIT OUR DIRECTX ONCE HERE AT THE START HERE!!!!!!!!!!!!!!!!!!!!!!!!!
    initDX8(hWnd);

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	hDll = LoadLibrary(cmdLine);
	if(!hDll) {
		MessageBox(NULL, "Could not load visualisation dll.", "Error", MB_OK);
		//cleanupDX8();
        PostQuitMessage(0);
	}
	get_module = (fd_get_module)GetProcAddress(hDll, "get_module");
	if(!get_module) {
		MessageBox(NULL, "Could not get address of get_module.", "Error", MB_OK);
		//cleanupDX8();
        PostQuitMessage(0);
	}
	//get funcion pointers for ScreenSaver
	get_module(&xbsFuncs);
	//tell screensaver to create itself
	xbsFuncs.Create(g_pD3DDevice, WindowXres, WindowYres, "TestSaver");
	//tell screensaver to start
	xbsFuncs.Start();

	//init time
	currentTime = GetTickCount();

    // Message loop. Note that this has been modified to allow
    // us to execute if no messages are being processed.
    while(1)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!GetMessage(&msg, NULL, 0, 0))
                break;
            DispatchMessage(&msg);
        }
        if (g_pD3DDevice != NULL)
			Render();
    }
    return 0;
}