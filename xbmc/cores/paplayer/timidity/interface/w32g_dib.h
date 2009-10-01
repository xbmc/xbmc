#ifndef __W32G_DIB_H__
#define __W32G_DIB_H__

#define W32G_DIB_PALS_MAX 16
#define W32G_DIB_MODIFIED_RECT_MAX 16

typedef struct w32g_dib_t_ {
	HBITMAP hbmp;
	int width;
	int height;
	char *bits;
	RGBQUAD pals[W32G_DIB_PALS_MAX];
	int pals_max;
	RECT modified_rect[W32G_DIB_MODIFIED_RECT_MAX];
	int modified_rect_max;
} w32g_dib_t;

w32g_dib_t *dib_create (int width, int height );
void dib_free ( w32g_dib_t *dib );
void dib_add_modified_rect ( w32g_dib_t *dib, RECT *lprc );
void dib_modified_rect_whole ( w32g_dib_t *dib );
void dib_set_pal1 ( w32g_dib_t *dib, int pal_index, RGBQUAD rq );
void dib_set_pals ( w32g_dib_t *dib, RGBQUAD *pals, int pals_max );
void dib_apply ( w32g_dib_t *dib, HDC hdc );

#endif // __W32G_DIB_H__
