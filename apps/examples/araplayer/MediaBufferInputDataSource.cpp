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

#include <media/MediaTypes.h>
#include "MediaBufferInputDataSource.h"
#include "AudioStream.h"
#include "AudioPDStream.h"
#include "AudioHASStream.h"
#include "AudioUtils.h"

MediaBufferInputDataSource::MediaBufferInputDataSource() :
	mIsOpened(false)
{
	memset(mBuf, 0x00, 4096);
}

MediaBufferInputDataSource::MediaBufferInputDataSource(audio_type_t audioType, audio_format_type_t format, unsigned int rate, unsigned int chan, void *stream)
{
	setChannels(chan);
	setSampleRate(rate);
	setPcmFormat(format);
	setAudioType(audioType);
	mStream = stream;
}

MediaBufferInputDataSource::~MediaBufferInputDataSource()
{
	if (isPrepared()) {
		close();
	}
}

bool MediaBufferInputDataSource::isPrepared()
{
	return mIsOpened;
}

bool MediaBufferInputDataSource::open()
{
	if (mIsOpened == false) {
		mIsOpened = true;
		return true;
	}
	
	return false;
}

bool MediaBufferInputDataSource::close()
{
	if (mIsOpened) {
		mIsOpened = false;
		return true;
	}

	return false;
}

ssize_t MediaBufferInputDataSource::read(unsigned char *buf, size_t size)
{
	AudioStream *pStream = (AudioStream *)mStream;
	AudioHttpDownloader *pDownloader = NULL;
	std::map<void *, AudioHttpDownloader *> dMap;
	std::map<void *, AudioHttpDownloader *>::iterator dIter;

	ssize_t used = 0;

	if (!isPrepared()) {
		return EOF;
	}

	switch (pStream->getStreamType()) {
	case STREAM_HTTP_PROGRESSIVE:
		dMap = ((AudioPDStream *)mStream)->getDownloaderMap();
		pDownloader = (AudioHttpDownloader *)(dMap.begin()->second);
		break;
	case STREAM_HTTP_HLS:
		dMap = ((AudioHASStream *)mStream)->getDownloaderMap();
		for (dIter = dMap.begin(); dIter != dMap.end(); dIter++) {
			pDownloader = (AudioHttpDownloader *)(dIter->second);
			if (pDownloader->mRequestType == request_type_t::HTTP_HLS_TS) {
				break;
			} else {
				pDownloader = NULL;
			}
		}
		break;
	default:
		DEBUG_TRACE(LOG_ERR, "Unknow type : %d", pStream->getStreamType());
		return 0;
	}

	if (pDownloader) {
		//DEBUG_TRACE(LOG_DEBUG, "size :%d , getChunkSize : %d", size, pDownloader->getChunkSize());
		if (pDownloader->getChunkSize() >= size) {
			used = pDownloader->chunkRead(buf, size);
		} else {
			memcpy(buf, mBuf, size);
			used = size;
		}
	}
	return used;
}
