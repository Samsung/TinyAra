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

#ifndef __AUDIO_HASSTREAM_H__
#define __AUDIO_HASSTREAM_H__

#include "AudioCircularBuffer.h"
#include "AudioStream.h"
#include "PlaylistInfo.h"
#include "AudioCommon.h"

class AudioHASStream : public AudioStream
{
public:
	AudioHASStream();
	virtual ~AudioHASStream();

	virtual int startStream() override;
	virtual int pauseStream() override;
	virtual int resumeStream() override;
	virtual int stopStream() override;

	int requestStream(std::string url, request_type_t type);
	static int parseHttpData(void *ptr, int type, size_t size, void *p);
	std::map<void *, AudioHttpDownloader *> &getDownloaderMap();
	void removeMapAll();

	static void playlistInfoCallback(void *p, const char *url, play_list_info_t &pInfo);

	std::map<int, std::string> mBandWidthMap;
	int mRequestBandWidth;
	int mRequestIdx;
	void *mTimer;

private:
	std::map<void *, AudioHttpDownloader *> mDownloaderMap;
	std::map<void *, AudioHttpDownloader *>::iterator mIter;
};

#endif
