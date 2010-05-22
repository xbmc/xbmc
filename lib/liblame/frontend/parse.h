
int     usage(FILE * const fp, const char *ProgramName);
int     short_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName);
int     long_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName,
                  int lessmode);
int     display_bitrates(FILE * const fp);

int     parse_args(lame_global_flags * gfp, int argc, char **argv, char *const inPath,
                   char *const outPath, char **nogap_inPath, int *max_nogap);

void    parse_close();

/* end of parse.h */
