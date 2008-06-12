#ifndef _MANGLE_H_
#define _MANGLE_H_
/*
  header for 8.3 name mangling interface 
*/

struct mangle_fns {
	void (*reset)(void);
	BOOL (*is_mangled)(const char *s, int snum);
	BOOL (*is_8_3)(const char *fname, BOOL check_case, BOOL allow_wildcards, int snum);
	BOOL (*check_cache)(char *s, size_t maxlen, int snum);
	void (*name_map)(char *OutName, BOOL need83, BOOL cache83, int default_case, int snum);
};
#endif /* _MANGLE_H_ */
