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

#ifndef __AUDIO_QUEUE_H__
#define __AUDIO_QUEUE_H__

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>
#include <functional>

class AudioQueue
{
public:
	AudioQueue();
	~AudioQueue();
	template <typename _Callable, typename... _Args>
	void enQueue(_Callable &&__f, _Args &&... __args)
	{
		std::unique_lock<std::mutex> lock(mQueueMtx);
		std::function<void()> func = std::bind(std::forward<_Callable>(__f), std::forward<_Args>(__args)...);
		mQueueData.push(func);
		mQueueCv.notify_one();
	}
	std::function<void()> deQueue();
	bool isEmpty();

private:
	std::queue<std::function<void()>> mQueueData;
	std::condition_variable mQueueCv;
	std::mutex mQueueMtx;
};

#endif
