
#ifndef XBIRREMOTE_H
#define XBIRREMOTE_H



extern XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define     XDEVICE_TYPE_IR_REMOTE           (&XDEVICE_TYPE_IR_REMOTE_TABLE)


#define XINPUT_IR_REMOTE_DISPLAY      213
#define XINPUT_IR_REMOTE_REVERSE      226
#define XINPUT_IR_REMOTE_PLAY         234
#define XINPUT_IR_REMOTE_FORWARD      227
#define XINPUT_IR_REMOTE_SKIP_MINUS   221
#define XINPUT_IR_REMOTE_STOP         224
#define XINPUT_IR_REMOTE_PAUSE        230
#define XINPUT_IR_REMOTE_SKIP_PLUS    223
#define XINPUT_IR_REMOTE_TITLE        229
#define XINPUT_IR_REMOTE_INFO         195

#define XINPUT_IR_REMOTE_UP           166
#define XINPUT_IR_REMOTE_DOWN         167
#define XINPUT_IR_REMOTE_LEFT         169
#define XINPUT_IR_REMOTE_RIGHT        168

#define XINPUT_IR_REMOTE_SELECT       11

#define XINPUT_IR_REMOTE_MENU         247
#define XINPUT_IR_REMOTE_BACK         216

#define XINPUT_IR_REMOTE_1            206
#define XINPUT_IR_REMOTE_2            205
#define XINPUT_IR_REMOTE_3            204
#define XINPUT_IR_REMOTE_4            203
#define XINPUT_IR_REMOTE_5            202
#define XINPUT_IR_REMOTE_6            201
#define XINPUT_IR_REMOTE_7            200
#define XINPUT_IR_REMOTE_8            199
#define XINPUT_IR_REMOTE_9            198
#define XINPUT_IR_REMOTE_0            207


typedef struct _XINPUT_IR_REMOTE
{
    BYTE    wButtons;
	BYTE    region;  // just a guess
	BYTE    counter; // some value that is changing while a button is pressed... could be the state of the buffer
} XINPUT_IR_REMOTE, *PIR_REMOTE;


#endif