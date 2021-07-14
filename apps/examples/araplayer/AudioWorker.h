/* ****************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#ifndef __AUDIO_WORKER_H__
#define __AUDIO_WORKER_H__

#include <sys/types.h>
#include <curl/curl.h>
#include <atomic>
#include <mutex>

#include "AudioQueue.h"
#include "AudioUtils.h"
#include "AudioHttpDownloader.h"

class AudioWorker
{
public:
	AudioWorker();
	virtual ~AudioWorker();

	void startWorker();
	void stopWorker();

	template <typename _Callable, typename... _Args>
	void enQueue(_Callable &&__f, _Args &&... __args)
	{
		mWorkerQueue.enQueue(__f, __args...);
	}
	std::function<void()> deQueue();
	bool isAlive();

	static AudioWorker &getWorker();
	void setCurl(void *);
	void enableCurl();
	void disableCurl();
	void setPlayer(std::shared_ptr<AraPlayer>);

private:
	bool processLoop();
	static void *audioLooper(void *);
	AudioHttpDownloader *findHttpDownloader(void *);
	void pollCurl(int msec);
	void measureBandwidth();

private:
	AudioQueue mWorkerQueue;
	std::atomic<bool> mIsRunning;
	int mRefCnt;
	pthread_t mWorkerThread;
	std::mutex mRefMtx;
	void *mCurl;
	int mCurlRunning;
	bool mEnableCurl;
	std::shared_ptr<AraPlayer> mPlayer;
	long mStacksize;
	int mPriority;
	double mBandwidth;
	unsigned int mMeasureCnt;
	const char *mThreadName;
};
#endif
