/*
 *	GTK plotting routines source file
 *
 *	Copyright (c) 1999 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: gpkplotting.c,v 1.11 2007/07/24 17:46:08 bouvigne Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gpkplotting.h"

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static gint num_plotwindows = 0;
static gint max_plotwindows = 10;
static GdkPixmap *pixmaps[10];
static GtkWidget *pixmapboxes[10];




/* compute a gdkcolor */
void
setcolor(GtkWidget * widget, GdkColor * color, gint red, gint green, gint blue)
{

    /* colors in GdkColor are taken from 0 to 65535, not 0 to 255.    */
    color->red = red * (65535 / 255);
    color->green = green * (65535 / 255);
    color->blue = blue * (65535 / 255);
    color->pixel = (gulong) (color->red * 65536 + color->green * 256 + color->blue);
    /* find closest in colormap, if needed */
    gdk_color_alloc(gtk_widget_get_colormap(widget), color);
}


void
gpk_redraw(GdkPixmap * pixmap, GtkWidget * pixmapbox)
{
    /* redraw the entire pixmap */
    gdk_draw_pixmap(pixmapbox->window,
                    pixmapbox->style->fg_gc[GTK_WIDGET_STATE(pixmapbox)],
                    pixmap, 0, 0, 0, 0, pixmapbox->allocation.width, pixmapbox->allocation.height);
}


static GdkPixmap **
findpixmap(GtkWidget * widget)
{
    int     i;
    for (i = 0; i < num_plotwindows && widget != pixmapboxes[i]; i++);
    if (i >= num_plotwindows) {
        g_print("findpixmap(): bad argument widget \n");
        return NULL;
    }
    return &pixmaps[i];
}

void
gpk_graph_draw(GtkWidget * widget, /* plot on this widged */
               int n,        /* number of data points */
               gdouble * xcord, gdouble * ycord, /* data */
               gdouble xmn, gdouble ymn, /* coordinates of corners */
               gdouble xmx, gdouble ymx, int clear, /* clear old plot first */
               char *title,  /* add a title (only if clear=1) */
               GdkColor * color)
{
    GdkPixmap **ppixmap;
    GdkPoint *points;
    int     i;
    gint16  width, height;
    GdkFont *fixed_font;
    GdkGC  *gc;

    gc = gdk_gc_new(widget->window);
    gdk_gc_set_foreground(gc, color);



    if ((ppixmap = findpixmap(widget))) {
        width = widget->allocation.width;
        height = widget->allocation.height;


        if (clear) {
            /* white background */
            gdk_draw_rectangle(*ppixmap, widget->style->white_gc, TRUE, 0, 0, width, height);
            /* title */
#ifdef _WIN32
            fixed_font = gdk_font_load("-misc-fixed-large-r-*-*-*-100-*-*-*-*-*-*");
#else
            fixed_font = gdk_font_load("-misc-fixed-medium-r-*-*-*-100-*-*-*-*-iso8859-1");
#endif

            gdk_draw_text(*ppixmap, fixed_font,
                          widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                          0, 10, title, strlen(title));
        }


        points = g_malloc(n * sizeof(GdkPoint));
        for (i = 0; i < n; i++) {
            points[i].x = .5 + ((xcord[i] - xmn) * (width - 1) / (xmx - xmn));
            points[i].y = .5 + ((ycord[i] - ymx) * (height - 1) / (ymn - ymx));
        }
        gdk_draw_lines(*ppixmap, gc, points, n);
        g_free(points);
        gpk_redraw(*ppixmap, widget);
    }
    gdk_gc_destroy(gc);
}



void
gpk_rectangle_draw(GtkWidget * widget, /* plot on this widged */
                   gdouble * xcord, gdouble * ycord, /* corners */
                   gdouble xmn, gdouble ymn, /* coordinates of corners */
                   gdouble xmx, gdouble ymx, GdkColor * color)
{
    GdkPixmap **ppixmap;
    GdkPoint points[2];
    int     i;
    gint16  width, height;
    GdkGC  *gc;


    gc = gdk_gc_new(widget->window);
    gdk_gc_set_foreground(gc, color);


    if ((ppixmap = findpixmap(widget))) {
        width = widget->allocation.width;
        height = widget->allocation.height;


        for (i = 0; i < 2; i++) {
            points[i].x = .5 + ((xcord[i] - xmn) * (width - 1) / (xmx - xmn));
            points[i].y = .5 + ((ycord[i] - ymx) * (height - 1) / (ymn - ymx));
        }
        width = points[1].x - points[0].x + 1;
        height = points[1].y - points[0].y + 1;
        gdk_draw_rectangle(*ppixmap, gc, TRUE, points[0].x, points[0].y, width, height);
        gpk_redraw(*ppixmap, widget);
    }
    gdk_gc_destroy(gc);
}



void
gpk_bargraph_draw(GtkWidget * widget, /* plot on this widged */
                  int n,     /* number of data points */
                  gdouble * xcord, gdouble * ycord, /* data */
                  gdouble xmn, gdouble ymn, /* coordinates of corners */
                  gdouble xmx, gdouble ymx, int clear, /* clear old plot first */
                  char *title, /* add a title (only if clear=1) */
                  int barwidth, /* bar width. 0=compute based on window size */
                  GdkColor * color)
{
    GdkPixmap **ppixmap;
    GdkPoint points[2];
    int     i;
    gint16  width, height, x, y, barheight;
    GdkFont *fixed_font;
    GdkGC  *gc;
    int     titleSplit;


    gc = gdk_gc_new(widget->window);
    gdk_gc_set_foreground(gc, color);


    if ((ppixmap = findpixmap(widget))) {
        width = widget->allocation.width;
        height = widget->allocation.height;


        if (clear) {
            /* white background */
            gdk_draw_rectangle(*ppixmap, widget->style->white_gc, TRUE, 0, 0, width, height);
            /* title */
#ifdef _WIN32
            fixed_font = gdk_font_load("-misc-fixed-large-r-*-*-*-100-*-*-*-*-*-*");
#else
            fixed_font = gdk_font_load("-misc-fixed-medium-r-*-*-*-100-*-*-*-*-iso8859-1");
#endif

            titleSplit = strcspn(title, "\n");

            if (titleSplit && (titleSplit != strlen(title))) {
                gdk_draw_text(*ppixmap, fixed_font,
                              widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                              0, 10, title, titleSplit);

                gdk_draw_text(*ppixmap, fixed_font,
                              widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                              0, 22, title + titleSplit + 1, (strlen(title) - titleSplit) - 1);


            }
            else {
                gdk_draw_text(*ppixmap, fixed_font,
                              widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                              0, 10, title, strlen(title));
            }
        }


        for (i = 0; i < n; i++) {
            points[1].x = .5 + ((xcord[i] - xmn) * (width - 1) / (xmx - xmn));
            points[1].y = .5 + ((ycord[i] - ymx) * (height - 1) / (ymn - ymx));
            points[0].x = points[1].x;
            points[0].y = height - 1;

            x = .5 + ((xcord[i] - xmn) * (width - 1) / (xmx - xmn));
            y = .5 + ((ycord[i] - ymx) * (height - 1) / (ymn - ymx));
            if (!barwidth)
                barwidth = (width / (n + 1)) - 1;
            barwidth = barwidth > 5 ? 5 : barwidth;
            barwidth = barwidth < 1 ? 1 : barwidth;
            barheight = height - 1 - y;
            /* gdk_draw_lines(*ppixmap,gc,points,2); */
            gdk_draw_rectangle(*ppixmap, gc, TRUE, x, y, barwidth, barheight);

        }
        gpk_redraw(*ppixmap, widget);
    }
    gdk_gc_destroy(gc);
}





/* Create a new backing pixmap of the appropriate size */
static  gint
configure_event(GtkWidget * widget, GdkEventConfigure * event, gpointer data)
{
    GdkPixmap **ppixmap;
    if ((ppixmap = findpixmap(widget))) {
        if (*ppixmap)
            gdk_pixmap_unref(*ppixmap);
        *ppixmap = gdk_pixmap_new(widget->window,
                                  widget->allocation.width, widget->allocation.height, -1);
        gdk_draw_rectangle(*ppixmap,
                           widget->style->white_gc,
                           TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    }
    return TRUE;
}



/* Redraw the screen from the backing pixmap */
static  gint
expose_event(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
    GdkPixmap **ppixmap;
    if ((ppixmap = findpixmap(widget))) {
        gdk_draw_pixmap(widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        *ppixmap,
                        event->area.x, event->area.y,
                        event->area.x, event->area.y, event->area.width, event->area.height);
    }

    return FALSE;
}





GtkWidget *
gpk_plot_new(int width, int height)
{
    GtkWidget *pixmapbox;

    pixmapbox = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(pixmapbox), width, height);
    gtk_signal_connect(GTK_OBJECT(pixmapbox), "expose_event", (GtkSignalFunc) expose_event, NULL);
    gtk_signal_connect(GTK_OBJECT(pixmapbox), "configure_event",
                       (GtkSignalFunc) configure_event, NULL);
    gtk_widget_set_events(pixmapbox, GDK_EXPOSURE_MASK);

    if (num_plotwindows < max_plotwindows) {
        pixmapboxes[num_plotwindows] = pixmapbox;
        pixmaps[num_plotwindows] = NULL;
        num_plotwindows++;
    }
    else {
        g_print("gtk_plotarea_new(): exceeded maximum of 10 plotarea windows\n");
    }

    return pixmapbox;
}
