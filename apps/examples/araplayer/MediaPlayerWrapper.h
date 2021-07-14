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

#ifndef __MEDIA_PLAYER_WRAPPER_H__
#define __MEDIA_PLAYER_WRAPPER_H__

#include <iostream>
#include <functional>
#include <memory>

#include <media/MediaPlayer.h>
#include <media/MediaTypes.h>
#include <media/MediaPlayerObserverInterface.h>
#include <media/FileInputDataSource.h>
#include <media/InputDataSource.h>
#include "MediaBufferInputDataSource.h"

class MediaPlayerWrapper
{
public:
	enum State {
		None = 0,
		Ready,
		Playing,
		Stop,
		Paused
	};

	MediaPlayerWrapper();
	virtual ~MediaPlayerWrapper() = default;

	void setState(State state)
	{
		mState = state;
	}
	State getState()
	{
		return mState;
	}

	unsigned int getReadBufferSize()
	{
		return readBufferSize;
	}

	int init(media::audio_type_t type, media::audio_format_type_t format, unsigned int rate, unsigned int chan, void *stream);
	int init(const char *);
	int deinit();
	int startPlay();
	int stopPlay();
	int pausePlay();
	int resumePlay();
	int setVolume(int volume);
	int setDataSource(std::unique_ptr<stream::InputDataSource> source);
	void setObserver(std::shared_ptr<MediaPlayerObserverInterface> observer);

private:
	std::shared_ptr<MediaPlayerObserverInterface> mObserver;
	unsigned int readBufferSize;
	State mState;
	media::MediaPlayer mp;
	bool isSourceSet;
};

#endif
