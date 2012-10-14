/* gridfs.c */

/*    Copyright 2009-2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "gridfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Memory allocation functions */
MONGO_EXPORT gridfs *gridfs_create( void ) {
  gridfs* GridFs = (gridfs*)bson_malloc(sizeof(gridfs));
  ASSIGN_SIGNATURE(GridFs, MONGO_SIGNATURE);  
  return GridFs;
}

MONGO_EXPORT void gridfs_dispose(gridfs *gfs) {
  check_destroyed_mongo_object( gfs );
  free(gfs);
}

MONGO_EXPORT gridfile *gridfile_create( void ) {
  gridfile* gfile = (gridfile*)bson_malloc(sizeof(gridfile));  
  memset( gfile, 0, sizeof ( gridfile ) );
  ASSIGN_SIGNATURE(gfile, MONGO_SIGNATURE);
  return gfile;
}

MONGO_EXPORT void gridfile_dispose(gridfile *gf) {
  check_destroyed_mongo_object( gf );
  free(gf);
}

MONGO_EXPORT void gridfile_get_descriptor(gridfile *gf, bson *out) {
  check_mongo_object( gf );
  *out =  *gf->meta;
}

/* Default chunk pre and post processing logic */
static int defaultPreProcessChunk(void** targetBuf, size_t* targetLen, void* srcBuf, size_t srcLen, int flags) {
  *targetBuf = srcBuf;
  *targetLen = srcLen;
  return 0;
}

static int defaultPostProcessChunk(void** targetBuf, size_t* targetLen, void* srcData, size_t srcLen, int flags) {   
  *targetBuf = srcData;
  *targetLen = srcLen;
  return 0;
}

static size_t defaultDendingDataNeededSize (int flags) {
  return DEFAULT_CHUNK_SIZE;
}
/* End of default functions for chunks pre and post processing */

static gridfs_preProcessingFunc preProcessChunk = defaultPreProcessChunk;
static gridfs_postProcessingFunc postProcessChunk = defaultPostProcessChunk;
static gridfs_pendingDataNeededSizeFunc pendingDataNeededSize = defaultDendingDataNeededSize;

static bson *chunk_new(bson_oid_t id, int chunkNumber, void** dataBuf, void* srcData, size_t len, int flags ) {
  bson *b = (bson*)bson_malloc(sizeof(bson));
  size_t dataBufLen = 0;

  if( preProcessChunk( dataBuf, &dataBufLen, srcData, len, flags) != 0 ) {
    return NULL;
  }
  bson_init(b);
  bson_append_oid(b, "files_id", &id);
  bson_append_int(b, "n", chunkNumber);
  bson_append_binary(b, "data", BSON_BIN_BINARY, (char*)(*dataBuf), (int)dataBufLen);
  bson_finish(b);
  return b;
}

static void chunk_free(bson *oChunk) {
  if( oChunk ) {
    bson_destroy(oChunk);
    bson_free(oChunk);
  }
}
/* End of memory allocation functions */

MONGO_EXPORT void setBufferProcessingProcs(gridfs_preProcessingFunc preProcessFunc, gridfs_postProcessingFunc postProcessFunc, gridfs_pendingDataNeededSizeFunc pendingDataNeededSizeFunc){
  preProcessChunk = preProcessFunc;
  postProcessChunk = postProcessFunc;
  pendingDataNeededSize = pendingDataNeededSizeFunc; 
}

/* -------------- */
/* gridfs methods */
/* -------------- */

static void gridfs_freeFields( gridfs *gfs ) {
  check_mongo_object ( gfs );
  if( gfs->dbname ) {
    bson_free((char*)gfs->dbname);
    gfs->dbname = NULL;
  }
  if( gfs->prefix ) {
    bson_free((char*)gfs->prefix);
    gfs->prefix = NULL;
  }
  if( gfs->files_ns ) {
    bson_free((char*)gfs->files_ns);
    gfs->files_ns = NULL;
  }
  if( gfs->chunks_ns ) {
    bson_free((char*)gfs->chunks_ns);
    gfs->chunks_ns = NULL;
  }      
}

/* gridfs constructor */
MONGO_EXPORT int gridfs_init(mongo *client, const char *dbname, const char *prefix, gridfs *gfs) {

  int options;
  bson b = INIT_BSON;
  bson_bool_t success;

  check_mongo_object( gfs );

  gfs->caseInsensitive = 0;
  gfs->client = client;

  /* Allocate space to own the dbname */
  gfs->dbname = (const char*)bson_malloc((int)strlen(dbname) + 1);
  strcpy((char*)gfs->dbname, dbname);

  /* Allocate space to own the prefix */
  if (prefix == NULL) {
    prefix = "fs";
  } gfs->prefix = (const char*)bson_malloc((int)strlen(prefix) + 1);
  strcpy((char*)gfs->prefix, prefix);

  /* Allocate space to own files_ns */
  gfs->files_ns = (const char*)bson_malloc((int)(strlen(prefix) + strlen(dbname) + strlen(".files") + 2));
  strcpy((char*)gfs->files_ns, dbname);
  strcat((char*)gfs->files_ns, ".");
  strcat((char*)gfs->files_ns, prefix);
  strcat((char*)gfs->files_ns, ".files");

  /* Allocate space to own chunks_ns */
  gfs->chunks_ns = (const char*)bson_malloc((int)(strlen(prefix) + strlen(dbname) + strlen(".chunks") + 2));
  strcpy((char*)gfs->chunks_ns, dbname);
  strcat((char*)gfs->chunks_ns, ".");
  strcat((char*)gfs->chunks_ns, prefix);
  strcat((char*)gfs->chunks_ns, ".chunks");

  bson_init(&b);
  bson_append_int(&b, "filename", 1);
  bson_finish(&b);
  options = 0;
  success = (mongo_create_index(gfs->client, gfs->files_ns, &b, options, NULL) == MONGO_OK);
  bson_destroy(&b);
  if (!success) {
    gridfs_freeFields( gfs );
    return MONGO_ERROR;
  }

  bson_init(&b);
  bson_append_int(&b, "files_id", 1);
  bson_append_int(&b, "n", 1);
  bson_finish(&b);
  options = MONGO_INDEX_UNIQUE;
  success = (mongo_create_index(gfs->client, gfs->chunks_ns, &b, options, NULL) == MONGO_OK);
  bson_destroy(&b);
  if (!success) {
    gridfs_freeFields( gfs );    
    return MONGO_ERROR;
  }

  return MONGO_OK;
}

/* gridfs destructor */
MONGO_EXPORT void gridfs_destroy(gridfs *gfs) {
  if (gfs == NULL) {
    return ;
  } 
  check_mongo_object( gfs );
  gridfs_freeFields( gfs );
  ASSIGN_SIGNATURE(gfs, MONGO_SIGNATURE_READY_TO_DISPOSE);  
}

/* gridfs accesors */

MONGO_EXPORT bson_bool_t gridfs_get_caseInsensitive(gridfs *gfs){
  check_mongo_object( gfs );
  return gfs->caseInsensitive;
}


MONGO_EXPORT void gridfs_set_caseInsensitive(gridfs *gfs, bson_bool_t newValue){
  check_mongo_object( gfs );
  gfs->caseInsensitive = newValue;
}

static char* upperFileName(const char* filename){
  char* upperName = (char*) bson_malloc((int)strlen( filename ) + 1 );
  strcpy(upperName, filename);
  _strupr(upperName);
  return upperName;
}

static int gridfs_insert_file(gridfs *gfs, const char *name, const bson_oid_t id, gridfs_offset length, const char *contenttype, int flags, int chunkSize) {
  bson command = INIT_BSON;
  bson ret = INIT_BSON;
  bson res = INIT_BSON;
  bson_iterator it = INIT_ITERATOR;
  bson q = INIT_BSON;
  int result;
  int64_t d;
  char *upperName = NULL;

   check_mongo_object( gfs );

  /* If you don't care about calculating MD5 hash for a particular file, simply pass the GRIDFILE_NOMD5 value on the flag param */
  if( !( flags & GRIDFILE_NOMD5 ) ) {  
    /* Check run md5 */
    bson_init(&command);
    bson_append_oid(&command, "filemd5", &id);
    bson_append_string(&command, "root", gfs->prefix);
    bson_finish(&command);
    result = mongo_run_command(gfs->client, gfs->dbname, &command, &res);
    bson_destroy(&command);
    if (result != MONGO_OK) {
      return result;
    } 
  } 

  /* Create and insert BSON for file metadata */
  bson_init(&ret);
  bson_append_oid(&ret, "_id", &id);
  if( gfs->caseInsensitive ) {
    upperName = upperFileName(name);    
  }
  if (name != NULL &&  *name != '\0') {
    bson_append_string(&ret, "filename", upperName ? upperName : name);
  }
  bson_append_long(&ret, "length", length);
  bson_append_int(&ret, "chunkSize", chunkSize);
  d = (bson_date_t)1000 *time(NULL);
  bson_append_date(&ret, "uploadDate", d);
  if( !( flags & GRIDFILE_NOMD5 ) ) {
    bson_find(&it, &res, "md5");
    bson_append_string(&ret, "md5", bson_iterator_string(&it));
    bson_destroy(&res);
  } else {
    bson_append_string(&ret, "md5", ""); 
  } 
  if (contenttype != NULL &&  *contenttype != '\0') {
    bson_append_string(&ret, "contentType", contenttype);
  }
  if ( upperName ) {
    bson_append_string(&ret, "realFilename", name);
  }
  bson_append_int(&ret, "flags", flags);
  bson_finish(&ret);

  bson_init(&q);
  bson_append_oid(&q, "_id", &id);
  bson_finish(&q);

  result = mongo_update(gfs->client, gfs->files_ns, &q, &ret, MONGO_UPDATE_UPSERT, NULL);

  bson_destroy(&ret);
  bson_destroy(&q);
  if( upperName ) {
    bson_free( upperName );
  }

  return result;
}

MONGO_EXPORT int gridfs_store_buffer(gridfs *gfs, const char *data, gridfs_offset length, const char *remotename, const char *contenttype, int flags ) {

  char const *end = data + length;
  const char *data_ptr = data;
  void* targetBuf = NULL;
  bson_oid_t id;
  int chunkNumber = 0;
  int chunkLen;
  bson *oChunk;
  int memAllocated = 0;
    
  check_mongo_object( gfs );

  /* Generate and append an oid*/
  bson_oid_gen(&id);

  /* Insert the file's data chunk by chunk */
  while (data_ptr < end) {
    chunkLen = DEFAULT_CHUNK_SIZE < (unsigned int)(end - data_ptr) ? DEFAULT_CHUNK_SIZE: (unsigned int)(end - data_ptr);
    oChunk = chunk_new(id, chunkNumber, &targetBuf, (void*)data_ptr, chunkLen, flags );
    memAllocated = targetBuf != data_ptr;
    mongo_insert(gfs->client, gfs->chunks_ns, oChunk, NULL);
    chunk_free(oChunk);
    chunkNumber++;
    data_ptr += chunkLen;
  }

  if( memAllocated ) {
    bson_free( targetBuf );
  }

  /* Inserts file's metadata */
  return gridfs_insert_file(gfs, remotename, id, length, contenttype, flags, DEFAULT_CHUNK_SIZE);
}

MONGO_EXPORT int gridfs_store_file(gridfs *gfs, const char *filename, const char *remotename, const char *contenttype, int flags ) {

  char buffer[DEFAULT_CHUNK_SIZE];
  FILE *fd;
  bson_oid_t id;
  int chunkNumber = 0;
  gridfs_offset length = 0;
  gridfs_offset chunkLen = 0;
  bson *oChunk;
  void* targetBuf = NULL;

  check_mongo_object( gfs );

  /* Open the file and the correct stream */
  if (strcmp(filename, "-") == 0) {
    fd = stdin;
  } else {
    fd = fopen(filename, "rb");
    if (fd == NULL) {
      return MONGO_ERROR;
    } 
  }

  /* Generate and append an oid*/
  bson_oid_gen(&id);

  /* Insert the file chunk by chunk */
  chunkLen = fread(buffer, 1, DEFAULT_CHUNK_SIZE, fd);
  do {
    oChunk = chunk_new(id, chunkNumber, &targetBuf, (void*)buffer, (size_t)chunkLen, flags );
    mongo_insert(gfs->client, gfs->chunks_ns, oChunk, NULL);
    chunk_free(oChunk);
    length += chunkLen;
    chunkNumber++;
    chunkLen = fread(buffer, 1, DEFAULT_CHUNK_SIZE, fd);
  } while (chunkLen != 0);

  /* Close the file stream */
  if (fd != stdin) {
    fclose(fd);
  } 

  /* Optional Remote Name */
  if (remotename == NULL ||  *remotename == '\0') {
    remotename = filename;
  }

  if( targetBuf && targetBuf != buffer ) {
    bson_free( targetBuf );
  }

  /* Inserts file's metadata */
  return gridfs_insert_file(gfs, remotename, id, length, contenttype, flags, DEFAULT_CHUNK_SIZE );
}

MONGO_EXPORT void gridfs_remove_filename(gridfs *gfs, const char *filename) {
  bson query = INIT_BSON;
  mongo_cursor *files;
  bson file = INIT_BSON;
  bson_iterator it = INIT_ITERATOR;
  bson_oid_t id;
  bson b = INIT_BSON;
  char *upperName = NULL;

  check_mongo_object( gfs );

  if( gfs->caseInsensitive ) {
    upperName = upperFileName(filename);
  }
  bson_init(&query);
  if( upperName ) {
    bson_append_string(&query, "filename", upperName);
  } else {
    bson_append_string(&query, "filename", filename);
  }
  bson_finish(&query);
  files = mongo_find(gfs->client, gfs->files_ns, &query, NULL, 0, 0, 0);
  bson_destroy(&query);
  if( upperName ) {
    bson_free( upperName );
  }

  /* Remove each file and it's chunks from files named filename */
  while (mongo_cursor_next(files) == MONGO_OK) {
    file = files->current;
    bson_find(&it, &file, "_id");
    id =  *bson_iterator_oid(&it);

    /* Remove the file with the specified id */
    bson_init(&b);
    bson_append_oid(&b, "_id", &id);
    bson_finish(&b);
    mongo_remove(gfs->client, gfs->files_ns, &b, NULL);
    bson_destroy(&b);

    /* Remove all chunks from the file with the specified id */
    bson_init(&b);
    bson_append_oid(&b, "files_id", &id);
    bson_finish(&b);
    mongo_remove(gfs->client, gfs->chunks_ns, &b, NULL);
    bson_destroy(&b);
  }

  mongo_cursor_destroy(files);
}

MONGO_EXPORT int gridfs_find_query(gridfs *gfs, bson *query, gridfile *gfile) {

  bson uploadDate = INIT_BSON;
  bson finalQuery = INIT_BSON;
  bson out = INIT_BSON;
  int i;

  check_mongo_object( gfs );
  check_mongo_object( query );
  check_mongo_object( gfile );

  bson_init(&uploadDate);
  bson_append_int(&uploadDate, "uploadDate",  - 1);
  bson_finish(&uploadDate);

  bson_init(&finalQuery);
  bson_append_bson(&finalQuery, "query", query);
  bson_append_bson(&finalQuery, "orderby", &uploadDate);
  bson_finish(&finalQuery);

  i = (mongo_find_one(gfs->client, gfs->files_ns,  &finalQuery, NULL, &out) == MONGO_OK);
  bson_destroy(&uploadDate);
  bson_destroy(&finalQuery);
  if (!i) {
    return MONGO_ERROR;
  } else {
    gridfile_init(gfs, &out, gfile);
    bson_destroy(&out);
    return MONGO_OK;
  }
}

MONGO_EXPORT int gridfs_find_filename(gridfs *gfs, const char *filename, gridfile *gfile)

 {
  bson query = INIT_BSON;
  int i;
  char *upperName = NULL;

  check_mongo_object( gfs );
  check_mongo_object( gfile );

  bson_init(&query);
  if( gfs->caseInsensitive ) {
    upperName = upperFileName( filename );
  }
  if( upperName ) {
    bson_append_string(&query, "filename", upperName);
  } else {
    bson_append_string(&query, "filename", filename);
  }
  bson_finish(&query);
  i = gridfs_find_query(gfs, &query, gfile);
  bson_destroy(&query);
  if( upperName ) {
    bson_free( upperName );
  }
  return i;
}

/* ---------------- */
/* gridfile methods */
/* ---------------- */

/* gridfile private methods forward declarations */
static void gridfile_flush_pendingchunk(gridfile *gfile);
static void gridfile_init_flags(gridfile *gfile);
static void gridfile_init_length(gridfile *gfile);
static void gridfile_init_chunkSize(gridfile *gfile);

/* gridfile constructors, destructors and memory management */

MONGO_EXPORT int gridfile_init(gridfs *gfs, bson *meta, gridfile *gfile){
  check_mongo_object( gfs );
  check_mongo_object( gfile );
  gfile->gfs = gfs;
  gfile->pos = 0;
  gfile->pending_len = 0;
  gfile->pending_data = NULL;
  gfile->meta = (bson*)bson_malloc(sizeof(bson));
  if (gfile->meta == NULL) {
    return MONGO_ERROR;
  } bson_copy(gfile->meta, meta);
  gridfile_init_chunkSize( gfile );
  gridfile_init_length( gfile );
  gridfile_init_flags( gfile );
  return MONGO_OK;
}

MONGO_EXPORT int gridfile_writer_done(gridfile *gfile) {

  int response;

  check_mongo_object( gfile );

  if (gfile->pending_len) {
    /* write any remaining pending chunk data.
     * pending data will always take up less than one chunk */
    gridfile_flush_pendingchunk(gfile);    
  }
  if( gfile->pending_data ) {
    bson_free(gfile->pending_data);    
    gfile->pending_data = NULL;   
  }

  /* insert into files collection */
  response = gridfs_insert_file(gfile->gfs, gfile->remote_name, gfile->id, gfile->length, gfile->content_type, gfile->flags, gfile->chunkSize);

  if( gfile->remote_name ) {
    bson_free(gfile->remote_name);
    gfile->remote_name = NULL;
  }
  if( gfile->content_type ) {
    bson_free(gfile->content_type);
    gfile->content_type = NULL;
  }

  return response;
}

static void gridfile_init_chunkSize(gridfile *gfile){
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "chunkSize") != BSON_EOO ) {
    if (bson_iterator_type(&it) == BSON_INT) {
      gfile->chunkSize = bson_iterator_int(&it);
    } else {
      gfile->chunkSize = (int)bson_iterator_long(&it);
    }
  } else {
    gfile->chunkSize = DEFAULT_CHUNK_SIZE;
  }
}

static void gridfile_init_length(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "length") != BSON_EOO ) {
    if (bson_iterator_type(&it) == BSON_INT) {
      gfile->length = (gridfs_offset)bson_iterator_int(&it);
    } else {
      gfile->length = (gridfs_offset)bson_iterator_long(&it);
    }
  } else {
    gfile->length = 0;
  }
}

static void gridfile_init_flags(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "flags") != BSON_EOO ) {
    gfile->flags = (gridfs_offset)bson_iterator_int(&it);
  } else {
    gfile->flags = 0;
  }
}

MONGO_EXPORT void gridfile_writer_init(gridfile *gfile, gridfs *gfs, const char *remote_name, const char *content_type, int flags ) {
  bson_oid_t *foid;
  gridfile tmpFile = INIT_GRIDFILE;

  check_mongo_object( gfs );
  check_mongo_object( gfile );

  gfile->gfs = gfs;
  if (gridfs_find_filename(gfs, remote_name, &tmpFile) == MONGO_OK) {
    if( gridfile_exists(&tmpFile) ) {
      /* If file exists, then let's initialize members dedicated to coordinate writing operations 
       with existing file metadata */
      foid = gridfile_get_id( &tmpFile );
      memcpy(&gfile->id, foid, sizeof( gfile->id ));    
      gridfile_init_length( &tmpFile );            
      gfile->length = tmpFile.length;  
      gfile->chunkSize = gridfile_get_chunksize( gfile );
      if( flags != GRIDFILE_DEFAULT) {
        gfile->flags = flags;
      } else {
        gridfile_init_flags( &tmpFile );
        gfile->flags = tmpFile.flags;
      }
    }
    gridfile_destroy( &tmpFile );
  } else {
    /* File doesn't exist, let's create a new bson id and initialize length to zero */
    bson_oid_gen(&(gfile->id));
    gfile->length = 0;
    /* File doesn't exist, lets use the flags passed as a parameter to this procedure call */
    gfile->flags = flags;
  }  

  /* We initialize chunk_num with zero, but it will get always calculated when calling 
     gridfile_load_pending_data_with_pos_chunk() or when calling gridfile_write_buffer() */
  gfile->chunk_num = 0; 
  gfile->pos = 0;

  gfile->remote_name = (char*)bson_malloc((int)strlen(remote_name) + 1);
  strcpy((char*)gfile->remote_name, remote_name);

  gfile->content_type = (char*)bson_malloc((int)strlen(content_type) + 1);
  strcpy((char*)gfile->content_type, content_type);  

  gfile->pending_len = 0;
  /* Let's pre-allocate DEFAULT_CHUNK_SIZE bytes into pending_data then we don't need to worry 
     about doing realloc everywhere we want use the pending_data buffer */
  gfile->pending_data = (char*) bson_malloc((int)pendingDataNeededSize(gfile->flags));
}

MONGO_EXPORT void gridfile_destroy(gridfile *gfile)

{
  check_mongo_object( gfile );
  if( gfile->meta ) { 
    bson_destroy(gfile->meta);
    bson_free(gfile->meta);
    gfile->meta = NULL;
  }
  ASSIGN_SIGNATURE(gfile, MONGO_SIGNATURE_READY_TO_DISPOSE);
}

/* gridfile accessors */

MONGO_EXPORT bson_oid_t *gridfile_get_id(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "_id") != BSON_EOO) {
    if (bson_iterator_type(&it) == BSON_OID) {
      return bson_iterator_oid(&it);
    } else {
      return &gfile->id;
    } 
  } else {
    return &gfile->id;
  }
}

MONGO_EXPORT bson_bool_t gridfile_exists(gridfile *gfile) {
  check_mongo_object( gfile );
  return (bson_bool_t)(gfile != NULL || gfile->meta == NULL);
}

MONGO_EXPORT const char *gridfile_get_filename(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( gfile->gfs->caseInsensitive && bson_find( &it, gfile->meta, "realFilename" ) != BSON_EOO ) {
    return bson_iterator_string(&it); 
  }
  if( bson_find(&it, gfile->meta, "filename") != BSON_EOO) {
    return bson_iterator_string(&it);
  } else {
    return gfile->remote_name;
  }
}

MONGO_EXPORT int gridfile_get_chunksize(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( gfile->chunkSize ) {
    return gfile->chunkSize;
  } else {
    if( bson_find(&it, gfile->meta, "chunkSize") != BSON_EOO ) {
      return bson_iterator_int(&it);
  } else {  
      return DEFAULT_CHUNK_SIZE;
    }
  }
}

MONGO_EXPORT gridfs_offset gridfile_get_contentlength(gridfile *gfile) {
  gridfs_offset estimatedLen;
  check_mongo_object( gfile );
  estimatedLen = gfile->pending_len ? gfile->chunk_num * gridfile_get_chunksize( gfile ) + gfile->pending_len : gfile->length;
  return estimatedLen > gfile->length ? estimatedLen : gfile->length;  
}

MONGO_EXPORT const char *gridfile_get_contenttype(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if (bson_find(&it, gfile->meta, "contentType")) {
    return bson_iterator_string(&it);
  } else {
    return NULL;
  } 
}

MONGO_EXPORT bson_date_t gridfile_get_uploaddate(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "uploadDate") != BSON_EOO) {
    return bson_iterator_date(&it);
  } else {
    return 0;
  }
}

MONGO_EXPORT const char *gridfile_get_md5(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "md5") != BSON_EOO ) {
    return bson_iterator_string(&it);
  } else {
    return NULL;
  }
}

MONGO_EXPORT void gridfile_set_flags(gridfile *gfile, int flags){
  check_mongo_object( gfile );
  gfile->flags = flags;
}

MONGO_EXPORT int gridfile_get_flags(gridfile *gfile){
  check_mongo_object( gfile );
  return gfile->flags;
}

MONGO_EXPORT const char *gridfile_get_field(gridfile *gfile, const char *name) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, name) != BSON_EOO) {
    return bson_iterator_value(&it);
  } else {
    return NULL;
  }
}

MONGO_EXPORT bson_bool_t gridfile_get_boolean(gridfile *gfile, const char *name) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, name) != BSON_EOO) {
    return bson_iterator_bool(&it);
  } else {
    return 0;
  }
}

MONGO_EXPORT void gridfile_get_metadata(gridfile *gfile, bson *out) {
  bson_iterator it = INIT_ITERATOR;

  check_mongo_object( gfile );

  if (bson_find(&it, gfile->meta, "metadata")) {
    bson_iterator_subobject(&it, out);
  } else {
    bson_empty(out);
  } 
}

/* ++++++++++++++++++++++++++++++++ */
/* gridfile data management methods */
/* ++++++++++++++++++++++++++++++++ */

MONGO_EXPORT int gridfile_get_numchunks(gridfile *gfile) {
  bson_iterator it = INIT_ITERATOR;
  gridfs_offset length;
  gridfs_offset chunkSize;
  double numchunks;

  check_mongo_object( gfile );

  bson_find(&it, gfile->meta, "length");

  if (bson_iterator_type(&it) == BSON_INT) {
    length = (gridfs_offset)bson_iterator_int(&it);
  } else {
    length = (gridfs_offset)bson_iterator_long(&it);
  } 

  bson_find(&it, gfile->meta, "chunkSize");
  chunkSize = bson_iterator_int(&it);
  numchunks = ((double)length / (double)chunkSize);
  return (numchunks - (int)numchunks > 0) ? (int)(numchunks + 1): (int)(numchunks);
}

static void gridfile_prepare_chunk_key_bson(bson *q, bson_oid_t *id, int chunk_num) {
  bson_init(q);
  bson_append_int(q, "n", chunk_num);
  bson_append_oid(q, "files_id", id);
  bson_finish(q);
}

static void gridfile_flush_pendingchunk(gridfile *gfile) {
  bson *oChunk;
  bson q = INIT_BSON;
  void* targetBuf = NULL;

  check_mongo_object( gfile );

  if (gfile->pending_len) {
    oChunk = chunk_new(gfile->id, gfile->chunk_num, &targetBuf, (void*)gfile->pending_data, gfile->pending_len, gfile->flags );
    gridfile_prepare_chunk_key_bson( &q, &gfile->id, gfile->chunk_num );    
    mongo_update(gfile->gfs->client, gfile->gfs->chunks_ns, &q, oChunk, MONGO_UPDATE_UPSERT, NULL);
    bson_destroy(&q);
    chunk_free(oChunk);
    gfile->chunk_num++;
    if (gfile->pos > gfile->length) {
      gfile->length = gfile->pos;
    }
    gfile->pending_len = 0;
  }
  if( targetBuf && targetBuf != gfile->pending_data ) {
    free( targetBuf );
  }
}

static void gridfile_load_pending_data_with_pos_chunk(gridfile *gfile) {
  int chunk_len;
  const char *chunk_data;
  bson_iterator it = INIT_ITERATOR;
  bson chk = INIT_BSON;
  void* targetBuffer = NULL;
  size_t targetBufferLen = 0;

  check_mongo_object( gfile );

  chk.dataSize = 0;
  gridfile_get_chunk(gfile, (int)(gfile->pos / DEFAULT_CHUNK_SIZE), &chk);
  if (chk.dataSize <= 5) {
    if( chk.data ) {
      bson_destroy( &chk );
    }
    bson_fatal_msg( 0, "The chunk didn't contain any fields... this has to be an internal error...");
    return;
  }
  bson_find(&it, &chk, "data");
  chunk_len = bson_iterator_bin_len(&it);
  chunk_data = bson_iterator_bin_data(&it);
  postProcessChunk( &targetBuffer, &targetBufferLen, (void*)chunk_data, (size_t)chunk_len, gfile->flags );
  gfile->pending_len = (int)targetBufferLen;
  gfile->chunk_num = (int)(gfile->pos / DEFAULT_CHUNK_SIZE);
  if( targetBufferLen ) {
    memcpy(gfile->pending_data, targetBuffer, targetBufferLen);
  }
  bson_destroy( &chk );
  if( targetBuffer && targetBuffer != chunk_data ) {
    bson_free( targetBuffer );
  }
  return;
}

MONGO_EXPORT void gridfile_write_buffer(gridfile *gfile, const char *data, gridfs_offset length) {

  bson *oChunk;
  bson q = INIT_BSON;
  int buf_pos, buf_bytes_to_write;    
  gridfs_offset bytes_left = length;
  void* targetBuf = NULL;
  int memAllocated = 0;

  check_mongo_object( gfile );
  
  gfile->chunk_num = (int)(gfile->pos / DEFAULT_CHUNK_SIZE);
  buf_pos = (int)(gfile->pos - (gfile->pos / DEFAULT_CHUNK_SIZE) * DEFAULT_CHUNK_SIZE);
  /* First let's see if our current position is an an offset > 0 from the beginning of the current chunk. 
     If so, then we need to preload current chunk and merge the data into it using the pending_data field
     of the gridfile gfile object. We will flush the data if we fill in the chunk */
  if( buf_pos ) {
    if( !gfile->pending_len ) {      
      gridfile_load_pending_data_with_pos_chunk( gfile );           
    }
    buf_bytes_to_write = (int)( buf_pos + length > DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE - buf_pos : length );
    memcpy( &gfile->pending_data[buf_pos], data, buf_bytes_to_write);
    if ( buf_bytes_to_write + buf_pos > gfile->pending_len ) {
      gfile->pending_len = buf_bytes_to_write + buf_pos;
    }
    gfile->pos += buf_bytes_to_write;
    if( buf_bytes_to_write + buf_pos >= DEFAULT_CHUNK_SIZE ) {
      gridfile_flush_pendingchunk(gfile);
    }   
    bytes_left -= buf_bytes_to_write;
    data += buf_bytes_to_write;
  }

  /* If there's still more data to be written and they happen to be full chunks, we will loop thru and 
     write all full chunks without the need for preloading the existing chunk */
  while( bytes_left >= DEFAULT_CHUNK_SIZE ) {
    oChunk = chunk_new(gfile->id, gfile->chunk_num, &targetBuf, (void*)data, DEFAULT_CHUNK_SIZE, gfile->flags );
    memAllocated = targetBuf != data;
    gridfile_prepare_chunk_key_bson( &q, &gfile->id, gfile->chunk_num);
    mongo_update(gfile->gfs->client, gfile->gfs->chunks_ns, &q, oChunk, MONGO_UPDATE_UPSERT, NULL);
    bson_destroy( &q );
    chunk_free(oChunk);
    bytes_left -= DEFAULT_CHUNK_SIZE;
    gfile->chunk_num++;
    gfile->pos += DEFAULT_CHUNK_SIZE;
    if (gfile->pos > gfile->length) {
      gfile->length = gfile->pos;
    }
    data += DEFAULT_CHUNK_SIZE;
  }  

  /* Finally, if there's still remaining bytes left to write, we will preload the current chunk and merge the 
     remaining bytes into pending_data buffer */
  if ( bytes_left > 0 ) {
    if( gfile->pos + bytes_left < gfile->length ) {
      gridfile_load_pending_data_with_pos_chunk(gfile);
    }
    memcpy(gfile->pending_data, data, (size_t) bytes_left);
    if(  bytes_left > gfile->pending_len ) {
      gfile->pending_len = (int) bytes_left;
    }
    gfile->pos += bytes_left;  
  }

  if( memAllocated ){
    bson_free( targetBuf );
  }
}

MONGO_EXPORT void gridfile_get_chunk(gridfile *gfile, int n, bson *out) {
  bson query = INIT_BSON;

  bson_oid_t *id;
  int result;

  check_mongo_object( gfile );
  check_mongo_object( out );

  bson_init(&query);  
  id = gridfile_get_id( gfile );
  bson_append_oid(&query, "files_id", id);
  bson_append_int(&query, "n", n);
  bson_finish(&query);

  result = (mongo_find_one(gfile->gfs->client, gfile->gfs->chunks_ns,  &query, NULL, out) == MONGO_OK);
  bson_destroy(&query);
  if (!result) {
    bson empty = INIT_BSON;
    bson_empty(&empty);
    bson_copy(out, &empty);
  }
}

MONGO_EXPORT mongo_cursor *gridfile_get_chunks(gridfile *gfile, int start, int size) {
  bson_iterator it = INIT_ITERATOR;
  bson_oid_t id;
  bson gte = INIT_BSON;
  bson query = INIT_BSON;
  bson orderby = INIT_BSON;
  bson command = INIT_BSON;
  mongo_cursor *cursor;

  check_mongo_object( gfile );

  if( bson_find(&it, gfile->meta, "_id") != BSON_EOO) {
    id =  *bson_iterator_oid(&it);
  } else {
    id = gfile->id;
  }

  bson_init(&query);
  bson_append_oid(&query, "files_id", &id);
  if (size == 1) {
    bson_append_int(&query, "n", start);
  } else {
    bson_init(&gte);
    bson_append_int(&gte, "$gte", start);
    bson_finish(&gte);
    bson_append_bson(&query, "n", &gte);
    bson_destroy(&gte);
  }
  bson_finish(&query);

  bson_init(&orderby);
  bson_append_int(&orderby, "n", 1);
  bson_finish(&orderby);

  bson_init(&command);
  bson_append_bson(&command, "query", &query);
  bson_append_bson(&command, "orderby", &orderby);
  bson_finish(&command);

  cursor = mongo_find(gfile->gfs->client, gfile->gfs->chunks_ns,  &command, NULL, size, 0, 0);

  bson_destroy(&command);
  bson_destroy(&query);
  bson_destroy(&orderby);

  return cursor;
}

static gridfs_offset gridfile_read_from_pending_buffer(gridfile *gfile, gridfs_offset totalBytesToRead, void* buf, int *first_chunk);
static gridfs_offset gridfile_load_from_chunks(gridfile *gfile, int total_chunks, gridfs_offset chunksize, mongo_cursor *chunks, char* buf, 
                                               gridfs_offset bytes_left);

MONGO_EXPORT gridfs_offset gridfile_read(gridfile *gfile, gridfs_offset size, char *buf) {
  mongo_cursor *chunks;  

  int first_chunk;  
  int total_chunks;
  gridfs_offset chunksize;
  gridfs_offset contentlength;
  gridfs_offset bytes_left;
  gridfs_offset realSize = 0;

  check_mongo_object( gfile );
  
  contentlength = gridfile_get_contentlength(gfile);
  chunksize = gridfile_get_chunksize(gfile);
  size = (contentlength - gfile->pos < size) ? contentlength - gfile->pos: size;
  bytes_left = size;

  first_chunk = (int)((gfile->pos) / chunksize);  
  total_chunks = (int)((gfile->pos + size - 1) / chunksize) - first_chunk + 1;
  
  if( (realSize = gridfile_read_from_pending_buffer( gfile, bytes_left, buf, &first_chunk )) > 0 ) {
    gfile->pos += realSize;    
    if( --total_chunks <= 0) {
      return realSize;
    };
    buf += realSize;
    bytes_left -= realSize;
    gridfile_flush_pendingchunk( gfile ); 
  }; 

  chunks = gridfile_get_chunks(gfile, first_chunk, total_chunks);
  realSize += gridfile_load_from_chunks( gfile, total_chunks, chunksize, chunks, buf, bytes_left);  
  mongo_cursor_destroy(chunks);

  gfile->pos += realSize;

  return realSize;
}

static gridfs_offset gridfile_read_from_pending_buffer(gridfile *gfile, gridfs_offset totalBytesToRead, void* buf, 
                                                       int *first_chunk){
  gridfs_offset realSize = 0;
  if( gfile->pending_len > 0 && *first_chunk == gfile->chunk_num) {    
    char *chunk_data;
    gridfs_offset chunksize = gridfile_get_chunksize(gfile);
    gridfs_offset ofs = gfile->pos - gfile->chunk_num * chunksize;
    realSize = gfile->pending_len - ofs > totalBytesToRead ? totalBytesToRead : gfile->pending_len - ofs;
    chunk_data = gfile->pending_data + ofs;
    memcpy( buf, chunk_data, (size_t)realSize );                
    (*first_chunk)++; 
  }; 
  return realSize;     
}

static gridfs_offset gridfile_fill_buf_from_chunk(gridfile *gfile, bson *chunk, gridfs_offset chunksize, char **buf, int *allocatedMem, void **targetBuf, 
                                         size_t *targetBufLen, gridfs_offset *bytes_left, int chunkNo);

static gridfs_offset gridfile_load_from_chunks(gridfile *gfile, int total_chunks, gridfs_offset chunksize, mongo_cursor *chunks, char* buf, 
                                               gridfs_offset bytes_left){
  int i;
  void* targetBuf = NULL; 
  size_t targetBufLen = 0;
  int allocatedMem = 0;
  gridfs_offset realSize = 0;
  
  for (i = 0; i < total_chunks; i++) {
    if( mongo_cursor_next(chunks) != MONGO_OK ){
      break;
    }
    realSize += gridfile_fill_buf_from_chunk( gfile, &chunks->current, chunksize, &buf, &allocatedMem, &targetBuf, &targetBufLen, &bytes_left, i); 
  }
  if( allocatedMem ) {
    bson_free( targetBuf );
  }
  return realSize;
}

static gridfs_offset gridfile_fill_buf_from_chunk(gridfile *gfile, bson *chunk, gridfs_offset chunksize, char **buf, int *allocatedMem, void **targetBuf, 
                                                  size_t *targetBufLen, gridfs_offset *bytes_left, int chunkNo){
  bson_iterator it = INIT_ITERATOR;
  gridfs_offset chunk_len;
  char *chunk_data;

  bson_find(&it, chunk, "data");
  chunk_len = bson_iterator_bin_len(&it);
  chunk_data = (char*)bson_iterator_bin_data(&it);  
  postProcessChunk( targetBuf, targetBufLen, (void*)(chunk_data), (size_t)chunk_len, gfile->flags );  
  *allocatedMem = *targetBuf != chunk_data;
  chunk_data = (char*)(*targetBuf);
  if (chunkNo == 0) {      
    chunk_data += (gfile->pos) % chunksize;
    *targetBufLen -= (size_t)( (gfile->pos) % chunksize );
  } 
  if (*bytes_left > *targetBufLen) {
    memcpy(*buf, chunk_data, *targetBufLen);
    *bytes_left -= *targetBufLen;
    *buf += *targetBufLen;
    return *targetBufLen;
  } else {
    memcpy(*buf, chunk_data, (size_t)(*bytes_left));
    return *bytes_left;
  } 
}

MONGO_EXPORT gridfs_offset gridfile_seek(gridfile *gfile, gridfs_offset offset) {
  gridfs_offset length;
  gridfs_offset chunkSize;
  gridfs_offset newPos;

  check_mongo_object( gfile );

  chunkSize = gridfile_get_chunksize( gfile );
  length = gridfile_get_contentlength( gfile ); 
  newPos = length < offset ? length : offset;  

  /* If we are seeking to the next chunk or prior to the current chunks let's flush the pending chunk */
  if (gfile->pending_len && (newPos >= (gfile->chunk_num + 1) * chunkSize || newPos < gfile->chunk_num * chunkSize)) {
    gridfile_flush_pendingchunk( gfile );
  };  
  gfile->pos = newPos;
  return newPos;
}

MONGO_EXPORT gridfs_offset gridfile_write_file(gridfile *gfile, FILE *stream) {
  int i;
  size_t len;
  bson chunk = INIT_BSON;
  bson_iterator it = INIT_ITERATOR;
  const char *data = NULL;
  void* targetBuf = NULL; 
  size_t targetBufLen = 0;
  int num;

  check_mongo_object( gfile );

  num = gridfile_get_numchunks(gfile);

  for (i = 0; i < num; i++) {
    gridfile_get_chunk(gfile, i, &chunk);
    bson_find(&it, &chunk, "data");
    len = bson_iterator_bin_len(&it);
    data = bson_iterator_bin_data(&it);    
    postProcessChunk( &targetBuf, &targetBufLen, (void*)data, (size_t)len, gfile->flags );    
    fwrite(targetBuf, sizeof(char), targetBufLen, stream);
    bson_destroy(&chunk);
  }

  if( targetBuf && targetBuf != data ) {
    bson_free( targetBuf );
  }
  return gridfile_get_contentlength(gfile);
}

static void gridfile_remove_chunks( gridfile *gfile, int deleteFromChunk){
  bson q = INIT_BSON;

  check_mongo_object( gfile );

  bson_init( &q );
  bson_append_oid(&q, "files_id", gridfile_get_id( gfile ));
  if( deleteFromChunk >= 0 ) {
    bson_append_start_object( &q, "n" );
      bson_append_int( &q, "$gte", deleteFromChunk );
    bson_append_finish_object( &q );
  }
  bson_finish( &q );
  mongo_remove( gfile->gfs->client, gfile->gfs->chunks_ns, &q, NULL);
  bson_destroy( &q );
}

MONGO_EXPORT gridfs_offset gridfile_truncate(gridfile *gfile, gridfs_offset newSize) {
  int deleteFromChunk;

  check_mongo_object( gfile );

  if( newSize < 0 ) {
    newSize = 0;
  } else if ( newSize > gridfile_get_contentlength( gfile ) ) {
    return gridfile_seek( gfile, gridfile_get_contentlength( gfile ) );    
  }
  if( newSize > 0 ) {
    deleteFromChunk = (int)(newSize / gridfile_get_chunksize( gfile )); 
    gridfile_seek(gfile, newSize);    
    if( gfile->pos % gridfile_get_chunksize( gfile ) ) {
      if( !gfile->pending_len ) {
        gridfile_load_pending_data_with_pos_chunk( gfile );      
      };
      gfile->pending_len = gfile->pos % gridfile_get_chunksize( gfile ); /* This will truncate the preloaded chunk */
      gridfile_flush_pendingchunk( gfile );
      deleteFromChunk++;
    }
    /* Now let's remove the trailing chunks resulting from truncation */
    gridfile_remove_chunks( gfile, deleteFromChunk );
    gfile->length = newSize;
  } else {
    /* Expected file size is zero. We will remove ALL chunks */
    gridfile_remove_chunks( gfile, -1 );    
    gfile->length = 0;
    gfile->pos = 0;
  }
  return gfile->length;
}