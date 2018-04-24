/* ****************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
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

#include <functional>
#include <debug.h>
#include "PlayerObserverWorker.h"

namespace media {
std::unique_ptr<PlayerObserverWorker> PlayerObserverWorker::mWorker;
std::once_flag PlayerObserverWorker::mOnceFlag;

PlayerObserverWorker::PlayerObserverWorker()
{
}
PlayerObserverWorker::~PlayerObserverWorker()
{
}

PlayerObserverWorker& PlayerObserverWorker::getWorker()
{
	std::call_once(PlayerObserverWorker::mOnceFlag, []() { mWorker.reset(new PlayerObserverWorker); });

	return *(mWorker.get());
}

int PlayerObserverWorker::entry()
{
	while (mIsRunning) {
		std::unique_lock<std::mutex> lock(mObserverQueue.getMutex());

		if (mObserverQueue.isEmpty()) {
			medvdbg("PlayerObserverWorker: wait\n");
			mObserverQueue.wait(lock);
		}

		if (!mObserverQueue.isEmpty()) {
			medvdbg("PlayerObserverWorker: wake\n");
			std::function<void()> run = mObserverQueue.deQueue();
			run();
		}
	}
	return 0;
}

player_result_t PlayerObserverWorker::startWorker()
{
	std::lock_guard<std::mutex> lock(mRefMtx);
	medvdbg("PlayerObserverWorker : startWorker\n");
	if (++mRefCnt == 1) {
		medvdbg("PlayerObserverWorker : create thread\n");
		mIsRunning = true;
		mWorkerThread = std::thread(std::bind(&PlayerObserverWorker::entry, this));
	}

	return PLAYER_OK;
}

void PlayerObserverWorker::stopWorker()
{
	std::lock_guard<std::mutex> lock(mRefMtx);
	medvdbg("PlayerObserverWorker : stopWorker\n");
	if (--mRefCnt <= 0) {
		medvdbg("PlayerObserverWorker : join thread\n");
		mIsRunning = false;
		if (mWorkerThread.joinable()) {
			mObserverQueue.notify_one();
			mWorkerThread.join();
		}
	}
}

MediaQueue& PlayerObserverWorker::getQueue()
{
	return mObserverQueue;
}
} // namespace media
