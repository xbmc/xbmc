#ifndef _GOOM_HASH_H
#define _GOOM_HASH_H

typedef struct GOOM_HASH_ENTRY GoomHashEntry;
typedef struct GOOM_HASH GoomHash;

typedef union {
    void *ptr;
    int i;
    float f;
} HashValue;

struct GOOM_HASH_ENTRY {
	char *key;
    HashValue value;
	GoomHashEntry *lower;
	GoomHashEntry *upper;
};

struct GOOM_HASH {
	GoomHashEntry *root;
};

GoomHash *goom_hash_new(void);
void goom_hash_free(GoomHash *gh);

void goom_hash_put(GoomHash *gh, const char *key, HashValue value);
HashValue *goom_hash_get(GoomHash *gh, const char *key);

void goom_hash_put_int(GoomHash *_this, const char *key, int i);
void goom_hash_put_float(GoomHash *_this, const char *key, float f);
void goom_hash_put_ptr(GoomHash *_this, const char *key, void *ptr);

#endif /* _GOOM_HASH_H */
