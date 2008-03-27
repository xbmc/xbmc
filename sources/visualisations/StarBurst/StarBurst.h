#include <xtl.h>
#include "XmlDocument.h"
#pragma comment (lib, "lib/xbox_dx8.lib" )


VOID SetupMatrices();
HRESULT InitGeometry();
void SetupCamera();
void SetupPerspective();
void SetupRotation(float x, float y, float z);
void LoadSettings();
extern "C" void Render();