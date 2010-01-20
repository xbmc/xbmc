/*
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 */

#include <stdlib.h>
#include <math.h>

#ifndef WIN32
	#include <unistd.h>
	#include <sys/time.h>
	#include <time.h>
#else
	#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <SDL.h>

#include <wiiuse.h>

#define PI 3.14159265358979323846
#define PI_DIV_180 0.017453292519943296
#define deg PI_DIV_180

#define MAX_WIIMOTES				2

GLint width = 1024, height = 768;
GLfloat backColor[4] = {1.0,1.0,1.0,1.0};

wiimote** wiimotes = NULL;

int last_dots[4][2] = {{0}};
int xcoord = 0;
int ycoord = 0;

#ifdef WIN32
	DWORD last_render;
#else
	struct timeval last_render;
	int last_sec = 0;
	int fps = 0;
#endif

enum render_mode_t {
	IR = 1,
	TEAPOT
};
enum render_mode_t render_mode = IR;

/* light information */
struct light_t {
	GLfloat position[4];
	GLfloat spotDirection[3];
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat spotCutoff;
	GLfloat spotExponent;
	GLfloat spotAttenuation[3];  /*  [0] = constant, [1] = linear, [2] = quadratic  */
};
struct light_t light = {
	{ 1.0, 1.0, -2.0, 1.0	},
	{ -1.0, -1.0, 2.0		},
	{ 0.0, 0.0, 0.0, 1.0	},
	{ 1.0, 1.0, 1.0, 1.0	},
	{ 1.0, 1.0, 1.0, 1.0	},
	180.0, 0.0,
	{ 1.0, 0.0, 0.0			}
};

/* material information */
struct material_t {
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat emission[4];
	GLfloat shininess;
};
struct material_t red_plastic = {
	{ 0.3, 0.0, 0.0, 1.0 },
	{ 0.3, 0.0, 0.0, 1.0 },
	{ 0.8, 0.6, 0.6, 1.0 },
	{ 0.0, 0.0, 0.0, 1.0 },
	32.0
};


void handle_event(struct wiimote_t* wm);
void display();
void update_light(GLenum l, struct light_t* lptr);
void set_material(struct material_t* mptr);
void resize_window(GLint new_width, GLint new_height);

void handle_event(struct wiimote_t* wm) {
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS))
		wiiuse_motion_sensing(wm, 1);
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS))
		wiiuse_motion_sensing(wm, 0);

	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_UP))
		wiiuse_set_ir(wm, 1);
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_DOWN))
		wiiuse_set_ir(wm, 0);

	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_B))
		wiiuse_toggle_rumble(wm);

	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
		int level;
		WIIUSE_GET_IR_SENSITIVITY(wm, &level);
		wiiuse_set_ir_sensitivity(wm, level+1);
	}
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
		int level;
		WIIUSE_GET_IR_SENSITIVITY(wm, &level);
		wiiuse_set_ir_sensitivity(wm, level-1);
	}

	#if 0
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_A)) {
		if (render_mode == IR)
			render_mode = TEAPOT;
		else
			render_mode = IR;
		resize_window(width, height);
	}
	#endif
}

#define DRAW_TRIANGLE(x, y, z, s)	do {							\
										glVertex3f(x, y-s, z);		\
										glVertex3f(x+s, y+s, z);	\
										glVertex3f(x-s, y+s, z);	\
									} while (0)

int can_render() {
	/* quick fps limit to ~60fps -- not too fancy, could be better */
	#ifdef WIN32
		if (GetTickCount() < (last_render + 16))
			return 0;
		last_render = GetTickCount();
		return 1;
	#else
		struct timeval now;
		long elapsed_usec = 0;

		gettimeofday(&now, NULL);

		if (now.tv_usec > 1000000) {
			now.tv_usec -= 1000000;
			++now.tv_sec;
		}

		if (now.tv_sec > last_render.tv_sec)
			elapsed_usec = ((now.tv_sec - last_render.tv_sec) * 1000000);

		if (now.tv_usec > last_render.tv_usec)
			elapsed_usec += now.tv_usec - last_render.tv_usec;
		else
			elapsed_usec += last_render.tv_usec - now.tv_usec;

		if (time(NULL) > last_sec) {
			printf("fps: %i\n", fps);
			fps = 0;
			last_sec = time(NULL);
		}

		if (elapsed_usec < 16000)
			return 0;

		last_render = now;
		++fps;

		return 1;
	#endif
}

void display() {
	int i, wm;
	float size = 5;

	if (!can_render())
		return;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (render_mode == IR) {
		/* draw the IR stuff */

		glDisable(GL_LIGHTING);

		glBegin(GL_TRIANGLES);
			/* green center */
			glColor3f(0.0, 1.0, 0.0);
			DRAW_TRIANGLE(width/2, height/2, 0, size);
		glEnd();

		for (wm = 0; wm < MAX_WIIMOTES; ++wm) {
			glBegin(GL_TRIANGLES);
				/* red ir */
				glColor3f(1.0, 0.0, 0.0);
				for (i = 0; i < 4; ++i) {
					if (wiimotes[wm]->ir.dot[i].visible)
						DRAW_TRIANGLE(wiimotes[wm]->ir.dot[i].rx, wiimotes[wm]->ir.dot[i].ry, 0, size);
				}

				/* yellow corrected ir */
				glColor3f(1.0, 1.0, 0.0);
				for (i = 0; i < 4; ++i) {
					if (wiimotes[wm]->ir.dot[i].visible)
						DRAW_TRIANGLE(wiimotes[wm]->ir.dot[i].x, wiimotes[wm]->ir.dot[i].y, 0, size);
				}

				/* blue cursor */
				glColor3f(0.0, 0.0, 1.0);
				DRAW_TRIANGLE(wiimotes[wm]->ir.x, wiimotes[wm]->ir.y-size, 0, size);
			glEnd();
		}
	} else {
		/* draw the teapot */
		gluLookAt(0.0, 0.0, -5.0,
				  0.0, 0.0, 0.0,
				  0.0, 1.0, 0.0);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		update_light(GL_LIGHT0, &light);
		set_material(&red_plastic);

		glRotatef(wiimotes[0]->orient.roll, 0.0f, 0.0f, 1.0f);
		glRotatef(wiimotes[0]->orient.pitch, 1.0f, 0.0f, 0.0f);


		glutSolidTeapot(1);
	}

	SDL_GL_SwapBuffers();
}


void update_light(GLenum l, struct light_t* lptr) {
	glLightfv(l, GL_POSITION, lptr->position);
	glLightfv(l, GL_DIFFUSE, lptr->diffuse);
	glLightfv(l, GL_SPECULAR, lptr->specular);
	glLightfv(l, GL_AMBIENT, lptr->ambient);
	glLightfv(l, GL_SPOT_DIRECTION, lptr->spotDirection);
	glLightf(l, GL_SPOT_CUTOFF, lptr->spotCutoff);
	glLightf(l, GL_SPOT_EXPONENT, lptr->spotExponent);
	glLightf(l, GL_CONSTANT_ATTENUATION, lptr->spotAttenuation[0]);
	glLightf(l, GL_LINEAR_ATTENUATION, lptr->spotAttenuation[1]);
	glLightf(l, GL_QUADRATIC_ATTENUATION, lptr->spotAttenuation[2]);
}


void set_material(struct material_t* mptr) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mptr->ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mptr->diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mptr->specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mptr->shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mptr->emission);
}


void resize_window(GLint new_width, GLint new_height) {
	int wm;

	width = new_width;
	height = new_height;

	if (new_height == 0)
		new_height = 1;

	SDL_SetVideoMode(width, height, 16, SDL_RESIZABLE | SDL_OPENGL);

	glViewport(0, 0, new_width, new_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (render_mode == IR)
		gluOrtho2D(0, width, height, 0);
	else
		gluPerspective(60.0f, (float)new_width/(float)new_height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	width = new_width;
	height = new_height;

	for (wm = 0; wm < MAX_WIIMOTES; ++wm)
		wiiuse_set_ir_vres(wiimotes[wm], width, height);
}

#ifndef WIN32
int main(int argc, char** argv) {
#else
int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#endif
	int found, connected;
	int wm;

	//printf("wiiuse version = %s\n", wiiuse_version());

	wiimotes =  wiiuse_init(MAX_WIIMOTES);
	found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
	if (!found)
		return 0;
	connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
	if (connected)
		printf("Connected to %i wiimotes (of %i found).\n", connected, found);
	else {
		printf("Failed to connect to any wiimote.\n");
		return 0;
	}
	wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1 | WIIMOTE_LED_4);
	wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_2 | WIIMOTE_LED_4);
	wiiuse_rumble(wiimotes[0], 1);

	#ifndef WIN32
		usleep(200000);
	#else
		Sleep(200);
	#endif

	wiiuse_rumble(wiimotes[0], 0);

	/* enable IR and motion sensing for all wiimotes */
	for (wm = 0; wm < MAX_WIIMOTES; ++wm) {
		wiiuse_motion_sensing(wiimotes[wm], 1);
		wiiuse_set_ir(wiimotes[wm], 1);
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return 0;
	}

	SDL_WM_SetCaption("wiiuse SDL IR Example", "wiiuse SDL IR Example");

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* set window size */
	width = wiimotes[0]->ir.vres[0];
	height = wiimotes[0]->ir.vres[1];
	SDL_SetVideoMode(width, height, 16, SDL_RESIZABLE | SDL_OPENGL);

	for (wm = 0; wm < MAX_WIIMOTES; ++wm)
		wiiuse_set_ir_vres(wiimotes[wm], width, height);

	/* set OpenGL stuff */
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0, 0, 0, 0);

	/* set the size of the window */
	resize_window(width, height);

	display();

	#ifdef WIN32
		last_render = GetTickCount();
	#endif

	while (1) {
		SDL_Event event;

		if (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_VIDEORESIZE:
				{
					/* resize the window */
					resize_window(event.resize.w, event.resize.h);
					break;
				}
				case SDL_QUIT:
				{
					/* shutdown */
					SDL_Quit();
					wiiuse_cleanup(wiimotes, MAX_WIIMOTES);
					return 0;
				}
				default:
				{
				}
			}
		}

		if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {
			/*
			 *	This happens if something happened on any wiimote.
			 *	So go through each one and check if anything happened.
			 */
			int i = 0;
			for (; i < MAX_WIIMOTES; ++i) {
				switch (wiimotes[i]->event) {
					case WIIUSE_EVENT:
						/* a generic event occured */
						handle_event(wiimotes[i]);
						break;

					default:
						break;
				}
			}
		}

		display();
	}
}
