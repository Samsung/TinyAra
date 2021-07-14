/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <iostream>
#include "AudioUtils.h"

// Mine-Type for audio stream
static const std::string AAC_MIME_TYPE = "audio/aac";
static const std::string AACP_MIME_TYPE = "audio/aacp";
static const std::string MPEG_MIME_TYPE = "audio/mpeg";
static const std::string MP4_MIME_TYPE = "audio/mp4";
static const std::string OPUS_MIME_TYPE = "audio/opus";

// Sample Rate(in Hz) tables
static const int sampleRateTable[3][3] = {
	{44100, 48000, 32000},
	{22050, 24000, 16000},
	{11025, 12000, 8000},
};
// Bit Rate (in kbps) tables
// V1 - MPEG 1, V2 - MPEG 2 and MPEG 2.5
// L1 - Layer 1, L2 - Layer 2, L3 - Layer 3
static const int bitRateTable[3][3][14] = {
	{
		{32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}, /* MPEG 1, Layer 1 */
		{32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},	/* MPEG 1, Layer 2 */
		{32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},	 /* MPEG 1, Layer 3 */
	},
	{
		{32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}, /* MPEG 2, Layer 1 */
		{8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},	  /* MPEG 2, Layer 2 */
		{8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},	  /* MPEG 2, Layer 3 */
	},
	{
		{32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}, /* MPEG 2.5, Layer 1 */
		{8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},	  /* MPEG 2.5, Layer 2 */
		{8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},	  /* MPEG 2.5, Layer 3 */
	},
};

void ARAPS_LOG(int level, const char *filename, unsigned int line, const char *function, const char *pf_format, ...)
{
	int nBuf;
	char szPrint[128];
	char szBuffer[512];
	va_list args;
	struct timeval stCurTime;
	struct tm stTime;

	va_start(args, pf_format);

	nBuf = vsprintf(szBuffer, pf_format, args);
	if (nBuf < 0) {
		std::cout << "DEBUG_TRACE ERROR..." << std::endl;
		return;
	}

	va_end(args);

	gettimeofday(&stCurTime, NULL);
	localtime_r(&stCurTime.tv_sec, &stTime);

	sprintf(szPrint, "[%02d:%02d:%02d.%03ld](%s:%d) >> ", stTime.tm_hour, stTime.tm_min, stTime.tm_sec, stCurTime.tv_usec / 1000, function, line);
	std::cout << szPrint << szBuffer << std::endl;
}

void WAITMS(int msec)
{
#if USED_SLEEP_SELECT
	struct timeval wait = {0, (msec * 1000)};
	(void)select(0, NULL, NULL, NULL, &wait);
#else
	usleep(msec * 1000);
#endif
}

void WAITNS(int nanosec)
{
#if USED_SLEEP_SELECT
	struct timeval wait = {0, (nanosec)};
	(void)select(0, NULL, NULL, NULL, &wait);
#else
	usleep(nanosec);
#endif
}

void toLowerString(std::string &str)
{
	for (char &c : str) {
		if ('A' <= c && c <= 'Z') {
			c += ('a' - 'A');
		}
	}
}

void toUpperString(std::string &str)
{
	for (char &c : str) {
		if ('a' <= c && c <= 'z') {
			c -= ('a' - 'A');
		}
	}
}

audio_type_t getAudioTypeFromPath(std::string datapath)
{
	std::string basename = datapath.substr(datapath.find_last_of("/") + 1);
	std::string extension;

	if (basename.find(".") == std::string::npos) {
		extension = "";
	} else {
		extension = basename.substr(basename.find_last_of(".") + 1);
	}

	toLowerString(extension);

	if (extension.compare("mp3") == 0) {
		medvdbg("audio type : mp3\n");
		return AUDIO_TYPE_MP3;
	} else if ((extension.compare("aac") == 0) || (extension.compare("mp4") == 0)) {
		medvdbg("audio type : aac\n");
		return AUDIO_TYPE_AAC;
	} else if ((extension.compare("opus") == 0)) {
		medvdbg("audio type : opus\n");
		return AUDIO_TYPE_OPUS;
	} else if ((extension.compare("flac") == 0)) {
		medvdbg("audio type : flac\n");
		return AUDIO_TYPE_FLAC;
	} else if ((extension.compare("") == 0) || (extension.compare("pcm") == 0) || (extension.compare("raw") == 0)) {
		medvdbg("audio type : pcm\n");
		return AUDIO_TYPE_PCM;
	} else if (extension.compare(AUDIO_EXT_TYPE_WAV) == 0) {
		medvdbg("audio type : wav\n");
		return AUDIO_TYPE_WAVE;
	} else {
		medvdbg("audio type : unknown\n");
		return AUDIO_TYPE_INVALID;
	}
}

audio_type_t getAudioTypeFromMimeType(std::string &mimeType)
{
	audio_type_t audioType;

	if (mimeType.empty()) {
		medwdbg("empty mime type!\n");
		audioType = AUDIO_TYPE_UNKNOWN;
	} else if ((mimeType.find(AAC_MIME_TYPE) != std::string::npos) ||
			   (mimeType.find(AACP_MIME_TYPE) != std::string::npos)) {
		audioType = AUDIO_TYPE_AAC;
	} else if (mimeType.find(MPEG_MIME_TYPE) != std::string::npos) {
		audioType = AUDIO_TYPE_MP3;
	} else if (mimeType.find(MP4_MIME_TYPE) != std::string::npos) {
		audioType = AUDIO_TYPE_AAC;
	} else if (mimeType.find(OPUS_MIME_TYPE) != std::string::npos) {
		audioType = AUDIO_TYPE_OPUS;
	} else {
		DEBUG_TRACE(LOG_DEBUG, "Unsupported mime type: %s\n", mimeType.c_str());
		audioType = AUDIO_TYPE_UNKNOWN;
	}

	return audioType;
}

bool mp3_header_parsing(unsigned char *header, unsigned int *channel, unsigned int *sampleRate)
{
	/**
*mp3_header is AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
*A - Frame sync
*B - MPEG Audio version
*...
*F - Sampling rate frequency
*.
*I - Channel Mode
*...
*/
	unsigned char bit;
	unsigned int mpegVersion;

	bit = header[1];
	bit >>= 3;
	bit &= (unsigned char)0x03;
	/* we need B information so Shift header[1] three times to the right, and & 0x03 */
	switch (bit) {
	case 0:
		mpegVersion = 2;
		break;
	case 2:
		mpegVersion = 1;
		break;
	case 3:
		mpegVersion = 0;
		break;
	default:
		DEBUG_TRACE(LOG_DEBUG, "Not Supported Format mpeg version : %u\n", mpegVersion);
		return false;
	}

	bit = header[2];
	bit >>= 2;
	bit &= 0x03;
	/* we need F information so Shift header[1] two times to the right, and & 0x03 */
	switch (bit) {
	case 0:
		*sampleRate = AUDIO_SAMPLE_RATE_44100;
		break;
	case 1:
		*sampleRate = AUDIO_SAMPLE_RATE_48000;
		break;
	case 2:
		*sampleRate = AUDIO_SAMPLE_RATE_32000;
		break;
	default:
		DEBUG_TRACE(LOG_DEBUG, "Not Supported Format sample rate : %u\n", sampleRate);
		return false;
	}

	*sampleRate >>= mpegVersion;

	bit = header[3];
	bit >>= 6;
	/* we need I information so Shift header[3] six times to the right */
	if (bit <= 2) {
		*channel = 2;
	} else {
		*channel = 1;
	}
	return true;
}

bool aac_header_parsing(unsigned char *header, unsigned int *channel, unsigned int *sampleRate)
{
	/**
*aac_header is AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP
*A - syncword 0xFFF, all bits must be 1
*....
*F - Sampling rate frequency
*..
*H - Channel Mode
*......
*/
	unsigned char bit;
	bit = header[2];
	bit >>= 2;
	bit &= 0x0F;
	/* we need F information so Shift header[2] two times to the right, and & 0x0F */
	switch (bit) {
	case 0:
		*sampleRate = AUDIO_SAMPLE_RATE_96000;
		break;
	case 1:
		*sampleRate = AUDIO_SAMPLE_RATE_88200;
		break;
	case 2:
		*sampleRate = AUDIO_SAMPLE_RATE_64000;
		break;
	case 3:
		*sampleRate = AUDIO_SAMPLE_RATE_48000;
		break;
	case 4:
		*sampleRate = AUDIO_SAMPLE_RATE_44100;
		break;
	case 5:
		*sampleRate = AUDIO_SAMPLE_RATE_32000;
		break;
	case 6:
		*sampleRate = AUDIO_SAMPLE_RATE_24000;
		break;
	case 7:
		*sampleRate = AUDIO_SAMPLE_RATE_22050;
		break;
	case 8:
		*sampleRate = AUDIO_SAMPLE_RATE_16000;
		break;
	case 9:
		*sampleRate = AUDIO_SAMPLE_RATE_12000;
		break;
	case 10:
		*sampleRate = AUDIO_SAMPLE_RATE_11025;
		break;
	case 11:
		*sampleRate = AUDIO_SAMPLE_RATE_8000;
		break;
	case 12:
		*sampleRate = AUDIO_SAMPLE_RATE_7350;
		break;
	default:
		DEBUG_TRACE(LOG_DEBUG, "Not Supported Format sample rate : %u\n", sampleRate);
		return false;
	}
	bit = header[2];
	bit <<= 2;
	bit |= (header[3] >> 6);
	bit &= 0x07;
	/* we need H information so Shift header[3] six times to the right, and & 0x07 */
	if (bit <= 0 || bit >= 8) {
		DEBUG_TRACE(LOG_DEBUG, "Invalid value bit : %d\n", bit);
		return false;
	}

	if (bit == 7) {
		*channel = 8;
	} else {
		*channel = (unsigned int)bit;
	}
	return true;
}

bool wave_header_parsing(unsigned char *header, unsigned int *channel, unsigned int *sampleRate, audio_format_type_t *pcmFormat)
{
	/**
*wave header is
*Chunk ID    (4byte) / .... (4byte) /            .... (4byte)                /
*.....       (4byte) / .... (4byte) / .. (2byte)   / NumChannels     (2byte) /
*sample Rate (4byte) / .... (4byte) / .. (2byte)   / Bits Per sample (2byte) /
*.....       (4byte) / .... (4byte)
*/
	unsigned short bitPerSample;
	*channel = header[23];
	*channel <<= 8;
	*channel |= header[22];

	*sampleRate = header[27];
	*sampleRate <<= 8;
	*sampleRate |= header[26];
	*sampleRate <<= 8;
	*sampleRate |= header[25];
	*sampleRate <<= 8;
	*sampleRate |= header[24];

	bitPerSample = header[35];
	bitPerSample <<= 8;
	bitPerSample |= header[34];
	/* wave header is Little Endian, so read from right byte */
	switch (bitPerSample) {
	case 8:
		*pcmFormat = AUDIO_FORMAT_TYPE_S8;
		break;
	case 16:
		*pcmFormat = AUDIO_FORMAT_TYPE_S16_LE;
		break;
	case 32:
		*pcmFormat = AUDIO_FORMAT_TYPE_S32_LE;
		break;
	default:
		DEBUG_TRACE(LOG_DEBUG, "Not Supported Format bit/sample : %u\n", bitPerSample);
		return false;
	}
	return true;
}

bool header_parsing(FILE *fp, audio_type_t audioType, unsigned int *channel, unsigned int *sampleRate, audio_format_type_t *pcmFormat)
{
	unsigned char *header;
	unsigned char tag[2];
	bool isHeader;

	switch (audioType) {
	case AUDIO_TYPE_MP3:
		isHeader = false;
		while (fread(tag, sizeof(unsigned char), 1, fp) > 0) {
			if (tag[0] == 0xFF) {
				isHeader = true;
				break;
			}
		}
		if (isHeader) {
			header = (unsigned char *)malloc(sizeof(unsigned char) * (MP3_HEADER_LENGTH + 1));
			if (header == NULL) {
				DEBUG_TRACE(LOG_DEBUG, "malloc failed error\n");
				return false;
			}
			int ret;
			ret = fseek(fp, -1, SEEK_CUR);
			if (ret != OK) {
				DEBUG_TRACE(LOG_DEBUG, "file seek failed errno : %d\n", errno);
				free(header);
				return false;
			}

			ret = fread(header, sizeof(unsigned char), MP3_HEADER_LENGTH, fp);
			if (ret != MP3_HEADER_LENGTH) {
				DEBUG_TRACE(LOG_DEBUG, "read failed ret : %d\n", ret);
				free(header);
				return false;
			}

			if (!mp3_header_parsing(header, channel, sampleRate)) {
				DEBUG_TRACE(LOG_DEBUG, "Header parsing failed\n");
				free(header);
				return false;
			}
		} else {
			medvdbg("no header\n");
			return false;
		}
		break;
	case AUDIO_TYPE_AAC:
		isHeader = false;
		while (fread(tag, sizeof(unsigned char), 1, fp) > 0) {
			if (tag[0] == 0xFF) {
				isHeader = true;
				break;
			}
		}
		if (isHeader) {
			header = (unsigned char *)malloc(sizeof(unsigned char) * (AAC_HEADER_LENGTH + 1));
			if (header == NULL) {
				medvdbg("malloc failed error\n");
				return false;
			}

			if (fseek(fp, -1, SEEK_CUR) != 0) {
				DEBUG_TRACE(LOG_DEBUG, "file seek failed error\n");
				free(header);
				return false;
			}

			if ((fread(header, sizeof(unsigned char), AAC_HEADER_LENGTH, fp) != AAC_HEADER_LENGTH) || !aac_header_parsing(header, channel, sampleRate)) {
				free(header);
				return false;
			}
		} else {
			medvdbg("no header\n");
			return false;
		}
		break;
	case AUDIO_TYPE_WAVE:
		header = (unsigned char *)malloc(sizeof(unsigned char) * (WAVE_HEADER_LENGTH + 1));
		if (header == NULL) {
			medvdbg("malloc failed error\n");
			return false;
		}

		if ((fread(header, sizeof(unsigned char), WAVE_HEADER_LENGTH, fp) != WAVE_HEADER_LENGTH) || !wave_header_parsing(header, channel, sampleRate, pcmFormat)) {
			free(header);
			return false;
		}
		break;
	default:
		medvdbg("does not support header parsing\n");
		return false;
	}

	if (header != NULL) {
		free(header);
	}

	if (fseek(fp, 0, SEEK_SET) != 0) {
		DEBUG_TRACE(LOG_DEBUG, "file seek failed error\n");
		return false;
	}
	return true;
}

int header_parsing(unsigned char *buffer, unsigned int bufferSize, audio_type_t audioType, unsigned int *channel, unsigned int *sampleRate, unsigned int *samplePerFrame, audio_format_type_t *pcmFormat)
{
	unsigned int headPoint;
	unsigned char *header;
	bool isHeader = false;
	switch (audioType) {
	case AUDIO_TYPE_MP3:
		while (headPoint < bufferSize) {
			if ((((buffer[headPoint] & 0xFF) << 8) | (buffer[headPoint + 1] & 0xF0)) == 0xFFF0) {
				if (checkHeader(&buffer[headPoint], channel, sampleRate, samplePerFrame) == true) {
					isHeader = true;
					break;
				}
			}

			headPoint++;
		}

		if (!isHeader) {
			return -1;
		}
		break;
	case AUDIO_TYPE_AAC:
		if (AAC_HEADER_LENGTH > bufferSize) {
			medvdbg("no header\n");
			return -1;
		}
		isHeader = false;
		headPoint = 0;
		while (headPoint < bufferSize) {
			if (buffer[headPoint] == 0xFF) {
				isHeader = true;
				break;
			}
			headPoint++;
		}
		if (isHeader && AAC_HEADER_LENGTH <= bufferSize - headPoint) {
			header = (unsigned char *)malloc(sizeof(unsigned char) * (AAC_HEADER_LENGTH + 1));
			if (header == NULL) {
				medvdbg("malloc failed error\n");
				return -1;
			}
			memcpy(header, buffer + headPoint, MP3_HEADER_LENGTH);
			if (!aac_header_parsing(header, channel, sampleRate)) {
				free(header);
				return -1;
			}
		} else {
			medvdbg("no header\n");
			return -1;
		}
		break;
	case AUDIO_TYPE_WAVE:
		if (WAVE_HEADER_LENGTH <= bufferSize) {
			header = (unsigned char *)malloc(sizeof(unsigned char) * (WAVE_HEADER_LENGTH + 1));
			if (header == NULL) {
				medvdbg("malloc failed error\n");
				return -1;
			}
			memcpy(header, buffer, WAVE_HEADER_LENGTH);
			if (!wave_header_parsing(header, channel, sampleRate, pcmFormat)) {
				free(header);
				return -1;
			}
		} else {
			medvdbg("no header\n");
			return -1;
		}
		break;
	default:
		medvdbg("does not support header parsing\n");
		return -1;
	}

	if (header != NULL) {
		free(header);
	}
	return headPoint;
}

bool checkHeader(unsigned char *buffer, unsigned int *channel, unsigned int *sampleRate, unsigned int *samplePerFrame)
{
	unsigned int header;
	unsigned int bitrate;
	unsigned int aau_size;
	unsigned int version_index;
	unsigned int layer_index;

	unsigned char version;
	unsigned char layer;
	unsigned char bitrate_index;
	unsigned char sample_rate_index;
	unsigned char padding;
	unsigned char channel_mode;

	header = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];

	version = (header >> 19) & 0x3;
	switch (version) {
	case MPEG_1:
		version_index = 0;
		break;
	case MPEG_2:
		version_index = 1;
		break;
	case MPEG_2_5:
		version_index = 2;
		break;
	default:
		return false;
	}

	layer = (header >> 17) & 0x3;
	switch (layer) {
	case LAYER_1:
		layer_index = 0;
		break;
	case LAYER_2:
		layer_index = 1;
		break;
	case LAYER_3:
		layer_index = 2;
		break;
	default:
		return false;
	}

	bitrate_index = (header >> 12) & 0xf;
	if ((bitrate_index == BITRATE_FREE) || bitrate_index == BITRATE_BAD) {
		return false;
	}

	sample_rate_index = (header >> 10) & 0x3;
	if (sample_rate_index == SAMPLE_RATE_UNDEFINED) {
		return false;
	}
	padding = (header >> 9) & 0x1;

	bitrate = bitRateTable[version_index][layer_index][bitrate_index - 1];
	*sampleRate = sampleRateTable[version_index][sample_rate_index];

	channel_mode = (header >> 8) & 0x3;
	if (channel_mode <= 2) {
		*channel = 2;
	} else {
		*channel = 1;
	}

	switch (layer) {
	case LAYER_1:
		aau_size = MPEG_LAYER1_AAU_SIZE(*sampleRate, bitrate, padding);
		break;
	case LAYER_2:
		aau_size = MPEG_LAYER2_AAU_SIZE(*sampleRate, bitrate, padding);
		break;
	case LAYER_3:
		if (version == MPEG_1) {
			aau_size = MPEG1_LAYER3_AAU_SIZE(*sampleRate, bitrate, padding);
		} else {
			aau_size = MPEG2_LAYER3_AAU_SIZE(*sampleRate, bitrate, padding);
		}
		break;
	}

	if (aau_size > 0) {
		*samplePerFrame = aau_size;
	}

	if (buffer[0] == buffer[aau_size] && buffer[1] == buffer[aau_size + 1]) {
		DEBUG_TRACE(LOG_DEBUG, "aau_size : %d version : %d bitrate : %d samplerate : %d\n", aau_size, version, bitrate, *sampleRate);
		return true;
	}

	return false;
}

struct wav_header_s {
	char headerRiff[4]; //"RIFF"
	uint32_t riffSize;
	char headerWave[4]; //"wave"
	char headerFmt[4];  //"fmt "
	uint32_t fmtSize;   //16 for pcm
	uint16_t format;	//1 for pcm
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitPerSample;
	char headerData[4]; //"data"
	uint32_t dataSize;
};

bool createWavHeader(FILE *fp)
{
	struct wav_header_s *header;
	header = (struct wav_header_s *)malloc(sizeof(struct wav_header_s));
	if (!header) {
		DEBUG_TRACE(LOG_DEBUG, "fail to malloc buffer\n");
		return false;
	}

	memset(header, 0xff, WAVE_HEADER_LENGTH);
	int ret;
	ret = fwrite(header, sizeof(unsigned char), WAVE_HEADER_LENGTH, fp);
	if (ret != WAVE_HEADER_LENGTH) {
		DEBUG_TRACE(LOG_DEBUG, "file write failed error %d\n", errno);
		free(header);
		return false;
	}
	if (fseek(fp, WAVE_HEADER_LENGTH, SEEK_SET) != 0) {
		DEBUG_TRACE(LOG_DEBUG, "file seek failed error\n");
		free(header);
		return false;
	}

	free(header);
	return true;
}

bool writeWavHeader(FILE *fp, unsigned int channel, unsigned int sampleRate, audio_format_type_t pcmFormat, unsigned int fileSize)
{
	/**
*wave header is
*Chunk ID 'RIFF' (4byte) / Chunk Size (4byte) / Fomat 'WAVE' (4byte) /
*Chunk ID 'fmt ' (4byte) / Chunk Size (4byte) / Audio Format (2byte) / NumChannels     (2byte) /
*sample Rate     (4byte) / Byte Rate  (4byte) / Block Align  (2byte) / Bits Per sample (2byte) /
*Chunk ID 'data' (4byte) / Chunk Size (4byte)
*/
	if (fseek(fp, 0, SEEK_SET) != 0) {
		DEBUG_TRACE(LOG_DEBUG, "file seek failed error\n");
		return false;
	}

	uint32_t byteRate = 0;
	uint16_t bitPerSample = 0;
	uint16_t blockAlign = 0;

	switch (pcmFormat) {
	case AUDIO_FORMAT_TYPE_S16_LE:
		bitPerSample = 16;
		break;
	case AUDIO_FORMAT_TYPE_S32_LE:
		bitPerSample = 32;
		break;
	default:
		DEBUG_TRACE(LOG_DEBUG, "does not support audio format.\n");
		return false;
	}

	blockAlign = channel * (bitPerSample >> 3);
	byteRate = sampleRate * blockAlign;

	struct wav_header_s *header;
	header = (struct wav_header_s *)malloc(sizeof(struct wav_header_s));
	if (header == NULL) {
		DEBUG_TRACE(LOG_DEBUG, "malloc failed error\n");
		return false;
	}

	strncpy(header->headerRiff, "RIFF", 4);

	header->riffSize = fileSize - 8;

	strncpy(header->headerWave, "WAVE", 4);
	strncpy(header->headerFmt, "fmt ", 4);

	header->fmtSize = 16;
	header->format = 1;
	header->channels = channel;
	header->sampleRate = sampleRate;
	header->byteRate = byteRate;
	header->blockAlign = blockAlign;
	header->bitPerSample = bitPerSample;

	strncpy(header->headerData, "data", 4);

	header->dataSize = fileSize - WAVE_HEADER_LENGTH;

	int ret = 0;
	ret = fwrite(header, sizeof(unsigned char), WAVE_HEADER_LENGTH, fp);

	if (ret != WAVE_HEADER_LENGTH) {
		DEBUG_TRACE(LOG_DEBUG, "file write failed error %d\n", errno);
		free(header);
		return false;
	}

	free(header);
	return true;
}

Timer::Timer()
{
	int ret;
	struct sched_param sparam;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_DEFAULT);
	sparam.sched_priority = 100;
	pthread_attr_setschedparam(&attr, &sparam);

	ret = pthread_create(&mTimerThread, &attr, timerThread, this);
	if (ret != OK) {
		DEBUG_TRACE(LOG_ERR, "Fail to create worker thread, return value : %d\n", ret);
		mIsRunning = false;
		return;
	}
	pthread_setname_np(mTimerThread, "Timer");

	DEBUG_TRACE(LOG_INFO, "timer started\n");
}

Timer::~Timer()
{
	pthread_join(mTimerThread, NULL);
	delete getInstance();
}

Timer *Timer::getInstance()
{
	static Timer *__mTimer;
	if (!__mTimer) {
		__mTimer = new Timer();
	}
	return __mTimer;
}

void Timer::timerClose()
{
	mIsRunning = false;
}

int Timer::newTimer(void **lphTmrElem)
{
	int ret = timerNewTimerElem(lphTmrElem);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "TIMER : NewTimer(set time) Error!!!");
		return -1;
	}
	return 0;
}

int Timer::setTimer(void *hAppTmrElem, void *hTmrElem, timerCallbackT cb, unsigned long msecTimeout, bool permanant)
{
	int ret = timerSetTimerElem(hAppTmrElem, hTmrElem, cb, msecTimeout, permanant);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "TIMER : SetTimer(set time) Error!!!");
		return -1;
	}
	return 0;
}

int Timer::resetTimer(void *hTmrElem)
{
	int ret = timerResetTimerElem(hTmrElem);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "TIMER : ResetTimer(reset time) Error!!!");
		return -1;
	}
	return 0;
}

void Timer::freeTimer(void *hTmrElem)
{
	int ret = timerFreeTimerElem(hTmrElem);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "TIMER : FreeTimer(free time) Error!!!");
	}
}

long Timer::getDurationTime(void *hTmrElem)
{
	return timerDurationElem(hTmrElem);
}

long Timer::getTick()
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return (tp.tv_sec - mTimerClass.start_tv_sec) * (1000 / DEFAULT_TIME_MSEC) + (tp.tv_nsec / (1000000 * DEFAULT_TIME_MSEC));
}

void *Timer::timerThread(void *arg)
{
	Timer *timer = static_cast<Timer *>(arg);
	struct timespec tp;
	lTimerElem_t *elem;
	long time = 0, rv = 0;

	memset((char *)&(timer->mTimerClass), 0, sizeof(timer_s));
	memset((char *)&(timer->mTimer), 0, sizeof(lTimerElem_t) * MAX_TIMER);

	timer->mTimerClass.max = MAX_TIMER;
	timer->mTimerClass.tick = DEFAULT_TIME_MSEC;
	timer->mTimerClass.idx = 0;

	clock_gettime(CLOCK_REALTIME, &tp);
	timer->mTimerClass.start_tv_sec = tp.tv_sec;

	timer->mIsRunning = true;

	DEBUG_TRACE(LOG_INFO, "TIMER Therad Start");

	while (timer->mIsRunning) {
		for (int i = 0; i < timer->mTimerClass.max; i++) {
			elem = &(timer->mTimer[i]);
			if ((elem->used == true) && (elem->hAppTmr != NULL) && (elem->expires > 0)) {
				time = timer->getTick();
				rv = time - elem->timeout;
				if (rv > 0 && (rv >= elem->expires)) {
					if (elem->cb) {
						if (elem->permant) {
							elem->timeout = time;
						}
						if (elem->cb) {
							elem->cb((void *)elem, elem->hAppTmr);
						}
					}
				} else {
					elem->current = rv;
				}
			}
		}

		WAITMS(DEFAULT_TIME_MSEC);
	}

	for (int j = 0; j < timer->mTimerClass.max; j++) {
		timer->timerFreeTimerElem(&(timer->mTimer[j]));
	}
}

int Timer::timerNewTimerElem(void **lphTmrElem)
{
	lTimerElem_t *elem;
	int i = 0;

	if (mTimerClass.idx >= mTimerClass.max) {
		mTimerClass.idx = 0;
	}

	for (i = mTimerClass.idx; i < mTimerClass.max; i++) {
		elem = &mTimer[i];
		mTimerClass.idx++;
		if (!elem->used) {
			elem->used = true;
			elem->idx = i;
			elem->hAppTmr = NULL;
			*lphTmrElem = (void *)elem;
			return 0;
		}
	}

	return -1;
}

int Timer::timerSetTimerElem(void *hAppTmrElem, void *hTmrElem, timerCallbackT cb, unsigned long msecTimeout, bool permant)
{
	struct timespec tp;
	lTimerElem_t *elem = (lTimerElem_t *)hTmrElem;

	if (!elem || !cb || !hAppTmrElem) {
		return -1;
	}

	clock_gettime(CLOCK_REALTIME, &tp);

	elem->hAppTmr = hAppTmrElem;
	elem->cb = cb;
	elem->timeout = (tp.tv_sec - mTimerClass.start_tv_sec) * (1000 / DEFAULT_TIME_MSEC) + (tp.tv_nsec / (1000000 * DEFAULT_TIME_MSEC));

	elem->expires = msecTimeout;
	elem->current = 0;
	elem->permant = permant;

	return 0;
}

int Timer::timerResetTimerElem(void *hTmrElem)
{
	lTimerElem_t *elem = (lTimerElem_t *)hTmrElem;
	if (!elem) {
		return 0;
	}

	elem->hAppTmr = NULL;
	elem->permant = false;
	elem->cb = NULL;
	elem->timeout = 0;
	elem->current = 0;
	elem->expires = 0;
	return 0;
}

int Timer::timerFreeTimerElem(void *hTmrElem)
{
	lTimerElem_t *elem = (lTimerElem_t *)hTmrElem;
	if (!elem) {
		return -1;
	}

	elem->hAppTmr = NULL;
	elem->permant = false;
	elem->cb = NULL;
	elem->timeout = 0;
	elem->current = 0;
	elem->expires = 0;
	elem->used = false;

	return 0;
}

long Timer::timerDurationElem(void *hTmrElem)
{
	lTimerElem_t *elem = (lTimerElem_t *)hTmrElem;
	if (!elem) {
		return -1;
	}

	return elem->expires;
}
