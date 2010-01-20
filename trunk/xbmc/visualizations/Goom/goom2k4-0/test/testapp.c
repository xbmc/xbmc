#include <goom/goom.h>
#include <stdio.h>

gint16 data[2][512];

int main()
{
    int i;
    PluginInfo *goom;
    goom = goom_init (640, 480);
    for (i = 0; i<100; i++)
    {
        fprintf(stderr,"*");
        goom_update (goom, data, 0, -1, 0, 0);
    }
    return 0;
}
