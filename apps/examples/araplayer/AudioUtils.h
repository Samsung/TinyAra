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

#ifndef __AUDIO_UTILS_H__
#define __AUDIO_UTILS_H__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <media/MediaTypes.h>

#include "AudioCommon.h"

#define CLEAN(x)  \
	if (x)        \
		delete x; \
	x = NULL;

using namespace media;

// MPEG version

#define MPEG_1 3
#define MPEG_2 2
#define MPEG_UNDEFINED 1
#define MPEG_2_5 0

// MPEG layer
#define LAYER_1 3
#define LAYER_2 2
#define LAYER_3 1
#define LAYER_UNDEFINED 0

// Bitrate index
#define BITRATE_FREE 0x0 // Variable Bit Rate
#define BITRATE_BAD 0xf  // Not allow

// Sample Rate index
#define SAMPLE_RATE_UNDEFINED 0x3

// aau size = frame samples * (1 / sample rate) * bitrate / 8 + padding
//            = frame samples * bitrate / 8 / sample rate + padding
// Number of frame samples is constant value as below table:
//        MPEG1  MPEG2(LSF)  MPEG2.5(LSF)
// Layer1  384     384         384
// Layer2  1152    1152        1152
// Layer3  1152    576         576
#define MPEG_LAYER1_AAU_SIZE(sr, br, pad) (384 * ((br)*1000) / 8 / (sr) + ((pad)*4))
#define MPEG_LAYER2_AAU_SIZE(sr, br, pad) (1152 * ((br)*1000) / 8 / (sr) + (pad))
#define MPEG1_LAYER3_AAU_SIZE(sr, br, pad) (1152 * ((br)*1000) / 8 / (sr) + (pad))
#define MPEG2_LAYER3_AAU_SIZE(sr, br, pad) (576 * ((br)*1000) / 8 / (sr) + (pad))

//***************************************************************************
// Name: log
//***************************************************************************/
#define DEBUG_TRACE(lv, ...) ARAPS_LOG(lv, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

void ARAPS_LOG(int level, const char *filename, unsigned int line, const char *function, const char *pf_format, ...);
void WAITMS(int msec);
void WAITNS(int nanosec);

void toLowerString(std::string &str);
void toUpperString(std::string &str);
audio_type_t getAudioTypeFromPath(std::string path);
audio_type_t getAudioTypeFromMimeType(std::string &mimeType);
bool header_parsing(FILE *fp, audio_type_t AudioType, unsigned int *channel, unsigned int *sample_rate, audio_format_type_t *pcmFormat);
int header_parsing(unsigned char *buffer, unsigned int bufferSize, audio_type_t audioType, unsigned int *channel, unsigned int *sampleRate, unsigned int *samplePerFrame, audio_format_type_t *pcmFormat);
bool createWavHeader(FILE *fp);
bool writeWavHeader(FILE *fp, unsigned int channel, unsigned int sampleRate, audio_format_type_t pcmFormat, unsigned int fileSize);
bool checkHeader(unsigned char *buffer, unsigned int *channel, unsigned int *sampleRate, unsigned int *samplePerFrame);

//***************************************************************************
// Name: Timer
//***************************************************************************/

#define MAX_TIMER 5
#define DEFAULT_TIME_MSEC 10

typedef void (*timerCallbackT)(void *elem, void *appelem);

class Timer
{
public:
	Timer();
	~Timer();

	static Timer *getInstance();
	void timerClose();
	int newTimer(void **lphTmrElem);
	int setTimer(void *hAppTmrElem, void *hTmrElem, timerCallbackT cb, unsigned long msecTimeout, bool permanant);
	int resetTimer(void *hTmrElem);
	void freeTimer(void *hTmrElem);
	long getDurationTime(void *hTmrElem);
	long getTick();

private:
	struct timer_s {
		short count;
		short max;
		short idx;
		long tick;
		long start_tv_sec;
		pthread_t tid;
	};

	typedef timer_s timer_t;

	struct lTimerElem_s {
		void *hAppTmr;
		bool used;
		bool permant;
		short idx;
		long timeout;
		long current;
		long expires;
		timerCallbackT cb;
	};

	typedef lTimerElem_s lTimerElem_t;

	bool mIsRunning;
	timer_t mTimerClass;
	lTimerElem_t mTimer[MAX_TIMER];

	pthread_t mTimerThread;

	int timerNewTimerElem(void **lphTmrElem);
	int timerSetTimerElem(void *hAppTmrElem, void *hTmrElem, timerCallbackT cb, unsigned long msecTimeout, bool permant);
	int timerResetTimerElem(void *hTmrElem);
	int timerFreeTimerElem(void *hTmrElem);
	long timerDurationElem(void *hTmrElem);

protected:
	static void *timerThread(void *);
};
#endif
