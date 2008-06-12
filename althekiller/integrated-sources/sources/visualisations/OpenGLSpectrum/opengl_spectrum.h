#ifndef OPENGL_SPECTRUM_H

extern void oglspectrum_configure(void);
extern void oglspectrum_read_config(void);

typedef struct
{
	gboolean tdfx_mode;
} OGLSpectrumConfig;

extern OGLSpectrumConfig oglspectrum_cfg;

#endif
