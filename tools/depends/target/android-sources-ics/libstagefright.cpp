/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define STAGEFRIGHT_EXPORT __attribute__ ((visibility ("default")))
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <utils/List.h>
#include <utils/RefBase.h>

#include <media/stagefright/MediaDefs.h>

namespace android {

const char *MEDIA_MIMETYPE_IMAGE_JPEG = "image/jpeg";

const char *MEDIA_MIMETYPE_VIDEO_VPX = "video/x-vnd.on2.vp8";
const char *MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
const char *MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
const char *MEDIA_MIMETYPE_VIDEO_H263 = "video/3gpp";
const char *MEDIA_MIMETYPE_VIDEO_MPEG2 = "video/mpeg2";
const char *MEDIA_MIMETYPE_VIDEO_RAW = "video/raw";

const char *MEDIA_MIMETYPE_AUDIO_AMR_NB = "audio/3gpp";
const char *MEDIA_MIMETYPE_AUDIO_AMR_WB = "audio/amr-wb";
const char *MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_I = "audio/mpeg-L1";
const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II = "audio/mpeg-L2";
const char *MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
const char *MEDIA_MIMETYPE_AUDIO_QCELP = "audio/qcelp";
const char *MEDIA_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
const char *MEDIA_MIMETYPE_AUDIO_G711_ALAW = "audio/g711-alaw";
const char *MEDIA_MIMETYPE_AUDIO_G711_MLAW = "audio/g711-mlaw";
const char *MEDIA_MIMETYPE_AUDIO_RAW = "audio/raw";
const char *MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
const char *MEDIA_MIMETYPE_AUDIO_AAC_ADTS = "audio/aac-adts";

const char *MEDIA_MIMETYPE_CONTAINER_MPEG4 = "video/mpeg4";
const char *MEDIA_MIMETYPE_CONTAINER_WAV = "audio/wav";
const char *MEDIA_MIMETYPE_CONTAINER_OGG = "application/ogg";
const char *MEDIA_MIMETYPE_CONTAINER_MATROSKA = "video/x-matroska";
const char *MEDIA_MIMETYPE_CONTAINER_MPEG2TS = "video/mp2ts";
const char *MEDIA_MIMETYPE_CONTAINER_AVI = "video/avi";
const char *MEDIA_MIMETYPE_CONTAINER_MPEG2PS = "video/mp2p";

const char *MEDIA_MIMETYPE_CONTAINER_WVM = "video/wvm";
#ifdef QCOM_HARDWARE
const char *MEDIA_MIMETYPE_AUDIO_EVRC = "audio/evrc";

const char *MEDIA_MIMETYPE_VIDEO_WMV = "video/x-ms-wmv";
const char *MEDIA_MIMETYPE_AUDIO_WMA = "audio/x-ms-wma";
const char *MEDIA_MIMETYPE_CONTAINER_ASF = "video/x-ms-asf";
const char *MEDIA_MIMETYPE_VIDEO_DIVX = "video/divx";
const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
const char *MEDIA_MIMETYPE_CONTAINER_AAC = "audio/aac";
const char *MEDIA_MIMETYPE_CONTAINER_QCP = "audio/vnd.qcelp";
const char *MEDIA_MIMETYPE_VIDEO_DIVX311 = "video/divx311";
const char *MEDIA_MIMETYPE_VIDEO_DIVX4 = "video/divx4";
const char *MEDIA_MIMETYPE_CONTAINER_MPEG2 = "video/mp2";
const char *MEDIA_MIMETYPE_CONTAINER_3G2 = "video/3g2";
#endif
const char *MEDIA_MIMETYPE_TEXT_3GPP = "text/3gpp-tt";

MetaData::MetaData() {
}

MetaData::MetaData(const MetaData &from) {
}

MetaData::~MetaData() {
}

void MetaData::clear() {
}

bool MetaData::remove(uint32_t key) {
  return false;
}

bool MetaData::setCString(uint32_t key, const char *value) {
	return false;
}

bool MetaData::setInt32(uint32_t key, int32_t value) {
  return false;
}

bool MetaData::setInt64(uint32_t key, int64_t value) {
  return false;
}

bool MetaData::setFloat(uint32_t key, float value) {
  return false;
}

bool MetaData::setPointer(uint32_t key, void *value) {
  return false;
}

bool MetaData::setRect(
        uint32_t key,
        int32_t left, int32_t top,
        int32_t right, int32_t bottom) {
  return false;
}

bool MetaData::findCString(uint32_t key, const char **value) {
  return false;
}

bool MetaData::findInt32(uint32_t key, int32_t *value) {
  return false;
}

bool MetaData::findInt64(uint32_t key, int64_t *value) {
  return false;
}

bool MetaData::findFloat(uint32_t key, float *value) {
  return false;
}

bool MetaData::findPointer(uint32_t key, void **value) {
  return false;
}

bool MetaData::findRect(
        uint32_t key,
        int32_t *left, int32_t *top,
        int32_t *right, int32_t *bottom) {
  return false;
}

bool MetaData::setData(
        uint32_t key, uint32_t type, const void *data, size_t size) {
  return false;
}

bool MetaData::findData(uint32_t key, uint32_t *type,
                        const void **data, size_t *size) const {
  return false;
}

MetaData::typed_data::typed_data()
    : mType(0),
      mSize(0) {
}

MetaData::typed_data::~typed_data() {
}

MetaData::typed_data::typed_data(const typed_data &from)
    : mType(from.mType),
      mSize(0) {
}

MetaData::typed_data &MetaData::typed_data::operator=(
        const MetaData::typed_data &from) {
    return *this;
}

void MetaData::typed_data::clear() {
}

void MetaData::typed_data::setData(
        uint32_t type, const void *data, size_t size) {
}

void MetaData::typed_data::getData(
        uint32_t *type, const void **data, size_t *size) const {
}

void MetaData::typed_data::allocateStorage(size_t size) {
}

void MetaData::typed_data::freeStorage() {
}

MediaBuffer::MediaBuffer(void *data, size_t size)
{}

MediaBuffer::MediaBuffer(size_t size)
{}

MediaBuffer::MediaBuffer(const sp<GraphicBuffer>& graphicBuffer)
{}

MediaBuffer::MediaBuffer(const sp<ABuffer> &buffer)
{}

void MediaBuffer::release() {
}

void MediaBuffer::claim() {
}

void MediaBuffer::add_ref() {
}

void *MediaBuffer::data() const {
  return 0;
}

size_t MediaBuffer::size() const {
  return 0;
}

size_t MediaBuffer::range_offset() const {
  return 0;
}

size_t MediaBuffer::range_length() const {
  return 0;
}

void MediaBuffer::set_range(size_t offset, size_t length) {
}

sp<GraphicBuffer> MediaBuffer::graphicBuffer() const {
}

sp<MetaData> MediaBuffer::meta_data() {
}

void MediaBuffer::reset() {
}

MediaBuffer::~MediaBuffer() {
}

void MediaBuffer::setObserver(MediaBufferObserver *observer) {
}

void MediaBuffer::setNextBuffer(MediaBuffer *buffer) {
}

MediaBuffer *MediaBuffer::nextBuffer() {
  return 0;
}

int MediaBuffer::refcount() const {
  return 0;
}

MediaBuffer *MediaBuffer::clone() {
  return 0;
}

MediaBufferGroup::MediaBufferGroup() {}

MediaBufferGroup::~MediaBufferGroup() {}

void MediaBufferGroup::add_buffer(MediaBuffer *buffer) {
}

status_t MediaBufferGroup::acquire_buffer(MediaBuffer **out) {
    return OK;
}

void MediaBufferGroup::signalBufferReturned(MediaBuffer *) {
}

MediaSource::MediaSource() {}

MediaSource::~MediaSource() {}

MediaSource::MediaSource(const MediaSource &) {}

MediaSource& MediaSource::operator=(const MediaSource &) {}

MediaSource::ReadOptions::ReadOptions() {
}

void MediaSource::ReadOptions::reset() {
}

void MediaSource::ReadOptions::setSeekTo(int64_t time_us, SeekMode mode) {
}

void MediaSource::ReadOptions::clearSeekTo() {
}

bool MediaSource::ReadOptions::getSeekTo(
        int64_t *time_us, SeekMode *mode) const {
    return false;
}

void MediaSource::ReadOptions::setLateBy(int64_t lateness_us) {
}

int64_t MediaSource::ReadOptions::getLateBy() const {
    return 0;
}

status_t
DataSource::getSize(off64_t *size)
{
  return 0;
}

String8
DataSource::getMIMEType() const
{
  return String8();
}

void
DataSource::RegisterDefaultSniffers()
{
}

sp<MediaExtractor>
MediaExtractor::Create(const sp<DataSource> &source, const char *mime)
{
  return 0;
}

sp<MediaSource>
OMXCodec::Create(
            const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName,
            uint32_t flags,
            const sp<ANativeWindow> &nativeWindow)
{
  return 0;
}

OMXClient::OMXClient()
{
}

status_t OMXClient::connect()
{
  return OK;
}

void OMXClient::disconnect()
{
}

class __attribute__ ((visibility ("default"))) UnknownDataSource : public DataSource {
public:
UnknownDataSource();

virtual status_t initCheck() const { return 0; }
virtual ssize_t readAt(off64_t offset, void *data, size_t size) { return 0; }
virtual status_t getSize(off64_t *size) { return 0; }

virtual ~UnknownDataSource() { }
};

UnknownDataSource foo;

UnknownDataSource::UnknownDataSource() { }
}
