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

#ifndef __ARA_PLAYER_H__
#define __ARA_PLAYER_H__

#include <curl/curl.h>
#include <list>
#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "MediaPlayerWrapper.h"
#include "AudioUtils.h"
#include "AudioQueue.h"
#include "AudioStream.h"
#include "AudioCommon.h"

class AraPlayer : public MediaPlayerObserverInterface, public std::enable_shared_from_this<AraPlayer>
{
public:
	AraPlayer();
	virtual ~AraPlayer();

	void init();
	void start(pid_t, ara_player_msg_t *);
	void stop(pid_t);
	void pause(pid_t);
	void resume(pid_t);
	void execute(pid_t pid, ara_player_msg_t *msg);

	virtual void onPlaybackStarted(MediaPlayer &mediaPlayer) override;
	virtual void onPlaybackFinished(MediaPlayer &mediaPlayer) override;
	virtual void onPlaybackStopped(MediaPlayer &mediaPlayer) override;
	virtual void onPlaybackError(MediaPlayer &mediaPlayer, player_error_t error) override;
	virtual void onStartError(MediaPlayer &mediaPlayer, player_error_t error) override;
	virtual void onStopError(MediaPlayer &mediaPlayer, player_error_t error) override;
	virtual void onPauseError(MediaPlayer &mediaPlayer, player_error_t error) override;
	virtual void onPlaybackPaused(MediaPlayer &mediaPlayer) override;

	std::map<pid_t, AudioStream *> &getStreamMap();

private:
	void initCurl();
	void initWorker();
	void notifySync();
	AudioStream *createStream(pid_t, ara_player_msg_t *);

private:
	std::map<pid_t, AudioStream *> mStreamMap;
	std::map<pid_t, AudioStream *>::iterator mIter;
	std::condition_variable mCv;
	std::mutex mCmdMtx;
	AudioStream *mCurrStream;
	AudioQueue mQueue;
	void *mCurl;
	int running;
};
#endif
