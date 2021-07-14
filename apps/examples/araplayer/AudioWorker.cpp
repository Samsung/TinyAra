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

#include <debug.h>
#include <sched.h>
#include <pthread.h>

#include "AraPlayer.h"
#include "AudioWorker.h"
#include "AudioStream.h"
#include "AudioPDStream.h"
#include "AudioHASStream.h"

AudioWorker::AudioWorker() :
	mStacksize(PTHREAD_STACK_DEFAULT * 4),
	mPriority(100),
	mThreadName("AudioWorker"),
	mIsRunning(false),
	mRefCnt(0),
	mWorkerThread(0),
	mEnableCurl(false),
	mMeasureCnt(0),
	mBandwidth(0)
{
	medvdbg("AudioWorker::AudioWorker()\n");
}

AudioWorker::~AudioWorker()
{
	medvdbg("AudioWorker::~AudioWorker()\n");
}

AudioWorker &AudioWorker::getWorker()
{
	static AudioWorker worker;
	return worker;
}

void AudioWorker::startWorker()
{
	std::unique_lock<std::mutex> lock(mRefMtx);
	++mRefCnt;
	DEBUG_TRACE(LOG_INFO, "%s::startWorker() - increase RefCnt : %d\n", mThreadName, mRefCnt);
	if (mRefCnt == 1) {
		int ret;
		struct sched_param sparam;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, mStacksize);
		sparam.sched_priority = mPriority;
		pthread_attr_setschedparam(&attr, &sparam);
		mIsRunning = true;
		ret = pthread_create(&mWorkerThread, &attr, static_cast<pthread_startroutine_t>(AudioWorker::audioLooper), this);
		if (ret != OK) {
			DEBUG_TRACE(LOG_ERR, "Fail to create worker thread, return value : %d\n", ret);
			--mRefCnt;
			mIsRunning = false;
			return;
		}
		pthread_setname_np(mWorkerThread, mThreadName);
	}

	DEBUG_TRACE(LOG_INFO, "worker started\n");
}

void AudioWorker::stopWorker()
{
	std::unique_lock<std::mutex> lock(mRefMtx);
	if (mRefCnt > 0) {
		--mRefCnt;
	}
	medvdbg("%s::stopWorker() - decrease RefCnt : %d\n", mThreadName, mRefCnt);
	if (mRefCnt <= 0) {
		std::atomic<bool> &refBool = mIsRunning;
		mWorkerQueue.enQueue([&refBool]() {
			refBool = false;
		});
		pthread_join(mWorkerThread, NULL);
		medvdbg("%s::stopWorker() - mWorkerthread exited\n", mThreadName);
	}
}

void AudioWorker::setCurl(void *curl)
{
	mCurl = curl;
}

void AudioWorker::setPlayer(std::shared_ptr<AraPlayer> player)
{
	mPlayer = player;
}

void AudioWorker::enableCurl()
{
	mEnableCurl = true;
}

void AudioWorker::disableCurl()
{
	mEnableCurl = false;
}

std::function<void()> AudioWorker::deQueue()
{
	return mWorkerQueue.deQueue();
}

AudioHttpDownloader *AudioWorker::findHttpDownloader(void *handle)
{
	std::map<pid_t, AudioStream *> sMap = mPlayer->getStreamMap();
	std::map<pid_t, AudioStream *>::iterator sIter;
	std::map<void *, AudioHttpDownloader *> dMap;
	std::map<void *, AudioHttpDownloader *>::iterator dIter;

	for (sIter = sMap.begin(); sIter != sMap.end(); sIter++) {
		switch (((AudioStream *)sIter->second)->getStreamType()) {
		case STREAM_HTTP_PROGRESSIVE:
			dMap = ((AudioPDStream *)sIter->second)->getDownloaderMap();
			break;
		case STREAM_HTTP_HLS:
		case STREAM_HTTP_DASH:
			dMap = ((AudioHASStream *)sIter->second)->getDownloaderMap();
			break;
		default:
			DEBUG_TRACE(LOG_INFO, "Unknow stream type [%d]", ((AudioStream *)sIter->second)->getStreamType());
			continue;
		}

		dIter = dMap.find(handle);
		if (dIter != dMap.end()) {
			DEBUG_TRACE(LOG_INFO, "found handle\n");
			return dIter->second;
		}
	}
}

void AudioWorker::measureBandwidth()
{
	std::map<pid_t, AudioStream *> sMap = mPlayer->getStreamMap();
	std::map<pid_t, AudioStream *>::iterator sIter;
	std::map<void *, AudioHttpDownloader *> dMap;
	std::map<void *, AudioHttpDownloader *>::iterator dIter;

	double val = 0;
	double totalVal = 0;
	int conns = 0;
	int ret = 0;

	if (mMeasureCnt % 5 == 0) {
		for (sIter = sMap.begin(); sIter != sMap.end(); sIter++) {
			switch (((AudioStream *)sIter->second)->getStreamType()) {
			case STREAM_HTTP_PROGRESSIVE:
				dMap = ((AudioPDStream *)sIter->second)->getDownloaderMap();
				break;
			case STREAM_HTTP_HLS:
			case STREAM_HTTP_DASH:
				dMap = ((AudioHASStream *)sIter->second)->getDownloaderMap();
				break;
			default:
				DEBUG_TRACE(LOG_INFO, "Unknow stream type [%d]", ((AudioStream *)sIter->second)->getStreamType());
				continue;
			}

			for (dIter = dMap.begin(); dIter != dMap.end(); dIter++) {
				ret = curl_easy_getinfo((CURL *)dIter->first, CURLINFO_SPEED_DOWNLOAD, &val);
				if ((ret == CURLE_OK) && (val > 0)) {
					totalVal += val;
					conns++;
				}
			}
		}
		mBandwidth = ((totalVal / 1024 / 1024) * 8) / conns;
		//DEBUG_TRACE(LOG_INFO, "Currnet Download Speed : %0.3f Mbps, Connections(%d)", mBandwidth, conns);
	}

	mMeasureCnt++;
}

void AudioWorker::pollCurl(int msec)
{
	CURLMsg *msg;
	CURLMcode mc;

	int numfds, msgs_left, repeats = 0;

	mc = curl_multi_perform((CURLM *)mCurl, &mCurlRunning);

	if (mc == CURLM_OK)
		mc = curl_multi_wait((CURLM *)mCurl, NULL, 0, msec, &numfds);

	if (mc != CURLM_OK) {
		//DEBUG_TRACE(LOG_ERR, "curl_multi_wait() failed, code %d", mc);
		return;
	}

	if (!numfds) {
		repeats++; /* count number of repeated zero numfds */
		if (repeats > 1)
			WAITMS(100); /* sleep 100 milliseconds */
	} else
		repeats = 0;

	/* See how the transfers went */
	while ((msg = curl_multi_info_read((CURLM *)mCurl, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			AudioHttpDownloader *httpDownloader = static_cast<AudioHttpDownloader *>(findHttpDownloader((void *)msg->easy_handle));
			if (httpDownloader) {
				DEBUG_TRACE(LOG_INFO, "HTTP Downlaod Complete??? %d", mc);
				httpDownloader->downloadComplete();
			} else {
				// error process...
				DEBUG_TRACE(LOG_INFO, "curl_multi_remove_handle %d", mc);
				curl_multi_remove_handle((CURLM *)mCurl, msg->easy_handle);
			}
		}
	}
	return;
}

bool AudioWorker::processLoop()
{
	if (mEnableCurl) {
		pollCurl(100);
		measureBandwidth();
		return true;
	}

	return false;
}

void *AudioWorker::audioLooper(void *arg)
{
	auto worker = static_cast<AudioWorker *>(arg);
	DEBUG_TRACE(LOG_INFO, "Start audioLooper\n");

	while (worker->mIsRunning) {
		while (worker->processLoop() && worker->mWorkerQueue.isEmpty()) {
			WAITMS(1);
		}

		std::function<void()> run = worker->deQueue();
		if (run != nullptr) {
			run();
		}
	}
	return NULL;
}

bool AudioWorker::isAlive()
{
	std::unique_lock<std::mutex> lock(mRefMtx);
	return mRefCnt == 0 ? false : true;
}
