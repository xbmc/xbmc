/* directfb.c
 * DirectFB graphics driver
 * (c) 2002 Sven Neumann <sven@directfb.org>
 *
 * This file is a part of the Links program, released under GPL.
 */

/* TODO:
 * - Store window size as driver params (?)
 * - Fix wrong colors on big-endian systems (fixed?)
 * - Make everything work correctly ;-)
 *
 * KNOWN PROBLEMS:
 * - If mouse drags don't work for you, update DirectFB
 *   (the upcoming 0.9.14 release fixes this).
 */


#include "cfg.h"

#ifdef GRDRV_DIRECTFB

#ifdef TEXT
#undef TEXT
#endif

#include <netinet/in.h>  /* for htons */

#include <directfb.h>

#include "links.h"
#include "directfb_cursors.h"


#define FOCUSED_OPACITY    0xFF
#define UNFOCUSED_OPACITY  0xC0

#define DIRECTFB_HASH_TABLE_SIZE  23
struct graphics_device **directfb_hash_table[DIRECTFB_HASH_TABLE_SIZE];

typedef struct _DFBDeviceData DFBDeviceData;
struct _DFBDeviceData
{
  DFBWindowID       id;
  IDirectFBWindow  *window;
  IDirectFBSurface *surface;
  int               mapped;
  DFBRegion         flip_region;
  int               flip_pending;
};


extern struct graphics_driver directfb_driver;

static IDirectFB             *dfb         = NULL;
static IDirectFBDisplayLayer *layer       = NULL;
static IDirectFBEventBuffer  *events      = NULL;
static DFBSurfacePixelFormat  pixelformat = DSPF_UNKNOWN;
static int                    event_timer = 0;


static inline void directfb_set_color  (IDirectFBSurface *surface, long color);
static void directfb_register_flip     (DFBDeviceData *data,
                                        int x, int y, int w, int h);
static void directfb_flip_surface      (void *pointer);
static void directfb_check_events      (void *pointer);
static void directfb_translate_key     (DFBWindowEvent *event,
                                        int *key, int *flag);
static void directfb_add_to_table      (struct graphics_device *gd);
static void directfb_remove_from_table (struct graphics_device *gd);
static struct graphics_device * directfb_lookup_in_table (DFBWindowID  id);


static unsigned char *
directfb_fb_init_driver (unsigned char *param, unsigned char *display)
{
  DFBDisplayLayerConfig  config;
  IDirectFBSurface      *arrow;
  DFBResult              ret;

  if (dfb)
    return NULL;

  DirectFBInit (&g_argc, (char ***) &g_argv);
  ret = DirectFBCreate (&dfb);

  if (ret)
    {
      char message[128];

      snprintf (message, sizeof (message), "%s\n", DirectFBErrorString (ret));
      return stracpy (message);
    }

  dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer);

  layer->GetConfiguration (layer, &config);

  pixelformat = config.pixelformat;

  directfb_driver.depth = (((DFB_BYTES_PER_PIXEL (pixelformat) & 0x7)) |
                           ((DFB_BITS_PER_PIXEL  (pixelformat) & 0x1F) << 3));

  /* endian test */
  if (htons (0x1234) == 0x1234)
    directfb_driver.depth |= 0x100;

  directfb_driver.x = config.width;
  directfb_driver.y = config.height;

  memset (directfb_hash_table, 0, sizeof (directfb_hash_table));

  if (dfb->CreateSurface (dfb, &arrow_desc, &arrow) == DFB_OK)
    {
      layer->SetCursorOpacity (layer, 0xE0);
      layer->SetCursorShape (layer, arrow, arrow_hot_x, arrow_hot_y);
      arrow->Release (arrow);
    }

  return NULL;
}

static struct graphics_device *
directfb_init_device (void)
{
  struct graphics_device *gd;
  DFBDeviceData          *data;
  IDirectFBWindow        *window;
  DFBWindowDescription    desc;

  if (!dfb || !layer)
    return NULL;

  desc.flags  = DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_POSX | DWDESC_POSY;
  desc.width  = directfb_driver.x;
  desc.height = directfb_driver.y;
  desc.posx   = 0;
  desc.posy   = 0;

  if (layer->CreateWindow (layer, &desc, &window) != DFB_OK)
    return NULL;

  gd = mem_alloc (sizeof (struct graphics_device));

  gd->size.x1 = 0;
  gd->size.y1 = 0;
  window->GetSize (window, &gd->size.x2, &gd->size.y2);

  gd->clip = gd->size;

  data = mem_alloc (sizeof (DFBDeviceData));
  
  data->window       = window;
  data->mapped       = 0;
  data->flip_pending = 0;

  window->GetSurface (window, &data->surface);
  window->GetID (window, &data->id);

  gd->drv = &directfb_driver;
  gd->driver_data = data;
  gd->user_data   = NULL;

  directfb_add_to_table (gd);

  if (!events)
    {
      window->CreateEventBuffer (window, &events);
      event_timer = install_timer (20, directfb_check_events, events);
    }
  else
    {
      window->AttachEventBuffer (window, events);
    }

  return gd;
}

static void
directfb_shutdown_device (struct graphics_device *gd)
{
  DFBDeviceData *data;

  if (!gd)
    return;

  data = gd->driver_data;

  unregister_bottom_half (directfb_flip_surface, data);
  directfb_remove_from_table (gd);

  data->surface->Release (data->surface);
  data->window->Release (data->window);

  mem_free (data);
  mem_free (gd);
}

static void
directfb_shutdown_driver (void)
{
  int i;

  if (!dfb)
    return;

  if (events)
    {
      kill_timer (event_timer);
      events->Release (events);
      events = NULL;
    }

  layer->Release (layer);
  dfb->Release (dfb);

  for (i = 0; i < DIRECTFB_HASH_TABLE_SIZE; i++)
    if (directfb_hash_table[i])
      mem_free (directfb_hash_table[i]);

  dfb = NULL;
}

static unsigned char *
directfb_get_driver_param (void)
{
  return NULL;
}

static int
directfb_get_empty_bitmap (struct bitmap *bmp)
{
  IDirectFBSurface      *surface;
  DFBSurfaceDescription  desc;

  bmp->data = bmp->flags = NULL;

  desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT;
  desc.width  = bmp->x;
  desc.height = bmp->y;

  if (dfb->CreateSurface (dfb, &desc, &surface) != DFB_OK)
    return 0;

  surface->Lock (surface, DSLF_READ | DSLF_WRITE, &bmp->data, &bmp->skip);

  bmp->flags = surface;

  return 0;
}

static int
directfb_get_filled_bitmap (struct bitmap *bmp, long color)
{
  IDirectFBSurface      *surface;
  DFBSurfaceDescription  desc;

  bmp->data = bmp->flags = NULL;

  desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT;
  desc.width  = bmp->x;
  desc.height = bmp->y;

  if (dfb->CreateSurface (dfb, &desc, &surface) != DFB_OK)
    return 0;

  directfb_set_color (surface, color);
  surface->FillRectangle (surface, 0, 0, bmp->x, bmp->y);
  surface->Lock (surface, DSLF_READ | DSLF_WRITE, &bmp->data, &bmp->skip);

  bmp->flags = surface;

  return 0;
}

static void
directfb_register_bitmap (struct bitmap *bmp)
{
  IDirectFBSurface *surface = bmp->flags;

  surface->Unlock (surface);
  bmp->data = NULL;
}

static void *
directfb_prepare_strip (struct bitmap *bmp, int top, int lines)
{
  IDirectFBSurface *surface = bmp->flags;

  surface->Lock (surface, DSLF_READ | DSLF_WRITE, &bmp->data, &bmp->skip);

  return ((unsigned char *) bmp->data + top * bmp->skip);
}

static void
directfb_commit_strip (struct bitmap *bmp, int top, int lines)
{
  IDirectFBSurface *surface = bmp->flags;

  surface->Unlock (surface);
  bmp->data = NULL;
}

static void
directfb_unregister_bitmap (struct bitmap *bmp)
{
  IDirectFBSurface *surface = bmp->flags;

  surface->Release (surface);
}

static void
directfb_draw_bitmap (struct graphics_device *gd, struct bitmap *bmp,
                      int x, int y)
{
  DFBDeviceData    *data = gd->driver_data;
  IDirectFBSurface *src  = bmp->flags;

  data->surface->Blit (data->surface, src, NULL, x, y);

  directfb_register_flip (data, x, y, bmp->x, bmp->y);
}

static void
directfb_draw_bitmaps (struct graphics_device *gd, struct bitmap **bmps,
                       int n, int x, int y)
{
  DFBDeviceData *data = gd->driver_data;
  struct bitmap *bmp  = *bmps;
  int x1 = x;
  int h  = 0;

  if (n < 1)
    return;

  do
    {
      IDirectFBSurface *src = bmp->flags;
      
      data->surface->Blit (data->surface, src, NULL, x, y);

      if (h < bmp->y)
        h = bmp->y;

      x += bmp->x;
      bmp++;
    }
  while (--n);

  directfb_register_flip (data, x1, y, x - x1, h);
}

static long
directfb_get_color (int rgb)
{
  return rgb;
}


static void
directfb_fill_area (struct graphics_device *gd,
                    int x1, int y1, int x2, int y2, long color)
{
  DFBDeviceData *data = gd->driver_data;
  int w = x2 - x1;
  int h = y2 - y1;

  directfb_set_color (data->surface, color);
  data->surface->FillRectangle (data->surface, x1, y1, w, h);

  directfb_register_flip (data, x1, y1, w, h);
}

static void
directfb_draw_hline (struct graphics_device *gd,
                     int left, int y, int right, long color)
{
  DFBDeviceData *data = gd->driver_data;

  right--;

  directfb_set_color (data->surface, color);
  data->surface->DrawLine (data->surface, left, y, right, y);

  directfb_register_flip (data, left, y, right - left, 1);
}

static void
directfb_draw_vline (struct graphics_device *gd,
                     int x, int top, int bottom, long color)
{
  DFBDeviceData *data = gd->driver_data;
  
  bottom--;

  directfb_set_color (data->surface, color);
  data->surface->DrawLine (data->surface, x, top, x, bottom);

  directfb_register_flip (data, x, top, 1, bottom - top);
}

static void
directfb_set_clip_area (struct graphics_device *gd, struct rect *r)
{
  DFBDeviceData *data   = gd->driver_data;
  DFBRegion      region = { r->x1, r->y1, r->x2 - 1, r->y2 - 1};

  gd->clip = *r;
  
  data->surface->SetClip (data->surface, &region);
}

static int
directfb_hscroll (struct graphics_device *gd, struct rect_set **set, int sc)
{
  DFBDeviceData *data = gd->driver_data;
  DFBRectangle   rect;

  *set = NULL;
  if (!sc)
    return 0;

  rect.x = gd->clip.x1;
  rect.y = gd->clip.y1;
  rect.w = gd->clip.x2 - rect.x;
  rect.h = gd->clip.y2 - rect.y;

  data->surface->Blit (data->surface,
                       data->surface, &rect, rect.x + sc, rect.y);

  directfb_register_flip (data, rect.x, rect.y, rect.w, rect.h);

  return 1;
}

static int
directfb_vscroll (struct graphics_device *gd, struct rect_set **set, int sc)
{
  DFBDeviceData *data = gd->driver_data;
  DFBRectangle   rect;

  *set = NULL;
  if (!sc)
    return 0;

  rect.x = gd->clip.x1;
  rect.y = gd->clip.y1;
  rect.w = gd->clip.x2 - rect.x;
  rect.h = gd->clip.y2 - rect.y;

  data->surface->Blit (data->surface,
                       data->surface, &rect, rect.x, rect.y + sc);

  directfb_register_flip (data, rect.x, rect.y, rect.w, rect.h);

  return 1;
}

void directfb_put_to_clipboard(struct graphics_device *gd, char *string,int length)
{
}

char *directfb_get_from_clipboard(struct graphics_device *gd)
{
        return NULL;
}

void directfb_request_clipboard(struct grphics_device *gd)
{
}



struct graphics_driver directfb_driver =
{
  "DirectFB",
  directfb_fb_init_driver,
  directfb_init_device,
  directfb_shutdown_device,
  directfb_shutdown_driver,
  directfb_get_driver_param,
  directfb_get_empty_bitmap,
  directfb_get_filled_bitmap,
  directfb_register_bitmap,
  directfb_prepare_strip,
  directfb_commit_strip,
  directfb_unregister_bitmap,
  directfb_draw_bitmap,
  directfb_draw_bitmaps,
  directfb_get_color,
  directfb_fill_area,
  directfb_draw_hline,
  directfb_draw_vline,
  directfb_hscroll,
  directfb_vscroll,
  directfb_set_clip_area,
  dummy_block,
  dummy_unblock,
  NULL,	 /*  set_title  */
  directfb_put_to_clipboard,
  directfb_request_clipboard,
  directfb_get_from_clipboard,
  0,	 /*  depth      */
  0, 0,	 /*  size       */
  0,     /*  flags      */
  0      /*  codepage   */
};


static inline void directfb_set_color (IDirectFBSurface *surface, long color)
{
  surface->SetColor (surface,
                     (color & 0xFF0000) >> 16,
                     (color & 0xFF00)   >> 8,
                     (color & 0xFF),
                     0xFF);
}

static void directfb_register_flip (DFBDeviceData *data,
                                    int x, int y, int w, int h)
{
  if (x < 0 || y < 0 || w < 1 || h < 1)
    return;

  w = x + w - 1;
  h = y + h - 1;

  if (data->flip_pending)
    {
      if (data->flip_region.x1 > x)  data->flip_region.x1 = x;
      if (data->flip_region.y1 > y)  data->flip_region.y1 = y;
      if (data->flip_region.x2 < w)  data->flip_region.x2 = w;
      if (data->flip_region.y2 < h)  data->flip_region.y2 = h;
    }
  else
    {
      data->flip_region.x1 = x;
      data->flip_region.y1 = y;
      data->flip_region.x2 = w;
      data->flip_region.y2 = h;
 
      data->flip_pending = 1;

      register_bottom_half (directfb_flip_surface, data);
    }
}

static void
directfb_flip_surface (void *pointer)
{
  DFBDeviceData *data = pointer;

  if (!data->flip_pending)
    return;

  data->surface->Flip (data->surface, &data->flip_region, 0);

  data->flip_pending = 0;
}

static void
directfb_check_events (void *pointer)
{
  struct graphics_device *gd   = NULL;
  DFBDeviceData          *data = NULL;
  DFBWindowEvent          event;
  DFBWindowEvent          next;

  if (!events)
    return;

  while (events->GetEvent (events, DFB_EVENT (&event)) == DFB_OK)
    {
      switch (event.type)
        {
        case DWET_GOTFOCUS:
        case DWET_LOSTFOCUS:
        case DWET_POSITION_SIZE:
        case DWET_SIZE:
        case DWET_KEYDOWN:
        case DWET_BUTTONDOWN:
        case DWET_BUTTONUP:
        case DWET_WHEEL:
        case DWET_MOTION:
          break;
        default:
          continue;
        }

      if (!data || data->id != event.window_id)
        {
          gd = directfb_lookup_in_table (event.window_id);
          if (!gd)
            continue;
        }

      data = gd->driver_data;

      switch (event.type)
        {
        case DWET_GOTFOCUS:
          data->window->SetOpacity (data->window, FOCUSED_OPACITY);
          break;

        case DWET_LOSTFOCUS:
          data->window->SetOpacity (data->window, UNFOCUSED_OPACITY);
          break;

        case DWET_POSITION_SIZE:
          if (!data->mapped)
            {
              struct rect r = { 0, 0, event.w, event.h };
              
              gd->redraw_handler (gd, &r);
              data->window->SetOpacity (data->window, FOCUSED_OPACITY);
              data->mapped = 1;
            }
          else
          /* fallthru */
        case DWET_SIZE:
          while ((events->PeekEvent (events, DFB_EVENT (&next)) == DFB_OK)   &&
                 (next.type == DWET_SIZE || next.type == DWET_POSITION_SIZE) &&
                 (next.window_id == data->id))
            events->GetEvent (events, DFB_EVENT (&event));

          gd->size.x2 = event.w;
          gd->size.y2 = event.h;
          gd->resize_handler (gd);
          break;

        case DWET_KEYDOWN:
          {
            int key, flag;

            directfb_translate_key (&event, &key, &flag);
            if (key)
              gd->keyboard_handler (gd, key, flag);
          }
          break;

        case DWET_BUTTONDOWN:
        case DWET_BUTTONUP:
          {
            int flags;

            if (event.type == DWET_BUTTONUP)
              {
                flags = B_UP;
                data->window->UngrabPointer (data->window);
              }
            else
              {
                flags = B_DOWN;
                data->window->GrabPointer (data->window);
              }

            switch (event.button)
              {
              case DIBI_LEFT:
                flags |= B_LEFT;
                break;
              case DIBI_RIGHT:
                flags |= B_RIGHT;
                break;
              case DIBI_MIDDLE:
                flags |= B_MIDDLE;
                break;
              default:
                continue;
              }

            gd->mouse_handler (gd, event.x, event.y, flags);
          }
          break;

        case DWET_WHEEL:
          gd->mouse_handler (gd, event.x, event.y,
                             B_MOVE |
                             (event.step > 0 ? B_WHEELUP : B_WHEELDOWN));

        case DWET_MOTION:
          {
            int flags;

            while ((events->PeekEvent (events, DFB_EVENT (&next)) == DFB_OK) &&
                   (next.type      == DWET_MOTION)                           &&
                   (next.window_id == data->id))
              events->GetEvent (events, DFB_EVENT (&event));

            switch (event.buttons)
              {
              case DIBM_LEFT:
                flags = B_DRAG | B_LEFT;
                break;
              case DIBM_RIGHT:
                flags = B_DRAG | B_RIGHT;
                break;
              case DIBM_MIDDLE:
                flags = B_DRAG | B_MIDDLE;
                break;
              default:
                flags = B_MOVE;
                break;
              }

            gd->mouse_handler (gd, event.x, event.y, flags);
          }
          break;

        case DWET_CLOSE:
          gd->keyboard_handler (gd, KBD_CLOSE, 0);
          break;

        default:
          break;
        }
    }

  event_timer = install_timer (20, directfb_check_events, events);
}

static void
directfb_translate_key (DFBWindowEvent *event, int *key, int *flag)
{
  *key  = 0;
  *flag = 0;

  if (event->modifiers & DIMM_CONTROL && event->key_id == DIKI_C)
    {
      *key = KBD_CTRL_C;
      return;
    }

  /* setting Shift seems to break things
   *
   *  if (event->modifiers & DIMM_SHIFT)
   *     *flag |= KBD_SHIFT;
   */
  if (event->modifiers & DIMM_CONTROL)
    *flag |= KBD_CTRL;
  if (event->modifiers & DIMM_ALT)
    *flag |= KBD_ALT;

  switch (event->key_symbol)
    {
    case DIKS_ENTER:        *key = KBD_ENTER;     break;
    case DIKS_BACKSPACE:    *key = KBD_BS;        break;
    case DIKS_TAB:          *key = KBD_TAB;       break;
    case DIKS_ESCAPE:       *key = KBD_ESC;       break;
    case DIKS_CURSOR_UP:    *key = KBD_UP;        break;
    case DIKS_CURSOR_DOWN:  *key = KBD_DOWN;      break;
    case DIKS_CURSOR_LEFT:  *key = KBD_LEFT;      break;
    case DIKS_CURSOR_RIGHT: *key = KBD_RIGHT;     break;
    case DIKS_INSERT:       *key = KBD_INS;       break;
    case DIKS_DELETE:       *key = KBD_DEL;       break;
    case DIKS_HOME:         *key = KBD_HOME;      break;
    case DIKS_END:          *key = KBD_END;       break;
    case DIKS_PAGE_UP:      *key = KBD_PAGE_UP;   break;
    case DIKS_PAGE_DOWN:    *key = KBD_PAGE_DOWN; break;
    case DIKS_F1:           *key = KBD_F1;        break;
    case DIKS_F2:           *key = KBD_F2;        break;
    case DIKS_F3:           *key = KBD_F3;        break;
    case DIKS_F4:           *key = KBD_F4;        break;
    case DIKS_F5:           *key = KBD_F5;        break;
    case DIKS_F6:           *key = KBD_F6;        break;
    case DIKS_F7:           *key = KBD_F7;        break;
    case DIKS_F8:           *key = KBD_F8;        break;
    case DIKS_F9:           *key = KBD_F9;        break;
    case DIKS_F10:          *key = KBD_F10;       break;
    case DIKS_F11:          *key = KBD_F11;       break;
    case DIKS_F12:          *key = KBD_F12;       break;

    default:
      if (DFB_KEY_TYPE (event->key_symbol) == DIKT_UNICODE)
        *key = event->key_symbol;
      break;
    }
}

static void
directfb_add_to_table (struct graphics_device *gd)
{
  DFBDeviceData           *data = gd->driver_data;
  struct graphics_device **devices;
  int i;

  i = data->id % DIRECTFB_HASH_TABLE_SIZE;
  
  devices = directfb_hash_table[i];

  if (devices)
    {
      int c = 0;

      while (devices[c++])
        ;

      devices = mem_realloc (devices, (c + 1) * sizeof (void *));
      devices[c-1] = gd;
      devices[c]   = NULL;
    }
  else
    {
      devices = mem_alloc (2 * sizeof (void *));
      devices[0] = gd;
      devices[1] = NULL;
    }

  directfb_hash_table[i] = devices;
}

static void
directfb_remove_from_table (struct graphics_device *gd)
{
  DFBDeviceData           *data = gd->driver_data;
  struct graphics_device **devices;
  int i, j, c;

  i = data->id % DIRECTFB_HASH_TABLE_SIZE;

  devices = directfb_hash_table[i];
  if (!devices)
    return;

  for (j = 0, c = -1; devices[j]; j++)
    if (devices[j] == gd)
      c = j;

  if (c < 0)
    return;

  memmove (devices + c, devices + c + 1, (j - c) * sizeof (void *));
  devices = mem_realloc (devices, j * sizeof (void *));

  directfb_hash_table[i] = devices;
}

static struct graphics_device *
directfb_lookup_in_table (DFBWindowID id)
{
  struct graphics_device **devices;
  int i;

  i = id % DIRECTFB_HASH_TABLE_SIZE;

  devices = directfb_hash_table[i];
  if (!devices)
    return NULL;

  while (*devices)
    {
      DFBDeviceData *data = (*devices)->driver_data;

      if (data->id == id)
        return *devices;

      devices++;
    }

  return NULL;
}

#endif /* GRDRV_DIRECTFB */
