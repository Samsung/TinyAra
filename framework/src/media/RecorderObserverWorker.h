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

#ifndef __MEDIA_RECORDEROBSERVERWORKER_H
#define __MEDIA_RECORDEROBSERVERWORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <iostream>
#include <fstream>

#include <tinyalsa/tinyalsa.h>
#include <media/MediaRecorder.h>
#include "MediaQueue.h"
#include "MediaRecorderImpl.h"
using namespace std;

namespace media {
class RecorderObserverWorker
{
public:
	RecorderObserverWorker();
	~RecorderObserverWorker();

	static RecorderObserverWorker& getWorker() {
		call_once(RecorderObserverWorker::mOnceFlag, []() {
			mWorker.reset(new RecorderObserverWorker);
		});

		return *(mWorker.get());
	}
	
	recorder_result_t startWorker();
	void stopWorker();
	MediaQueue& getQueue();

private:
	int entry();
	void increaseRef();
	void decreaseRef();

private:
	static unique_ptr<RecorderObserverWorker> mWorker;
	static once_flag mOnceFlag;
	int mRefCnt;
	bool mIsRunning;
	std::thread mWorkerThread;
	MediaQueue mObserverQueue; // observer queue
	std::mutex mRefMtx;  // reference cnt mutex
};
} // namespace media

#endif
