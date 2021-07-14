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
#ifndef __AUDIO_HTTPDOWNLOADER_H__
#define __AUDIO_HTTPDOWNLOADER_H__

#define USED_CURL_TRACE 0
#define PIECE_CHUNK_SIZE (1024 * 2)

#include <iostream>
#include <list>
#include <map>
#include "AudioCircularBuffer.h"
#include "AudioCommon.h"

class AudioHttpDownloader
{
public:
	AudioHttpDownloader();
	~AudioHttpDownloader();

	void setState(http_state_t);
	http_state_t getState();

	int startHttpDownload(std::map<void *, AudioHttpDownloader *> &, char *url, StreamDataCB);
	int pauseHttpDownload();
	int resumeHttpDownload();
	int stopHttpDownload();

	void downloadComplete();
	void freeDownloader();
	int responseHeader();

	double getDownloadSize();

	void *mStream;
	void *mHandle;
	std::string mRequestUrl;
	request_type_t mRequestType;

	size_t getChunkSize();
	size_t chunkRead(unsigned char *buf, size_t size);
	bool getIsHeader() const;
	void setIsHeader(bool);
	void createBuffer(size_t buffSize);
	std::string &getContentType();
	size_t chunkWrite(unsigned char *ptr, size_t len);
	void *getCurlHandle();
	AudioCircularBuffer *getBuffer();

private:
	int remnantBufSize;
	int headerBufSize;
	long response_code;
	std::string mContentType;

	http_state_t mState;
	char *headerBuf;

	void downloadInformation();
	void writePlayloadData(void *ptr, size_t len, void *userp);

#if USED_CURL_TRACE
	static size_t CurlStateTrace(void *handle, int type, char *data, size_t size, void *userp);
#endif
	static size_t headerFunction(void *ptr, size_t size, size_t nmemb, void *userp);
	static size_t dataFunction(void *ptr, size_t size, size_t nmemb, void *userp);

	void chunkClear();
	void chunkFree();

	StreamDataCB parseHttpDataCB;
	AudioCircularBuffer *mBuffer;

	bool mIsHeader;
};

#endif
