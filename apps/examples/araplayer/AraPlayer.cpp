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

#include "AraPlayer.h"
#include "AudioHttpDownloader.h"
#include "MediaPlayerWrapper.h"
#include "AudioPDStream.h"
#include "AudioHASStream.h"
#include "AudioLocalStream.h"
#include "AudioWorker.h"

AraPlayer::AraPlayer() :
	mCurrStream(nullptr)
{
}

AraPlayer::~AraPlayer()
{
}

void AraPlayer::initWorker()
{
	DEBUG_TRACE(LOG_INFO, "worker initialized");
	return notifySync();
}

void AraPlayer::init()
{
	std::unique_lock<std::mutex> lock(mCmdMtx);
	AudioWorker &aw = AudioWorker::getWorker();
	Timer::getInstance();
	initCurl();
	aw.setCurl(mCurl);
	aw.startWorker();
	aw.enQueue(&AraPlayer::initWorker, shared_from_this());
	mCv.wait(lock);
}

void AraPlayer::notifySync()
{
	std::unique_lock<std::mutex> lock(mCmdMtx);
	mCv.notify_one();
}

void AraPlayer::initCurl()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	mCurl = (void *)curl_multi_init();
	curl_multi_perform((CURLM *)mCurl, &running);
}

// TO-DO msg will be changed in MessagePort CB.
AudioStream *AraPlayer::createStream(pid_t pid, ara_player_msg_t *msg)
{
	AudioStream *newStream = nullptr;
	AudioWorker &aw = AudioWorker::getWorker();

	switch (msg->streamType)
	{
	case STREAM_LOCAL:
		newStream = new AudioLocalStream;
		aw.disableCurl();
		break;
	case STREAM_HTTP_PROGRESSIVE:
		newStream = new AudioPDStream;
		aw.enableCurl();
		break;
	case STREAM_HTTP_HLS:
		newStream = new AudioHASStream;
		aw.enableCurl();
		break;
	default:
		DEBUG_TRACE(LOG_ERR, "Stream Type Error!!![%d]", msg->streamType);
		return NULL;
	}

	aw.setPlayer(shared_from_this());
	newStream->setMainCurl(mCurl);
	newStream->setState(msg);
	newStream->setPid(pid);
	newStream->getMediaPlayer()->setObserver(shared_from_this());
	mStreamMap.insert(std::make_pair(pid, newStream));

	return newStream;
}

void AraPlayer::execute(pid_t pid, ara_player_msg_t *msg)
{
	std::unique_lock<std::mutex> lock(mCmdMtx);

	if (mCurrStream != nullptr && mCurrStream->getPriority() < msg->priority) {
		std::cout << "The player using the current device has a higher priority than your player." << std::endl;
		// TO-DO response to App.
		return;
	}

	AudioWorker &aw = AudioWorker::getWorker();

	switch (msg->cmdType)
	{
	case CMD_START:
		aw.enQueue(&AraPlayer::start, shared_from_this(), pid, msg);
		break;
	case CMD_STOP:
		aw.enQueue(&AraPlayer::stop, shared_from_this(), pid);
		break;
	case CMD_PAUSE:
		aw.enQueue(&AraPlayer::pause, shared_from_this(), pid);
		break;
	case CMD_RESUME:
		aw.enQueue(&AraPlayer::resume, shared_from_this(), pid);
	default:
		break;
	}
}

void AraPlayer::start(pid_t pid, ara_player_msg_t *msg)
{
	AudioStream *nStream = nullptr;

	mIter = mStreamMap.find(pid);
	if (mIter == mStreamMap.end()) {
		nStream = createStream(pid, msg);

	} else {
		nStream = (AudioStream *)mIter->second;
		ara_player_stream_t prevStreamType = nStream->getStreamType();
		ara_player_stream_t currStreamType = msg->streamType;
		if (prevStreamType != currStreamType) {
			if (prevStreamType == STREAM_HTTP_PROGRESSIVE) {
				delete nStream;
			}
			mStreamMap.erase(pid);
			nStream = createStream(pid, msg);
		}

		if (currStreamType == STREAM_HTTP_PROGRESSIVE) {
			AudioWorker &aw = AudioWorker::getWorker();
			aw.enableCurl();
		}
	}

	mCurrStream = nStream;

	if (nStream->getMediaPlayer()->getState() == MediaPlayerWrapper::State::None) {
		nStream->setState(msg);
		nStream->startStream();
	}
}

void AraPlayer::stop(pid_t pid)
{
	mIter = mStreamMap.find(pid);
	if (mIter != mStreamMap.end()) {
		if (((AudioStream *)mIter->second)->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Playing) {
			AudioWorker &aw = AudioWorker::getWorker();
			aw.disableCurl();
			mIter->second->stopStream();
		} else {
			DEBUG_TRACE(LOG_DEBUG, "not playing\n");
		}
	} else {
		DEBUG_TRACE(LOG_INFO, "The corresponding pid does not exist.");
	}
}

void AraPlayer::pause(pid_t pid)
{
	mIter = mStreamMap.find(pid);
	if (mIter != mStreamMap.end()) {
		if (((AudioStream *)mIter->second)->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Playing) {
			AudioWorker &aw = AudioWorker::getWorker();
			aw.disableCurl();
			mIter->second->pauseStream();
		} else {
			DEBUG_TRACE(LOG_DEBUG, "not playing\n");
		}
	} else {
		DEBUG_TRACE(LOG_INFO, "The corresponding pid does not exist.");
	}
}

void AraPlayer::resume(pid_t pid)
{
	mIter = mStreamMap.find(pid);
	if (mIter != mStreamMap.end()) {
		if (((AudioStream *)mIter->second)->getMediaPlayer()->getState() == MediaPlayerWrapper::State::Paused) {
			AudioWorker &aw = AudioWorker::getWorker();
			aw.enableCurl();
			mIter->second->resumeStream();
		} else {
			DEBUG_TRACE(LOG_DEBUG, "not playing\n");
		}
	} else {
		DEBUG_TRACE(LOG_INFO, "The corresponding pid does not exist.");
	}
}

std::map<pid_t, AudioStream *> &AraPlayer::getStreamMap()
{
	return mStreamMap;
}

void AraPlayer::onPlaybackStarted(MediaPlayer &mediaPlayer)
{
	DEBUG_TRACE(LOG_INFO, "onPlaybackStarted");
}

void AraPlayer::onPlaybackFinished(MediaPlayer &mediaPlayer)
{
	DEBUG_TRACE(LOG_INFO, "onPlaybackFinished");
}

void AraPlayer::onPlaybackError(MediaPlayer &mediaPlayer, player_error_t error)
{
	DEBUG_TRACE(LOG_INFO, "onPlaybackError");
}

void AraPlayer::onStartError(MediaPlayer &mediaPlayer, player_error_t error)
{
	DEBUG_TRACE(LOG_INFO, "onStartError");
}

void AraPlayer::onPauseError(MediaPlayer &mediaPlayer, player_error_t error)
{
	DEBUG_TRACE(LOG_INFO, "onPauseError");

	//TO-DO compare the MediaPlayer ID
	mCurrStream->getMediaPlayer()->stopPlay();
}

void AraPlayer::onPlaybackStopped(MediaPlayer &mediaPlayer)
{
	DEBUG_TRACE(LOG_INFO, "onPlaybackStopped");
	//TO-DO compare the MediaPlayer ID
	mCurrStream->getMediaPlayer()->deinit();
}

void AraPlayer::onStopError(MediaPlayer &mediaPlayer, player_error_t error)
{
	DEBUG_TRACE(LOG_INFO, "onStopError");
}

void AraPlayer::onPlaybackPaused(MediaPlayer &mediaPlayer)
{
	DEBUG_TRACE(LOG_INFO, "onPlaybackPaused");
}
