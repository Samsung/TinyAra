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

#ifndef __AUDIO_STREAM_H__
#define __AUDIO_STREAM_H__

#include <iostream>
#include "MediaPlayerWrapper.h"
#include "AudioHttpDownloader.h"
#include "AudioUtils.h"

#define HTTP_STREAM 0
#define HTTP_HEADER 1

class AudioStream
{
public:
	AudioStream();
	virtual ~AudioStream();

	virtual int startStream() = 0;
	virtual int pauseStream() = 0;
	virtual int resumeStream() = 0;
	virtual int stopStream() = 0;

	void setState(ara_player_msg_t *);
	void setPid(pid_t);

	ara_player_stream_t getStreamType();
	uint8_t getPriority();
	std::string getUrl();
	void setMainCurl(void *curl);
	void *getMainCurl();
	void setAudioInformation(audio_type_t audioType, audio_format_type_t format, unsigned int channels, unsigned int sample_rate, AudioStream *stream);

	std::shared_ptr<MediaPlayerWrapper> getMediaPlayer();

protected:
	pid_t mPid;
	ara_player_msg_t *mState;
	std::shared_ptr<MediaPlayerWrapper> mPlayer;
	void *mMainCurl;
};

#endif //__AUDIO_STREAM_H__
