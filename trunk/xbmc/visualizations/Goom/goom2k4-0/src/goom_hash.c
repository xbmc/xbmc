#include "goom_hash.h"
#include <string.h>
#include <stdlib.h>

static GoomHashEntry *entry_new(const char *key, HashValue value) {

	GoomHashEntry *entry = (GoomHashEntry*)malloc(sizeof(GoomHashEntry));

	entry->key = (char *)malloc(strlen(key)+1);
	strcpy(entry->key,key);
	entry->value = value;
	entry->lower = NULL;
	entry->upper = NULL;

	return entry;
}

static void entry_free(GoomHashEntry *entry) {
	if (entry!=NULL) {
		entry_free(entry->lower);
		entry_free(entry->upper);
		free(entry->key);
		free(entry);
	}
}

static void entry_put(GoomHashEntry *entry, const char *key, HashValue value) {
	int cmp = strcmp(key,entry->key);
	if (cmp==0) {
		entry->value = value;
	}
	else if (cmp > 0) {
		if (entry->upper == NULL)
			entry->upper = entry_new(key,value);
		else
			entry_put(entry->upper, key, value);
	}
	else {
		if (entry->lower == NULL)
			entry->lower = entry_new(key,value);
		else
			entry_put(entry->lower, key, value);
	}
}

static HashValue *entry_get(GoomHashEntry *entry, const char *key) {

	int cmp;
	if (entry==NULL)
		return NULL;
	cmp = strcmp(key,entry->key);
	if (cmp > 0)
		return entry_get(entry->upper, key);
	else if (cmp < 0)
		return entry_get(entry->lower, key);
	else
		return &(entry->value);
}

GoomHash *goom_hash_new(void) {
	GoomHash *_this = (GoomHash*)malloc(sizeof(GoomHash));
	_this->root = NULL;
	return _this;
}

void goom_hash_free(GoomHash *_this) {
	entry_free(_this->root);
	free(_this);
}

void goom_hash_put(GoomHash *_this, const char *key, HashValue value) {
	if (_this->root == NULL)
		_this->root = entry_new(key,value);
	else
		entry_put(_this->root,key,value);
}

HashValue *goom_hash_get(GoomHash *_this, const char *key) {
	return entry_get(_this->root,key);
}

void goom_hash_put_int(GoomHash *_this, const char *key, int i) {
    HashValue value;
    value.i = i;
    goom_hash_put(_this,key,value);
}

void goom_hash_put_float(GoomHash *_this, const char *key, float f) {
    HashValue value;
    value.f = f;
    goom_hash_put(_this,key,value);
}

void goom_hash_put_ptr(GoomHash *_this, const char *key, void *ptr) {
    HashValue value;
    value.ptr = ptr;
    goom_hash_put(_this,key,value);
}


