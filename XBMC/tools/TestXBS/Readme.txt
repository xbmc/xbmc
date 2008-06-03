TestXBS v1.0 by Warren
This program was made to assist with screensaver (XBS) development for XBMC.
It is simply a wrapper program that calls the XBS start and stop routines and does the basic DirectX8 
render loop calling the XBS render func to draw the pretty pictures.

Right now the program is hardcoded to 720x480 using defines but is easily changed. In future versions I would like to have this configurable via a config file.

Also, I have a bunch of DirectX initialisation stuff in there that is specific to what I am doing. You should edit the initD3d() function to suit your needs (I didn't feel like wading through XBMC code to see exactly how it sets D3D up).

To use this app, run it with the path to the XBS you want to test as a commandline option ie:
TestXBS.exe path\to\ScreenSaver.xbs

I have also modified the TemplateXBS package provided by the XBMC team to provide a Test build target so it automatically links with the Windows Directx8 libraries instead of the XBOX library stub. You just have to edit the properties for the Test build target so that Debugging->Command points to your TestXBS.exe
