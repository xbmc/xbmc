#ifndef PLAYER_THUMBNAIL_H
#define PLAYER_THUMBNAIL_H

#ifdef  __cplusplus
extern "C" {
#endif

void * thumbnail_res_alloc(void);
int thumbnail_find_stream_info(void *handle, const char* filename);
int thumbnail_find_stream_info_end(void *handle);
int thumbnail_decoder_open(void *handle, const char* filename);
int thumbnail_extract_video_frame(void * handle, int64_t time, int flag);
int thumbnail_read_frame(void *handle, char* buffer);
void thumbnail_get_video_size(void *handle, int* width, int* height);
float thumbnail_get_aspect_ratio(void *handle);
void thumbnail_get_duration(void *handle, int64_t *duration);
int thumbnail_get_key_metadata(void* handle, char* key, const char** value);
int thumbnail_get_key_data(void* handle, char* key, const void** data, int* data_size);
void thumbnail_get_video_rotation(void *handle, int* rotation);
int thumbnail_decoder_close(void *handle);
void thumbnail_res_free(void* handle);

#ifdef  __cplusplus
}
#endif

#endif
