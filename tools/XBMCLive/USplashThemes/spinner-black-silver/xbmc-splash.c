#include <usplash-theme.h>
#include <usplash_backend.h>

/**********************************************************************************
 This is a USplash based theme for the awsome XBMC Media Center for linux.
 Graphics were composed mainly of the official XBMC logos.
 
 Basically, this is the work of DuDuke (watch below).
 I just did some graphical variations.
 
 Any questions:
 XBMC Forumname: Beatzeps08
 
 Visit DuDuke's blog at:
 http://du-duke.blogspot.com/ 
 for some more usplash, xbmc stuff.

 some code snippets were taken from:
 http://gnome-look.org/content/show.php/MacX+Usplash+Theme?content=73611
 
 ===================================================================================
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ==================================================================================
 
**********************************************************************************/

extern struct usplash_pixmap pixmap_xbmc_1920_1200;
extern struct usplash_pixmap pixmap_xbmc_1920_1080;
extern struct usplash_pixmap pixmap_xbmc_1680_1050;
extern struct usplash_pixmap pixmap_xbmc_1440_900;
extern struct usplash_pixmap pixmap_xbmc_1280_1024;
extern struct usplash_pixmap pixmap_xbmc_1366_768;
extern struct usplash_pixmap pixmap_xbmc_1280_720;
extern struct usplash_pixmap pixmap_xbmc_1024_768;
extern struct usplash_pixmap pixmap_xbmc_800_600;
extern struct usplash_pixmap pixmap_xbmc_640_480;

extern struct usplash_pixmap pixmap_xbmc_spinner;
extern struct usplash_font font_helvB10;

void t_init(struct usplash_theme* theme);
void t_clear_progressbar(struct usplash_theme* theme);
void t_draw_progressbar(struct usplash_theme* theme, int percentage);
void t_animate_step(struct usplash_theme* theme, int pulsating);
void spinner(struct usplash_theme* theme);

struct usplash_theme usplash_theme;
struct usplash_theme usplash_theme_1920_1080;
struct usplash_theme usplash_theme_1680_1050;
struct usplash_theme usplash_theme_1440_900;
struct usplash_theme usplash_theme_1280_1024;
struct usplash_theme usplash_theme_1366_768;
struct usplash_theme usplash_theme_1280_720;
struct usplash_theme usplash_theme_1024_768;
struct usplash_theme usplash_theme_800_600;
struct usplash_theme usplash_theme_640_480;

static int spinner_x, spinner_y, spinner_part_width, spinner_height;
static int current_count = 0;
static int current_step = 0;
static int spinner_num_steps = 12;

// spinner_speed can be between 1 and 25
// there are 12 images in the spinner, so a value of 2 will make
// it spin around approx. once per second
static int spinner_speed = 2;

/** ----------------------------------------------------------------------- **/

struct usplash_theme usplash_theme = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1920_1080,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1920_1200,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1920_1080 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1680_1050,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1920_1080,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1680_1050 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1440_900,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1680_1050,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1440_900 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1280_1024,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1440_900,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1280_1024 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1366_768,
	.ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1280_1024,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1366_768 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1280_720,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1366_768,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1280_720 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_1024_768,
	.ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1280_720,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_1024_768 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_800_600,
	.ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_xbmc_1024_768,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_800_600 = {
	.version = THEME_VERSION,
	.next = &usplash_theme_640_480,
	.ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_xbmc_800_600,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,
	
	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

struct usplash_theme usplash_theme_640_480 = {
	.version = THEME_VERSION,
	.next = NULL,
	.ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_xbmc_640_480,
	.font   = &font_helvB10,

	/* Palette indexes */
	.background             = 0x01,
	.progressbar_background = 0x00,
	.progressbar_foreground = 0x1E,

	/* Functions */
	.init = t_init,
	.clear_progressbar = t_clear_progressbar,
	.draw_progressbar = t_draw_progressbar,
	.animate_step = t_animate_step,
};

/** ---------------------------------------------------------- **/

/* init usplash */
void t_init(struct usplash_theme *theme) {
	// determine spinner position and dimensions
	spinner_height = pixmap_xbmc_spinner.height;
	spinner_part_width = pixmap_xbmc_spinner.width / spinner_num_steps;
	spinner_x = (theme->pixmap->width / 2) - (spinner_part_width / 2);
	spinner_y = (theme->pixmap->height / 2) + (theme->pixmap->height / 8) - (pixmap_xbmc_spinner.height / 2);
	
	// set text box dimensions and size
	theme->text_width = 450;
	//theme->text_x = (theme->pixmap->width / 2) - (theme->text_width / 2);
	theme->text_x = 20;
	theme->text_y = (theme->pixmap->height * 0.60);
	theme->text_height = 60;
	
	// set theme color indexes
	theme->background             = 0;
	theme->progressbar_background = 0;
	theme->progressbar_foreground = 243;
	theme->text_background        = 0;
	theme->text_foreground        = 75;
	theme->text_success           = 65;
	theme->text_failure           = 80;
}

/******
 * Animation callback - called 25 times per second by Usplash 
 * 
 * Param:	struct usplash_theme* theme	- theme being used 
 * 			int pulsating - boolean int
 */
void t_animate_step(struct usplash_theme* theme, int pulsating) {
	current_count = current_count + 1;
	
	// increase test int for slower spinning
	if(current_count == spinner_speed) {
		spinner(theme);
		current_count = 0;	
	}
}

/********
 * Animate the spinner
 *  helper function to aid in animation of spinner
 */
void spinner(struct usplash_theme* theme) {
	current_step = current_step + 1;
	
	int x0 = (spinner_part_width * current_step) - spinner_part_width;
	int y0 = 0;
	
	// if current step > number of images in the spinner, then reset to beginning (at end or circular spinner)
	if(current_step >= spinner_num_steps) {
		current_step = 0;
	}
	
	// call usplash_put_part for the small or large spinner image
	usplash_put_part(spinner_x, spinner_y, spinner_part_width, spinner_height, &pixmap_xbmc_spinner, x0, y0);
}

/** not used for now **/
void t_clear_progressbar(struct usplash_theme *theme) { }
void t_draw_progressbar(struct usplash_theme *theme, int percentage) { }
