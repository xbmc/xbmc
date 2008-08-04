#include "goomsl_hash.h"
#include <string.h>
#include <stdlib.h>

static GoomHashEntry *entry_new(const char *key, HashValue value) {

  int len = strlen(key);
	GoomHashEntry *entry = (GoomHashEntry*)malloc(sizeof(GoomHashEntry));

	entry->key = (char *)malloc(len+1);
	memcpy(entry->key,key,len+1);
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

GoomHash *goom_hash_new() {
	GoomHash *_this = (GoomHash*)malloc(sizeof(GoomHash));
	_this->root = NULL;
  _this->number_of_puts = 0;
	return _this;
}

void goom_hash_free(GoomHash *_this) {
	entry_free(_this->root);
	free(_this);
}

void goom_hash_put(GoomHash *_this, const char *key, HashValue value) {
  _this->number_of_puts += 1;
	if (_this->root == NULL)
		_this->root = entry_new(key,value);
	else
		entry_put(_this->root,key,value);
}

HashValue *goom_hash_get(GoomHash *_this, const char *key) {
  if (_this == NULL) return NULL;
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

/* FOR EACH */

static void _goom_hash_for_each(GoomHash *_this, GoomHashEntry *entry, GH_Func func)
{
  if (entry == NULL) return;
  func(_this, entry->key, &(entry->value));
  _goom_hash_for_each(_this, entry->lower, func);
  _goom_hash_for_each(_this, entry->upper, func);
}

void goom_hash_for_each(GoomHash *_this, GH_Func func) {
  _goom_hash_for_each(_this, _this->root, func);
}

int goom_hash_number_of_puts(GoomHash *_this) {
  return _this->number_of_puts;
}
