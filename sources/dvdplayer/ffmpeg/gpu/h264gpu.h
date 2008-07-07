#include "../libavcodec/h264.h"
#include "gpu_utils.h"
#include "../libavcodec/h264data.h"
#include "../libavutil/common.h"

void gpu_init(H264Context *h);
void gpu_motion(H264Context *h);
