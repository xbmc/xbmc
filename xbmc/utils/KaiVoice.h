#ifndef _KAIVOICE_H_
#define _KAIVOICE_H_

#include <xonline.h>

const FLOAT VOICE_SEND_INTERVAL   = 0.3f;	// 300ms
const DWORD MAX_VOICE_PER_SEND    = 15;		// 15 frames per UDP send
const DWORD COMPRESSED_FRAME_SIZE = 20;		// 20 bytes

#endif // _KAIVOICE_H_