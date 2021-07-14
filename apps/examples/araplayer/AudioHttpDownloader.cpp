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

#include <curl/curl.h>
#include "AudioHttpDownloader.h"
#include "AudioUtils.h"
#include "AudioCommon.h"
#include "AraPlayer.h"

AudioHttpDownloader::AudioHttpDownloader()
{
	mHandle = NULL;
	mState = HTTP_DOWNLOAD_IDLE;
	headerBuf = NULL;
	response_code = 0;
	mIsHeader = false;
	mBuffer = nullptr;
}

AudioHttpDownloader::~AudioHttpDownloader()
{
	DEBUG_TRACE(LOG_INFO, "AudioHttpDownloader Free");
	freeDownloader();
	CLEAN(mBuffer);
}

void AudioHttpDownloader::setState(http_state_t state)
{
	mState = state;
}

http_state_t AudioHttpDownloader::getState()
{
	return mState;
}

#if USED_CURL_TRACE
size_t AudioHttpDownloader::CurlStateTrace(void *handle, int type, char *data, size_t size, void *userp)
{
	switch (type) {
	case CURLINFO_TEXT:
	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
		DEBUG_TRACE(LOG_DEBUG, "%s", data);
		break;
	case CURLINFO_DATA_IN:
	case CURLINFO_DATA_OUT:
	case CURLINFO_SSL_DATA_IN:
	case CURLINFO_SSL_DATA_OUT:
	default:
		break;
	}

	return 0;
}
#endif

int AudioHttpDownloader::startHttpDownload(std::map<void *, AudioHttpDownloader *> &downloaderMap, char *url, StreamDataCB cb)
{
	if (url == NULL) {
		DEBUG_TRACE(LOG_ERR, "url is NULL!");
		return -1;
	}

	if (mHandle == NULL) {
		if (NULL == (mHandle = curl_easy_init())) {
			DEBUG_TRACE(LOG_ERR, "curl_easy_init() error!");
			return -1;
		}

		downloaderMap.insert(std::make_pair(mHandle, this));
	} else {
		curl_easy_reset((CURL *)mHandle);
	}

#if USED_CURL_TRACE
	curl_easy_setopt((CURL *)mHandle, CURLOPT_DEBUGFUNCTION, CurlStateTrace);
	curl_easy_setopt((CURL *)mHandle, CURLOPT_DEBUGDATA, this);
	curl_easy_setopt((CURL *)mHandle, CURLOPT_VERBOSE, 1L);
#endif
	curl_easy_setopt((CURL *)mHandle, CURLOPT_URL, url);

	// Header Callback
	curl_easy_setopt((CURL *)mHandle, CURLOPT_HEADERFUNCTION, headerFunction);
	curl_easy_setopt((CURL *)mHandle, CURLOPT_HEADERDATA, this);

	// Data Callback
	curl_easy_setopt((CURL *)mHandle, CURLOPT_WRITEFUNCTION, dataFunction);
	curl_easy_setopt((CURL *)mHandle, CURLOPT_WRITEDATA, this);

	curl_easy_setopt((CURL *)mHandle, CURLOPT_BUFFERSIZE, 4096);
	curl_easy_setopt((CURL *)mHandle, CURLOPT_FOLLOWLOCATION, 1L);

	// HTTPS
	curl_easy_setopt((CURL *)mHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	parseHttpDataCB = cb;

	setState(HTTP_DOWNLOADING);
	curl_multi_add_handle((CURLM *)((AudioStream *)mStream)->getMainCurl(), (CURL *)mHandle);

	DEBUG_TRACE(LOG_INFO, "http request [%s]", url);
	return 0;
}

int AudioHttpDownloader::pauseHttpDownload()
{
	int ret;
	setState(HTTP_DOWNLOAD_PAUSE);

	ret = curl_easy_pause((CURL *)mHandle, CURLPAUSE_ALL);
	if (ret != CURLE_OK) {
		DEBUG_TRACE(LOG_ERR, "curl pause errro!");
	}

	return ret;
}

int AudioHttpDownloader::resumeHttpDownload()
{
	int ret;
	setState(HTTP_DOWNLOADING);

	ret = curl_easy_pause((CURL *)mHandle, CURLPAUSE_CONT);
	if (ret != CURLE_OK) {
		DEBUG_TRACE(LOG_ERR, "curl pause errro!");
	}

	return 0;
}

int AudioHttpDownloader::stopHttpDownload()
{
	freeDownloader();
	setState(HTTP_DOWNLOAD_IDLE);
	mIsHeader = false;
	return 0;
}

void AudioHttpDownloader::downloadComplete()
{
	if (mHandle && ((getState() != HTTP_DOWNLOAD_COMPLETE) && (getState() != HTTP_DOWNLOAD_IDLE))) {
		downloadInformation();
		curl_multi_remove_handle((CURLM *)((AudioStream *)mStream)->getMainCurl(), (CURL *)mHandle);
		setState(HTTP_DOWNLOAD_COMPLETE);
	}
}

void AudioHttpDownloader::freeDownloader()
{
	downloadComplete();
	if (mHandle) {
		curl_easy_cleanup((CURL *)mHandle);
		mHandle = NULL;
	}

	remnantBufSize = 0;
}

double AudioHttpDownloader::getDownloadSize()
{
	double val = 0;
	int ret;

	if (mHandle) {
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_SIZE_DOWNLOAD, &val);

		if ((CURLE_OK == ret) && (val > 0)) {
			DEBUG_TRACE(LOG_INFO, "download length : %0.0f bytes", val);
		}
	}

	return val;
}

void AudioHttpDownloader::downloadInformation()
{
	double val;
	int ret;

	if (mHandle) {
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_RESPONSE_CODE, &val);
		if ((ret == CURLE_OK) && (val > 200 && val < 700)) {
			DEBUG_TRACE(LOG_INFO, "Response Code: %lld", val);
		}

		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &val);
		if ((ret == CURLE_OK) && (val > 0)) {
			DEBUG_TRACE(LOG_INFO, "Content Length: %0.0f bytes", val);
		}

		/* check for bytes downloaded */
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_SIZE_DOWNLOAD, &val);
		if ((ret == CURLE_OK) && (val > 0)) {
			DEBUG_TRACE(LOG_INFO, "Data downloaded: %0.0f bytes", val);
		}

		/* check for total download time */
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_TOTAL_TIME, &val);
		if ((ret == CURLE_OK) && (val > 0)) {
			DEBUG_TRACE(LOG_INFO, "Total download time: %0.3f sec", val);
		}

		/* check for average download speed */
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_SPEED_DOWNLOAD, &val);
		if ((ret == CURLE_OK) && (val > 0)) {
			DEBUG_TRACE(LOG_INFO, "Average download speed: %0.3f Mbps", (val / 1024 / 1024) * 8);
		}
	}
}

size_t AudioHttpDownloader::headerFunction(void *ptr, size_t size, size_t nmemb, void *userp)
{
	AudioHttpDownloader *pHttpDownload = static_cast<AudioHttpDownloader *>(userp);
	size_t length;

	if (pHttpDownload == NULL) {
		DEBUG_TRACE(LOG_DEBUG, "pHttpDownload is NULL");
		return 0;
	}

	if (pHttpDownload->headerBuf == NULL) {
		pHttpDownload->headerBufSize = 0;
	}

	length = size * nmemb;

	pHttpDownload->headerBuf = (char *)realloc(pHttpDownload->headerBuf, pHttpDownload->headerBufSize + length + 1);
	if (pHttpDownload->headerBuf == NULL) {
		DEBUG_TRACE(LOG_ERR, "not enough memory (realloc returned NULL)");
		return 0;
	}

	memcpy(&(pHttpDownload->headerBuf[pHttpDownload->headerBufSize]), static_cast<char *>(ptr), length);
	pHttpDownload->headerBufSize += length;
	pHttpDownload->headerBuf[pHttpDownload->headerBufSize] = 0;

	if (strcmp(static_cast<char *>(ptr), "\r\n") == 0) {
		curl_easy_getinfo((CURL *)pHttpDownload->mHandle, CURLINFO_RESPONSE_CODE, &(pHttpDownload->response_code));

		switch (pHttpDownload->response_code) {
		case 200:
		case 206:
			pHttpDownload->parseHttpDataCB((void *)pHttpDownload->headerBuf, HTTP_HEADER, (unsigned int)pHttpDownload->headerBufSize, pHttpDownload);
			break;
		default:
			break;
		}

		CLEAN(pHttpDownload->headerBuf);
	}

	return (size * nmemb);
}

size_t AudioHttpDownloader::dataFunction(void *ptr, size_t size, size_t nmemb, void *userp)
{
	AudioHttpDownloader *pHttpDownload = static_cast<AudioHttpDownloader *>(userp);

	if (pHttpDownload == NULL) {
		DEBUG_TRACE(LOG_DEBUG, "pHttpDownload is NULL");
		return 0;
	}

	//DEBUG_TRACE(LOG_DEBUG, "%d", size*nmemb);

	switch (pHttpDownload->getState()) {
	case HTTP_DOWNLOADING:
		pHttpDownload->parseHttpDataCB(ptr, HTTP_STREAM, size * nmemb, pHttpDownload);
		break;
	case HTTP_DOWNLOAD_PAUSE:
	case HTTP_DOWNLOAD_COMPLETE:
	case HTTP_DOWNLOAD_IDLE:
	default:
		break;
	}

	return size * nmemb;
}

int AudioHttpDownloader::responseHeader()
{
	// TO-DO header parsing(MP3, AAC, ...)
	char *contentStr = NULL;
	double content_length = 0;
	int ret;

	if (mHandle) {
		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_CONTENT_TYPE, &contentStr);
		if (ret == CURLE_OK) {
			mContentType = contentStr;
			DEBUG_TRACE(LOG_INFO, "content type : %s", mContentType.c_str());
		}

		ret = curl_easy_getinfo((CURL *)mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
		if (ret == CURLE_OK) {
			DEBUG_TRACE(LOG_INFO, "content length : %.0f", content_length);
		}
	}

	return 0;
}

size_t AudioHttpDownloader::chunkWrite(unsigned char *ptr, size_t len)
{
	while (mBuffer->capacity() - mBuffer->size() < len) {
		WAITNS(0);
	}

	return mBuffer->write(ptr, len);
}

void AudioHttpDownloader::chunkClear()
{
	mBuffer->clear();
}

void AudioHttpDownloader::chunkFree()
{
	CLEAN(mBuffer);
}

size_t AudioHttpDownloader::getChunkSize()
{
	if (mBuffer) {
		return mBuffer->size();
	}

	return 0;
}

size_t AudioHttpDownloader::chunkRead(unsigned char *buf, size_t size)
{
	return mBuffer->read(buf, size);
}

bool AudioHttpDownloader::getIsHeader() const
{
	return mIsHeader;
}

void AudioHttpDownloader::setIsHeader(bool val)
{
	mIsHeader = val;
}

void AudioHttpDownloader::createBuffer(size_t buffSize)
{
	mBuffer = new AudioCircularBuffer(buffSize);
}

std::string &AudioHttpDownloader::getContentType()
{
	return mContentType;
}

void *AudioHttpDownloader::getCurlHandle()
{
	return mHandle;
}

AudioCircularBuffer *AudioHttpDownloader::getBuffer()
{
	return mBuffer;
}
