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

#include <iostream>
#include <memory>
#include "AudioStream.h"
#include "MediaPlayerWrapper.h"

AudioStream::AudioStream() :
	mPid(-1)
{
	mPlayer = std::make_shared<MediaPlayerWrapper>();
	mMainCurl = nullptr;
}

AudioStream::~AudioStream()
{
}

void AudioStream::setState(ara_player_msg_t *state)
{
	mState = state;
}

void AudioStream::setMainCurl(void *curl)
{
	mMainCurl = curl;
}

void *AudioStream::getMainCurl()
{
	return mMainCurl;
}

void AudioStream::setPid(pid_t pid)
{
	mPid = pid;
}

ara_player_stream_t AudioStream::getStreamType()
{
	return mState->streamType;
}

uint8_t AudioStream::getPriority()
{
	return mState->priority;
}

std::string AudioStream::getUrl()
{
	return mState->url;
}

void AudioStream::setAudioInformation(audio_type_t audioType, audio_format_type_t format, unsigned int channels, unsigned int sample_rate, AudioStream *stream)
{
	mState->audioType = audioType;
	mState->channels = channels;
	mState->sampleRate = sample_rate;
	mPlayer->init(audioType, format, channels, sample_rate, (void *)stream);
	DEBUG_TRACE(LOG_INFO, "audioType:%d, SampleRate:%d, Channels:%d", (int)audioType, sample_rate, channels);
}

std::shared_ptr<MediaPlayerWrapper> AudioStream::getMediaPlayer()
{
	return mPlayer;
}
