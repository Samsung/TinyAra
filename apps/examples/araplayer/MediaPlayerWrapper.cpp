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

#include "MediaPlayerWrapper.h"
#include "AraPlayer.h"

MediaPlayerWrapper::MediaPlayerWrapper() :
	isSourceSet(false)
{
	mState = None;
	readBufferSize = 4096;
}

void MediaPlayerWrapper::setObserver(std::shared_ptr<MediaPlayerObserverInterface> observer)
{
	mObserver = observer;
}

int MediaPlayerWrapper::setDataSource(std::unique_ptr<stream::InputDataSource> source)
{
	player_result_t ret = PLAYER_ERROR_NONE;

	if (isSourceSet == false) {
		ret = mp.create();
		if (ret != PLAYER_ERROR_NONE) {
			DEBUG_TRACE(LOG_ERR, "Mediaplayer create failed");
		}

		ret = mp.setDataSource(std::move(source));
		if (ret != PLAYER_ERROR_NONE) {
			DEBUG_TRACE(LOG_ERR, "Mediaplayer setDataSource failed");
			mp.destroy();
			return -1;
		}

		ret = mp.setObserver(mObserver);
		if (ret != PLAYER_ERROR_NONE) {
			DEBUG_TRACE(LOG_ERR, "Mediaplayer setObserver failed");
			mp.destroy();
			return -1;
		}

		isSourceSet = true;
	}

	mState = Ready;

	return ret;
}

int MediaPlayerWrapper::init(audio_type_t type, audio_format_type_t format, unsigned int rate, unsigned int chan, void *stream)
{
	return setDataSource(std::move(std::unique_ptr<MediaBufferInputDataSource>(new MediaBufferInputDataSource(type, format, rate, chan, stream))));
}

int MediaPlayerWrapper::init(const char *url)
{
	auto source = std::move(std::unique_ptr<FileInputDataSource>(new FileInputDataSource(url)));
	source->setPcmFormat(AUDIO_FORMAT_TYPE_S16_LE);
	return setDataSource(std::move(source));
}

int MediaPlayerWrapper::deinit()
{
	player_result_t ret = PLAYER_ERROR_NONE;

	if (mState == None) {
		DEBUG_TRACE(LOG_ERR, "media player deinit invalid state[%d]", mState);
		return -1;
	}

	if (mState == Stop) {
		ret = mp.unprepare();
		if (ret != PLAYER_ERROR_NONE) {
			DEBUG_TRACE(LOG_ERR, "Mediaplayer unprepare failed");
		}
	}

	ret = mp.destroy();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer destroy failed");
	}

	mState = None;
	isSourceSet = false;

	return 0;
}

int MediaPlayerWrapper::startPlay()
{
	player_result_t ret = PLAYER_ERROR_NONE;

	if (mState != Ready) {
		DEBUG_TRACE(LOG_ERR, "media player start invalid state[%d]", mState);
		return -1;
	}

	ret = mp.prepare();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer prepare failed");
		mp.destroy();
		mState = None;
		return -1;
	}

	ret = mp.start();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer prepare failed");
		mp.unprepare();
		mp.destroy();
		mState = None;
		return -1;
	}

	mState = Playing;

	return 0;
}

int MediaPlayerWrapper::stopPlay()
{
	player_result_t ret = PLAYER_ERROR_NONE;

	if (mState < Playing) {
		DEBUG_TRACE(LOG_ERR, "media player stop invalid state[%d]", mState);
		return -1;
	}

	ret = mp.stop();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer stop failed");
		mp.unprepare();
		mp.destroy();
		mState = None;
		return -1;
	}

	mState = Stop;
	return 0;
}

int MediaPlayerWrapper::pausePlay()
{
	player_result_t ret = PLAYER_ERROR_NONE;

	if (mState != Playing) {
		DEBUG_TRACE(LOG_ERR, "media player pause invalid state[%d]", mState);
		return -1;
	}

	ret = mp.pause();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer pause failed");
		return -1;
	}

	mState = Paused;
	return 0;
}

int MediaPlayerWrapper::resumePlay()
{
	player_result_t ret = PLAYER_ERROR_NONE;

	DEBUG_TRACE(LOG_INFO, "");

	if (mState != Paused) {
		DEBUG_TRACE(LOG_ERR, "media player resume invalid state[%d]", mState);
		return -1;
	}

	ret = mp.start();
	if (ret != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "Mediaplayer start failed");
		return -1;
	}

	mState = Playing;
	return 0;
}

int MediaPlayerWrapper::setVolume(int volume)
{
	uint8_t vol = 0;
	DEBUG_TRACE(LOG_INFO, "setVolume is %d", volume);

	if (mp.getVolume(&vol) != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "MediaPlayer::getVolume failed");
	} else {
		DEBUG_TRACE(LOG_INFO, "Volume was %d", vol);
	}

	if (mp.setVolume(volume) != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "MediaPlayer::setVolume failed");
	}

	vol = 0;
	if (mp.getVolume(&vol) != PLAYER_ERROR_NONE) {
		DEBUG_TRACE(LOG_ERR, "MediaPlayer::getVolume failed");
	} else {
		DEBUG_TRACE(LOG_INFO, "Now, Volume is %d", vol);
	}

	return 0;
}
