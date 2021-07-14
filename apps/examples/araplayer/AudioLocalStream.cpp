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

#include "AudioLocalStream.h"
#include "MediaPlayerWrapper.h"

AudioLocalStream::AudioLocalStream()
{
}

AudioLocalStream::~AudioLocalStream()
{
}

int AudioLocalStream::startStream()
{
	int ret = -1;

	ret = mPlayer->init(mState->url);
	ret = mPlayer->startPlay();

	return ret;
}

int AudioLocalStream::pauseStream()
{
	int ret = -1;

	ret = mPlayer->pausePlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "pausePlay error!");
		return ret;
	}

	return ret;
}

int AudioLocalStream::resumeStream()
{
	int ret = -1;

	ret = mPlayer->resumePlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "resumePlay error!");
		return ret;
	}

	return ret;
}

int AudioLocalStream::stopStream()
{
	int ret = -1;
	ret = mPlayer->stopPlay();
	if (ret < 0) {
		DEBUG_TRACE(LOG_ERR, "Media stopPlay error!");
		return ret;
	}

	return ret;
}
