//
// File:   glError.h
// Author: fatray
//
// Created on 02 December 2007, 16:08
//

#ifndef _GLERROR_H
#define	_GLERROR_H

// no need to include GL in here, 
// if someone wants GL errors they probably already included it.


/*
 * if we are debugging, print all glErrors to stderr.
 * Remeber that glErrors are buffered, this just prints any in the buffer.
 */
#ifdef NDEBUG
#define glError()
#else
#define glError() { \
	GLenum err; \
	while ((err = glGetError()) != GL_NO_ERROR) \
		fprintf(stderr, "glError: %s at %s:%u\n", \
			(char *)gluErrorString(err), __FILE__, __LINE__); \
}
#endif	/* glError */

#endif	/* _GLERROR_H */

