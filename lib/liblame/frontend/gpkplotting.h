/*
 *	GTK plotting routines include file
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

#ifndef LAME_GPKPLOTTING_H
#define LAME_GPKPLOTTING_H

#include <gtk/gtk.h>

/* allocate a graphing widget */
GtkWidget *gpk_plot_new(int width, int height);

/* graph a function in the graphing widged */
void    gpk_graph_draw(GtkWidget * widget,
                       int n, gdouble * xcord, gdouble * ycord,
                       gdouble xmn, gdouble ymn, gdouble xmx, gdouble ymx,
                       int clear, char *title, GdkColor * color);

/* draw a rectangle in the graphing widget */
void    gpk_rectangle_draw(GtkWidget * widget, /* plot on this widged */
                           gdouble xcord[2], gdouble ycord[2], /* corners */
                           gdouble xmn, gdouble ymn, /* coordinates of corners */
                           gdouble xmx, gdouble ymx, GdkColor * color); /* color to use */

/* make a bar graph in the graphing widged */
void    gpk_bargraph_draw(GtkWidget * widget,
                          int n, gdouble * xcord, gdouble * ycord,
                          gdouble xmn, gdouble ymn, gdouble xmx, gdouble ymx,
                          int clear, char *title, int bwidth, GdkColor * color);

/* set forground color  */
void    setcolor(GtkWidget * widget, GdkColor * color, int red, int green, int blue);

#endif
