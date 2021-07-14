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

#include "AudioHASStream.h"
#include "MediaPlayerWrapper.h"
#include "HASParser.h"
#include "AraPlayer.h"
#include "AudioWorker.h"

AudioHASStream::AudioHASStream()
{
	mRequestIdx = 0;
	mRequestBandWidth = -1;
	mTimer = NULL;
	HASParser::getInstance()->setCallback_GetPlaylistInfo(this, playlistInfoCallback);
}

AudioHASStream::~AudioHASStream()
{
	removeMapAll();
}

int AudioHASStream::startStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	pHttpDownloader = new AudioHttpDownloader();
	if (pHttpDownloader == NULL) {
		DEBUG_TRACE(LOG_ERR, "AudioHttpDownloader error!");
		return -1;
	}

	pHttpDownloader->mRequestUrl = std::string(mState->url);

	if (strstr(pHttpDownloader->mRequestUrl.c_str(), ".m3u8")) {
		DEBUG_TRACE(LOG_INFO, "Manifest request");
		pHttpDownloader->mRequestType = request_type_s::HTTP_HLS_MANIFEST;
	} else if (strstr(pHttpDownloader->mRequestUrl.c_str(), ".ts")) {
		DEBUG_TRACE(LOG_INFO, "TS request");
		pHttpDownloader->mRequestType = request_type_s::HTTP_HLS_TS;
	} else {
		DEBUG_TRACE(LOG_INFO, "Unsupport Request Type");
		return -1;
	}

	pHttpDownloader->mStream = this;
	ret = pHttpDownloader->startHttpDownload(mDownloaderMap, (char *)pHttpDownloader->mRequestUrl.c_str(), parseHttpData);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "startHttpDownload error!");
		return -1;
	}

	return ret;
}

int AudioHASStream::pauseStream()
{
	int ret = -1;
	AudioHttpDownloader *pHttpDownloader;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->pauseHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "pauseHttpDownload error!");
		}
	}

	ret = mPlayer->pausePlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "pausePlay error!");
		return ret;
	}

	return ret;
}

int AudioHASStream::resumeStream()
{
	int ret = -1;
	AudioHttpDownloader *pHttpDownloader;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->resumeHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "resumeHttpDownload error!");
		}
	}

	return ret;
}

int AudioHASStream::stopStream()
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		pHttpDownloader = (AudioHttpDownloader *)mIter->second;

		ret = pHttpDownloader->stopHttpDownload();
		if (ret < 0) {
			DEBUG_TRACE(LOG_ERR, "stopHttpDownload error!");
		}
	}

	ret = mPlayer->stopPlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "Media stopPlay error!");
		return ret;
	}

	return ret;
}

int AudioHASStream::requestStream(std::string url, request_type_t type)
{
	AudioHttpDownloader *pHttpDownloader;
	int ret = -1;

	pHttpDownloader = new AudioHttpDownloader();
	if (pHttpDownloader == NULL) {
		DEBUG_TRACE(LOG_ERR, "AudioHttpDownloader error!");
		return -1;
	}

	pHttpDownloader->mRequestUrl = std::string(url);
	pHttpDownloader->mRequestType = type;

	pHttpDownloader->mStream = this;
	ret = pHttpDownloader->startHttpDownload(mDownloaderMap, (char *)pHttpDownloader->mRequestUrl.c_str(), parseHttpData);
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "startHttpDownload error!");
		return -1;
	}

	if (type == request_type_s::HTTP_HLS_TS) {
		mRequestIdx++;
	}
	return ret;
}

std::map<void *, AudioHttpDownloader *> &AudioHASStream::getDownloaderMap()
{
	return mDownloaderMap;
}

void AudioHASStream::removeMapAll()
{
	for (mIter = mDownloaderMap.begin(); mIter != mDownloaderMap.end(); mIter++) {
		delete mIter->second;
	}
}

int AudioHASStream::parseHttpData(void *ptr, int type, size_t len, void *p)
{
	AudioHttpDownloader *pHttpDownloader = static_cast<AudioHttpDownloader *>(p);
	AudioStream *pStream = (AudioStream *)pHttpDownloader->mStream;
	size_t ret = 0;

	switch (type) {
	case HTTP_HEADER:
		DEBUG_TRACE(LOG_INFO, "\n%s", ptr);
		pHttpDownloader->responseHeader();
		break;
	case HTTP_STREAM:
		switch (pHttpDownloader->mRequestType) {
		case request_type_s::HTTP_HLS_MANIFEST:
			DEBUG_TRACE(LOG_INFO, "HTTP_HLS_MANIFEST : %s", pHttpDownloader->mRequestUrl.c_str());
			HASParser::getInstance()->parsePlaylist(pHttpDownloader->mRequestUrl.c_str(), (char *)ptr, len);
			break;
		case request_type_s::HTTP_HLS_TS:
			if (!pHttpDownloader->getIsHeader()) {
				audio_type_t audioType = getAudioTypeFromMimeType(pHttpDownloader->getContentType());
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
						pHttpDownloader->createBuffer(bufferSize);

						// TO DO : There is a crash issue when printing the log here.. do not know the exact cause yet..
						((AudioStream *)pHttpDownloader->mStream)->setAudioInformation(audioType, AUDIO_FORMAT_TYPE_S16_LE, sampleRate, channels, pStream);

						ret = pHttpDownloader->chunkWrite((unsigned char *)ptr + headPoint, len - headPoint);

						pHttpDownloader->setIsHeader(true);
					}
				}
				break;
				default:
				break;
				}
			} else {
				if (pStream->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Ready) {
					if (pHttpDownloader->getChunkSize() > (pHttpDownloader->getBuffer()->capacity() / BUFFERING_TIME_VALUE)) {
						if (0 > (ret = ((AudioHASStream *)pHttpDownloader->mStream)->getMediaPlayer()->startPlay())) {
							DEBUG_TRACE(LOG_ERR, "RTMediaPlayerWrapper startPlay error!");
							return ret;
						}
					}
				} else if (pStream->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Paused) {
					if (pHttpDownloader->getChunkSize() > (pHttpDownloader->getBuffer()->capacity() / BUFFERING_TIME_VALUE)) {
						if (0 > (ret = ((AudioHASStream *)pHttpDownloader->mStream)->getMediaPlayer()->resumePlay())) {
							DEBUG_TRACE(LOG_ERR, "RTMediaPlayerWrapper resumePlay error!");
							return ret;
						}
					}
				}
				ret = pHttpDownloader->chunkWrite((unsigned char *)ptr, len);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return len;
}

void AudioHASStream::playlistInfoCallback(void *p, const char *url, play_list_info_t &pInfo)
{
	AudioHASStream *pHasStream = static_cast<AudioHASStream *>(p);
	AudioWorker &aw = AudioWorker::getWorker();
	has_segment_info_t outSegInfo;
	int ret;

	switch (pInfo.hasType) {
	case HLS_MANIFEST:
		DEBUG_TRACE(LOG_INFO, "playlist : %d", pInfo.type);
		switch (pInfo.type) {
		case MASTER:
			for (std::vector<int>::iterator iter = pInfo.bandWidthQuality.begin(); iter != pInfo.bandWidthQuality.end(); ++iter) {
				ret = HASParser::getInstance()->getPlaylistInfo(pInfo.playlistUrl.c_str(), (*iter), &outSegInfo);
				if (ret > 0) {
					if (pHasStream->mRequestBandWidth < 0) {
						pHasStream->mRequestBandWidth = (*iter);
					}
					pHasStream->mBandWidthMap.insert(std::make_pair((*iter), outSegInfo.url));
					aw.enQueue(&AudioHASStream::requestStream, pHasStream, outSegInfo.url, request_type_s::HTTP_HLS_MANIFEST);
				}
			}
			break;
		case SINGLE:
		case MEDIA:
			DEBUG_TRACE(LOG_INFO, "playlist : %d", pInfo.type);
			if (pHasStream->mRequestIdx == 0) {
				ret = HASParser::getInstance()->getSegmentInfo(pInfo.playlistUrl.c_str(), pHasStream->mRequestIdx, pHasStream->mRequestBandWidth, &outSegInfo);
				if (ret > 0) {
					aw.enQueue(&AudioHASStream::requestStream, pHasStream, outSegInfo.url, request_type_s::HTTP_HLS_TS);
				}
			}
			break;
		default:
			break;
		}
		break;
	case DASH_MPD:
	default:
		DEBUG_TRACE(LOG_ERR, "Unsupport HAS Type");
		break;
	}

	//DEBUG_TRACE(LOG_DEBUG, "%s", url);
	return;
}
