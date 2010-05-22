/*
 * frontend/console.h
 *
 * This
 *
 *
 */

#ifndef LAME_CONSOLE_H
#define LAME_CONSOLE_H

#if defined(_WIN32)  &&  !defined(__CYGWIN__)
# include <windows.h>
#endif

typedef struct {
    unsigned long ClassID;
    unsigned long ClassProt;
    FILE   *Console_fp;      /* filepointer to stream reporting information */
    FILE   *Error_fp;        /* filepointer to stream fatal error reporting information */
    FILE   *Report_fp;       /* filepointer to stream reports (normally a text file or /dev/null) */
#if defined(_WIN32)  &&  !defined(__CYGWIN__)
    HANDLE  Console_Handle;
#endif
    int     disp_width;
    int     disp_height;
    char    str_up[10];
    char    str_clreoln[10];
    char    str_emph[10];
    char    str_norm[10];
    char    Console_buff[2048];
    int     Console_file_type;
} Console_IO_t;

extern Console_IO_t Console_IO;
extern int frontend_open_console(void);
extern void frontend_close_console(void);

extern void frontend_msgf(const char *format, va_list ap);
extern void frontend_debugf(const char *format, va_list ap);
extern void frontend_errorf(const char *format, va_list ap);

int     console_printf(const char *format, ...);
int     error_printf(const char *format, ...);
int     report_printf(const char *format, ...);

void    console_flush(void);
void    error_flush(void);
void    report_flush(void);

void    console_up(int n_lines);

void    set_debug_file(const char *fn);

#endif /* LAME_CONSOLE_H */

/* end of console.h */
