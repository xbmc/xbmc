/* drivers.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#include "cfg.h"

#ifdef G

#include "links.h"

int F = 0;

struct graphics_driver *drv = NULL;

#ifdef GRDRV_X
extern struct graphics_driver x_driver;
#endif
#ifdef GRDRV_SVGALIB
extern struct graphics_driver svga_driver;
#endif
#ifdef GRDRV_FB
extern struct graphics_driver fb_driver;
#endif
#ifdef GRDRV_DIRECTFB
extern struct graphics_driver directfb_driver;
#endif
#ifdef GRDRV_PMSHELL
extern struct graphics_driver pmshell_driver;
#endif
#ifdef GRDRV_ATHEOS
extern struct graphics_driver atheos_driver;
#endif
#ifdef __XBOX__
extern struct graphics_driver xbox_driver;
#endif


struct graphics_driver *graphics_drivers[] = {
#ifdef GRDRV_PMSHELL
	&pmshell_driver,
#endif
#ifdef GRDRV_ATHEOS
	&atheos_driver,
#endif
#ifdef GRDRV_X
	&x_driver,
#endif
#ifdef GRDRV_SVGALIB
	&svga_driver,
#endif
#ifdef GRDRV_FB
	&fb_driver,
#endif
#ifdef GRDRV_DIRECTFB
	&directfb_driver,
#endif
#ifdef __XBOX__
	&xbox_driver,
#endif
	NULL
};

int dummy_block(struct graphics_device *dev)
{
	return 0;
}

void dummy_unblock(struct graphics_device *dev)
{
}

unsigned char *list_graphics_drivers()
{
	unsigned char *d = init_str();
	int l = 0;
	struct graphics_driver **gd;
	for (gd = graphics_drivers; *gd; gd++) {
		if (l) add_to_str(&d, &l, " ");
		add_to_str(&d, &l, (*gd)->name);
	}
	return d;
}

/* Tries to initialize graphics driver with given (or default) parameters. */

unsigned char *init_graphics_driver(struct graphics_driver *gd, unsigned char *param, unsigned char *display)
{
	unsigned char *p = param;
	struct driver_param *dp=get_driver_param(gd->name);

        if (!param ||
            !*param)
                p = dp->param;

	gd->codepage=dp->codepage;
	return gd->init_driver(p,display);
}

unsigned char *init_graphics(unsigned char *driver, unsigned char *param, unsigned char *display)
{
	unsigned char *s = init_str();
	int l = 0;
	struct graphics_driver **gd;

        for (gd = graphics_drivers; *gd; gd++) {
                if (!driver ||
                    !*driver ||
                    !_stricmp((*gd)->name, driver)) {
                        unsigned char *r = init_graphics_driver(*gd, param, display);

                        if (!r) { /* No error message returned, driver works OK */
                                mem_free(s);
				drv = *gd;
				F = 1;
				return NULL;
			}
			if (!l) {
                                if (!driver ||
                                    !*driver)
                                        add_to_str(&s, &l, "Could not initialize any graphics driver. Tried the following drivers:\n");
                                else
                                        add_to_str(&s, &l, "Could not initialize graphics driver ");
			}
			add_to_str(&s, &l, (*gd)->name);
			add_to_str(&s, &l, ":\n");
			add_to_str(&s, &l, r);
			mem_free(r);
		}
	}
	if (!l) {
		add_to_str(&s, &l, "Unknown graphics driver ");

                if (driver)
                        add_to_str(&s, &l, driver);

		add_to_str(&s, &l, ".\nThe following graphics drivers are supported:\n");
		for (gd = graphics_drivers; *gd; gd++) {
                        if (gd != graphics_drivers)
                                add_to_str(&s, &l, ", ");

			add_to_str(&s, &l, (*gd)->name);
		}
		add_to_str(&s, &l, "\n");
	}
	return s;
}

void shutdown_graphics(void)
{
	if (drv){
		drv->shutdown_driver();
	}
}

void update_driver_param(void)
{
	if (drv){
		struct driver_param *dp = get_driver_param(drv->name);

                dp->codepage = drv->codepage;

                if (dp->param)
                        mem_free(dp->param);

		dp->param = stracpy(drv->get_driver_param());
	}
}

#if defined(GRDRV_SVGALIB)|defined(GRDRV_FB)

struct graphics_driver *virtual_device_driver;
struct graphics_device **virtual_devices;
int n_virtual_devices = 0;
struct graphics_device *current_virtual_device;

int virtual_device_timer;

int init_virtual_devices(struct graphics_driver *drv, int n)
{
	if (n_virtual_devices) {
		internal("init_virtual_devices: already initialized");
		return -1;
	}

        virtual_devices = mem_calloc(n * sizeof(struct graphics_device *));
        if (!virtual_devices)
                return -1;
	n_virtual_devices = n;
	virtual_device_driver = drv;
	virtual_device_timer = -1;
	current_virtual_device = NULL;
	return 0;
}

struct graphics_device *init_virtual_device(void *xxx)
{
	int i;
	for (i = 0; i < n_virtual_devices; i++) if (!virtual_devices[i]) {
		struct graphics_device *dev = mem_calloc(sizeof(struct graphics_device));

                if (!dev)
                        return NULL;

		dev->drv = virtual_device_driver;
		dev->size.x2 = virtual_device_driver->x;
		dev->size.y2 = virtual_device_driver->y;
		current_virtual_device = virtual_devices[i] = dev;
		virtual_device_driver->set_clip_area(dev, &dev->size);
		return dev;
	}
	return NULL;
}

void virtual_device_timer_fn(void *p)
{
	virtual_device_timer = -1;
        if (current_virtual_device &&
            current_virtual_device->redraw_handler) {
		drv->set_clip_area(current_virtual_device, &current_virtual_device->size);
		current_virtual_device->redraw_handler(current_virtual_device, &current_virtual_device->size);
	}
}

void switch_virtual_device(int i)
{
        if (i < 0 ||
            i >= n_virtual_devices ||
            !virtual_devices[i])
                return;

	current_virtual_device = virtual_devices[i];
	if (virtual_device_timer == -1)
		virtual_device_timer = install_timer(0, virtual_device_timer_fn, NULL);
}

void shutdown_virtual_device(struct graphics_device *dev)
{
        int i;

        for (i = 0; i < n_virtual_devices; i++)
                if (virtual_devices[i] == dev) {
                        virtual_devices[i] = NULL;
                        mem_free(dev);

                        if (current_virtual_device != dev)
                                return;

                        for (; i < n_virtual_devices; i++)
                                if (virtual_devices[i]) {
                                        switch_virtual_device(i);
                                        return;
                                }
                        for (i = 0; i < n_virtual_devices; i++)
                                if (virtual_devices[i]) {
                                        switch_virtual_device(i);
                                        return;
                                }
                        current_virtual_device = NULL;
                        return;
                }
        mem_free(dev);
        /*internal("shutdown_virtual_device: device not initialized");*/
}

void shutdown_virtual_devices()
{
	int i;
        if (!n_virtual_devices) {
		internal("shutdown_virtual_devices: already shut down");
		return;
	}
        for (i = 0; i < n_virtual_devices; i++)
                if (virtual_devices[i])
                        internal("shutdown_virtual_devices: virtual device %d is still active", i);

	mem_free(virtual_devices);
	n_virtual_devices = 0;
        if (virtual_device_timer != -1){
                kill_timer(virtual_device_timer);
                virtual_device_timer = -1;
        }
}

#endif

#endif
