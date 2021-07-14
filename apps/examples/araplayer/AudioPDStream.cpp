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

#include "AudioPDStream.h"
#include "MediaPlayerWrapper.h"

AudioPDStream::AudioPDStream()
{
}

AudioPDStream::~AudioPDStream()
{
	removeMapAll();
}

int AudioPDStream::startStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	pHttpDownloader = new AudioHttpDownloader();
	if (pHttpDownloader == NULL) {
		DEBUG_TRACE(LOG_ERR, "MultiRoomHttpDownloader error!");
		return -1;
	}

	pHttpDownloader->mStream = this;
	ret = pHttpDownloader->startHttpDownload(mDownloaderMap, mState->url, parseHttpData);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "startHttpDownload error!");
		return -1;
	}

	return ret;
}

int AudioPDStream::pauseStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->pauseHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "pauseHttpDownload error!");
			return ret;
		}
	}

	ret = mPlayer->pausePlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "pausePlay error!");
		return ret;
	}

	return ret;
}

int AudioPDStream::resumeStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->resumeHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "resumeHttpDownload error!");
			return ret;
		}
	}
	return ret;
}

int AudioPDStream::stopStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->stopHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "stopHttpDownload error!");
			return ret;
		}
	}

	ret = mPlayer->stopPlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "Media stopPlay error!");
		return ret;
	}

	return ret;
}

std::map<void *, AudioHttpDownloader *> &AudioPDStream::getDownloaderMap()
{
	return mDownloaderMap;
}

void AudioPDStream::removeMapAll()
{
	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		delete mIter->second;
	}
}

int AudioPDStream::parseHttpData(void *ptr, int type, size_t len, void *p)
{
	AudioHttpDownloader *pDownloader = static_cast<AudioHttpDownloader *>(p);
	AudioStream *pStream = (AudioStream *)pDownloader->mStream;
	size_t ret = 0;

	switch (type) {
	case HTTP_HEADER:
		ret = pDownloader->responseHeader();
		break;
	case HTTP_STREAM:
		if (!pDownloader->getIsHeader()) {
			audio_type_t audioType = getAudioTypeFromMimeType(pDownloader->getContentType());
			unsigned int channels;
			unsigned int sampleRate;
			unsigned int samplePerFrame;
			switch (audioType) {
			case AUDIO_TYPE_MP3:
			case AUDIO_TYPE_AAC: {
				int headPoint = header_parsing((unsigned char *)ptr, len, audioType, &channels, &sampleRate, &samplePerFrame, NULL);

				if (headPoint != -1) {
					// Frame length (in ms) = (samples per frame / sample rate (in hz)) * 1000
					size_t bufferSize = (((float)samplePerFrame / (float)sampleRate) * 1000000) * 10; //10 sec frame length
					pDownloader->createBuffer(bufferSize);

					// TO DO : There is a crash issue when printing the log here.. do not know the exact cause yet..
					((AudioStream *)pDownloader->mStream)->setAudioInformation(audioType, AUDIO_FORMAT_TYPE_S16_LE, sampleRate, channels, pStream);

					ret = pDownloader->chunkWrite((unsigned char *)ptr + headPoint, len - headPoint);

					pDownloader->setIsHeader(true);
				}
			}
			break;
			default:
			break;
			}
		} else {
			if (pStream->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Ready) {
				if (pDownloader->getChunkSize() > (pDownloader->getBuffer()->capacity() / BUFFERING_TIME_VALUE)) {
					if (0 > (ret = ((AudioPDStream *)pDownloader->mStream)->getMediaPlayer()->startPlay())) {
						DEBUG_TRACE(LOG_ERR, "RTMediaPlayerWrapper startPlay error!");
						return ret;
					}
				}
			} else if (pStream->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Paused) {
				if (pDownloader->getChunkSize() > (pDownloader->getBuffer()->capacity() / BUFFERING_TIME_VALUE)) {
					if (0 > (ret = ((AudioPDStream *)pDownloader->mStream)->getMediaPlayer()->resumePlay())) {
						DEBUG_TRACE(LOG_ERR, "RTMediaPlayerWrapper resumePlay error!");
						return ret;
					}
				}
			}

			ret = pDownloader->chunkWrite((unsigned char *)ptr, len);
		}
		break;
	default:
		break;
	}

	return len;
}
