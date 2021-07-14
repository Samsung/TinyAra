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

#ifndef __AUDIO_LOCALSTREAM_H__
#define __AUDIO_LOCALSTREAM_H__

#include "AudioHttpDownloader.h"
#include "MediaPlayerWrapper.h"
#include "AudioCircularBuffer.h"
#include "AudioUtils.h"
#include "AudioStream.h"

class AudioLocalStream : public AudioStream
{
public:
	AudioLocalStream();
	~AudioLocalStream();

protected:
	virtual int startStream();
	virtual int pauseStream();
	virtual int resumeStream();
	virtual int stopStream();
};

#endif
