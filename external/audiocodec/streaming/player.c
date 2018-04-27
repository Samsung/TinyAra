/******************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include "internal_defs.h"
#include "debug.h"
#include <audiocodec/streaming/player.h>


/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define PV_SUCCESS  0
#define PV_FAILURE  -1

#define PLAYER_DEBUG    audvdbg
#define PLAYER_ERROR    auddbg
#define PLAYER_ASSERT   MY_ASSERT

#define CHECK_AUDIO_TYPE(type) (AUDIO_TYPE_UNKNOWN < (type) && (type) < AUDIO_TYPE_MAX)

// Mask to extract the version, layer, sampling rate parts of the MP3 header,
// which should be same for all MP3 frames.
#define MP3_FRAME_HEADER_MASK 0xfffe0c00

/****************************************************************************
 * Private Declarations
 ****************************************************************************/

/**
 * @struct  priv_data_s
 * @brief   Player private data structure define.
 */
struct priv_data_s {
	ssize_t mCurrentPos;        /* read position when decoding */
	uint32_t mFixedHeader;      /* mp3 frame header */
};

typedef struct priv_data_s priv_data_t;
typedef struct priv_data_s *priv_data_p;


static uint32_t _u32_at(const uint8_t *ptr)
{
	return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

static bool _parse_header(uint32_t header, size_t *frame_size)
{
	*frame_size = 0;

	RETURN_VAL_IF_FAIL((header & 0xffe00000) == 0xffe00000, false);

	unsigned version = (header >> 19) & 3;
	RETURN_VAL_IF_FAIL(version != 0x01, false);

	unsigned layer = (header >> 17) & 3;

	RETURN_VAL_IF_FAIL(layer != 0x00, false);

	unsigned bitrate_index = (header >> 12) & 0x0f;

	if (bitrate_index == 0 || bitrate_index == 0x0f) {
		return false;           // Disallow "free" bitrate.
	}

	unsigned sampling_rate_index = (header >> 10) & 3;

	RETURN_VAL_IF_FAIL((sampling_rate_index != 3), false);

	static const int kSamplingRateV1[] = { 44100, 48000, 32000 };
	int sampling_rate = kSamplingRateV1[sampling_rate_index];
	if (version == 2 /* V2 */) {
		sampling_rate /= 2;
	} else if (version == 0 /* V2.5 */) {
		sampling_rate /= 4;
	}

	unsigned padding = (header >> 9) & 1;

	if (layer == 3) {
		// layer I
		static const int kBitrateV1[] = {
			32, 64, 96, 128, 160, 192, 224, 256,
			288, 320, 352, 384, 416, 448
		};

		static const int kBitrateV2[] = {
			32, 48, 56, 64, 80, 96, 112, 128,
			144, 160, 176, 192, 224, 256
		};

		int bitrate = (version == 3 /* V1 */)
					  ? kBitrateV1[bitrate_index - 1]
					  : kBitrateV2[bitrate_index - 1];

		*frame_size = (12000 * bitrate / sampling_rate + padding) * 4;
	} else {
		// layer II or III
		static const int kBitrateV1L2[] = {
			32, 48, 56, 64, 80, 96, 112, 128,
			160, 192, 224, 256, 320, 384
		};

		static const int kBitrateV1L3[] = {
			32, 40, 48, 56, 64, 80, 96, 112,
			128, 160, 192, 224, 256, 320
		};

		static const int kBitrateV2[] = {
			8, 16, 24, 32, 40, 48, 56, 64,
			80, 96, 112, 128, 144, 160
		};

		int bitrate;
		if (version == 3 /* V1 */) {
			bitrate = (layer == 2 /* L2 */)
					  ? kBitrateV1L2[bitrate_index - 1]
					  : kBitrateV1L3[bitrate_index - 1];
		} else {
			// V2 (or 2.5)
			bitrate = kBitrateV2[bitrate_index - 1];
		}

		if (version == 3 /* V1 */) {
			*frame_size = 144000 * bitrate / sampling_rate + padding;
		} else {
			// V2 or V2.5
			size_t tmp = (layer == 1 /* L3 */) ? 72000 : 144000;
			*frame_size = tmp * bitrate / sampling_rate + padding;
		}
	}

	return true;
}

static ssize_t _source_read_at(rbstream_p fp, ssize_t offset, void *data, size_t size)
{
	int retVal = rbs_seek(fp, offset, SEEK_SET);
	RETURN_VAL_IF_FAIL((retVal == 0), 0);

	return rbs_read(data, 1, size, fp);
}

// Resync to next valid MP3 frame in the file.
static bool mp3_resync(rbstream_p fp, uint32_t match_header, ssize_t *inout_pos, uint32_t *out_header)
{
	PLAYER_DEBUG("[%s] Line %d, match_header %#x, *pos %d\n", __FUNCTION__, __LINE__, match_header, *inout_pos);

	if (*inout_pos == 0) {
		// Skip an optional ID3 header if syncing at the very beginning of the datasource.
		for (;;) {
			uint8_t id3header[10];
			int retVal = _source_read_at(fp, *inout_pos, id3header, sizeof(id3header));
			RETURN_VAL_IF_FAIL((retVal == (ssize_t) sizeof(id3header)), false);

			if (memcmp("ID3", id3header, 3)) {
				break;
			}
			// Skip the ID3v2 header.
			size_t len = ((id3header[6] & 0x7f) << 21)
						 | ((id3header[7] & 0x7f) << 14)
						 | ((id3header[8] & 0x7f) << 7)
						 | (id3header[9] & 0x7f);

			len += 10;

			*inout_pos += len;
		}
	}

	ssize_t pos = *inout_pos;
	bool valid = false;

	const int32_t kMaxReadBytes = 1024;
	const int32_t kMaxBytesChecked = 8 * 1024;  //128 * 1024;
	uint8_t buf[kMaxReadBytes];
	ssize_t bytesToRead = kMaxReadBytes;
	ssize_t totalBytesRead = 0;
	ssize_t remainingBytes = 0;
	bool reachEOS = false;
	uint8_t *tmp = buf;

	do {
		if (pos >= *inout_pos + kMaxBytesChecked) {
			PLAYER_DEBUG("[%s] Line %d, resync range < kMaxBytesChecked %d\n", __FUNCTION__, __LINE__, kMaxBytesChecked);
			break;              // Don't scan forever.
		}

		if (remainingBytes < 4) {
			if (reachEOS) {
				break;
			}

			memcpy(buf, tmp, remainingBytes);
			bytesToRead = kMaxReadBytes - remainingBytes;

			/*
			 * The next read position should start from the end of
			 * the last buffer, and thus should include the remaining
			 * bytes in the buffer.
			 */
			totalBytesRead = _source_read_at(fp, pos + remainingBytes, buf + remainingBytes, bytesToRead);

			if (totalBytesRead <= 0) {
				break;
			}

			reachEOS = (totalBytesRead != bytesToRead);
			remainingBytes += totalBytesRead;
			tmp = buf;
			continue;
		}

		uint32_t header = _u32_at(tmp);

		if (match_header != 0 && (header & MP3_FRAME_HEADER_MASK) != (match_header & MP3_FRAME_HEADER_MASK)) {
			++pos;
			++tmp;
			--remainingBytes;
			continue;
		}

		size_t frame_size;
		if (!_parse_header(header, &frame_size)) {
			++pos;
			++tmp;
			--remainingBytes;
			continue;
		}

		// We found what looks like a valid frame,
		// now find its successors.
		ssize_t test_pos = pos + frame_size;
		PLAYER_DEBUG("[%s] Line %d, valid frame at pos %#x + framesize %#x = %#x\n", __FUNCTION__, __LINE__, pos, frame_size, test_pos);
		valid = true;
		const int FRAME_MATCH_REQUIRED = 2;
		for (int j = 0; j < FRAME_MATCH_REQUIRED; ++j) {
			uint8_t temp[4];
			ssize_t retval = _source_read_at(fp, test_pos, temp, sizeof(temp));
			if (retval < (ssize_t) sizeof(temp)) {
				valid = false;
				break;
			}

			uint32_t test_header = _u32_at(temp);

			if ((test_header & MP3_FRAME_HEADER_MASK) != (header & MP3_FRAME_HEADER_MASK)) {
				PLAYER_DEBUG("[%s] Line %d, invalid frame at pos1 %#x\n", __FUNCTION__, __LINE__, test_pos);
				valid = false;
				break;
			}

			size_t test_frame_size;
			if (!_parse_header(test_header, &test_frame_size)) {
				PLAYER_DEBUG("[%s] Line %d, invalid frame at pos2 %#x\n", __FUNCTION__, __LINE__, test_pos);
				valid = false;
				break;
			}

			PLAYER_DEBUG("[%s] Line %d, valid frame at pos %#x + framesize %#x = %#x\n", __FUNCTION__, __LINE__, test_pos, test_frame_size, test_pos + test_frame_size);
			test_pos += test_frame_size;
		}

		if (valid) {
			*inout_pos = pos;

			if (out_header != NULL) {
				*out_header = header;
			}

			PLAYER_DEBUG("[%s] Line %d, find header %#x at pos %d(%#x)\n", __FUNCTION__, __LINE__, header, pos, pos);
		}

		++pos;
		++tmp;
		--remainingBytes;
	} while (!valid);

	return valid;
}

// Initialize the MP3 reader.
bool mp3_init(rbstream_p mFp, ssize_t *offset, uint32_t *header)
{
	// Sync to the first valid frame.
	bool success = mp3_resync(mFp, 0, offset, header);
	RETURN_VAL_IF_FAIL((success == true), false);

	// Policy: Pop out data when *offset updated!
	rbs_seek_ext(mFp, *offset, SEEK_SET);

	size_t frame_size;
	return _parse_header(*header, &frame_size);
}

// Get the next valid MP3 frame.
bool mp3_get_frame(rbstream_p mFp, ssize_t *offset, uint32_t fixed_header, void *buffer, uint32_t *size)
{
	size_t frame_size;

	for (;;) {
		ssize_t n = _source_read_at(mFp, *offset, buffer, 4);
		RETURN_VAL_IF_FAIL((n == 4), false);

		uint32_t header = _u32_at((const uint8_t *)buffer);

		if ((header & MP3_FRAME_HEADER_MASK) == (fixed_header & MP3_FRAME_HEADER_MASK)
			&& _parse_header(header, &frame_size)) {
			break;
		}

		// Lost sync.
		ssize_t pos = *offset;
		if (!mp3_resync(mFp, fixed_header, &pos, NULL /*out_header */)) {
			// Unable to mp3_resync. Signalling end of stream.
			return false;
		}

		*offset = pos;
		// Policy: Pop out data when mCurrentPos updated!
		rbs_seek_ext(mFp, *offset, SEEK_SET);

		// Try again with the new position.
	}

	ssize_t n = _source_read_at(mFp, *offset, buffer, frame_size);
	RETURN_VAL_IF_FAIL((n == (ssize_t) frame_size), false);

	PLAYER_DEBUG("[%s] Line %d, pos %#x, framesize %#x\n", __FUNCTION__, __LINE__, *offset, frame_size);

	*size = frame_size;
	*offset += frame_size;
	// Policy: Pop out data when mCurrentPos updated!
	rbs_seek_ext(mFp, *offset, SEEK_SET);

	return true;
}

bool mp3_check_type(rbstream_p rbsp)
{
	bool result = false;
	uint8_t id3header[10];

	int retVal = _source_read_at(rbsp, 0, id3header, sizeof(id3header));
	RETURN_VAL_IF_FAIL((retVal == (ssize_t) sizeof(id3header)), false);

	if (0 == memcmp("ID3", id3header, 3)) {
		return true;
	}

	int value = rbs_ctrl(rbsp, OPTION_ALLOW_TO_DEQUEUE, 0);
	ssize_t pos = 0;
	result = mp3_resync(rbsp, 0, &pos, NULL);
	rbs_ctrl(rbsp, OPTION_ALLOW_TO_DEQUEUE, value);

	return result;
}

// Resync to next valid MP3 frame in the file.
static bool aac_resync(rbstream_p fp, ssize_t *inout_pos)
{
	ssize_t pos = *inout_pos;
	bool valid = false;

	const int32_t kMaxReadBytes = 1024;
	const int32_t kMaxBytesChecked = 4 * 1024;  //128 * 1024;
	uint8_t buf[kMaxReadBytes];
	ssize_t bytesToRead = kMaxReadBytes;
	ssize_t totalBytesRead = 0;
	ssize_t remainingBytes = 0;
	bool reachEOS = false;
	uint8_t *tmp = buf;

	do {
		if (pos >= *inout_pos + kMaxBytesChecked) {
			break;              // Don't scan forever.
		}

		if (remainingBytes < 6) {
			// syncword... frame length, 6 bytes in total
			if (reachEOS) {
				break;
			}

			memcpy(buf, tmp, remainingBytes);
			bytesToRead = kMaxReadBytes - remainingBytes;

			/*
			 * The next read position should start from the end of
			 * the last buffer, and thus should include the remaining
			 * bytes in the buffer.
			 */
			totalBytesRead = _source_read_at(fp, pos + remainingBytes, buf + remainingBytes, bytesToRead);

			if (totalBytesRead <= 0) {
				break;
			}

			reachEOS = (totalBytesRead != bytesToRead);
			remainingBytes += totalBytesRead;
			tmp = buf;
			continue;
		}

		if ((tmp[0] != 0xff) || ((tmp[1] & 0xf6) != 0xf0)) {
			++pos;
			++tmp;
			--remainingBytes;
			continue;
		}
		//int protectionAbsent = tmp[1] & 0x01;
		int frame_size = (tmp[3] & 0x03) << 11 | tmp[4] << 3 | tmp[5] >> 5;
		//int headerSize = protectionAbsent ? 7 : 9;

		// We found what looks like a valid frame,
		// now find its successors.

		ssize_t test_pos = pos + frame_size;

		valid = true;
		const int FRAME_MATCH_REQUIRED = 3;
		for (int j = 0; j < FRAME_MATCH_REQUIRED; ++j) {
			uint8_t temp[6];
			ssize_t retval = _source_read_at(fp, test_pos, temp, sizeof(temp));
			if (retval < (ssize_t) sizeof(temp)) {
				valid = false;
				break;
			}

			if ((temp[0] != 0xff) || ((temp[1] & 0xf6) != 0xf0)) {
				valid = false;
				break;
			}

			int test_frame_size = (temp[3] & 0x03) << 11 | temp[4] << 3 | temp[5] >> 5;

			test_pos += test_frame_size;
		}

		if (valid) {
			*inout_pos = pos;
		}

		++pos;
		++tmp;
		--remainingBytes;
	} while (!valid);

	return valid;
}

// Initialize the aac reader.
bool aac_init(rbstream_p mFp, ssize_t *offset)
{
	// Sync to the first valid frame.
	bool success = aac_resync(mFp, offset);
	RETURN_VAL_IF_FAIL((success == true), false);

	// Policy: Pop out data when *offset updated!
	rbs_seek_ext(mFp, *offset, SEEK_SET);
	return true;
}

// Get the next valid aac frame.
bool aac_get_frame(rbstream_p mFp, ssize_t *offset, void *buffer, uint32_t *size)
{
	size_t frame_size = 0;
	uint8_t *buf = (uint8_t *) buffer;

	for (;;) {
		ssize_t n = _source_read_at(mFp, *offset, buffer, 6);
		RETURN_VAL_IF_FAIL((n == 6), false);

		if ((buf[0] == 0xff) && ((buf[1] & 0xf6) == 0xf0)) {
			//int protectionAbsent = buffer[1] & 0x01;
			frame_size = (buf[3] & 0x03) << 11 | buf[4] << 3 | buf[5] >> 5;
			//int headerSize = protectionAbsent ? 7 : 9;
			break;
		}
		// Lost sync.
		ssize_t pos = *offset;
		RETURN_VAL_IF_FAIL(aac_resync(mFp, &pos), false);

		*offset = pos;
		rbs_seek_ext(mFp, *offset, SEEK_SET);
		// Try again with the new position.
	}

	ssize_t n = _source_read_at(mFp, *offset, buffer, frame_size);
	RETURN_VAL_IF_FAIL((n == (ssize_t) frame_size), false);

	*size = frame_size;
	*offset += frame_size;
	rbs_seek_ext(mFp, *offset, SEEK_SET);

	return true;
}

bool aac_check_type(rbstream_p rbsp)
{
	bool result = false;
	uint8_t syncword[4];

	ssize_t rlen = _source_read_at(rbsp, 0, syncword, sizeof(syncword));
	RETURN_VAL_IF_FAIL((rlen == (ssize_t) sizeof(syncword)), false);

	RETURN_VAL_IF_FAIL((0 != memcmp("ADIF", syncword, 4)), false);  // not support ADIF

	int value = rbs_ctrl(rbsp, OPTION_ALLOW_TO_DEQUEUE, 0);
	ssize_t pos = 0;
	result = aac_resync(rbsp, &pos);
	rbs_ctrl(rbsp, OPTION_ALLOW_TO_DEQUEUE, value);

	return result;
}

int _get_audio_type(rbstream_p rbsp)
{
	if (mp3_check_type(rbsp)) {
		return AUDIO_TYPE_MP3;
	}

	if (aac_check_type(rbsp)) {
		return AUDIO_TYPE_AAC;
	}

	return type_unknown;
}

bool _get_frame(pv_player_p player)
{
	priv_data_p priv = (priv_data_p) player->priv_data;
	PLAYER_ASSERT(priv != NULL);

	if (player->audio_type == AUDIO_TYPE_MP3) {
		tPVMP3DecoderExternal *mp3_ext = (tPVMP3DecoderExternal *) player->dec_ext;
		return mp3_get_frame(player->rbsp, &priv->mCurrentPos, priv->mFixedHeader, (void *)mp3_ext->pInputBuffer, (uint32_t *)&mp3_ext->inputBufferCurrentLength);
	} else if (player->audio_type == AUDIO_TYPE_AAC) {
		tPVMP4AudioDecoderExternal *aac_ext = (tPVMP4AudioDecoderExternal *) player->dec_ext;
		return aac_get_frame(player->rbsp, &priv->mCurrentPos, (void *)aac_ext->pInputBuffer, (uint32_t *)&aac_ext->inputBufferCurrentLength);
	}

	PLAYER_DEBUG("[%s] unsupported audio type: %d\n", __FUNCTION__, player->audio_type);
	return false;
}

int _init_decoder(pv_player_p player)
{
	priv_data_p priv = (priv_data_p) player->priv_data;
	PLAYER_ASSERT(priv != NULL);

	if (player->audio_type == AUDIO_TYPE_MP3) {
		player->dec_ext = calloc(1, sizeof(tPVMP3DecoderExternal));
		RETURN_VAL_IF_FAIL((player->dec_ext != NULL), PV_FAILURE);

		player->dec_mem = calloc(1, pvmp3_decoderMemRequirements());
		RETURN_VAL_IF_FAIL((player->dec_mem != NULL), PV_FAILURE);

		player->config_func(player->cb_data, player->audio_type, player->dec_ext);

		pvmp3_InitDecoder(player->dec_ext, player->dec_mem);

		priv->mCurrentPos = 0;
		bool ret = mp3_init(player->rbsp, &priv->mCurrentPos, &priv->mFixedHeader);
		RETURN_VAL_IF_FAIL((ret == true), PV_FAILURE);
	} else if (player->audio_type == AUDIO_TYPE_AAC) {
		player->dec_ext = calloc(1, sizeof(tPVMP4AudioDecoderExternal));
		RETURN_VAL_IF_FAIL((player->dec_ext != NULL), PV_FAILURE);

		player->dec_mem = calloc(1, PVMP4AudioDecoderGetMemRequirements());
		RETURN_VAL_IF_FAIL((player->dec_mem != NULL), PV_FAILURE);

		player->config_func(player->cb_data, player->audio_type, player->dec_ext);

		PVMP4AudioDecoderResetBuffer(player->dec_mem);
		Int err = PVMP4AudioDecoderInitLibrary(player->dec_ext, player->dec_mem);
		RETURN_VAL_IF_FAIL((err == MP4AUDEC_SUCCESS), PV_FAILURE);

		priv->mCurrentPos = 0;
		bool ret = aac_init(player->rbsp, &priv->mCurrentPos);
		RETURN_VAL_IF_FAIL((ret == true), PV_FAILURE);
	}

	return PV_SUCCESS;
}

int _frame_decoder(pv_player_p player, pcm_data_p pcm)
{
	//priv_data_p priv = (priv_data_p) player->priv_data;
	//PLAYER_ASSERT(priv != NULL);

	if (player->audio_type == AUDIO_TYPE_MP3) {
		tPVMP3DecoderExternal tmp_ext = *((tPVMP3DecoderExternal *) player->dec_ext);
		tPVMP3DecoderExternal *mp3_ext = &tmp_ext;

		mp3_ext->inputBufferUsedLength = 0;

		ERROR_CODE errorCode = pvmp3_framedecoder(mp3_ext, player->dec_mem);
		RETURN_VAL_IF_FAIL((errorCode == NO_DECODING_ERROR), PV_FAILURE);

		pcm->length = mp3_ext->outputFrameSize;
		pcm->samples = mp3_ext->pOutputBuffer;
		pcm->channels = mp3_ext->num_channels;
		pcm->samplerate = mp3_ext->samplingRate;
		return PV_SUCCESS;
	} else if (player->audio_type == AUDIO_TYPE_AAC) {
		tPVMP4AudioDecoderExternal *aac_ext = (tPVMP4AudioDecoderExternal *) player->dec_ext;

		aac_ext->inputBufferUsedLength = 0;
		aac_ext->remainderBits = 0;

		Int decoderErr = PVMP4AudioDecodeFrame(aac_ext, player->dec_mem);
		RETURN_VAL_IF_FAIL((decoderErr == MP4AUDEC_SUCCESS), PV_FAILURE);

		pcm->length = aac_ext->frameLength * aac_ext->desiredChannels;
		pcm->samples = aac_ext->pOutputBuffer;
		pcm->channels = aac_ext->desiredChannels;
		pcm->samplerate = aac_ext->samplingRate;
		return PV_SUCCESS;
	}

	return PV_FAILURE;
}

static size_t _input_callback(void *data, rbstream_p rbsp)
{
	pv_player_p player = (pv_player_p) data;
	PLAYER_ASSERT(player != NULL);

	size_t wlen = 0;
	RETURN_VAL_IF_FAIL((player->input_func != NULL), wlen);
	wlen = player->input_func(player->cb_data, player);

	return wlen;
}

size_t pv_player_pushdata(pv_player_p player, const void *data, size_t len)
{
	PLAYER_ASSERT(player != NULL);
	PLAYER_ASSERT(data != NULL);

	static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&s_mutex);
	len = rbs_write(data, 1, len, player->rbsp);
	pthread_mutex_unlock(&s_mutex);

	return len;
}

size_t pv_player_dataspace(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);
	return rb_avail(&player->ringbuffer);
}

bool pv_player_dataspace_is_empty(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);
	return !rb_used(&player->ringbuffer);
}

int pv_player_get_audio_type(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);

	if (!CHECK_AUDIO_TYPE(player->audio_type)) {
		player->audio_type = _get_audio_type(player->rbsp);
	}

	return player->audio_type;
}

int pv_player_init_decoder(pv_player_p player, int audio_type)
{
	PLAYER_ASSERT(player != NULL);

	// If user did not tell the audio type, then try to get it from stream.
	if (!CHECK_AUDIO_TYPE(player->audio_type)) {
		PLAYER_ASSERT(player->rbsp != NULL);
		player->audio_type = _get_audio_type(player->rbsp);
	}

	PLAYER_DEBUG("audio_type %d\n", player->audio_type);

	int ret = _init_decoder(player);
	RETURN_VAL_IF_FAIL((ret == PV_SUCCESS), PV_FAILURE);
}

bool pv_player_get_frame(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);

	return _get_frame(pv_player_p player);
}

int pv_player_frame_decode(pv_player_p player, pcm_data_p pcm)
{
	PLAYER_ASSERT(player != NULL);
	PLAYER_ASSERT(pcm != NULL);

	return _frame_decoder(pv_player_p player, pcm_data_p pcm);
}

int pv_player_init(pv_player_p player, size_t rbuf_size, void *user_data, config_func_f config_func, input_func_f input_func, output_func_f output_func)
{
	PLAYER_ASSERT(player != NULL);

	priv_data_p priv = (priv_data_p) malloc(sizeof(priv_data_t));
	RETURN_VAL_IF_FAIL((priv != NULL), PV_FAILURE);

	// init private data
	priv->mCurrentPos = 0;
	priv->mFixedHeader = 0;

	// init player data
	player->cb_data = user_data;
	player->config_func = config_func;
	player->input_func = input_func;
	player->output_func = output_func;

	player->dec_ext = NULL;
	player->dec_mem = NULL;
	player->priv_data = priv;

	// init ring-buffer and open it as a stream
	rb_init(&player->ringbuffer, rbuf_size);
	player->rbsp = rbs_open(&player->ringbuffer, _input_callback, (void *)player);
	RETURN_VAL_IF_FAIL((player->rbsp != NULL), PV_FAILURE);

	rbs_ctrl(player->rbsp, OPTION_ALLOW_TO_DEQUEUE, 1);
	return PV_SUCCESS;
}

int pv_player_finish(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);

	// close stream
	rbs_close(player->rbsp);
	player->rbsp = NULL;

	// free ring-buffer instance
	rb_free(&player->ringbuffer);

	// free decoder external buffer
	//if (player->dec_ext != NULL)
	{
		free(player->dec_ext);
		player->dec_ext = NULL;
	}

	// free decoder buffer
	//if (player->dec_mem != NULL)
	{
		free(player->dec_mem);
		player->dec_mem = NULL;
	}

	// free private data buffer
	//if (player->priv_data != NULL)
	{
		free(player->priv_data);
		player->priv_data = NULL;
	}

	return PV_SUCCESS;
}

int pv_player_run(pv_player_p player)
{
	PLAYER_ASSERT(player != NULL);

	RETURN_VAL_IF_FAIL((player->input_func != NULL), PV_FAILURE);
	RETURN_VAL_IF_FAIL((player->output_func != NULL), PV_FAILURE);
	RETURN_VAL_IF_FAIL((player->config_func != NULL), PV_FAILURE);
	RETURN_VAL_IF_FAIL((player->rbsp != NULL), PV_FAILURE);

	player->audio_type = pv_player_get_audio_type(player->rbsp);
	RETURN_VAL_IF_FAIL(CHECK_AUDIO_TYPE(player->audio_type), PV_FAILURE);

	int ret = pv_player_init_decoder(player, player->audio_type);
	RETURN_VAL_IF_FAIL((ret == PV_SUCCESS), PV_FAILURE);

	while (pv_player_get_frame(player)) {
		pcm_data_t pcm;
		if (PV_SUCCESS == pv_player_frame_decode(player, &pcm)) {
			player->output_func(player->cb_data, player, &pcm);
		}
	}

	return PV_SUCCESS;
}

